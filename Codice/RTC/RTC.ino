/* Gestione RTC per Irrigo con ESP8266
 * CONNESSIONI:
 * SDA -> ESP pin 4 (GPIO02)
 * SCL -> ESP pin 5 (GPIO14)
 * Alimentazione 3.3 V
 * 
 * 
 * 
 * Mappatura pins <> GPIO
 * 
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1; 
 * 
 * 
 */

/*
 * TimeRTC.pde
 * Example code illustrating Time library with Real Time Clock.
 * This example is identical to the example provided with the Time Library,
 * only the #include statement has been changed to include the DS3232RTC 
 * library.
 */


 
#include <DS3232RTC.h> //http://github.com/JChristensen/DS3232RTC
#include <Time.h>      //http://www.arduino.cc/playground/Code/Time  
#include <Wire.h>      //http://arduino.cc/en/Reference/Wire
 
void setup(void)
{
    Serial.begin(9600);
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
    if(timeStatus() != timeSet) 
        Serial.println("Unable to sync with the RTC");
    else
        Serial.println("RTC has set the system time");      
}
 
void loop(void)
{
    digitalClockDisplay();  
    delay(1000);
}
 
void digitalClockDisplay(void)
{
    // digital clock display of the time
    Serial.print(hour());
    printDigits(minute());
    printDigits(second());
    Serial.print(' ');
    Serial.print(day());
    Serial.print(' ');
    Serial.print(month());
    Serial.print(' ');
    Serial.print(year()); 
    Serial.println(); 
}
 
void printDigits(int digits)
{
    // utility function for digital clock display: 
    // prints preceding colon and leading 0
    Serial.print(':');
    if(digits < 10)
        Serial.print('0');
    Serial.print(digits);
}
