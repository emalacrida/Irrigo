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
   Time Input widget on V21  Set interval B1 Start/Stop+

   Time Input widget on V22  Set interval B2 Start/Stop
   Zone C active     on V30
   Time Input widget on V31  Set interval C1 Start/Stop
   Time Input widget on V32  Set interval C2 Start/Stop

   Terminal widget   on V100

 **************************************************************/

//#define BLYNK_DEBUG           // slowdown x10 operation more detailed print
#define BLYNK_PRINT Serial
#include <Blynk.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>          //for OTA
#include <WiFiUdp.h>              //for OTA
#include <ArduinoOTA.h>           //for OTA
#include <BlynkSimpleEsp8266.h>
//#include <SimpleTimer.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <SPI.h>

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

//SimpleTimer timer;
BlynkTimer timer;
WidgetRTC rtc;
WidgetTerminal terminal(V100);

char Date[16];
char Time[16];
char auth[] = "92d1a7e6379a4d49b455278358ef0d65";
char ssid[] = "adircalam2";     // casa
char pass[] = "casa.em.230552";
//char ssid[] = "emtabAP";      // tablet
//char pass[] = "irrigo2305";
//char ssid[] = "xxxxxx";       // tablet
//char pass[] = "xxxxxx";
long startsecondswd;            // weekday start time in seconds
long stopsecondswd;             // weekday stop  time in seconds
long nowseconds;                // time now in seconds
bool isFirstConnect = true;

String displaycurrenttimepluswifi;//
String WeekDayString;
bool WeekDay[7];
String var;

int wifisignal;   
/*
.
.
.
definizione variabili necessarie all'applicativo
.
.
.
*/
bool updating = 0;
unsigned long updateStarted_time = 0;

void setup()
{
  /*
  .
  .
  .
  Setup valori iniziali delle variabili
  .
  .
  */
  
  Serial.begin(115200);
  Serial.println("\Starting");
  Blynk.begin(auth, ssid, pass, server);        // Wifi and  Blynk server Connection
  int mytimeout = millis() / 1000;
  while (Blynk.connect() == false) {            // try to connect to server for 10 seconds
    if ((millis() / 1000) > mytimeout + 8) {    // try local server if not connected within 9 seconds
      break;
    }
  }

  rtc.begin();
/*
.
.
.
Timer intervals settings
OTA 
.
.
.
*/

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

BLYNK_CONNECTED() {                           // Server values sync at 1 connection ?????
  if (isFirstConnect) {
    Blynk.syncAll();

    terminal.println("");
    terminal.println("");
    terminal.print("**************************************");
    PrintLogString(" - Irrigo started |");
    PrintLogString(" - Server synced  |");
    terminal.println();
    terminal.println("**************************************");
    terminal.flush();
    Blynk.notify("TIMER STARTING!!!!");
    isFirstConnect = false;
    digitalWrite(LED_BLU, HIGH);              // blu led on > Blynk connected
  }
}

/*
.
.
.
*/

void loop()
{
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
}

/*
.
.
.
*/
