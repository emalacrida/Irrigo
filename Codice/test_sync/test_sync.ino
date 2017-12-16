/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest
  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.
    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app
  Blynk library is licensed under MIT license
  This example code is in public domain.
 *************************************************************
  You can synchronize the state of widgets with hardware states,
  even if hardware resets or looses connection temporarily
  Project setup in the Blynk app:
    Slider widget (0...1024) on V0
    Value display (0...1024) on V2
    Button widget on digital pin (connected to an LED)
 *************************************************************/

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <Blynk.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>          //for OTA
#include <WiFiUdp.h>              //for OTA
#include <ArduinoOTA.h>           //for OTA
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <SPI.h>


#define server "blynk-cloud.com"   // or "blynk.cloud-com" for Blynk's cloud server
#define TestLED 2                  // on board LED pin assignment

#define HOSTNAME "test_sync"
#define VERSION "1.00"

BlynkTimer timer;
WidgetRTC rtc;

char auth[] = "b72c10d38b834d82ba274703ced16a51"; //test su @csbno.net
//char ssid[] = "adircalam2";     // casa
//char pass[] = "casa.em.230552";
char ssid[] = "emtabAP";     // casa
char pass[] = "irrigo2305";

//char data_fil[0xff];
String data_fil;

int i = 0;

long startA1;            // ora di start e stop Zone A
long stopA1;
bool TodayActiveA1;
long startA2;
long stopA2;
bool TodayActiveA2;

// This function will run every time Blynk connection is established
BLYNK_CONNECTED()
{
  // Request Blynk server to re-send latest values for all pins
  Serial.println("Prima di syncall");
  Blynk.syncAll();
  Serial.println("Dopo di syncall");

  // You can also update individual virtual pins like this:
  //Blynk.syncVirtual(V0, V2);

  // Let's write your hardware uptime to Virtual Pin 2
  int value = millis() / 1000;
  Blynk.virtualWrite(V2, value);
}

/*BLYNK_CONNECTED() {                           // Server values sync at 1 connection ?????
  if (isFirstConnect) {
    Blynk.syncAll();
    //Blynk.syncVirtual(V0, V1, V2, V3, V10, V11, V12, V20, V21, V22, V30, V31, V32);
    Serial.println("mi sono sincronizzato");
    Serial.println("");
    Serial.println("************************************");
    Serial.println("|  Irrigo started - Server synced  |");
    Serial.println("************************************");
    //terminal.flush();
    isFirstConnect = false;
    //Blynk.notify("Irrigo: mi sono connesso!");
    digitalWrite(LED_BLU, HIGH);              // blu led on > Blynk connected
  }
  }*/

BLYNK_WRITE(V0)
{
  //if (param.asInt() == 1) {
    // Use of syncAll() will cause this function to be called
    // Parameter holds last slider value
    int sliderValue0 = param.asInt();
    Serial.print("Slider value: ");
    Serial.println(sliderValue0);
    Blynk.virtualWrite(V2, sliderValue0);
    i = sliderValue0;
    //  buildstring(String(sliderValue0));
  //}
}

BLYNK_WRITE(V1) {
  buildstring("B");
}

BLYNK_WRITE(V3) {
  Serial.println("Mando la mail");
  Blynk.email("notifica da test", "Hanno premuto un tasto");
}


/*
BLYNK_WRITE(V2) {
  // You'll get uptime value here as result of syncAll()
  int uptime = param.asInt();
  Serial.print("Uptime value: ");
  Serial.println(uptime);
}
*/
void setup()
{
  // Debug console
  Serial.begin(115200);
  Serial.println("Starting");

  Blynk.begin(auth, ssid, pass, server);
  /*  int mytimeout = millis() / 1000;
    while (Blynk.connect() == false) {            // try to connect to server for 10 seconds
      if ((millis() / 1000) > mytimeout + 8) {    // try local server if not connected within 9 seconds
        break;
      }
  */


  buildstring("A");
  Serial.println("Prima di rtcbeg");
  rtc.begin();
  Serial.println("Dopo di rtcbeg");
  // timer.setInterval(10000L, buildstring("A"));
}

void loop()
{
  Blynk.run();
  timer.run();
}

void buildstring(String runname){
  data_fil = "parte costante" + runname;
  //strcat(data_fil, "parte 1 di stringa ");
  //strcat(data_fil, runname);
  Serial.println("pippo " + runname + i);
  i = i + 1;
}

