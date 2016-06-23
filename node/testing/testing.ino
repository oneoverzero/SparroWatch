/*
  Test program for SparroWatch
  (C) 2016 OOZLabs
  written by luisfcorreia

  Reads VCC from ESP8266 internal ADC,
  reads humidity and temperature from DHT22
  and posts it to emonCMS and sleeps for 'n' seconds
*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "DHT.h" // from Adafruit Github

const char* ssid     = "your_ap_wifi_ssid";
const char* password = "your_ap_wifi_password";
const char* host = "emoncms.org";
const char* apikey = "your_api_key";

String node = "1";
String baseurl = "/input/post.json?apikey=" + String(apikey) + "&node=" + node + "&json=";

// ADC should read VCC
ADC_MODE(ADC_VCC);

// DHT pin and type
#define DHTPIN 4
#define DHTTYPE DHT22

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

  Serial.begin(115200);
  Serial.println("Init serial 4 debug");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // read vcc voltage with ADC
  voltage = ESP.getVcc() / 1024.0;

  // make 5 attempts to get temperature and humidity
  humtem = "";
  count = 5;
  while (count >= 0) {
    delay(500);
    Serial.println("Read DHT");
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
  Serial.println(url);

  count = 5;
  while (count >= 0) {
    delay(500);

    if (!client.connect(host, 443)) {
      Serial.println("Could not connect");
    } else {
      break;
    }
    count = count - 1;
  }

  Serial.println("Send stuff");
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: SparroWatch\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("something");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println("something else");
}
void loop() {
  Serial.println("and goodnight");
  ESP.deepSleep(SLEEP_TIME, WAKE_RFCAL);
  yield();
}
