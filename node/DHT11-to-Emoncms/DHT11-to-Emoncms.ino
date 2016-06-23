#include <Arduino.h>

#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"
}
#include <WiFiClientSecure.h>

#include "DHT.h"

#define DHTPIN D5 // what digital pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

ADC_MODE(ADC_VCC); //vcc read-mode

#include "credentials.h"

String url = "";
String humtemp = "";

WiFiClientSecure client;

void initWiFi() {
    Serial.print("connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("MAC Address: " + WiFi.macAddress());
}

void setup()
{
    Serial.begin(115200);
    initWiFi();
    dht.begin();
}

void loop()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  delay(2000);
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C \t");

  humtemp = ",humidity:" + String(h) + ",temperature:" + String(t);
  /* read vcc voltage with ADC */
  float voltage = 0.00;
  voltage = ESP.getVcc() / 1024.00;

  Serial.print("Voltage:     ");
  Serial.print(voltage);
  Serial.println(" V");

  // create final URL
  url = baseurl + "{voltage:" + voltage + humtemp + "}";
  // post data to EmonCMS
  if (!client.connect(host, 443)) {
   return;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
  "Host: " + host + "\r\n" +
  "User-Agent: SparroWatch\r\n" +
  "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  String line = client.readStringUntil('\n');


  Serial.println();
  Serial.println("closing connection. going to sleep...");
  // go to deepsleep for 1 minute
  system_deep_sleep_set_option(0);
  system_deep_sleep(1 * 60 * 1000000);
  delay(1000);
}