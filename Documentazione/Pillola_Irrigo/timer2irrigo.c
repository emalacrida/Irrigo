
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
char auth[] = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
char ssid[] = "xxxxxxxxxxxxxxx";     // casa
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

/*
.
.
Definizione variabili locali
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
Setup valori iniziali varibili
Abilitazione monitor seriale
Connessione WiFi e Blynk
.
.
*/
  rtc.begin();

  timer.setInterval(10000L, TimeImputSync);   // check every 10 SECONDS if schedule should run today
  timer.setInterval(30000L, reconnectBlynk);  // check every 30s if still connected to server
  timer.setInterval(5000L, clockvalue);       // check value for time
  timer.setInterval(5000L, sendWifi);         // Wi-Fi singal

/*
.
.
OTA
.
.
*/
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

void sendWifi()                               //
{
  wifisignal = map(WiFi.RSSI(), -105, -40, 0, 100);
}

void clockvalue()                             // Digital clock display of the time on terminal top
{
  int gmthour = hour();
  if (gmthour == 24) {
    gmthour = 0;
  }
  String displayhour =   String(gmthour, DEC);
  int hourdigits = displayhour.length();
  if (hourdigits == 1) {
    displayhour = "0" + displayhour;
  }
  String displayminute = String(minute(), DEC);
  int minutedigits = displayminute.length();
  if (minutedigits == 1) {
    displayminute = "0" + displayminute;
  }
  displaycurrenttimepluswifi = "                                          Clock:  " + displayhour + ":" + displayminute + "               Signal:  " + wifisignal + " %";
  Blynk.setProperty(V100, "label", displaycurrenttimepluswifi);
}

void TimeImputSync() {                  // check if schedule should run today
  if (year() != 1970) {
    if (ZoneA_AutoFlag == 1) {
      Blynk.syncVirtual(V11);         // sync timeinput widget A1
      Blynk.syncVirtual(V12);         // sync timeinput widget A2
    }
    if (ZoneB_AutoFlag == 1) {
      Blynk.syncVirtual(V21);         // sync timeinput widget B1
      Blynk.syncVirtual(V22);         // sync timeinput widget B2
    }
    if (ZoneC_AutoFlag == 1) {
      Blynk.syncVirtual(V31);         // sync timeinput widget C1
      Blynk.syncVirtual(V32);         // sync timeinput widget C2
    }
  }
}


void resetTerminal()               // non piu usato ???????????????
{
  terminal.println();
  terminal.println();
  terminal.println();
  terminal.println("Selezionato nuovo MODO");
  terminal.println("Attendi aggiornamento (10 seconds as maximum)");
  terminal.println();
  terminal.println();
  terminal.println();
  terminal.flush();
}

void resetManual()
{
  Blynk.virtualWrite(V0, 0);                    // Turn OFF Manual Mode Widget
  SetValveStatus(0, 0, 0, 0);                   // Turn OFF all valves and widget
  PrintLogString(" - Valvole CHIUSE");
}


/*
.
.
Codice gestione aperture e chiusura valvole
.
.
*/

void reconnectBlynk() {
  if (!Blynk.connected()) {
    if (Blynk.connect())
    {
      BLYNK_LOG("Reconnected");
      digitalWrite(LED_BLU, HIGH);
    }
    else
    {
      BLYNK_LOG("Not reconnected");
      digitalWrite(LED_BLU, LOW);
    }
  }
}

BLYNK_WRITE(V101)              // SW reset
{
  terminal.println("");
  terminal.println("");
  terminal.println("!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!");
  sprintf(Time, "%02d:%02d:%02d", hour(), minute(), second());
  terminal.print(Time);
  terminal.println(" - Irrigo is restarting");
  terminal.println("!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!");
  terminal.flush();
  delay(5000);
  ESP.restart();
}

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
}

/*
.
.
.
Funzioni locali
.
.
*/
