/*  Sistema di controllo della irrigazione via internet attraverso Blynk basato su NodeMcu (ESP8266
 *  
 *  D8 elettrovalvola 1 circuito A
 *  D7 elettrovalvola 2 circuito B
 *  D7 elettrovalvola 3 circuito C
 *  D7 elettrovalvola 4 circuito D
 *  
 *  D4 Led biltin e led rosso TBD
 *  D3 led giallo             Connessione WIFi
 *  D2 led blu                TBD
 *  
 */
 
//#define BLYNK_DEBUG           // slowdown x10 operation more detailed print
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space

#include <time.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Ethernet.h>
#include <NTPtime.h>
//#include <BlynkSimpleEthernet.h> //!!!ATTENZIONE DA ERRORE COMP!!!
#include <SimpleTimer.h>
#include <SPI.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
//#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>

// HW resources map
                        //D0
                        //D1 
#define LED_BLU 04      //D2
#define LED_GIALLO 0    //D3
#define LED_ROSSO 02    //D4 builtin led e rosso 
#define ZONA_D 14       //D5-elettrovalvola 4                        
#define ZONA_C 12       //D6-elettrovalvola 3
#define ZONA_B 13       //D7-elettrovalvola 2
#define ZONA_A 15       //D8-elettrovalvola 1
                        //D9
                        //D10


// Specific installation setup parameters ======================
#define TIMEZONE 1
#define DAYLIGHTSAVINGTIME 0
//#define thingspeakAPIkey "xxx"
#define AUTH "92d1a7e6379a4d49b455278358ef0d65"   //Token Blynk Irrigo
#define SID  "AndroidAP"     // wifi SSID
#define PAS  "wezh2722"

SimpleTimer timer;
WidgetRTC rtc;

void connectWiFi(const char* ssid = SID, const char* pass = PAS, int timeout = 10);
void readNTP();
void clockDisplay();
void interruptHandler();
//void handleRoot();
//void timefromblynk();





void setup() 
{

// Timer intervals definitions
  timer.setInterval(6000000L, UpdateTime);           //ogni 10 minuti
  timer.setInterval(5000L, clockDisplay);
  //timer.setInterval(10000L, timefromblynk);
  //timer.setInterval(15000L, MainLoop);      
  
  pinMode(LED_ROSSO, OUTPUT);
  pinMode(LED_GIALLO, OUTPUT);
  pinMode(LED_BLU, OUTPUT);
  pinMode(ZONA_A, OUTPUT);
  pinMode(ZONA_B, OUTPUT);
  pinMode(ZONA_C, OUTPUT);
  pinMode(ZONA_D, OUTPUT);

//  digitalWrite(LED_GIALLO, LOW);
//  digitalWrite(LED_BLU, LOW);
//  digitalWrite(LED_ROSSO, LOW);
  digitalWrite(ZONA_A, LOW);
  digitalWrite(ZONA_B, LOW);
  digitalWrite(ZONA_C, LOW);
  digitalWrite(ZONA_D, LOW);


  Serial.begin(115200);
  connectWiFi();
//  Blynk.config(AUTH);
//  rtc.begin();
  UpdateTime();      //inizializza valore di dta e ora da NTP

}

void loop() 
{
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
/*    digitalWrite(LED_GIALLO, HIGH);
    delay(500);
    digitalWrite(LED_GIALLO, LOW);
    delay(500);*/
//  Blynk.run();
  timer.run();
//  Blynk.run();

/*/  Codice per test connessioni led e rele'
    digitalWrite(LED_GIALLO, HIGH);
    delay(500);
    digitalWrite(LED_GIALLO, LOW);
    delay(500);
    digitalWrite(LED_BLU, HIGH);
    delay(500);
    digitalWrite(LED_BLU, LOW);
    delay(500);
    digitalWrite(LED_ROSSO, HIGH);
    delay(500);
    digitalWrite(LED_ROSSO, LOW);
    delay(500);
/*    digitalWrite(ZONA_A, HIGH);
    delay(1000);
    digitalWrite(ZONA_B, HIGH);
    delay(1000);
    digitalWrite(ZONA_C, HIGH);
    delay(1000);
    digitalWrite(ZONA_D, HIGH);
    delay(1000);
    digitalWrite(ZONA_A, LOW);
    delay(1000);
    digitalWrite(ZONA_B, LOW);
    delay(1000);
    digitalWrite(ZONA_C, LOW);
    delay(1000);
    digitalWrite(ZONA_D, LOW);
    delay(1000);*/
}

void connectWiFi(const char* ssid, const char* pass, int timeout)
{
  int timeoutCounter = 0;
  while (WiFi.status() != WL_CONNECTED) 
  {
    timeoutCounter = 0;
    static bool LEDstatus = HIGH;
    BLYNK_LOG("Connecting to %s", ssid);
    LEDstatus = !LEDstatus;
//    Serial.println(LEDstatus);
    digitalWrite(LED_BLU, LEDstatus);
   
    if (pass && strlen(pass)) 
    {
      WiFi.begin(ssid, pass);
    } else {
      WiFi.begin(ssid);
    }
      while ((WiFi.status() != WL_CONNECTED) & (timeoutCounter < timeout * 2)) 
      {
      timeoutCounter += 1;
      ::delay(500);
      }
  }
  BLYNK_LOG("Connected to WiFi");
  digitalWrite(LED_BLU, HIGH);
  IPAddress myip = WiFi.localIP();
  BLYNK_LOG("My IP: %d.%d.%d.%d", myip[0], myip[1], myip[2], myip[3]);
}


void UpdateTime()                     //NTP time
{
  unsigned long epoch = getTime(TIMEZONE, DAYLIGHTSAVINGTIME);
  int hours = (epoch % 86400L) / 3600;
  int minutes = (epoch % 3600) / 60;
  int seconds = (epoch % 60);
  char timeString[8];
  setTime(epoch); // Set the system time to the epoch value

  Serial.println(epoch);
  sprintf(timeString, "%02d:%02d:%02d", hours, minutes, seconds);
  BLYNK_LOG("The NTP time is %s", timeString);       // UTC is the time at Greenwich Meridian (GMT)
}

// Digital clock display of the time
void clockDisplay()
{
  // You can call hour(), minute(), ... at any time
  // Please see Time library examples for details

  String currentTime = String(hour()) + ":" + minute() + ":" + second();
  String currentDate = String(day()) + " " + month() + " " + year();
  Serial.print("Current time: ");
  Serial.print(currentTime);
  Serial.print(" ");
  Serial.print(currentDate);
  Serial.println();

  // Send time to the App
  Blynk.virtualWrite(V1, currentTime);
  // Send date to the App
  Blynk.virtualWrite(V2, currentDate);
}


