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

/*
.
.
.
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
/////////////////////
BLYNK_WRITE(V11)                                            // Zone A1 timing input and save
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
      TodayActiveA1 = true;                                  // setta flag attivo oggi
      if (t.hasStartTime())                                  // if start time is set
      {
        startA1 = (t.getStartHour() * 3600) + (t.getStartMinute() * 60); // salva start time
      }
      else
      {
        startA1 = 90000;                                             // Stop time not is set
      }
      if (t.hasStopTime())
      {
        stopA1 = (t.getStopHour() * 3600) + (t.getStopMinute() * 60);  // salva start time
      }
      else
      {
        stopA1 = 90000;
      }
    }
    TodayActiveA1 = false;
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
      TodayActiveA2 = true;                                 // setta flag attivo oggi
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
      TodayActiveC2 = false;
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

/*
.
.
.
.
*/