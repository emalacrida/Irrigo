
/**************************************************************

   IRRIGO Sistema di controllo irrigazione via internet utilizzando Blynk.
   derivato da:
   http://community.blynk.cc/t/automatic-scheduler-esp-01-with-4-time-input-widgets/10658
   Versione in sviluppo

   App project setup:
   RTC widget (no pin required!!!)
   Blynk's widgets.

   Manual/Auto button       on V0
   On-off button Zone A     on V1
   On-off button Zone B     on V2
   On-off button Zone C     on V3
   Terminal                 on V100          Label will be the clock + wifi signal!!!!

   Blue led                 on  D2   4       WiFi connected
   Yellow led               on  D3   0       OTA loading
   Red led                  on  D4   2       =! bulitin blue led

   A Zone electrovalve      on  D8  15
   B Zone electrovalve      on  D7  13
   C Zone electrovalve      on  D6  12
   D Zone electrovalve      on  D5  14       Only HW connected not SW supported

   App project setup:
   Zone A active     on V10
   Time Input widget on V11  Set interval A1 Start/Stop
   Time Input widget on V12  Set interval A2 Start/Stop
   Zone B active     on V20
   Time Input widget on V21  Set interval B1 Start/Stop
   Time Input widget on V22  Set interval B2 Start/Stop
   Zone C active     on V30
   Time Input widget on V31  Set interval C1 Start/Stop
   Time Input widget on V32  Set interval C2 Start/Stop

   Terminal widget   on V100

 **************************************************************/

//#define BLYNK_DEBUG           // slowdown x10 operation more detailed print
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>          //for OTA
#include <WiFiUdp.h>              //for OTA
#include <ArduinoOTA.h>           //for OTA
#include <BlynkSimpleEsp8266.h>
#include <SimpleTimer.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

#define LED_BLU 04           //D2
#define LED_GIALLO 00        //D3
#define LED_ROSSO 02         //D4 builtin led e rosso 

#define ZoneD_valve 14       //D5-elettrovalvola 4                        
#define ZoneC_valve 12       //D6-elettrovalvola 3
#define ZoneB_valve 13       //D7-elettrovalvola 2
#define ZoneA_valve 15       //D8-elettrovalvola 1

#define server "blynk-cloud.com"   // or "blynk.cloud-com" for Blynk's cloud server
#define TestLED 2                  // on board LED pin assignment

#define HOSTNAME "irrigo"
#define VERSION "1.01"

SimpleTimer timer;
WidgetRTC rtc;
WidgetTerminal terminal(V100);

char Date[16];
char Time[16];
char auth[] = "xxxxxxexxxxxxxxxxxxxxxxxxxxxx";
char ssid[] = "xxxxxxxxxxx";     // casa
char pass[] = "xxxxxxxxxxx";

long startsecondswd;            // weekday start time in seconds
long stopsecondswd;             // weekday stop  time in seconds
long nowseconds;                // time now in seconds
bool isFirstConnect = true;

String displaycurrenttimepluswifi;
String WeekDayString;
String var;

int wifisignal;
int manual = 0;
int oldstatus;

int ZoneA_AutoFlag;               // = 1 se attivo AUTO = 0 se inattivo
int ZoneB_AutoFlag;
int ZoneC_AutoFlag;
int ZoneD_AutoFlag;               // RFU

int SaveZoneA_AutoFlag;            // save state swicking from auto to manual
int SaveZoneB_AutoFlag;
int SaveZoneC_AutoFlag;
int SaveZoneD_AutoFlag;            // RFU

long startA1;            // ora di start e stop Zone A
long stopA1;
long startA2;
long stopA2;

long startB1;            // ora di start e stop Zone B
long stopB1;
long startB2;
long stopB2;

long startC1;            // ora di start e stop Zone C
long stopC1;
long startC2;
long stopC2;

long startD1;            // ora di start e stop Zone D
long stopD1;
long startD2;
long stopD2;

bool updating = 0;
unsigned long updateStarted_time = 0;

void setup()
{
  /*
  .
  .
  Inizializzazione variabili
  .
  .
  */

  Serial.begin(115200);
  Serial.println("\Starting");
  //  Blynk.begin(auth, ssid, pass, server);
  Blynk.begin(auth, ssid, pass, server);
  int mytimeout = millis() / 1000;
  while (Blynk.connect() == false) {            // try to connect to server for 10 seconds
    if ((millis() / 1000) > mytimeout + 8) {    // try local server if not connected within 9 seconds
      break;
    }
  }
  
/*
 .
 .
 Timer setup
 .
 .
*/  

  // Set OTA port - Port defaults to 8266     // OTA setup
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("emiot-Irrigo");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"2305");

  ArduinoOTA.onStart([]() {
    digitalWrite(LED_GIALLO, HIGH);
    Serial.println("OTA Start");
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

  digitalWrite(LED_GIALLO, LOW);
}

/*
.
.
.

*/


void loop()
{
  //  server.handleClient();

  ArduinoOTA.handle();
  if (!updating) {
    Blynk.run();
    timer.run();
    digitalWrite(LED_ROSSO, manual);
  } else {
    if ((millis() - updateStarted_time) > 300000) {
      updating = false;
    }
  }
  
/*
.
.
.
*/
