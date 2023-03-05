#include <WiFi.h>
#include <AsyncMqttClient.h>
#include "ESPAsyncWebServer.h"
#include <Arduino.h>
#include <Ubidots.h>
#include <string.h>

#define WIFI_SSID "Quang Dam"
#define WIFI_PASSWORD "30111971"

#define MQTT_HOST IPAddress(192, 168, 1, 27)   //specify your Raspberry Pi IP Address
#define MQTT_PORT 1883  
const char* UBIDOTS_TOKEN = "BBFF-vBfcnUDfKlrjXjOExMjVr04w4l3GVR";
const char* PARAM_INPUT_1 = "state";
Ubidots ubidots(UBIDOTS_TOKEN, UBI_HTTP);
String outputState();
// Variables will change:
int led_local_dht;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// Create AsyncWebServer object on port 80

//MQTT Topics
#define MQTT_SUB_TEMP "esp32/temp"
#define MQTT_SUB_HUM  "esp32/humi"
#define MQTT_SUB_GAS  "esp32/gas"
#define MQTT_PUB_LED_DHT  "esp32/led_dht"
AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

AsyncWebServer server(80);
AsyncEventSource events("/events");

unsigned long lastTime = 0;  
unsigned long timerDelay = 30000;

String temperature_Celsius =  "18.5";
String Humidity = "70";
String Gas = "70";

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
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
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSub = mqttClient.subscribe(MQTT_SUB_TEMP, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);
  packetIdSub = mqttClient.subscribe(MQTT_SUB_HUM, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);
  packetIdSub = mqttClient.subscribe(MQTT_SUB_GAS, 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);
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
void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}
void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
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

  if (String(topic) == "esp32/temp"){
  Serial.print("n Temperature: ");
  Serial.println(message);
  temperature_Celsius = message;
  }

  if (String(topic) == "esp32/humi"){
  Serial.print("n Humidity: ");
  Serial.println(message);
  Humidity = message;
  }
  if (String(topic) == "esp32/gas"){
  Serial.print("n GasValue: ");
  Serial.println(message);
  Gas = message;
  }
}

//config webserver
String processor(const String& var){
  if(var == "BUTTONPLACEHOLDER"){
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<h4>Output - GPIO 2 - State <span id=\"outputState\"></span></h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  //Serial.println(var);
  if(var == "TEMPERATURE_C"){
    return String(temperature_Celsius);
  }
  if(var == "TEMPERATURE_F"){
    return String(Humidity);
  }
  if(var == "HUMIDITY"){
    return String(Gas);
  }
}
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>DHT Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #4B1D3F; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); }
    .reading { font-size: 2rem; }
    .card.temperature { color: #0e7c7b; }
    .card.humidity { color: #17bebb; }
    h2 {font-size: 3.0rem;}
    p {font-size: 2.0rem;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <div class="topnav">
    <h3>DHT WEB SERVER</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4><p><span class="reading"><span id="temp_celcius">%TEMPERATURE_C%</span> &deg;C</span></p>
      </div>
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> HUMIDITY</h4><p><span class="reading"><span id="temp_fahrenheit">%TEMPERATURE_F%</span> &percnt;</span></p>
      </div>
      <div class="card humidity">
        <h4><i class="fas fa-tint"></i> GAS</h4><p><span class="reading"><span id="hum">%HUMIDITY%</span> ppm</span></p>
      </div>
    </div>
  </div>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('temperature_Celsius', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp_celcius").innerHTML = e.data;
 }, false);
 
 source.addEventListener('temperature_Fahrenheit', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp_fahrenheit").innerHTML = e.data;
 }, false);
 source.addEventListener('humidity', function(e) {
  console.log("humidity", e.data);
  document.getElementById("hum").innerHTML = e.data;
 }, false);
 
function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?state=1", true); }
  else { xhr.open("GET", "/update?state=0", true); }
  xhr.send();
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var inputChecked;
      var outputStateM;
      if( this.responseText == 1){ 
        inputChecked = true;
        outputStateM = "On";
      }
      else { 
        inputChecked = false;
        outputStateM = "Off";
      }
      document.getElementById("output").checked = inputChecked;
      document.getElementById("outputState").innerHTML = outputStateM;
    }
  };
  xhttp.open("GET", "/state", true);
  xhttp.send();
}, 1000 ) ;

}
  
</script>
</body>
</html>
)rawliteral";


String outputState(){
  if(digitalRead(27)){
    return "checked";
  }
  else {
    return "";
  }
  return "";
}
void getreading ()
{
  temperature_Celsius = temperature_Celsius;
  Humidity = Humidity;
  Gas = Gas;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); 

  pinMode(26, OUTPUT);
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
   mqttClient.onPublish(onMqttPublish);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  connectToWifi();

  // Handle Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      led_local_dht = inputMessage.toInt();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    Serial.println(led_local_dht);
    request->send(200, "text/plain", "OK");
  });

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
}

void loop() {

    getreading();
    ubidots.add("Gas",Gas.toInt());
  bool bufferSent_gas = false;
  bufferSent_gas = ubidots.send("esp32", "gas");  // Will send data to a device label that matches the device Id
  if (bufferSent_gas) {
    // Do something if values were sent properly
    Serial.println("Values gas sent by the device");
  }
      ubidots.add("Temperature",temperature_Celsius.toInt());
  bool bufferSent_temp = false;
  bufferSent_temp = ubidots.send("esp32", "temperature");  // Will send data to a device label that matches the device Id
  if (bufferSent_temp) {
    // Do something if values were sent properly
    Serial.println("Values temperature sent by the device");
  }
 events.send(String(Gas).c_str(),"humidity",millis());
 events.send(String(temperature_Celsius).c_str(),"temperature_Celsius",millis());
 events.send(String(Humidity).c_str(),"temperature_Fahrenheit",millis());
  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:



  // save the reading. Next time through the loop, it'll be the lastButtonState:



float value_dht = ubidots.get("esp32","led_dht");
if (value_dht == ERROR_VALUE)
led_local_dht= led_local_dht;
else if ((value_dht != led_local_dht) && (value_dht != ERROR_VALUE))
led_local_dht = value_dht;

  // Evaluates the results obtained
  if (led_local_dht != ERROR_VALUE) {
    Serial.print("Value_dht:");
    Serial.println(led_local_dht);
  }

    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_LED_DHT, 1, true, String(led_local_dht).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_LED_DHT, packetIdPub2);
    Serial.printf("Message: %.2f n", led_local_dht);
  // if (led_local_dht == float(0))
  // digitalWrite(27, LOW);
  // else
  // digitalWrite(27, HIGH);


  float value_gas = ubidots.get("esp32","led_gas");

  // Evaluates the results obtained
  if (value_gas != ERROR_VALUE) {
    Serial.print("Value:");
    Serial.println(value_gas);
  }

  if (value_gas == float(0))
  digitalWrite(26, LOW);
  else
  digitalWrite(26, HIGH);

}