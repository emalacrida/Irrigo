#define BLYNK_PRINT Serial
#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>

char ssid[] = "xxxxxx";       // tablet
char pass[] = "xxxxxx";

char auth[] = "YourAuthToken";

void setup()
{
  Serial.begin(9600); // See the connection status in Serial Monitor
  Blynk.begin(auth);  // Here your Arduino connects to the Blynk Cloud.
  Blynk.begin(auth, ssid, pass, server);
}

void loop()
{
  Blynk.run(); // All the Blynk Magic happens here...
}
