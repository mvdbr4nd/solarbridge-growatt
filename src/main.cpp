#include <main.h>
#include <growatt.h>
#include <webserver.h>

#include <LittleFS.h>
FS* fileSystem = &LittleFS;
LittleFSConfig fileSystemConfig = LittleFSConfig();

// global variables
char username[60] = "";
char password[60] = "";

bool ConnectionPossible=false;

bool reset1 = false;
bool reset2 = false;

// You can select BUILTIN_LED or D2 to toggle the led for status indication. 
// which means you don't need to wire D1 to an external LED and resistor. 
int led = D2; 			// BUILTIN_LED;
int meteradapter = D1; 

unsigned long startMillis;    //some global variables available anywhere in the program
unsigned long currentMillis;
unsigned long period = 2000;    //time between pulses, initial 2000 ms
unsigned long startMillis2;    //some global variables available anywhere in the program
unsigned long currentMillis2;
unsigned long InitialGetdatafrequency = 20000; //only first time to get data from Growatt, then time will be set to Getdatafrequency)
unsigned long Getdatafrequency = 60000; //time between data transfers from GroWatt
unsigned long Getdatafrequencyset = 0; //time between data transfers from GroWatt
unsigned long timebetweenpulses = 2000; //time between pulses calculated (initial)
bool shouldSaveConfig = false;
bool ActualPowerZero = false;

int PulsesGenerated = 0;    //pulses generated in the periods from last change    (when blink, PulsesGenerated++ and reset after calculation of correction)
float NewDayTotal = -1;
float PrevDayTotal = -1;     //shoud be -1
float ActualPower = 0;
float PowerCorrection = 0;
int NumberofPeriodSinceUpdate = 0;
int EnergyfromDaytotal = 0;
float CorrectedPowerNextPeriod = 0;
int MaxNumberofCorrections = 0;
bool newday = false;        // must be false first time

void configModeCallback (WiFiManager *myWiFiManager) {
	Serial.println("Entered config mode");
	Serial.println(WiFi.softAPIP());
	//if you used auto generated SSID, print it
	Serial.println(myWiFiManager->getConfigPortalSSID());
}

//callback notifying us of the need to save config
void saveConfigCallback () {
	Serial.println("Should save config");
	shouldSaveConfig = true;
}

void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);

	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
	
	pinMode(led, OUTPUT);
  	digitalWrite(led, HIGH);
  	
	pinMode(meteradapter, OUTPUT);
  	digitalWrite(meteradapter, HIGH);

	fileSystemConfig.setAutoFormat(true);
	fileSystem->setConfig(fileSystemConfig);

	//read configuration from FS json
	Serial.println("mounting FS...");
	if (fileSystem->begin()) {
		Serial.println("mounted file system");
		if (fileSystem->exists("/config.json")) {
			//file exists, reading and loading
			Serial.println("reading config file");
			File configFile = fileSystem->open("/config.json", "r");
			if (configFile) {
				Serial.println("opened config file");

				StaticJsonDocument<200> doc;
				DeserializationError error = deserializeJson(doc, configFile);
				if (error) {
					Serial.println("failed to load json config");
					return;
				}

				// Extract each characters by one by one
				while (configFile.available()) {
					Serial.print((char)configFile.read());
				}
				Serial.println();
				Serial.println("\nparsed json");
			
				strcpy(username, doc["username"]);
				strcpy(password, doc["password"]);

			} else {
				Serial.println("failed to load json config");
			}
		}
	} else {
		Serial.println("failed to mount FS");
	}

	// setup wifi
	WiFiManagerParameter cust_username("Username", "username", username, 60);
	WiFiManagerParameter cust_password("Password", "password", password, 60);

	WiFiManager wifiManager;
	wifiManager.setSaveConfigCallback(saveConfigCallback);
	wifiManager.setAPCallback(configModeCallback);    
	wifiManager.addParameter(&cust_username);
	wifiManager.addParameter(&cust_password);
	wifiManager.autoConnect("SolarBridgeAP");

	wifiManager.setConfigPortalTimeout(180);

	Serial.println("connected...yeey :)");

	//read updated parameters
	strcpy(username, cust_username.getValue());
	strcpy(password, cust_password.getValue());
	Serial.println(username);
	Serial.println(password);

	delay(2000);

	Serial.println("local ip");
	Serial.println(WiFi.hostname());
	Serial.println(WiFi.localIP());
	Serial.println(WiFi.gatewayIP());
	Serial.println(WiFi.subnetMask());
	Serial.println(WiFi.dnsIP());
	
	//write config to config.json
	if (shouldSaveConfig) {
		Serial.println("saving config to    FS");    
		StaticJsonDocument<200> doc;

		doc["username"] = username;
		doc["password"] = password;
		
		File configFile = fileSystem->open("/config.json", "w");
		if (!configFile) {
			Serial.println("failed to open config file for writing");
		}
		
		// Serialize JSON to file
		if (serializeJson(doc, configFile) == 0) {
			Serial.println(F("Failed to write to file"));
		}
		
		// Close the file
		configFile.close();
		//end save
	}
	WiFi.begin();

  	// OTA support
  	setup_OTA();

	server.begin();
}

void setup_OTA() {
	// Port defaults to 8266
	// ArduinoOTA.setPort(8266);

	// Hostname defaults to esp8266-[ChipID]
	// ArduinoOTA.setHostname("myesp8266");

	// No authentication by default
	// ArduinoOTA.setPassword((const char *)"123");

  	ArduinoOTA.onStart([]() {
    	Serial.println("Start");
  	});
  	ArduinoOTA.onEnd([]() {
    	Serial.println("\nEnd");
  	});
  	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    	Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  	});
  	ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
}

void blinkled() {
	if (led != LED_BUILTIN) {
		digitalWrite(led, HIGH);    
	}
	else {
		digitalWrite(led, LOW);    
	}
	
	digitalWrite(meteradapter, HIGH);
	delay(100);                             
	digitalWrite(meteradapter, LOW);
	
	if (led != LED_BUILTIN) {
		digitalWrite(led, LOW);
	} 
	else {
		digitalWrite(led, HIGH);
	}	
}

void loop() {
	// put your main code here, to run repeatedly:
	webserver();
 
  	// handle OTA if needed
  	ArduinoOTA.handle();
	yield();

	if ((reset1) && (reset2)) { 
			delay(2000);
			Serial.println("Reset requested");
			Serial.println("deleting configuration file");
			fileSystem->remove("/config.json");
			WiFiManager wifiManager;
			wifiManager.resetSettings();
			delay(500);
			ESP.reset();
	}

	currentMillis = millis();    //get the current "time" (actually the number of milliseconds since the program started)
	if (currentMillis - startMillis >= period) { //test whether the pulsetime has eleapsed
		Serial.print("PulsesGen.: ");Serial.print(PulsesGenerated);
		Serial.print(" ** EnergyDay: ");Serial.print(EnergyfromDaytotal);
		Serial.print(" ** Act.Power: ");Serial.print(ActualPower);
		Serial.print(" ** N.Periods :");Serial.print(NumberofPeriodSinceUpdate);
		Serial.print(" ** PowerCorre.: ");Serial.print(PowerCorrection);
		Serial.print(" ** Corr.Power: ");Serial.print(CorrectedPowerNextPeriod);
		Serial.print(" ** timebetw.pulses: ");Serial.print(timebetweenpulses);
		Serial.print(" ** newday: ");Serial.println(newday);

		if (!ActualPowerZero) {
			blinkled();
		}
		PulsesGenerated++;
		period = timebetweenpulses;
		startMillis = currentMillis;    //IMPORTANT to save the start time of the current LED state.
	}

	currentMillis2 = millis();    //get the current "time" (actually the number of milliseconds since the program started)
	if (currentMillis2 - startMillis2 >= Getdatafrequencyset) { //test whether the period has elapsed to get new data from the server
		startMillis2 = currentMillis2;    //IMPORTANT to save the start time of the current LED state.
		if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
			getdata();
		}
	}
}