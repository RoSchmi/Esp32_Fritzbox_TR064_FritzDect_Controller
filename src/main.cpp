// Program Esp32_Fritzbox_TR064_FritzDect_Controller
// by RoSchmi
// modifiziert nach Projekt
// https://github.com/christophreuter/TR-064
// von Dr. Christoph Reuter
// Modifiziert am 5.7.2021

// The used TR-064 library is a modification of:
// https://github.com/Aypac/Arduino-TR-064-SOAP-Library
//
// With this modification https transmission can be used to
// access Fritzbox via Tr-064 protocol

// Settings which have to be done on the Fritzbox can be found here:
// https://www.schlaue-huette.de/apis-co/fritz-tr064/
//
// By default the TR-064 protocol is not activated on the Fritzbox.
// To enable TR-064 follow this:
// -->FritzBox Weboberfläche-->Expertenansicht
// Heimnetz » Heimnetzübersicht » Netzwerkeinstellungen
// --> Zugriff für Anwendungen zulassen --> neu starten
// --> Usernamen/Passwort angeben (System » FritzBox Benutzer)
// Benutzereinstellungen: „FRITZBox Einstellungen“ und „Sprachnachrichten, Faxnachrichten, FRITZApp Fon und Anrufliste“ aktivieren.
// wenn alles richtig, öffne: http://fritz.box:49000/tr64desc.xml

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <tr064.h>
#include "config_secret.h"
#include "config.h"

#include "WiFiClientSecure.h"

// Default Esp32 stack size of 8192 byte is not enough for some applications.
// --> configure stack size dynamically from code to 16384
// https://community.platformio.org/t/esp32-stack-configuration-reloaded/20994/4
// Patch: Replace C:\Users\thisUser\.platformio\packages\framework-arduinoespressif32\cores\esp32\main.cpp
// with the file 'main.cpp' from folder 'patches' of this repository, then use the following code to configure stack size
#if !(USING_DEFAULT_ARDUINO_LOOP_STACK_SIZE)
   uint16_t USER_CONFIG_ARDUINO_LOOP_STACK_SIZE = 8192;
  //uint16_t USER_CONFIG_ARDUINO_LOOP_STACK_SIZE = 16384;
  //uint16_t USER_CONFIG_ARDUINO_LOOP_STACK_SIZE = 32768;
#endif

X509Certificate myX509Certificate = myfritzbox_root_ca;

#if TRANSPORT_PROTOCOL == 1
    static WiFiClientSecure wifi_client;
    Protocol protocol = Protocol::useHttps;
  #else
    static WiFiClient wifi_client;
    Protocol protocol = Protocol::useHttp;
#endif

    
HTTPClient http;
static HTTPClient * httpPtr = &http;
        
WiFiMulti wiFiMulti;
 
//-------------------------------------------------------------------------------------
// Get your router settings here
//-------------------------------------------------------------------------------------
// Wifi network name (SSID)
const char* wifi_ssid = IOT_CONFIG_WIFI_SSID; 
// Wifi network password
const char* wifi_password = IOT_CONFIG_WIFI_PASSWORD;
// The username if you created an account, "admin" otherwise
const char* fuser = FRITZ_USER;
// The password for the aforementioned account.
const char* fpass = FRITZ_PASSWORD;

// IP address or URL of your router. For http transmission this can be "192.168.179.1" for most FRITZ!Boxes
// For https transmission you MUST use the URL (e.g. fritz.box)
//const char* IP = "192.168.179.1";
const char* IP = FRITZ_IP_ADDRESS;

// Port of the API of your router. This should be 49000 for http and 49443 for https
#if TRANSPORT_PROTOCOL == 1
    const int PORT = 49443;
#else
    const int PORT = 49000;
#endif
// -------------------------------------------------------------------------------------
 
// TR-064 connection
TR064 connection(PORT, IP, fuser, fpass, protocol, wifi_client, httpPtr, myX509Certificate);
 
// Die AIN der DECT!200 Steckdose findet sich im FritzBox Webinterface
//const String Steckdose1 = "12345 1234567";

const String Steckdose1 = FRITZ_DEVICE_AIN_01;

// forward declarations
void ensureWIFIConnection();
void SetSwitch(String AIN, String state);
void GetDeviceInfo(String AIN);
void GetGenericDeviceInfo(String AIN);
void SetDeviceName(String AIN, String newName);
void WahlRundruf();
void serialEvent();

void setup() {
    Serial.begin(115200);
    while(!Serial);
    Serial.println("boot...");
 
    // Connect to wifi
    ensureWIFIConnection();
    Serial.println(F("WIFI connected..."));

    delay(5000);

    // Bei Problemen kann hier die Debug Ausgabe aktiviert werden
    connection.debug_level = DEBUG_VERBOSE;
    
    connection.init();

    connection.showHomeauto_Services();

    //SetDeviceName(Steckdose1, "FRITZ!DECT 210 #1");
    GetDeviceInfo(Steckdose1);
    //GetGenericDeviceInfo("1");
    //SetSwitch(Steckdose1, "ON");
    //SetSwitch(Steckdose1, "OFF");

    //WahlRundruf();
  
while (true)
    {
        delay(5000);
        Serial.println("Infinite loop");
    }
    
    
}
 
void loop() {
    delay(200);
    //GetGenericDeviceInfo(Steckdose1);
    GetDeviceInfo(Steckdose1);
    delay(500);
    while (true)
    {
        delay(5000);
        Serial.println("Infinite loop in loop()");
    }
    //GetGenericDeviceInfo(Steckdose1);
    delay(500);
    //Serial.println("Making Wahlrundruf");
    //WahlRundruf();

    GetDeviceInfo(Steckdose1);
    delay(500);
    SetSwitch(Steckdose1, "ON");
    Serial.println(F("Switched on"));
    delay(20000);
    SetSwitch(Steckdose1, "OFF");
     Serial.println(F("Switched off"));
    delay(20000);


    while (true)
    {
        delay(200);
    }
}

void SetDeviceName(String AIN, String newName)
{
    ensureWIFIConnection();
    String paramsb[][2] = {{"NewAIN", AIN}, {"NewDeviceName", newName}};
    connection.action("urn:dslforum-org:service:X_AVM-DE_Homeauto:1", "SetDeviceName", paramsb, 2);
}

void SetSwitch(String AIN, String state) {
    ensureWIFIConnection();
    String paramsb[][2] = {{"NewAIN", AIN}, {"NewSwitchState", state}};
    connection.action("urn:dslforum-org:service:X_AVM-DE_Homeauto:1", "SetSwitch", paramsb, 2);
}
 
void GetDeviceInfo(String AIN) {
    ensureWIFIConnection();
    
    String paramsb[][2] = {{"NewAIN", AIN}};
    
    String reqb[][2] = {{"NewMultimeterPower", ""}, {"NewTemperatureCelsius", ""}};
    connection.action("urn:dslforum-org:service:X_AVM-DE_Homeauto:1", "GetSpecificDeviceInfos", paramsb, 1, reqb, 2);
    float power = reqb[0][1].toInt() / 100.0;
    float temp = reqb[1][1].toInt() / 10.0;
    Serial.print("Stromverbrauch: ");
    Serial.print(power, 1);
    Serial.println(" W");
    Serial.print("Temperatur: ");
    Serial.print(temp, 1);
    Serial.println(" °C");
}

void GetGenericDeviceInfo(String index) {
    ensureWIFIConnection();
    String paramsb[][2] = {{"NewIndex", index}, {"NewAIN", ""}};
    String reqb[][2] = {{"NewAIN", ""}};
    connection.action("urn:dslforum-org:service:X_AVM-DE_Homeauto:1", "GetGenericDeviceInfos", paramsb, 2, reqb, 1);
    String name = reqb[0][1];
    
    Serial.print("Read AIN: ");
    Serial.print(name);
    
}

  //  Rundruffunktion über TR064 an der Fritzbox auslösen
void WahlRundruf() {
      String service = "urn:dslforum-org:service:X_VoIP:1";

  // Die Telefonnummer **9 ist der Fritzbox-Rundruf.
  String call_params[][2] = {{"NewX_AVM-DE_PhoneNumber", "**9"}};
  connection.action(service, "X_AVM-DE_DialNumber", call_params, 1);

  // Warte vier Sekunden bis zum auflegen
  delay(4000);
  connection.action(service, "X_AVM-DE_DialHangup");
  
}
 
void ensureWIFIConnection() {
    if((wiFiMulti.run() != WL_CONNECTED)) {
        wiFiMulti.addAP(wifi_ssid, wifi_password);
        while ((wiFiMulti.run() != WL_CONNECTED)) {
            delay(100);
        }
    }
}