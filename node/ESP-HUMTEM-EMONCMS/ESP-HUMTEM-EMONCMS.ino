/*
  Test program for SparroWatch
  (C) 2016 OOZLabs
  written by luisfcorreia

  Reads VCC value, posts it to emonCMS and sleeps for 'n' seconds
*/
#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"
}

#include <WiFiClientSecure.h>
#include "credentials.h"
#include "DHT.h"

// ADC should read VCC
ADC_MODE(ADC_VCC);

// DHT pin and type
#define DHTPIN 4
#define DHTTYPE DHT11

// sleep intervals in us
#define minutes 1
#define SLEEP_TIME 60 * minutes * 1000000

float voltage;
float h;
float t;
String url = "";
String humtem = "";
int count;

WiFiClientSecure client;
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // read vcc voltage with ADC
  voltage = ESP.getVcc() / 1024.0;

  // make 5 attempts to get temperature and humidity
  humtem = "";
  count = 5;
  while (count >= 0) {
    delay(500);
    h = dht.readHumidity();
    t = dht.readTemperature();
    if (!isnan(h) || !isnan(t)) {
      humtem = ",esph:" + String(h) + ",espt:" + String(t);
      break;
    }
    count = count - 1;
  }

  // create final URL
  url = baseurl + "{espvcc:" + voltage + humtem + "}";

  // post data to EmonCMS
  if (!client.connect(host, 443)) {
    return;
  }
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266-readCenas\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
  ESP.deepSleep(SLEEP_TIME);
  yield();
}
void loop() {}
