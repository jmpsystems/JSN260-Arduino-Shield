#include <Debug.h>
#include <jsn260.h>
#include <Arduino.h>

#define SSID      "PJW_2G"
#define KEY       "12345678"
#define AUTH       "WPA" 

#define MY_IP          "192.168.1.133"
#define SUBNET         "255.255.255.0"
#define GATEWAY        "192.168.1.254"

#define SERVER_PORT    1234
#define PROTOCOL       "UDP"

#define spi_CS  8
 
JSN260 JSN260(&Serial3);
boolean returnValue=false;

void setup() {
 
    Serial3.begin(115200);
    Serial.begin(115200);
    pinMode(spi_CS,OUTPUT);
    Serial.println("--------- JSN260 UDP Server with WPA Test --------");
    // wait for initilization of JSN260
    delay(1000);
    //  JSN260.reset();
    delay(1000);
    JSN260.join(SSID, KEY, AUTH);
    delay(1000);
    JSN260.staticIP(MY_IP, SUBNET, GATEWAY);

    if (!JSN260.connect(SERVER_PORT, PROTOCOL)) {
        Serial.println("Failed connect " SSID);
        Serial.println("Please Restart");
    } else {
        Serial.println("Successfully join " SSID);
        JSN260.sendCommand("at+wstat\r");
        delay(5);
        char c;
        while(JSN260.receive((uint8_t *)&c, 1, 100) > 0) {
            Serial.print((char)c);
        }
        delay(2000);
        JSN260.sendCommand("at+exit\r");
        delay(5);
        Serial.println("Enter Data mode successfully. You can send or receive data with host server");
    }
}
unsigned int time_point = 0;

void loop() {
  if(JSN260.available()) {
    Serial.print((char)JSN260.read());
  }
   if(Serial.available()) {
    JSN260.print((char)Serial.read());
  }
  
}
