#include <growatt.h>

String JSESSIONID;
String SERVERID;

String todayval = "Not Connected Yet";
String monthval= "Not Connected Yet";
bool cookiestep= false;

WiFiClient client;

void getdata() {
	//get new data from the growatt server.
	
	MD5Builder md5;
	md5.begin();
	md5.add(password);    // md5 of the user:realm:user
	md5.calculate();
	String password = md5.toString();

	char character;
	String pwd = "";
	for(unsigned int i = 0; i < password.length(); i=i+2){    //replacing leading 0 for c
		if (password[i]=='0'){
				character = 'c';
		}else{
			character = password[i];
		}
		pwd = pwd + character + password[i+1] ;
	}
	password = pwd;

	HTTPClient http;
	const char * headerkeys[] = {"User-Agent","Set-Cookie","Cookie","Date","Content-Type","Connection"} ;
	size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
	
	http.begin(client, "http://server.growatt.com/LoginAPI.do");
	http.setReuse(true);
	http.setUserAgent("Dalvik/2.1.0 (Linux; U; Android 9; ONEPLUS A6003 Build/PKQ1.180716.001");
	http.addHeader("Content-type", "application/x-www-form-urlencoded");

	http.collectHeaders(headerkeys,headerkeyssize);

	int code = http.POST("password=" + password + "&userName="+username); //Send the request
	if ((code=200) || (code = 301) || (code = 302)) {
		JSESSIONID = "";
		SERVERID = "";
		String res = http.getString();
		String totCookie = http.header("Set-Cookie").c_str();
		int isteken = totCookie.indexOf("JSESSIONID=") +    11;
		int puntkommateken = totCookie.indexOf(";",isteken);
		JSESSIONID = totCookie.substring(isteken, puntkommateken);
		isteken = totCookie.indexOf("SERVERID=", puntkommateken ) + 9;
		puntkommateken = totCookie.indexOf(";",isteken);
		SERVERID = totCookie.substring(isteken, puntkommateken);
		if ((JSESSIONID != "")&&(SERVERID != "")){
				cookiestep= true;
				ConnectionPossible = true;
		}
	}

	http.begin(client, "http://server-api.growatt.com/newPlantAPI.do?action=getUserCenterEnertyData");
	http.addHeader("Cookie", "JSESSIONID="    + JSESSIONID + ";" + "SERVERID="    + SERVERID);
	http.setReuse(true);
	http.setUserAgent("Dalvik/2.1.0 (Linux; U; Android 9; ONEPLUS A6003 Build/PKQ1.180716.001");
	http.addHeader("Content-type", "application/x-www-form-urlencoded");
	http.addHeader("Referer", "http://server.growatt.com/LoginAPI.do");

	http.collectHeaders(headerkeys,headerkeyssize);
	code = http.POST("language=1"); //Send the request
	if ((code=200) || (code = 301) || (code = 302)) {
		String payload = http.getString();; //Get the request response payload
				
		//const size_t capacity = JSON_ARRAY_SIZE(0) + JSON_OBJECT_SIZE(24);
		const size_t capacity = 850;
		DynamicJsonDocument jsonBuffer(capacity);

		auto error = deserializeJson(jsonBuffer, payload);
		if (error) {
			Serial.print(F("deserializeJson() failed with code "));
			Serial.println(error.c_str());
			return;
		} 
		else {
			ActualPower    = jsonBuffer["powerValue"];
			float MonthValue    = jsonBuffer["monthValue"];
			NewDayTotal = jsonBuffer["todayValue"];

			todayval = String(NewDayTotal)+ " KWh";
			monthval = String(MonthValue)+ " KWh";

			Serial.print("PrevDayTotal :");
			Serial.println(PrevDayTotal);
			Serial.print("NewDayTotal :");
			Serial.println(NewDayTotal);

			if (NewDayTotal == 0) {                         
				//it will be a new day or a new counting period.
				Serial.println("A new day has started.");
				newday = true;
				PrevDayTotal=0;
				PulsesGenerated = 0;
				NumberofPeriodSinceUpdate=0;
				MaxNumberofCorrections= 0;
				CorrectedPowerNextPeriod = 0;    
				PowerCorrection= 0;
			}

			if (!newday){
				Serial.println("No new day registered yet so impossible to correct to current totals.");
			}
						
			NumberofPeriodSinceUpdate++;
			EnergyfromDaytotal = (int)(NewDayTotal *1000);    //increase energy read from Growatt from the daytotals (in Wh)
			if ((NewDayTotal != PrevDayTotal) && (PrevDayTotal != -1) && (newday==true)) {
							
					Serial.print("EnergyfromDaytotal :");
					Serial.println(EnergyfromDaytotal);
	
					Serial.print("PulsesGenerated :");
					Serial.println(PulsesGenerated); //energy accounted for by pulses since last update (in Wh)

					float EnergytobeCorrected = EnergyfromDaytotal - PulsesGenerated;
					
					Serial.print("EnergytobeCorrected :");
					Serial.println(EnergytobeCorrected); //energy accounted for by pulses since last update (in Wh)

					Serial.print("NumberofPeriodSinceUpdate :");
					Serial.println(NumberofPeriodSinceUpdate); 
	
					PowerCorrection = (EnergytobeCorrected/NumberofPeriodSinceUpdate); //Correction of Power in W
					Serial.print("PowerCorrection :");
					Serial.println(PowerCorrection);

					MaxNumberofCorrections = NumberofPeriodSinceUpdate;
	
					NumberofPeriodSinceUpdate=0; //reset the number of periods since last update
					PrevDayTotal = NewDayTotal;
				}

				if ((PrevDayTotal == -1) && (NewDayTotal > 0)) {
					Serial.print("PrevDayTotal UPDATED! :");
					PrevDayTotal = NewDayTotal;
					Serial.println(PrevDayTotal);
				}

				Serial.print("ActualPower :");
				Serial.println(ActualPower);

				if (ActualPower > 1) {
					ActualPowerZero = false;
					if (NumberofPeriodSinceUpdate <= MaxNumberofCorrections){
						CorrectedPowerNextPeriod = ActualPower + PowerCorrection;
					}
					else{
						CorrectedPowerNextPeriod = ActualPower + PowerCorrection;
					}
					Serial.print("CorrectedPowerNextPeriod :");
					Serial.println(CorrectedPowerNextPeriod);

					timebetweenpulses = 3600000/CorrectedPowerNextPeriod; //calculation of the interval
					Serial.print("timebetweenpulses :");
					Serial.println(timebetweenpulses);
				}
				else{
					//1 hour interval when ActualPower = 0
					ActualPowerZero = true;
					timebetweenpulses = 360000;
					CorrectedPowerNextPeriod = 0;
					PowerCorrection = 0;
				}
			Getdatafrequencyset = Getdatafrequency;
		}
	}                                
	http.end();    //Close connection
}