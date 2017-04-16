#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <secrets.h>

// MQTT config
#define MQTT_CLIENT_ID              "livingroom_dht11"

// DHT sensor
#define DHT_PIN                     13
#define DHT_TYPE                    DHT11
#define DHT_TOPIC                   "/house/livingroom/sensor_dht"
#define JSON_CHAR_BUFFER_SIZE       200
#define SLEEP_SEC                   1800

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void mqttReconnect() {
  while (!mqttClient.connected()) { //loop until we're reconnected
    Serial.println("[MQTT] INFO: Attempting connection...");
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("[MQTT] INFO: connected");
      mqttClient.subscribe(DHT_TOPIC); //subscribe to all light topics
      Serial.print("[MQTT] INFO: subscribing to: ");
      Serial.println(DHT_TOPIC);
    } else {
      Serial.print("[MQTT] ERROR: failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println("[MQTT] DEBUG: try again in 5 seconds");
      delay(2000); //wait 5 seconds before retrying
    }
  }
}

void wifiSetup() {
  Serial.print("[WIFI] INFO: Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(MQTT_CLIENT_ID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //connect to wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("[WIFI] INFO: WiFi connected");
  Serial.println("[WIFI] INFO: IP address: ");
  Serial.println(WiFi.localIP());
}

void postDht() {
  char buffer[JSON_CHAR_BUFFER_SIZE + 1];

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("ERROR: Failed to read from DHT sensor!");
    delay(500);
    ESP.restart();
  } else {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    root["temperature"] = String(t);
    root["humidity"] = String(h);

    root.prettyPrintTo(Serial);
    Serial.println();

    root.printTo(buffer, JSON_CHAR_BUFFER_SIZE);
    mqttClient.publish(DHT_TOPIC, buffer, true);
    Serial.println("[MQTT] Pushing data to MQTT");
  }
}

void setup() {
  Serial.begin(115200); //init the serial

  dht.begin();
  wifiSetup();

  // init the MQTT connection
  mqttClient.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
}

void loop() {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
  postDht();
  ESP.deepSleep(SLEEP_SEC * 1000000);
}
