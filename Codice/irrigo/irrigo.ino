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
#include <SimpleTimer.h>
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
#define VERSION "2.00"

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
//char ssid[] = "em-AndroidAP";       // tablet
//char pass[] = "irrigo123";
long startsecondswd;            // weekday start time in seconds
long stopsecondswd;             // weekday stop  time in seconds
long nowseconds;                // time now in seconds
bool isFirstConnect = true;

String data_fil;
String displaycurrenttimepluswifi;//
String WeekDayString;
bool WeekDay[7];
String var;

int wifisignal;                   // WiFi signal for terminal header
int manual = 0;
int oldstatus;

int ZoneA_AutoFlag;               // = 1 se attivo AUTO = 0 se inattivo
int ZoneB_AutoFlag;
int ZoneC_AutoFlag;
int ZoneD_AutoFlag;               // RFU

bool ZoneA_ValveFlag;             // Status of valve for log print
bool ZoneB_ValveFlag;
bool ZoneC_ValveFlag;
bool ZoneD_ValveFlag;

int SaveZoneA_AutoFlag;            // save state swicking from auto to manual
int SaveZoneB_AutoFlag;
int SaveZoneC_AutoFlag;
int SaveZoneD_AutoFlag;            // RFU

long startA1;            // ora di start e stop Zone A
long stopA1;
bool TodayActiveA1;
long startA2;
long stopA2;
bool TodayActiveA2;

long startB1;            // ora di start e stop Zone B
long stopB1;
bool TodayActiveB1;
long startB2;
long stopB2;
bool TodayActiveB2;

long startC1;            // ora di start e stop Zone C
long stopC1;
bool TodayActiveC1;
long startC2;
long stopC2;
bool TodayActiveC2;

long startD1;            // ora di start e stop Zone D
long stopD1;
bool TodayActiveD1;
long startD2;
long stopD2;
bool TodayActiveD2;

bool updating = 0;
unsigned long updateStarted_time = 0;

void setup()
{
  pinMode(TestLED, OUTPUT);
  pinMode(LED_BLU, OUTPUT);
  pinMode(LED_GIALLO, OUTPUT);
  pinMode(LED_ROSSO, OUTPUT);

  pinMode(ZoneA_valve, OUTPUT);
  pinMode(ZoneB_valve, OUTPUT);
  pinMode(ZoneC_valve, OUTPUT);
  pinMode(ZoneD_valve, OUTPUT);        // RFU

  digitalWrite(TestLED, LOW);          // set LED OFF
  digitalWrite(LED_BLU, LOW);
  digitalWrite(LED_GIALLO, LOW);
  digitalWrite(LED_ROSSO, LOW);

  digitalWrite(ZoneA_valve, LOW);      // Switch OFF elettrovalve
  digitalWrite(ZoneB_valve, LOW);
  digitalWrite(ZoneC_valve, LOW);
  digitalWrite(ZoneD_valve, LOW);      //RFU

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

  rtc.begin();

  ///////////////////////////////////// SET INTERVAL TIMER /////////////////////////////////////////////

  timer.setInterval(10000L, TimeImputSyncAzone);   // check every 10 SECONDS if schedule should run today
  timer.setInterval(10100L, TimeImputSyncBzone);
  timer.setInterval(10200L, TimeImputSyncCzone);
  timer.setInterval(20000L, AutoTiming);
  timer.setInterval(10000L, reconnectBlynk);  // check every ?0s if still connected to server
  timer.setInterval(5000L, clockvalue);       // check value for time
  timer.setInterval(5000L, sendWifi);         // Wi-Fi singal

  ///////////////////////////////////// OTA ///////////////////////////////////////////////////////////

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
    digitalWrite(LED_GIALLO, LOW);
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

/*  if (Blynk.connected()){
    Blynk.syncAll();
    Serial.println("Connesso e sincronizzato");
    terminal.println("================================");
    terminal.println(" Server synced ");
    terminal.flush();
  }*/
  
  //Blynk.syncAll();
}

BLYNK_CONNECTED() {                           // Server values sync at 1 connection ?????
  if (isFirstConnect) {
  //Blynk.syncAll();
  //Blynk.syncVirtual(V0, V1, V2, V3, V10, V11, V12, V20, V21, V22, V30, V31, V32);
  Serial.println("mi sono sincronizzato");
  terminal.println("");
  terminal.println("************************************");
  terminal.println("|  Irrigo started - Server synced  |");
  terminal.println("************************************");
  terminal.flush();
  isFirstConnect = false;
  //Blynk.notify("Irrigo: mi sono connesso!");
  digitalWrite(LED_BLU, HIGH);              // blu led on > Blynk connected
  }
}

void sendWifi()                               // WiFi signal to display on terminal top
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

void TimeImputSyncAzone()                    // check if schedule should run today
{
  long durataAutoUpdateA = 0;
  if (year() != 1970) {
    durataAutoUpdateA = millis();
    if (ZoneA_AutoFlag == 1) {
      Blynk.syncVirtual(V11);           // sync timeinput widget A1
      Blynk.syncVirtual(V12);           // sync timeinput widget A2
    }
    durataAutoUpdateA = millis() - durataAutoUpdateA;
    //Serial.print("A ");
    //Serial.println(durataAutoUpdateA);
  }
}

void TimeImputSyncBzone()                    // check if schedule should run today
{
  long durataAutoUpdateB = 0;
  if (year() != 1970) {
    durataAutoUpdateB = millis();
    if (ZoneB_AutoFlag == 1) {
      Blynk.syncVirtual(V21);           // sync timeinput widget B1
      Blynk.syncVirtual(V22);           // sync timeinput widget B2
    }
    durataAutoUpdateB = millis() - durataAutoUpdateB;
    //Serial.print("B ");
    //Serial.println(durataAutoUpdateB);
  }
}

void TimeImputSyncCzone()                    // check if schedule should run today
{
  long durataAutoUpdateC = 0;
  if (year() != 1970) {
    durataAutoUpdateC = millis();
    if (ZoneC_AutoFlag == 1) {
      Blynk.syncVirtual(V31);           // sync timeinput widget C1
      Blynk.syncVirtual(V32);           // sync timeinput widget C2
    }
    durataAutoUpdateC = millis() - durataAutoUpdateC;
    //Serial.print("C ");
    //Serial.println(durataAutoUpdateC);
  }
}

void AutoTiming() {                                      // open and close valve according to auto setup
  if (ZoneA_AutoFlag == 1) {                             // case A Zone auto is active
    if (TodayActiveA1) {                                 // if A1 actve today
      if (startA1 == 90000) {                            // test for start and stop time inserted
        Serial.println("A1 start time non definito");
      }
      if (stopA1 == 90000) {
        Serial.println("A1 stop time non definito");
      }
      nowseconds = ((hour() * 3600) + (minute() * 60) + second());
      if ((nowseconds >= startA1 && nowseconds <= stopA1)) {     // time to switch ON
        if (!ZoneA_ValveFlag)  {                          // only if no already set
          PrintLogString(" - Zona A - AUTO ON");
          digitalWrite(ZoneA_valve, 1);                    // A valve open
          Blynk.virtualWrite(V1, 1);                       // set Zone valve indicator widget
          ZoneA_ValveFlag = 1;
        }
      }
      else {
        if (ZoneA_ValveFlag && !(nowseconds >= startA2 && nowseconds <= stopA2)) { // time to switch OFF                          // only if already set
          digitalWrite(ZoneA_valve, 0);                    // A valve close
          Blynk.virtualWrite(V1, 0);                       // set Zone valve indicator widget
          PrintLogString(" - Zona A - AUTO OFF");
          ZoneA_ValveFlag = 0;
        }
      }
    }
    if (TodayActiveA2) {
      if (startA2 == 90000) {
        Serial.println("A2 start time non definito");
      }
      if (stopA2 == 90000) {
        Serial.println("A2 stop time non definito");
      }
      nowseconds = ((hour() * 3600) + (minute() * 60) + second());
      if ((nowseconds >= startA2 && nowseconds <= stopA2)) {
        if (!ZoneA_ValveFlag) {                        // only if no already set
          PrintLogString(" - Zona A - AUTO ON");
          digitalWrite(ZoneA_valve, 1);                  // A valve open
          Blynk.virtualWrite(V1, 1);                     // set Zone valve indicator widget
          ZoneA_ValveFlag = 1;
        }
      }
      else {
        if (ZoneA_ValveFlag && !(nowseconds >= startA1 && nowseconds <= stopA1)) {                            // only if already set
          digitalWrite(ZoneA_valve, 0);                  // A valve close
          Blynk.virtualWrite(V1, 0);                     // set Zone valve indicator widget
          PrintLogString(" - Zona A - AUTO OFF");
          ZoneA_ValveFlag = 0;
        }
      }
    }
  }
  if (ZoneB_AutoFlag == 1) {                             // case B Zone auto is active
    if (TodayActiveB1) {
      if (startB1 == 90000) {                           // test for start and stop time inserted
        Serial.println("B1 start time non definito");
      }
      if (stopB1 == 90000) {
        Serial.println("B1 stop time non definito");
      }
      nowseconds = ((hour() * 3600) + (minute() * 60) + second());
      if ((nowseconds >= startB1 && nowseconds <= stopB1)) {
        if (!ZoneB_ValveFlag) {                           // only if no already set
          PrintLogString(" - Zona B - AUTO ON");
          digitalWrite(ZoneB_valve, 1);                  // B valve open
          Blynk.virtualWrite(V2, 1);                     // set Zone valve indicator widget
          ZoneB_ValveFlag = 1;
        }
      }
      else {
        if (ZoneB_ValveFlag && !(nowseconds >= startB2 && nowseconds <= stopB2)) {                            // only if already set
          digitalWrite(ZoneB_valve, 0);                  // B valve close
          Blynk.virtualWrite(V2, 0);                     // set Zone valve indicator widget
          PrintLogString(" - Zona B - AUTO OFF");
          ZoneB_ValveFlag = 0;
        }
      }
    }
    if (TodayActiveB2) {
      if (startB2 == 90000) {
        Serial.println("B2 start time non definito");
      }
      if (stopB2 == 90000) {
        Serial.println("B2 stop time non definito");
      }
      nowseconds = ((hour() * 3600) + (minute() * 60) + second());
      if ((nowseconds >= startB2 && nowseconds <= stopB2)) {
        if (!ZoneB_ValveFlag) {                           // only if no already set
          PrintLogString(" - Zona B - AUTO ON");
          digitalWrite(ZoneB_valve, 1);                  // B valve open
          Blynk.virtualWrite(V2, 1);                     // set Zone valve indicator widget
          ZoneB_ValveFlag = 1;
        }
      }
      else {
        if (ZoneB_ValveFlag && !(nowseconds >= startB1 && nowseconds <= stopB1)) {   // only if already set
          digitalWrite(ZoneB_valve, 0);                  // B valve close
          Blynk.virtualWrite(V2, 0);                     // set Zone valve indicator widget
          PrintLogString(" - Zona B - AUTO OFF");
          ZoneB_ValveFlag = 0;
        }
      }
    }
  }
  if (ZoneC_AutoFlag == 1) {                                 // case C Zone auto is active
    if (TodayActiveC1) {
      if (startC1 == 90000) {                            // test for start and stop time inserted
        Serial.println("C1 start time non definito");
      }
      if (stopC1 == 90000) {
        Serial.println("C1 stop time non definito");
      }
      nowseconds = ((hour() * 3600) + (minute() * 60) + second());
      if ((nowseconds >= startC1 && nowseconds <= stopC1)) {
        if (!ZoneC_ValveFlag) {                           // only if no already set
          PrintLogString(" - Zona C - AUTO ON");
          digitalWrite(ZoneC_valve, 1);                  // C valve open
          Blynk.virtualWrite(V3, 1);                     // set Zone valve indicator widget
          ZoneC_ValveFlag = 1;
        }
      }
      else {
        if (ZoneC_ValveFlag && !(nowseconds >= startC2 && nowseconds <= stopC2)) {                            // only if already set
          digitalWrite(ZoneC_valve, 0);                  // C valve close
          Blynk.virtualWrite(V3, 0);                     // set Zone valve indicator widget
          PrintLogString(" - Zona C - AUTO OFF");
          ZoneC_ValveFlag = 0;
        }
      }
    }
    if (TodayActiveC2) {
      if (startC2 == 90000) {
        Serial.println("C2 start time non definito");
      }
      if (stopC2 == 90000) {
        Serial.println("C2 stop time non definito");
      }
      nowseconds = ((hour() * 3600) + (minute() * 60) + second());
      if ((nowseconds >= startC2 && nowseconds <= stopC2)) {
        if (!ZoneC_ValveFlag) {                           // only if no already set
          PrintLogString(" - Zona C - AUTO ON");
          digitalWrite(ZoneC_valve, 1);                  // C valve open
          Blynk.virtualWrite(V3, 1);                     // set Zone valve indicator widget
          ZoneC_ValveFlag = 1;
        }
      }
      else {
        if (ZoneC_ValveFlag && !(nowseconds >= startC1 && nowseconds <= stopC1)) {                            // only if already set
          digitalWrite(ZoneC_valve, 0);                  // C valve close
          Blynk.virtualWrite(V3, 0);                     // set Zone valve indicator widget
          PrintLogString(" - Zona C - AUTO OFF");
          ZoneC_ValveFlag = 0;
        }
      }
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

BLYNK_WRITE(V9)
{
  if (param.asInt() == 1) {
    Blynk.syncAll();
    Serial.print("sync manuale");
  }
}

BLYNK_WRITE(V0)                                 // Manual/Auto selection
{
  if (param.asInt() == 1) {                     // if widget is ON: MANUAL function selected
    manual = 1;
    digitalWrite(LED_ROSSO, manual);
    SaveAutoMode ();                            // Save AUTO configuration status
    SetAutoFlag(0, 0, 0, 0);                    // set auto flag to OFF for all Zones
    SetValveStatus(LOW, LOW, LOW, LOW);         // Switch OFF valves
    PrintLogString(" - Modo MANUALE");
    //PrintLogString(" - Valvole CHIUSE");
  }

  else {                                         // If widget is OFF:  AUTO function selected
    manual = 0;                                  // set manual flag OFF
    digitalWrite(LED_ROSSO, manual);
    RestoreOldAutoStatus ();                     // restore last saved auto configurtion
    SetValveStatus(LOW, LOW, LOW, LOW);          // Switch OFF elettrovalve
    PrintLogString(" - Modo AUTOMATICO");
    //PrintLogString(" - Valvole CHIUSE");
  }
}

BLYNK_WRITE(V1)                                 // A-Zone elettrovalve manual open/close
{
  if (manual == 0) {                            // current is mode AUTO
    SaveAutoMode ();                            // save AUTO curent mode
    manual = 1;                                 // set mode to Manual
    Blynk.virtualWrite(V0, 1);                  // Turn ON Manual Widget
    SetAutoFlag(0, 0, 0, 0);                    // wsitch OFF AUTO flags and wigget
    PrintLogString(" - Valvole CHIUSE");
    PrintLogString(" - Modo MANUALE");

    if (param.asInt() == 1) {                    // If OPEN acivated
      //      digitalWrite(ZoneA_valve, HIGH);           // set Zones valves OPEN
      //      Blynk.virtualWrite(V1, HIGH);              // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(1, 0, 0, 0);                // Switch ON Zone A valve and OFF all other
      //PrintLogString(" - Valvole CHIUSE");
      PrintLogString(" - A APERTA manuale");
    }
  }
  else {                                         // current is mode MANUAL
    if (param.asInt() == 1) {                    // If OPEN acivated
      //      digitalWrite(ZoneA_valve, HIGH);           // set Zones valves OPEN
      //      Blynk.virtualWrite(V1, HIGH);              // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(1, 0, 0, 0);                // Switch ON Zone A valve and OFF all other
      //PrintLogString(" - C e D CHIUSE");
      PrintLogString(" - A APERTA");
    }
    else {                                       // if CLOSE activated
      //      digitalWrite(ZoneA_valve, LOW);            // set Zones valves CLOSE
      //      Blynk.virtualWrite(V1, LOW);               // set Zones valve indicator widget OFF
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 0, 0, 0);                // Switch ON Zone A valve and OFF all other
      PrintLogString(" - A CHIUSA");
    }
  }
}

BLYNK_WRITE(V2)                                // B Zone manual open/close
{
  if (manual == 0) {                         // current is mode auto
    SaveAutoMode ();                         // save AUTO curent mode
    manual = 1;                              // set mode to Manual
    Blynk.virtualWrite(V0, 1);               // Turn ON Manual Widget
    SetAutoFlag(0, 0, 0, 0);                 // wsitch OFF AUTO flags and wigget

    PrintLogString(" - Modo MANUALE");

    if (param.asInt() == 1) {                     // If OPEN acivated
      //      digitalWrite(ZoneB_valve, HIGH);            // set Zones valves OPEN
      //      Blynk.virtualWrite(V2, HIGH);               // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 1, 0, 0);                // Switch ON Zone A valve and OFF all other
      PrintLogString(" - A e C CHIUSE");
      PrintLogString(" - B APERTA");
    }
  }
  else {                                          // current is mode MANUAL
    if (param.asInt() == 1) {                     // If OPEN acivated
      //      digitalWrite(ZoneB_valve, HIGH);            // set Zones valves OPEN
      //      Blynk.virtualWrite(V2, HIGH);               // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 1, 0, 0);                // Switch ON Zone A valve and OFF all other
      PrintLogString(" - A e C CHIUSE");
      PrintLogString(" - B APERTA");
    }
    else {                                       // if CLOSE activated
      //      digitalWrite(ZoneB_valve, LOW);            // set Zones valves CLOSE
      //      Blynk.virtualWrite(V2, LOW);               // set Zones valve indicator widget OFF
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 0, 0, 0);                // Switch ON Zone A valve and OFF all other
      PrintLogString(" - B CHIUSA");
    }
  }
}

BLYNK_WRITE(V3)                                // C Zone manual open/close
{
  if (manual == 0) {                         // current is mode auto
    SaveAutoMode ();                         // save AUTO curent mode
    manual = 1;                              // set mode to Manual
    Blynk.virtualWrite(V0, 1);               // Turn ON Manual Widget
    SetAutoFlag(0, 0, 0, 0);                 // wsitch OFF AUTO flags and wigget
    PrintLogString(" - Modo MANUALE");

    if (param.asInt() == 1) {                    // If OPEN acivated
      //      digitalWrite(ZoneC_valve, HIGH);           // set Zones valves OPEN
      //      Blynk.virtualWrite(V3, HIGH);              // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 0, 1, 0);                // Switch ON Zone C valve and OFF all other
      PrintLogString(" - A e B CHIUSE");
      PrintLogString(" - C APERTA");
    }
  }
  else {                                         // current is mode MANUAL
    if (param.asInt() == 1) {                    // If OPEN acivated
      //      digitalWrite(ZoneC_valve, HIGH);           // set Zones valves OPEN
      //      Blynk.virtualWrite(V3, HIGH);              // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 0, 1, 0);                // Switch ON Zone C valve and OFF all other
      PrintLogString(" - A e B CHIUSE");
      PrintLogString(" - C APERTA");
    }
    else {                                       // if CLOSE activated
      //      digitalWrite(ZoneC_valve, LOW);            // set Zones valves CLOSE
      //      Blynk.virtualWrite(V3, LOW);               // set Zones valve indicator widget OFF
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 0, 0, 0);                // Switch ON Zone A valve and OFF all other

      PrintLogString(" - C CHIUSA");
    }
  }
}

/* RFU Not in use
  BLYNK_WRITE(V4)                              // D Zone manual open/close
  {
  if (manual == 0) {                         // current is mode AUTO
    SaveAutoMode ();                         // save AUTO curent mode
    manual = 1;                              // set mode to Manual
    Blynk.virtualWrite(V0, 1);               // Turn ON Manual Widget
    SetAutoFlag(0, 0, 0, 0);                 // wsitch OFF AUTO flags and wigget

    PrintLogString(" - Modo MANUALE");

    if (param.asInt() == 1) {                    // If OPEN acivated
      //digitalWrite(ZoneA_valve, HIGH);           // set Zones valves OPEN
      //Blynk.virtualWrite(V4, HIGH);              // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 0, 0, 1);                // Switch ON Zone A valve and OFF all other
      PrintLogString(" - Valvole CHIUSE");
      PrintLogString(" - C APERTA");
    }
  }
  else {                                         // current is mode MANUAL
    if (param.asInt() == 1) {                    // If OPEN acivated
      //digitalWrite(ZoneC_valve, HIGH);           // set Zones valves OPEN
      //Blynk.virtualWrite(V4, HIGH);              // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 0, 0, 1);                // Switch ON Zone A valve and OFF all other
      PrintLogString(" - Valvole CHIUSE");
      PrintLogString(" - C APERTA");
    }
    else {                                       // if CLOSE activated
      //digitalWrite(ZoneA_valve, LOW);            // set Zones valves CLOSE
      //Blynk.virtualWrite(V4, LOW);               // set Zones valve indicator widget OFF
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 0, 0, 0);                // Switch ON Zone A valve and OFF all other

      PrintLogString(" - C CHIUSA");
    }
  }
  }
*/

BLYNK_WRITE(V10)                                            // Zone A auto mode enable/diseble
{
  if (param.asInt() == 1)
  {
    ZoneA_AutoFlag = 1;
    Blynk.virtualWrite(V10, 1);
    Blynk.virtualWrite(V0, 0);                              // Turn OFF Manual Mode Widget

    SetValveStatus(0, 0, 0, 0);                             // Turn OFF all valves and widget
    PrintLogString(" - A AUTO-mode ON");
    //    timer.setTimeout(50, resetTerminal);
    //    timer.setTimeout(50, resetManual);
    //    timer.setTimeout(50, SaveAutoMode);
  }
  else
  {
    ZoneA_AutoFlag = 0;
    Blynk.virtualWrite(V10, 0);
    PrintLogString(" - A AUTO mode OFF");
  }
  manual = 0;
  digitalWrite(LED_ROSSO, manual);
}

BLYNK_WRITE(V20)                                            // Zone B auto mode enable/diseble
{
  if (param.asInt() == 1)
  {
    ZoneB_AutoFlag = 1;
    Blynk.virtualWrite(V20, 1);
    Blynk.virtualWrite(V0, 0);                             // Turn OFF Manual Mode Widget
    digitalWrite(LED_ROSSO, manual);
    SetValveStatus(0, 0, 0, 0);                            // Turn OFF all valves and widget
    PrintLogString(" - B AUTO-mode ON");
    //    timer.setTimeout(50, resetTerminal);
    //    timer.setTimeout(50, resetManual);
    //    timer.setTimeout(50, SaveAutoMode);
  }
  else
  {
    ZoneB_AutoFlag = 0;
    Blynk.virtualWrite(V20, 0);
    PrintLogString(" - B AUTO-mode OFF");
  }
  manual = 0;
  digitalWrite(LED_ROSSO, manual);
}

BLYNK_WRITE(V30)                                             // Zone C auto mode enable/diseble
{
  if (param.asInt() == 1)
  {
    ZoneC_AutoFlag = 1;
    Blynk.virtualWrite(V30, 1);
    Blynk.virtualWrite(V0, 0);
    digitalWrite(LED_ROSSO, manual);
    SetValveStatus(0, 0, 0, 0);                            // Turn OFF all valves and widget
    PrintLogString(" - C AUTO-mode ON");
    //    timer.setTimeout(50, resetTerminal);
    //    timer.setTimeout(50, resetManual);
    //    timer.setTimeout(50, SaveAutoMode);
  }
  else
  {
    ZoneC_AutoFlag = 0;
    Blynk.virtualWrite(V30, 0);
    PrintLogString(" - C AUTO-mode OFF");
  }
  manual = 0;
  digitalWrite(LED_ROSSO, manual);
}

BLYNK_WRITE(V11)                                            // Zone A1 timing input and save
{
  if (ZoneA_AutoFlag == 1) {                                // case A Zone is active
    TimeInputParam t(param);
    int dayadjustment = -1;                                  // Week day adjustment
    if (weekday() == 1) {                                    // Sunday, Time lib is day 1 and Blynk is day 7
      dayadjustment =  6;
    }
    if (t.isWeekdaySelected(weekday() + dayadjustment)) {     // If today is active week day set flag
      TodayActiveA1 = true;                                  // setta flag attivo oggi
      if (t.hasStartTime()) {                                 // if start time is set
        startA1 = (t.getStartHour() * 3600) + (t.getStartMinute() * 60); // salva start time
      }
      else {
        startA1 = 90000;                                             // Stop time not is set
      }
      if (t.hasStopTime()) {
        stopA1 = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);  // salva start time
      }
      else {
        stopA1 = 90000;
      }
    }
    else {
      TodayActiveA1 = false;
    }
  }
}

BLYNK_WRITE(V12)                                            // Zone A2 timing input and save
{
  if (ZoneA_AutoFlag == 1)                                  // case A Zone is active
  {
    TimeInputParam t(param);
    int dayadjustment = -1;                                  // Week day adjustment
    if (weekday() == 1) {                                    // Sunday, Time lib is day 1 and Blynk is day 7
      dayadjustment =  6;
    }
    if (t.isWeekdaySelected(weekday() + dayadjustment))      // If today is active week day set flag                                      // se oggi attivo
    {
      TodayActiveA2 = true;                                  // setta flag attivo oggi
      if (t.hasStartTime())                                  // if start time is set
      {
        startA2 = (t.getStartHour() * 3600) + (t.getStartMinute() * 60); // salva start time
      }
      else
      {
        startA2 = 90000;
      }
      if (t.hasStopTime())
      {
        stopA2 = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);  // salva start time
      }
      else
      {
        stopA2 = 90000;                                             // Stop time not is set
      }
    }
    else
    {
      TodayActiveA2 = false;
    }
  }
}
BLYNK_WRITE(V21)                                            // Zone B1 timing input and save
{
  if (ZoneB_AutoFlag == 1)                                  // case B Zone is active
  {
    TimeInputParam t(param);
    int dayadjustment = -1;                                  // Week day adjustment
    if (weekday() == 1) {                                    // Sunday, Time lib is day 1 and Blynk is day 7
      dayadjustment =  6;
    }
    if (t.isWeekdaySelected(weekday() + dayadjustment))      // If today is active week day set flag                                      // se oggi attivo
    {
      TodayActiveB1 = true;                                  // setta flag attivo oggi
      if (t.hasStartTime())                                  // if start time is set
      {
        startB1 = (t.getStartHour() * 3600) + (t.getStartMinute() * 60); // salva start time
      }
      else
      {
        startB1 = 90000;
      }
      if (t.hasStopTime())
      {
        stopB1 = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);  // salva start time
      }
      else
      {
        stopB1 = 90000;                                             // Stop time not is set
      }
    }
    else
    {
      TodayActiveB1 = false;
    }
  }
}

BLYNK_WRITE(V22)                                            // Zone B2 timing input and save
{
  if (ZoneB_AutoFlag == 1)                                  // case B Zone is active
  {
    TimeInputParam t(param);
    int dayadjustment = -1;                                  // Week day adjustment
    if (weekday() == 1) {                                    // Sunday, Time lib is day 1 and Blynk is day 7
      dayadjustment =  6;
    }
    if (t.isWeekdaySelected(weekday() + dayadjustment))      // If today is active week day set flag                                      // se oggi attivo
    {
      TodayActiveB2 = true;                                  // setta flag attivo oggi
      if (t.hasStartTime())                                  // if start time is set
      {
        startB2 = (t.getStartHour() * 3600) + (t.getStartMinute() * 60); // salva start time
      }
      else
      {
        startB2 = 90000;
      }
      if (t.hasStopTime())
      {
        stopB2 = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);  // salva start time
      }
      else
      {
        stopB2 = 90000;                                             // Stop time not is set
      }
    }
    else
    {
      TodayActiveB2 = false;
    }
  }
}

BLYNK_WRITE(V31)                                             // Zone C1 timing input and save
{
  if (ZoneC_AutoFlag == 1)                                   // case C Zone is active
  {
    TimeInputParam t(param);
    int dayadjustment = -1;                                  // Week day adjustment
    if (weekday() == 1) {                                    // Sunday, Time lib is day 1 and Blynk is day 7
      dayadjustment =  6;
    }
    if (t.isWeekdaySelected(weekday() + dayadjustment))      // If today is active week day set flag                                      // se oggi attivo
    {
      TodayActiveC1 = true;                                  // setta flag attivo oggi
      if (t.hasStartTime())                                  // if start time is set
      {
        startC1 = (t.getStartHour() * 3600) + (t.getStartMinute() * 60); // salva start time
      }
      else
      {
        startC1 = 90000;
      }
      if (t.hasStopTime())
      {
        stopC1 = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);  // salva start time
      }
      else
      {
        stopC1 = 90000;                                             // Stop time not is set
      }
    }
    else
    {
      TodayActiveC1 = false;
    }
  }
}

BLYNK_WRITE(V32)                                            // Zone C2 timing input and save
{
  if (ZoneC_AutoFlag == 1)                                  // case C Zone is active
  {
    TimeInputParam t(param);
    int dayadjustment = -1;                                  // Week day adjustment
    if (weekday() == 1) {                                    // Sunday, Time lib is day 1 and Blynk is day 7
      dayadjustment =  6;
    }
    if (t.isWeekdaySelected(weekday() + dayadjustment))      // If today is active week day set flag                                      // se oggi attivo
    {
      TodayActiveC2 = true;                                 // setta flag attivo oggi
      if (t.hasStartTime())                                  // if start time is set
      {
        startC2 = (t.getStartHour() * 3600) + (t.getStartMinute() * 60); // salva start time
      }
      else
      {
        startC2 = 90000;
      }
      if (t.hasStopTime())
      {
        stopC2 = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);  // salva start time
      }
      else
      {
        stopC2 = 90000;                                             // Stop time not is set
      }
    }
    else
    {
      TodayActiveC2 = false;
    }
  }
}

/* salvato per sostituzione
   BLYNK_WRITE(V11)                                            // Zone A1 timing input
  {
  if (ZoneA_AutoFlag == 1)                                  // case A Zone is active
  {
    //sprintf(Date, "%02d/%02d/%04d",  day(), month(), year());
    //sprintf(Time, "%02d:%02d:%02d", hour(), minute(), second());
    //Serial.print(Date);
    //Serial.print(Time);
    //Serial.println(weekday());

    TimeInputParam t(param);

    int dayadjustment = -1;                                  // Week day adjustment
    if (weekday() == 1) {                                    // Sunday, Time lib is day 1 and Blynk is day 7
      dayadjustment =  6;
    }

    if (t.isWeekdaySelected(weekday() + dayadjustment))      // If today is active week day                                        // se oggi attivo
    {
      WeekDayString = "";                                    // Active days string build
      for (int i = 1; i <= 7; i++) {                         // Process weekdays (1. Mon, 2. Tue, 3. Wed, ...)
        if (t.isWeekdaySelected(i)) {
          switch (i) {
            case 1:
              var = "Lu ";
              break;
            case 2:
              var = "Ma ";
              break;
            case 3:
              var = "Me ";
              break;
            case 4:
              var = "Gi ";
              break;
            case 5:
              var = "Ve ";
              break;
            case 6:
              var = "Sa ";
              break;
            case 7:
              var = "Do ";
              break;
          }
          WeekDayString = WeekDayString + var;
        }
        else {
          WeekDayString = WeekDayString + "-- ";
        }
      }
      // fine weekDayActive

      terminal.println("");
      terminal.print("A1 - Attivo: ");                       // "A1 attivo"
      //terminal.println(".......Test.....");
      terminal.println(WeekDayString);                       // "Lu -- Me Gi Ve -- --"
      terminal.flush();

      if (t.hasStartTime())                                  // if start time is set
      {
        if (t.hasStopTime())                                 // if stop time is set
        {
          terminal.print(String("A1 - Start:  ") + t.getStartHour() + ":" + t.getStartMinute());
          terminal.println(String(" - stop: ") + t.getStopHour() + ":" + t.getStopMinute());
          terminal.flush();

          nowseconds = ((hour() * 3600) + (minute() * 60) + second());
          startsecondswd = (t.getStartHour() * 3600) + (t.getStartMinute() * 60);
          stopsecondswd = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);

          if (nowseconds >= startsecondswd)                  // if now > start
          {
            if (nowseconds <= stopsecondswd)                 // if now < stop
            {
              digitalWrite(ZoneA_valve, 1);                  // A valve open
              Blynk.virtualWrite(V1, 1);
              //PrintLogString("A Aperta Auto");              //  event log
            }
            else                                             // now > stop
            {
              digitalWrite(ZoneA_valve, 0);                  // A valve close
              Blynk.virtualWrite(V1, 0);
            }
            //SetValveStatus(0, 0, 0, 0);                    // A valve close
            //PrintLogString("A Chiusa Auto");              // event log
          }
          else                                               // now < start time
          {
            // fuori da intervallo di tempo programmato valvola rimane chiusa
            // nesun messaggio nessun log
          }
        }
        else                                                 // stop time not set
        {
          terminal.println("STOP non definito");             // terminal msg: attivo senza stop
          terminal.flush();
        }
      }
      else                                                   // start time not set
      {
        terminal.println("START non definito");              // terminal msg: attivo senza start
        terminal.flush();
      }
    }
    else                                                     // not active day
    {
      // nessuna azione
      // nessuna messaggio
    }
  }
  else                                                       // zona non attiva
  {
    // nessun messaggio
  }
  }
  // ============= end of Zone A1 auto open and close timing code ================

*/

void reconnectBlynk() {
  if (!Blynk.connected()) {
    digitalWrite(LED_BLU, LOW);
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



BLYNK_WRITE(V101)                                           // SW reset
{
  if (param.asInt() == 1) {
    terminal.println("");
    terminal.println("");
    terminal.println("!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!");
    sprintf(Time, "%02d:%02d:%02d", hour(), minute(), second());
    terminal.print(Time);
    terminal.println(" - Irrigo is restarting");
    terminal.println("!*!*!*!*!*!*!*!*!*!*!*!*!*!*!*!");
    terminal.flush();
    Blynk.notify("Irrigo - SW reset received");
    delay(5000);
    ESP.restart();
  }
}

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

void SaveAutoMode () {                   // save AUTO flags status setting MANUAL

  if (ZoneA_AutoFlag == 1) {
    SaveZoneA_AutoFlag = 1;
  }
  else {
    SaveZoneA_AutoFlag = 0;
  }
  if (ZoneB_AutoFlag == 1)  {
    SaveZoneB_AutoFlag = 1;
  }
  else {
    SaveZoneB_AutoFlag = 0;
  }
  if (ZoneC_AutoFlag == 1)  {
    SaveZoneC_AutoFlag = 1;
  }
  else {
    SaveZoneC_AutoFlag = 0;
  }
  if (ZoneD_AutoFlag == 1)  {
    SaveZoneD_AutoFlag = 1;
  }
  else {
    SaveZoneD_AutoFlag = 0;
  }
}

void RestoreOldAutoStatus ()                             // ripristina stato auto togliendo manual
{
  ZoneA_AutoFlag = SaveZoneA_AutoFlag;
  Blynk.virtualWrite(V10, String(SaveZoneA_AutoFlag));
  ZoneB_AutoFlag = SaveZoneB_AutoFlag;
  Blynk.virtualWrite(V20, String(SaveZoneB_AutoFlag));
  ZoneC_AutoFlag = SaveZoneC_AutoFlag;
  Blynk.virtualWrite(V30, String(SaveZoneC_AutoFlag));
  //  ZoneD_AutoFlag = SaveZoneD_AutoFlag;
  //  Blynk.virtualWrite(V40, String(SaveZoneD_AutoFlag));

  SetValveStatus(LOW, LOW, LOW, LOW);                     // Switch OFF all elettrovalves
}

void SetValveStatus(bool VA, bool VB, bool VC, bool VD)
{
  digitalWrite(ZoneA_valve, VA);                 // set Zone valves
  Blynk.virtualWrite(V1, VA);                    // set Zone valve indicator widget to corresponding status
  digitalWrite(ZoneB_valve, VB);
  Blynk.virtualWrite(V2, VB);
  digitalWrite(ZoneC_valve, VC);
  Blynk.virtualWrite(V3, VC);
  //    digitalWrite(ZoneD_valve, VD);
  //    Blynk.virtualWrite(V4, MWD);
}

void SetAutoFlag(bool FA, bool FB, bool FC, bool FD)
{
  ZoneA_AutoFlag = FA;
  ZoneB_AutoFlag = FB;
  ZoneC_AutoFlag = FC;
  //    ZoneD_AutoFlag = FD;
  Blynk.virtualWrite(V10, FA);
  Blynk.virtualWrite(V20, FB);
  Blynk.virtualWrite(V30, FC);
  //    Blynk.virtualWrite(V40, FD);
}

void PrintLogString(String S)
{
  terminal.println();
  sprintf(Date, "%02d/%02d/%04d",  day(), month(), year());
  sprintf(Time, "%02d:%02d:%02d", hour(), minute(), second());
  terminal.print(Date);
  terminal.print("-");
  terminal.print(Time);
  terminal.print(S);
  terminal.flush();
}
/*
  void AutoApriChiudi(String ZoneRun)
  {
  if (("start" + ZoneRun) = 90000)                             // test for start and stop time inserted
  {
    Serial.println(ZoneRun + " start time non definito");
  }
  if (("stop" + ZoneRun) = 90000)
  {
    Serial.println(ZoneRun + " stop time non definito");
  }
  nowseconds = ((hour() * 3600) + (minute() * 60) + second());

  if ((nowseconds >= startA1 && nowseconds <= stopA1))
  {
    if (!ZoneA_ValveFlag)                            // only if not set yet
    {
      PrintLogString(" - Zona A - AUTO ON");
      digitalWrite(ZoneA_valve, 1);                  // A valve open
      Blynk.virtualWrite(V1, 1);                     // set Zone valve indicator widget
      ZoneA_ValveFlag = 1;
    }
  }
  else
  {
    if (ZoneA_ValveFlag)
    {
      digitalWrite(ZoneA_valve, 0);                  // A valve close
      Blynk.virtualWrite(V1, 0);                     // set Zone valve indicator widget
      PrintLogString(" - Zona A - AUTO OFF");
      ZoneA_ValveFlag = 0;
    }
  }
  }
*/
