/*
  Test program for SparroWatch
  (C) 2016 OOZLabs
  written by luisfcorreia

  Reads VCC value, posts it to emonCMS and sleeps for 'n' seconds
*/

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

  // try post data to EmonCMS
  count = 5;
  while (count >= 0) {
    if (client.connect(host, 443)) {
      break;
    }
    delay(500);
    count = count - 1;
  }
  if (count == 0){
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
}
void loop() {
  ESP.deepSleep(SLEEP_TIME,WAKE_RFCAL);
  yield();
}