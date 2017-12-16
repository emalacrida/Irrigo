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

/*
.
.
.
.
*/

  ///////////////////////////////////// SET INTERVAL TIMER /////////////////////////////////////////////
  rtc.begin();


  timer.setInterval(10000L, TimeImputSyncAzone);   // check every 10 SECONDS if schedule should run today
  timer.setInterval(10100L, TimeImputSyncBzone);
  timer.setInterval(10200L, TimeImputSyncCzone);
  timer.setInterval(20000L, AutoTiming);
  timer.setInterval(10000L, reconnectBlynk);  // check every ?0s if still connected to server
  timer.setInterval(5000L, clockvalue);       // check value for time
  timer.setInterval(5000L, sendWifi);         // Wi-Fi singal

 /*
 .
 .
 .
 OTA
 .
 .
 .
 */
 

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
  if (year() != 1970)
  {
    durataAutoUpdateA = millis();
    if (ZoneA_AutoFlag == 1) {
      Blynk.syncVirtual(V11);           // sync timeinput widget A1
      Blynk.syncVirtual(V12);           // sync timeinput widget A2
    }
    durataAutoUpdateA = millis() - durataAutoUpdateA;
    Serial.print("A ");
    Serial.println(durataAutoUpdateA);
  }
}

void TimeImputSyncBzone()                    // check if schedule should run today
{
  long durataAutoUpdateB = 0;
  if (year() != 1970)
  {
    durataAutoUpdateB = millis();
    if (ZoneB_AutoFlag == 1) {
      Blynk.syncVirtual(V21);           // sync timeinput widget B1
      Blynk.syncVirtual(V22);           // sync timeinput widget B2
    }
    durataAutoUpdateB = millis() - durataAutoUpdateB;
    Serial.print("B ");
    Serial.println(durataAutoUpdateB);
  }
}

void TimeImputSyncCzone()                    // check if schedule should run today
{
  long durataAutoUpdateC = 0;
  if (year() != 1970)
  {
    durataAutoUpdateC = millis();
    if (ZoneC_AutoFlag == 1) {
      Blynk.syncVirtual(V31);           // sync timeinput widget C1
      Blynk.syncVirtual(V32);           // sync timeinput widget C2
    }
    durataAutoUpdateC = millis() - durataAutoUpdateC;
    Serial.print("C ");
    Serial.println(durataAutoUpdateC);
  }
}

void AutoTiming()                               // open and close valve according to auto setup
{
  if (ZoneA_AutoFlag == 1)                            // case A Zone auto is active
  {
    /*  for debug
        Serial.print("Zona A1: ");
        Serial.print(startA1, DEC);
        Serial.print("  -  ");
        Serial.println(stopA1, DEC);

        Serial.print("Zona A2: ");
        Serial.print(startA2, DEC);
        Serial.print("  -  ");
        Serial.println(stopA2, DEC);
    */

    if (startA1 == 90000)                             // test for start and stop time inserted
    {
      Serial.println("A1 start time non definito");
    }
    if (stopA1 == 90000)
    {
      Serial.println("A1 stop time non definito");
    }
    if (startA2 == 90000)
    {
      Serial.println("A2 start time non definito");
    }
    if (stopA2 == 90000)
    {
      Serial.println("A1 stop time non definito");
    }

    nowseconds = ((hour() * 3600) + (minute() * 60) + second());

    if ((nowseconds >= startA1 && nowseconds <= stopA1) || (nowseconds >= startA2 && nowseconds <= stopA2))
    {
      if (!ZoneA_ValveFlag)
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

  if (ZoneB_AutoFlag == 1)                                  // case B Zone auto is active
  {
    /*  for debu
      Serial.print("Zona B1: ");
      Serial.print(startB1, DEC);
      Serial.print("  -  ");
      Serial.println(stopB1, DEC);

      Serial.print("Zona B2: ");
      Serial.print(startB2, DEC);
      Serial.print("  -  ");
      Serial.println(stopB2, DEC);
    */
    if (startB1 == 90000)                             // test for start and stop time inserted
    {
      Serial.println("B1 start time non definito");
    }
    if (stopB1 == 90000)
    {
      Serial.println("B1 stop time non definito");
    }
    if (startB2 == 90000)
    {
      Serial.println("B2 start time non definito");
    }
    if (stopB2 == 90000)
    {
      Serial.println("B2 stop time non definito");
    }

    nowseconds = ((hour() * 3600) + (minute() * 60) + second());

    if ((nowseconds >= startB1 && nowseconds <= stopB1) || (nowseconds >= startB2 && nowseconds <= stopB2))
    {
      if (!ZoneB_ValveFlag)
      {
        PrintLogString(" - Zona B - AUTO ON");
        digitalWrite(ZoneB_valve, 1);                  // B valve open
        Blynk.virtualWrite(V2, 1);                     // set Zone valve indicator widget
        ZoneB_ValveFlag = 1;
      }
    }
    else
    {
      if (ZoneB_ValveFlag)
      {
        digitalWrite(ZoneB_valve, 0);                  // B valve close
        Blynk.virtualWrite(V2, 0);                     // set Zone valve indicator widget
        PrintLogString(" - Zona B - AUTO OFF");
        ZoneB_ValveFlag = 0;
      }
    }

  }
  if (ZoneC_AutoFlag == 1)                                  // case C Zone auto is active
  {
    /* for debug
      Serial.print("Zona C1: ");
      Serial.print(startC1, DEC);
      Serial.print("  -  ");
      Serial.println(stopC1, DEC);

      Serial.print("Zona C2: ");
      Serial.print(startC2, DEC);
      Serial.print("  -  ");
      Serial.println(stopC2, DEC);
    */
    if (startC1 == 90000)                             // test for start and stop time inserted
    {
      Serial.println("C1 start time non definito");
    }
    if (stopC1 == 90000)
    {
      Serial.println("C1 stop time non definito");
    }
    if (startC2 == 90000)
    {
      Serial.println("C2 start time non definito");
    }
    if (stopC2 == 90000)
    {
      Serial.println("C1 stop time non definito");
    }

    nowseconds = ((hour() * 3600) + (minute() * 60) + second());

    if ((nowseconds >= startC1 && nowseconds <= stopC1) || (nowseconds >= startC2 && nowseconds <= stopC2))
    {
      if (!ZoneC_ValveFlag)
      {
        PrintLogString(" - Zona C - AUTO ON");
        digitalWrite(ZoneC_valve, 1);                  // C valve open
        Blynk.virtualWrite(V3, 1);                     // set Zone valve indicator widget
        ZoneC_ValveFlag = 1;
      }
    }
    else
    {
      if (ZoneC_ValveFlag)
      {
        digitalWrite(ZoneC_valve, 0);                  // C valve close
        Blynk.virtualWrite(V3, 0);                     // set Zone valve indicator widget
        PrintLogString(" - Zona C - AUTO OFF");
        ZoneC_ValveFlag = 0;
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

/*
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
