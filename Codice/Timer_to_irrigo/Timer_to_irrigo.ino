
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
//BlynkTimer timer;
WidgetRTC rtc;
WidgetTerminal terminal(V100);

char Date[16];
char Time[16];
char auth[] = "92d1a7e6379a4d49b455278358ef0d65";
char ssid[] = "adircalam2";     // casa
char pass[] = "casa.em.230552";
//char ssid[] = "em-AndroidAP";   // telefono
//char pass[] = "12345678";
//char ssid[] = "BiblioStaff";    // telefono
//char pass[] = "wsx.okn2a";
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

  timer.setInterval(10000L, TimeImputSync);   // check every 10 SECONDS if schedule should run today
  timer.setInterval(30000L, reconnectBlynk);  // check every 30s if still connected to server
  timer.setInterval(5000L, clockvalue);       // check value for time
  timer.setInterval(5000L, sendWifi);         // Wi-Fi singal

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

/*  Blynk.virtualWrite(V110, "Ready\n");
  Blynk.virtualWrite(V110, "IP address: ");
  Blynk.virtualWrite(V110, WiFi.localIP());*/
  
  digitalWrite(LED_GIALLO, LOW);
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


BLYNK_WRITE(V0)                                 // Manual/Auto selection
{
  if (param.asInt() == 1) {                     // if widget is ON: MANUAL function selected
    manual = 1;
    SaveAutoMode ();                            // Save AUTO configuration status
    SetAutoFlag(0, 0, 0, 0);                    // set auto flag to OFF for all Zones
    SetValveStatus(LOW, LOW, LOW, LOW);         // Switch OFF valves
    PrintLogString(" - Attivato MANUALE");
    //PrintLogString(" - Valvole CHIUSE");
  }

  else {                                         // If widget is OFF:  AUTO function selected
    manual = 0;                                  // set manual flag OFF
    RestoreOldAutoStatus ();                     // restore last saved auto configurtion
    SetValveStatus(LOW, LOW, LOW, LOW);          // Switch OFF elettrovalve
    PrintLogString(" - Attivato AUTOMATICO");
    //PrintLogString(" - Valvole CHIUSE");
  }
}

BLYNK_WRITE(V1)                                 // A-Zone elettrovalve manual open/close
{
  if (manual == 0) {                         // current is mode AUTO
    SaveAutoMode ();                         // save AUTO curent mode
    manual = 1;                              // set mode to Manual
    Blynk.virtualWrite(V0, 1);               // Turn ON Manual Widget
    SetAutoFlag(0, 0, 0, 0);                 // wsitch OFF AUTO flags and wigget
    PrintLogString(" - Valvole CHIUSE");
    PrintLogString(" - Attivato MANUALE");

    if (param.asInt() == 1) {                    // If OPEN acivated
      //      digitalWrite(ZoneA_valve, HIGH);           // set Zones valves OPEN
      //      Blynk.virtualWrite(V1, HIGH);              // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(1, 0, 0, 0);                // Switch ON Zone A valve and OFF all other
      //PrintLogString(" - Valvole CHIUSE");
      PrintLogString(" - A APERTA");
    }
  }
  else {                                         // current is mode MANUAL
    if (param.asInt() == 1) {                    // If OPEN acivated
      //      digitalWrite(ZoneA_valve, HIGH);           // set Zones valves OPEN
      //      Blynk.virtualWrite(V1, HIGH);              // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(1, 0, 0, 0);                // Switch ON Zone A valve and OFF all other
      //PrintLogString(" - Valvole CHIUSE");
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

    PrintLogString(" - Attivato MANUALE");

    if (param.asInt() == 1) {                     // If OPEN acivated
      //      digitalWrite(ZoneB_valve, HIGH);            // set Zones valves OPEN
      //      Blynk.virtualWrite(V2, HIGH);               // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 1, 0, 0);                // Switch ON Zone A valve and OFF all other
      PrintLogString(" - Valvole CHIUSE");
      PrintLogString(" - B APERTA");
    }
  }
  else {                                          // current is mode MANUAL
    if (param.asInt() == 1) {                     // If OPEN acivated
      //      digitalWrite(ZoneB_valve, HIGH);            // set Zones valves OPEN
      //      Blynk.virtualWrite(V2, HIGH);               // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 1, 0, 0);                // Switch ON Zone A valve and OFF all other
      PrintLogString(" - Valvole CHIUSE");
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
    PrintLogString(" - Attivato MANUALE");

    if (param.asInt() == 1) {                    // If OPEN acivated
      //      digitalWrite(ZoneC_valve, HIGH);           // set Zones valves OPEN
      //      Blynk.virtualWrite(V3, HIGH);              // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 0, 1, 0);                // Switch ON Zone C valve and OFF all other
      PrintLogString(" - Valvole CHIUSE");
      PrintLogString(" - C APERTA");
    }
  }
  else {                                         // current is mode MANUAL
    if (param.asInt() == 1) {                    // If OPEN acivated
      //      digitalWrite(ZoneC_valve, HIGH);           // set Zones valves OPEN
      //      Blynk.virtualWrite(V3, HIGH);              // set Zones valve indicator widget ON
      // In caso di accensione di una sola elettrvalvola alla volta sostituire le due righe sopra con la seguente
      SetValveStatus(0, 0, 1, 0);                // Switch ON Zone C valve and OFF all other
      PrintLogString(" - Valvole CHIUSE");
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

/* Not in use
  BLYNK_WRITE(V4)                              // D Zone manual open/close
  {
  if (manual == 0) {                         // current is mode AUTO
    SaveAutoMode ();                         // save AUTO curent mode
    manual = 1;                              // set mode to Manual
    Blynk.virtualWrite(V0, 1);               // Turn ON Manual Widget
    SetAutoFlag(0, 0, 0, 0);                 // wsitch OFF AUTO flags and wigget

    PrintLogString(" - Attivato MANUALE");

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

BLYNK_WRITE(V10)                                            // Attiva/Disattiva Zone A
{
  if (param.asInt() == 1)
  {
    ZoneA_AutoFlag = 1;
    Blynk.virtualWrite(V10, 1);

    PrintLogString(" - A AUTO-mode ON");
    //    timer.setTimeout(50, resetTerminal);
    timer.setTimeout(50, resetManual);
    //    timer.setTimeout(50, SaveAutoMode);
  }
  else
  {
    ZoneA_AutoFlag = 0;
    Blynk.virtualWrite(V10, 0);

    PrintLogString(" - A AUTO mode OFF");
  }
}

BLYNK_WRITE(V20)                                            // Attiva/Disattiva Zone B
{
  if (param.asInt() == 1)                          
  {
    PrintLogString(" - B AUTO-mode ON");

    //    timer.setTimeout(50, resetTerminal);
    timer.setTimeout(50, resetManual);
    //    timer.setTimeout(50, SaveAutoMode);
    ZoneB_AutoFlag = 1;
    Blynk.virtualWrite(V20, 1);
  }
  else
  {
    ZoneB_AutoFlag = 0;
    Blynk.virtualWrite(V20, 0);

    PrintLogString(" - B AUTO-mode OFF");
  }
}

BLYNK_WRITE(V30)                             // Attiva/Disattiva Zone C
{
  if (param.asInt() == 1)
  {
    PrintLogString(" - C AUTO-mode ON");

    //    timer.setTimeout(50, resetTerminal);
    timer.setTimeout(50, resetManual);
    //    timer.setTimeout(50, SaveAutoMode);
    ZoneC_AutoFlag = 1;
    Blynk.virtualWrite(V30, 1);
  } else
  {
    ZoneC_AutoFlag = 0;
    Blynk.virtualWrite(V30, 0);

    PrintLogString(" - C AUTO-mode OFF");
  }
}

BLYNK_WRITE(V11)                                      // Zone A2 timing input
{
  if (ZoneA_AutoFlag == 1)                            // case A Zone is active
  {
    sprintf(Date, "%02d/%02d/%04d",  day(), month(), year());
    sprintf(Time, "%02d:%02d:%02d", hour(), minute(), second());
    Serial.print(Date);
    Serial.print(Time);
    Serial.println(weekday());
    PrintLogString(Time);

    TimeInputParam t(param);

    int dayadjustment = -1;                           // Week day adjustment
    if (weekday() == 1) {                             // Sunday, Time lib is day 1 and Blynk is day 7
      dayadjustment =  6;
    }

    if (t.isWeekdaySelected(weekday() + dayadjustment)) {   // If today is active day
      if (t.hasStartTime())                                 // If start time set Process it
      {
        /*        terminal.println();
                terminal.print(String("A1 -  start: ") + t.getStartHour() + ":" + t.getStartMinute());
                terminal.flush();*/
      }
      if (t.hasStopTime())                                  // IF stop time set Process it
      {
        /*        terminal.println(String("    stop: ") + t.getStopHour() + ":" + t.getStopMinute());
                terminal.flush();*/
      }

      // WeekDayActive();

      char WeekDay[7] = {'Lu','Ma','M','G','V','S','D'};
      WeekDayString = "";                                  // active days string building
      for (int i = 1; i <= 7; i++) {                       // Process weekdays (1 Mon, 2 Tue, 3 Wed, ...)
        WeekDayString = WeekDayString + WeekDay[i];
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
      /*      terminal.println(String("A1 - attiva: ") + WeekDayString);
            terminal.flush();*/
    }

    // Valve open/close according start and stop time selected

    nowseconds = ((hour() * 3600) + (minute() * 60) + second());
    startsecondswd = (t.getStartHour() * 3600) + (t.getStartMinute() * 60);
    if (nowseconds >= startsecondswd)
    {
      if (nowseconds <= startsecondswd + 90) {           // 90s on 60s timer ensures 1 trigger command is sent

        //digitalWrite(TestLED, HIGH);                   // set LED ON
        Blynk.virtualWrite(V1, 1);                       // set A valve and widget ON
        digitalWrite(ZoneA_valve, HIGH);
        PrintLogString(" - A1 ATTIVATO");
      }
    }
    else {
      //    terminal.println("Zona A NON ATTIVATO oggi");
      //    terminal.flush();
    }
    stopsecondswd = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);
    //Serial.println(stopsecondswd);                          // used for debugging
    if (nowseconds >= stopsecondswd)                          // Close time elapsed
    {
      //digitalWrite(TestLED, LOW);
      Blynk.virtualWrite(V1, 0);

      if (nowseconds <= stopsecondswd + 90) { // 90s on 60s timer ensures 1 trigger command is sent
        digitalWrite(TestLED, LOW); // set LED OFF
        Blynk.virtualWrite(V1, 0);
        PrintLogString("Spengo A AUTO");
        // code here to switch the relay OFF
      }
    }
    else                                                      // Time to open valve
    {
      if (nowseconds >= startsecondswd) {
        digitalWrite(TestLED, HIGH); // set LED ON
        Blynk.virtualWrite(V1, 1);
        PrintLogString("Apro A AUTO");
      }
    }
  }
  else
  { // case A Zone NOT active
    PrintLogString("A Zone AUTO not active");
    // nothing to do today, check again in 30 SECONDS time
  }
}


BLYNK_WRITE(V12)                                      // Zone A2 timing input
{
  if (ZoneA_AutoFlag == 1)                            // case A Zone is active
  {
    sprintf(Date, "%02d/%02d/%04d",  day(), month(), year());
    sprintf(Time, "%02d:%02d:%02d", hour(), minute(), second());
    Serial.print(Date);
    Serial.print(Time);
    Serial.println(weekday());

    TimeInputParam t(param);

    int dayadjustment = -1;               // Week day adjustment
    if (weekday() == 1) {                 // Sunday, Time lib is day 1 and Blynk is day 7
      dayadjustment =  6;
    }

    if (t.isWeekdaySelected(weekday() + dayadjustment)) {   // If today is active day
      if (t.hasStartTime())                                 // If start time is set Process it
      {
        terminal.print(String("A2 -  start: ") + t.getStartHour() + ":" + t.getStartMinute());
        terminal.flush();
      }
      if (t.hasStopTime())                                  // IF stop time is set Process it
      {
        terminal.println(String("    stop: ") + t.getStopHour() + ":" + t.getStopMinute());
        terminal.flush();
      }


      // WeekDayActive();

      WeekDayString = "";                                  // active days string building
      for (int i = 1; i <= 7; i++) {  // Process weekdays (1. Mon, 2. Tue, 3. Wed, ...)
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
      terminal.println(String("A2 - attiva: ") + WeekDayString);
      terminal.flush();
    }

    // Valve open/close according start and stop time selected

    nowseconds = ((hour() * 3600) + (minute() * 60) + second());
    startsecondswd = (t.getStartHour() * 3600) + (t.getStartMinute() * 60);
    if (nowseconds >= startsecondswd)
    {
      //      terminal.print("Zona A apre alle: ");
      //      terminal.print(String(" ") + t.getStartHour() + ":" + t.getStartMinute());
      //      terminal.flush();
      if (nowseconds <= startsecondswd + 90) {  // 90s on 60s timer ensures 1 trigger command is sent

        //digitalWrite(TestLED, HIGH);                   // set LED ON
        Blynk.virtualWrite(V1, 1);                       // set A valve and widget ON
        digitalWrite(ZoneA_valve, HIGH);
        PrintLogString(" - A1 ATTIVATO");
      }
    }
    else {
      //    terminal.println("Zona A NON ATTIVATO oggi");
      //    terminal.flush();
    }
    stopsecondswd = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);
    //Serial.println(stopsecondswd);  // used for debugging
    if (nowseconds >= stopsecondswd) {
      digitalWrite(TestLED, LOW); // set LED OFF
      Blynk.virtualWrite(V1, 0);

      if (nowseconds <= stopsecondswd + 90) { // 90s on 60s timer ensures 1 trigger command is sent
        digitalWrite(TestLED, LOW); // set LED OFF
        Blynk.virtualWrite(V1, 0);
        PrintLogString("Spengo A AUTO");
        // code here to switch the relay OFF
      }
    }
    else {
      if (nowseconds >= startsecondswd) {
        digitalWrite(TestLED, HIGH); // set LED ON
        Blynk.virtualWrite(V1, 1);
        PrintLogString("Apro A AUTO");
      }
    }
  }
  else
  { // case A Zone NOT active
    terminal.println("Zona A oggi INACTIVE");
    terminal.flush();
    // nothing to do today, check again in 30 SECONDS time
  }
}



BLYNK_WRITE(V21)                      // Zone B1 timing
{
  if (ZoneB_AutoFlag == 1)                         // Case Zone B active
  {

  }

}

BLYNK_WRITE(V22)                      // Zone B2 timing
{
  if (ZoneB_AutoFlag == 1)                         // se A attivo
  {

  }

}


BLYNK_WRITE(V31)                                   // Zone C1 timing
{
  if (ZoneC_AutoFlag == 1)                         // se C attivo
  {
    sprintf(Date, "%02d/%02d/%04d",  day(), month(), year());
    sprintf(Time, "%02d:%02d:%02d", hour(), minute(), second());
    Serial.print(Date);
    Serial.print(Time);
    Serial.println(weekday());
    PrintLogString(Time);

    TimeInputParam t(param);
    // salva valori impostati
  }
  else
  { // se C non attivo
    // non si fa nulla
  }


}

BLYNK_WRITE(V32)                      // Zone C2 timing
{
  if (ZoneC_AutoFlag == 1)                         // se C attivo
  {

  }


}

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

/*String WeekDayActive()
  {
  WeekDayString = "";
  for (int i = 1; i <= 7; i++) {  // Process weekdays (1. Mon, 2. Tue, 3. Wed, ...)
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
  Serial.println(WeekDayString);
  return WeekDayString;
  }
*/

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
  terminal.write(Time);
  terminal.print(S);
  terminal.flush();
}

/*String WeekDayActive()
  {
  WeekDayString = "";
  for (int i = 1; i <= 7; i++) {  // Process weekdays (1. Mon, 2. Tue, 3. Wed, ...)
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
  Return WeekDayString;
  }
*/

