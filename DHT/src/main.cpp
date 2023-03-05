#include <Wifi.h>
#include "DHT.h"
#include <AsyncMqttClient.h>

//wifi
#define WIFI_SSID "Quang Dam"
#define WIFI_PASSWORD "30111971"

//dht11 config
#define DHTTYPE DHT11 // DHT 22 (AM2302), AM2321

//MQTT config
#define MQTT_HOST IPAddress(192, 168, 1, 27)
#define MQTT_PORT 1883
#define MQTT_PUB_TEMP "esp32/temp"
#define MQTT_PUB_HUM "esp32/humi"
#define MQTT_SUB_LED_DHT  "esp32/led_dht"

uint8_t DHTPin = 4;
DHT dht(DHTPin, DHTTYPE);

float temperature_Celsius;
float Humidity;
String led_state_dht;

unsigned long lastTime = 0;  
unsigned long timerDelay = 5000;  // send readings timer

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

void getDHTReadings(){
 
   Humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature_Celsius = dht.readTemperature();
}

void connectToWifi(){
  Serial.println("Connecting to WiFi nha a...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt(){
  Serial.println("Connecting to Mqtt nha a...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %dn", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); 
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSub = mqttClient.subscribe(MQTT_SUB_LED_DHT, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}
void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}
void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}
void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}


void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.println(payload);
  Serial.print("  topic: ");
  Serial.println(topic);
  String message;
  for (int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
    message += (char)payload[i];
  }

  if (String(topic) == "esp32/led_dht"){
  Serial.print("n Led_state_dht: ");
  Serial.println(message);
  led_state_dht = message;
  }
  Serial.println("led_state_dht:");
Serial.println(led_state_dht);
}


void setup() {
  Serial.begin(115200);
  pinMode(DHTPin, INPUT);
  pinMode(27, OUTPUT);
  dht.begin();
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectToWifi();  
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    
    //getDHTReadings();
    
   // if (isnan(temperature_Celsius) || isnan(Humidity)) {
     // Serial.println(F("Failed to read from DHT sensor!"));
     // return;
    //}
int count;
temperature_Celsius ++;
Humidity ++;
count ++;
if (count > 10)
{
  temperature_Celsius =0;
  Humidity =0;
}


    Serial.printf("Temperature = %.2f ºC \n", temperature_Celsius);
    Serial.printf("Humidity= %f %\n", Humidity);
    Serial.println();

    //Publish an mqtt message on topics...
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temperature_Celsius).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_TEMP, packetIdPub1);
    Serial.printf("Message: %.2f n", temperature_Celsius);

    // Publish an MQTT message on topic esp32/humidity
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(Humidity).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_HUM, packetIdPub2);
    Serial.printf("Message: %.2f n", Humidity);
    
    lastTime = millis();
  }

  if (led_state_dht.toFloat() == float(0))
  digitalWrite(27, LOW);
  else
  digitalWrite(27, HIGH);

}