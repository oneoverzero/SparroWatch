#include <ESP8266WiFi.h>
#include <ESP8266WiFiMesh.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h> 
#include <WiFiServer.h>
#include <Config.h>

#define MODE_READ 1
#define MODE_SETUP_AP 2
#define MODE_AP 3
#define MODE_TRANSMIT 4

#define SSID_PREFIX          "Node"
#define SERVER_IP_ADDR      "192.168.4.1"
#define SERVER_PORT       4011

String ssid_prefix = String(SSID_PREFIX);
unsigned int wifi_timeout=10000, // 10 seconds
             http_timeout=10000, // 10 seconds
             server_distance=1000,
             data_interval=5 * 60 * 1000, // 5 minutes
             mode=MODE_READ,
             last_read;

float read_temperature() {
  return 24.0;
}

unsigned int read_humidity() {
  return 60;
}

String manageRequest(String request)
{
  /* Print out received message */
  Serial.print("received: ");
  Serial.println(request);

  /* return a string to send back */
  char response[60] = "";
  sprintf(response, "OK everything is fine!");
  return response;
}

boolean httpPost(char * host, uint16_t port, char * url)
{
  HTTPClient http;
  bool ret = false;

  unsigned long start = millis();

  // configure target server and url
  http.begin(host, port, url); 

  // start connection and send HTTP header
  int httpCode = http.GET();
  if(httpCode) {
      // HTTP header has been send and Server response header has been handled
      // file found at server
      if(httpCode == 200) {
        String payload = http.getString();
        //Serial.println(payload);
        ret = true;
      }
  } else {
      //Serial.println("failed!");
  }
  return ret;
}

bool send(char *payload) {
  char url[200];
  uint16_t port = 80;
  
  sprintf(url, "/input/post.json?apikey=%s&%s", apikey, payload);
  return httpPost(host, port, url);
}

char *payload(float temperature, unsigned int humidity) {
  char payload[200], str_temp[6];

  dtostrf(temperature, 4, 2, str_temp);
  sprintf(payload, "node=%s&json={humidity:%d,temp:%s}", node_id, humidity, str_temp);
  return payload;
}

bool connect_and_send(char *payload) {
  unsigned int start = 0;
  bool ret = true;
    
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    start += 500;
    if (start > wifi_timeout) {
        WiFi.disconnect(true);
        return false;
    }
  }

  ret = send(payload);
  if(ret) {
    server_distance = 0;
  }

  WiFi.disconnect(true);
  return ret;
}

bool exchangeInfo(String message, WiFiClient curr_client)
{
  int wait = 1000;
  
  curr_client.println( message.c_str() );

  while(curr_client.connected() && !curr_client.available() && wait--)
    delay(3);

  /* Return false if the client isn't ready to communicate */
  if (WiFi.status() == WL_DISCONNECTED || !curr_client.connected())
    return false;

  String response = curr_client.readStringUntil('\r');
  curr_client.readStringUntil('\n');

  Serial.println(response);

  if (response.length() <= 2) 
    return false;

  if (response.startsWith("OK")) {
    return true;
  }
  return false;
}

bool connectToNode(String target_ssid, String message)
{
  WiFiClient curr_client;
  WiFi.begin( target_ssid.c_str() );
  int wait = 1500;

  WiFi.mode(WIFI_STA);
  while((WiFi.status() == WL_DISCONNECTED) && wait--)
    delay(3);

  /* If the connection timed out */
  if (WiFi.status() != 3) {
    Serial.println("Not connected to wifi");
    return false;
  }
  
  Serial.println(WiFi.localIP());

  /* Connect to the node's server */
  if (!curr_client.connect(SERVER_IP_ADDR, SERVER_PORT)) {
    Serial.println("Not connected to server");
    return false;
  }

  if (!exchangeInfo(message, curr_client)) {
    Serial.println("Failed to send message");
    WiFi.disconnect();
    return false;
  }

  curr_client.stop();
  WiFi.disconnect();
  return true;
}

unsigned int find_nearest_server_and_transmit(char *payload) {
  unsigned int next_node_distance = 1000,
               node_distance;
  int n = WiFi.scanNetworks();

  for (int i = 0; i < n; ++i) {
    String current_ssid = WiFi.SSID(i);
    int index = current_ssid.indexOf(SSID_PREFIX);
    uint32_t node_distance = (current_ssid.substring(index + ssid_prefix.length())).toInt();

    /* Connect to any _suitable_ APs which contain _ssid_prefix */
    if (index >= 0 && (node_distance < server_distance)) {
      Serial.println(current_ssid);
      Serial.println(node_distance);

      if (connectToNode(current_ssid, payload)) {
        Serial.println("successfully sent");
        next_node_distance = node_distance;
      }
      WiFi.disconnect(true);
    }
  }

  WiFi.disconnect(true);
  return next_node_distance;
}

void setup() {
	Serial.begin(115200);
	delay(10);

	Serial.println();
	Serial.println();
	Serial.println("Starting operations");
}

void loop() {
  bool has_current_payload = false;
  char *current_payload;
  float temperature;
  unsigned int humidity;
  String ssid;
  ESP8266WiFiMesh node = ESP8266WiFiMesh(server_distance, manageRequest);

  switch (mode) {
    case MODE_READ:
      temperature = read_temperature();
      humidity = read_humidity();

      current_payload = payload(temperature, humidity);
      has_current_payload = true;
      if(connect_and_send(current_payload)) {
        mode = MODE_SETUP_AP;
        has_current_payload = false;
        server_distance = 0;
      } else {
        mode = MODE_TRANSMIT;
        server_distance = 1000;
      }
      last_read = millis();
      break;
    case MODE_SETUP_AP:
      node.begin();
      mode = MODE_AP;
      break;
    case MODE_AP:
      node.acceptRequest();
      Serial.println(WiFi.localIP());
      if (millis() - last_read > data_interval) {
        WiFi.disconnect();
        mode = MODE_READ;
      }
      break;
    case MODE_TRANSMIT:
      server_distance = find_nearest_server_and_transmit(current_payload) + 1;
      if (millis() - last_read > data_interval) {
        mode = MODE_READ;
      }
      break;
    default:
      Serial.println("node in unknown mode, resetting");
      break;
  }
}
