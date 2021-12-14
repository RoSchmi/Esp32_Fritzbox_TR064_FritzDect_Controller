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
// In file config.h it can be selected if http or https is used.
// The certificate needed for https (and how it can be got)
// can be found in config.h 
// In file config_secret.h the WiFi credentials and the credentials
// of the Fritzbox have to be entered (use config_secret_template.h
// as a template)

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
 
// The AIN of the DECT!200 switchable power socket can be found in FritzBox Webinterface
// an on the device
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
    
    // optinal delay
    delay(5000);

    // In the case of issues the Debug output can be activated here
    connection.debug_level = DEBUG_VERBOSE;
    //connection.debug_level = DEBUG_ERROR;

    connection.init();
    
    // if wanted show functions of homeauto service
    // connection.showHomeauto_Services();
    
    // if wanted rename the power socket
    //SetDeviceName(Steckdose1, "FRITZ!DECT 210 #1");  
}
 
void loop() {
    delay(200);
    
    GetDeviceInfo(Steckdose1);

    // If wanted get genericDeviceInfo
    GetGenericDeviceInfo("0");   // Index homeautomation device list
    delay(2000);
    // Switch on
    SetSwitch(Steckdose1, "ON");
    Serial.println("Switched on");
    delay(10000);
    SetSwitch(Steckdose1, "OFF");
    Serial.println("Switched off");
    delay(5000);
    GetDeviceInfo(Steckdose1);

    // if wanted ring phone
    // "Wählhilfe" in Fritzbox must be activated
    // https://service.avm.de/help/de/FRITZ-Box-Fon-WLAN-7490/017p1/hilfe_fon_waehlhilfe
    WahlRundruf();
 
    while (true)
    {
        delay(5000);
        Serial.println("Funtions ended, now performing infinite loop");
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
    String paramsb[][2] = {{"NewIndex", index}};
    String reqb[][2] = {{"NewAIN", ""}};
    connection.action("urn:dslforum-org:service:X_AVM-DE_Homeauto:1", "GetGenericDeviceInfos", paramsb, 1, reqb, 1);
    String name = reqb[0][1];
    
    Serial.print("Read AIN: ");
    Serial.print(name);
    
}

  //  Rundruffunktion über TR064 an der Fritzbox auslösen
void WahlRundruf() {
      String service = "urn:dslforum-org:service:X_VoIP:1";

  // Die Telefonnummer **9 ist der Fritzbox-Rundruf.
  // This function needs the "Wählhilfe" in Fritzbox to be
  // activated
  // https://service.avm.de/help/de/FRITZ-Box-Fon-WLAN-7490/017p1/hilfe_fon_waehlhilfe
  String call_params[][2] = {{"NewX_AVM-DE_PhoneNumber", "**9"}};
  connection.action(service, "X_AVM-DE_DialNumber", call_params, 1);

  // Warte 10 Sekunden bis zum Auflegen
  delay(10000);
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