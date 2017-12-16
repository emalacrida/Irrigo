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

  ///////////////////////////////////// SET INTERVAL TIMER /////////////////////////////////////////////
  rtc.begin();


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
}

BLYNK_CONNECTED() {                           // Server values sync at 1 connection ?????
  if (isFirstConnect) {
    Blynk.syncAll();
    //Blynk.syncVirtual(V10, V11, V12, V20, V21,V22, V30, V31, V32);
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

/*
.
.
.
*/

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

/*
.
.
.
.
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
