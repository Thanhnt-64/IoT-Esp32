#include <Wifi.h>
#include <AsyncMqttClient.h>

//wifi
#define WIFI_SSID "Quang Dam"
#define WIFI_PASSWORD "30111971"

//dht11 config

//MQTT config
#define MQTT_HOST IPAddress(192, 168, 1, 27)
#define MQTT_PORT 1883
#define MQTT_PUB_GAS "esp32/gas"

int Gas_pin = 34;

int Gas_value;

unsigned long lastTime = 0;  
unsigned long timerDelay = 4000;  // send readings timer

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

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
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  pinMode(Gas_pin, INPUT);
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectToWifi();  
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    
    Gas_value = analogRead(Gas_pin);
    
    Serial.println(Gas_value);
    Serial.println();

    //Publish an mqtt message on topics...
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_GAS, 1, true, String(Gas_value).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", MQTT_PUB_GAS, packetIdPub1);
    Serial.printf("Message: %.2f n", Gas_value);


    lastTime = millis();
  }
}