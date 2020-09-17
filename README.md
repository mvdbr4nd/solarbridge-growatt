# solarbridge-growatt

SolarBridge - Growatt for platformio version by Mvdbr4nd
Forked from: SolarBridge v2.2.1 by Oepi-Loepi
    Version 2.1, corrected for daily totals
    Version 2.2, problem wih some logins solved
    Version 2.2.1, minor issue improved. When power is 0, the interval was set to 1 hr so no pulses were set during this time even when power returns
    All rights reserved
    Do not use for commercial purposes

- This project is a PlatformIO version, and converted to use ArduinoJson6 and latest WifiManager solving some issues with the older wifimanager.
- This project is dependending on several libs which are defined in the platformio.ini file and will be downloaded automaticly by platformio

This bridge will get the data from the GroWatt web interface (API) and create S0 pulses and led flashes
Each pulse/flash will suggest 1 Watt

On GPIO 4 a resistor (330 ohm and red LED are connected in series 
PIN D2 ----  Resistor 33Ohm) ---- (Long LED lead ---- LED ---- Short LED Lead) ----- GND
 
On GPIO 5 a PC817 optocoupler is connected
PIN D1 ----  PC817 (anode, pin 1, spot)
GND -------  PC817 (cathode, pin 2)
 
Pin 3 and 4 of the PC817 will be a potential free contact
 
After uploading the sketch to the Wemos D1 mini, connect to the AutoConnectAP wifi. 
Goto 192.168.4.1 in a webbrowser and fill in all data including GroWatt credentials.

Note:
- Settings are saved on the Flash (wemos mini) and will reloaded from startup. 
- Resetting the bridge will clear all settings (WIFI and Username Password).
- The external LED is optional, you can also use the internal buildin led (see blinkled function)