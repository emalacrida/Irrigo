/**************************************************************
 *
 * Blynk is a platform with iOS and Android apps to control
 * Arduino, Raspberry Pi and the likes over the Internet.
 * You can easily build graphic interfaces for all your
 * projects by simply dragging and dropping widgets.
 *
 *   Downloads, docs, tutorials: http://www.blynk.cc
 *   Blynk community:            http://community.blynk.cc
 *   Social networks:            http://www.fb.com/blynkapp
 *                               http://twitter.com/blynk_app
 *
 * Blynk library is licensed under MIT license
 * This example code is in public domain.
 *
 **************************************************************
 * This example runs directly on ESP8266 chip.
 *
 * You need to install this for ESP8266 development:
 *   https://github.com/esp8266/Arduino
 *
 * Please be sure to select the right ESP8266 module
 * in the Tools -> Board menu!
 *
 * NOTE: SmartConfig might not work in your environment.
 *       Please try basic ESP8266 SmartConfig examples
 *       before using this sketch!
 *
 * Change Blynk auth token to run :)
 *
 **************************************************************/

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Ethernet.h>

//const char R_LED = D7;

const char ssid[] = "bibliowifi";           //  your network SSID (name)
const char pass[] = "";       //  your network password
const char server[] = "navigazione.wifi";     //wifi biblioteche

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "7bb63a4186694150b71c66ce29ee6c9c";

EthernetClient client;

void setup()
{
      
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (cnt++ >= 10) {
      WiFi.beginSmartConfig();
      while (1) {
        delay(1000);
        if (WiFi.smartConfigDone()) {
          Serial.println();
          Serial.println("SmartConfig: Success");
          break;
        }
        Serial.print("|");
      }
    }

  } 
  BLYNK_LOG("Connected to WiFi");

  IPAddress myip = WiFi.localIP();
  BLYNK_LOG("My IP: %d.%d.%d.%d", myip[0], myip[1], myip[2], myip[3]);
  WiFi.printDiag(Serial);

  Serial.println("comincio post/get");

  if (client.connect(server, 80)) {
    Serial.println("connected to PIC");
    // Make a HTTP request:
    Serial.println("inizio richiesta pagina di login");
    client.println("GET HTTP/1.1");
    client.println("Host: navigazione.wifi/login");
    client.println("Connection: close");
    client.println();
    Serial.println("fine richiesta pagina di login"); 
    // visualizza pagina ricevuta
    
    delay(5000);

    client.println("GET HTTP/1.1");
    client.println("Host: navigazione.wifi/login");
    client.println("Authentication: Basic MTExMTE6MTExMTE=");
 
    client.println("Connection: close");
    Serial.println("Connesione chiusa");
    client.println();
  } 
  else {
    // if you didn't get a connection to the server:
    Serial.println("connection to PIC failed");
  }


//  Blynk.config(auth);
}




void loop()
{
//  Blynk.run();

}

