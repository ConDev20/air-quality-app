#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>

#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <Arduino_JSON.h>
#include <EEPROM.h>

#include <SPIFFS.h>
#include <sps30.h>

#define WIFI_NUM_RETRIES  10
#define EEPROM_SIZE       16 // define the number of bytes you want to access
#define EEPROM_START_ADDRESS 0

int ret;
// static struct sps30_measurement m;

// Replace with your network credentials
const char* ssid = "QuestTest";
const char* password = "12345678";

unsigned long previous_time = 0;
unsigned long Delay = 15000;

float treshold_values[4] = {10.0, 10.0, 10.0, 10.0};

void Wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("ESP32 WIFI Connected to Access Point");
}

void Get_IPAddress(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WIFI Connected!");
  Serial.println("IP address of Connected WIFI: ");
  Serial.println(WiFi.localIP());
}

void Wifi_disconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  int retries = 0;
  Serial.println("Disconnected from WIFI");
  Serial.print("Connection Lost Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Reconnecting...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  while (WiFi.status() != WL_CONNECTED) {
    if (retries == WIFI_NUM_RETRIES ) {
      Serial.println("Reseting ESP...");
      ESP. restart();
    }
    delay(1000);
    Serial.println("Reconnecting to WiFi...");
    retries++;
  }
}

void write_treshold_to_eeprom(int treshold_id)
{
  EEPROM.begin(EEPROM_SIZE);
  int address = sizeof(float) * treshold_id;
  EEPROM.writeFloat(address, treshold_values[treshold_id]);
  EEPROM.commit();
  EEPROM.end();
}

void read_tresholds_from_eeprom()
{
  EEPROM.begin(EEPROM_SIZE);
  int address = EEPROM_START_ADDRESS;
  float current_value = 10;
  for (size_t i = 0; i < 4; i++)
  {
    float current_value = EEPROM.readFloat(address);
    if (!isnan(current_value)) {
      treshold_values[i] = current_value;
    }
    address += sizeof(float);
  }
  EEPROM.end();
}

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

Adafruit_BME280 bme; // BME280 connect to ESP32 I2C (GPIO 21 = SDA, GPIO 22 = SCL)

JSONVar treshold_vals;
JSONVar readings;
JSONVar relay_status;

float make_float_pretty(float number, int decimal_places)
{
  if(decimal_places == 0)
    return (int)number;
  float multiplier = pow(10, decimal_places);
  return ((int)(number * multiplier)) / multiplier;
}

String getReadings() {
  readings["pm1"] = make_float_pretty(4.4,1);
  readings["pm25"] = make_float_pretty(4.5,1);
  readings["pm4"] = make_float_pretty(5.5,1);
  readings["pm10"] = make_float_pretty(8,1);
  readings["typical_size"] = 15;
  readings["temperature"] = String(bme.readTemperature());
  readings["pressure"] =  String(bme.readPressure()/100);  
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Get treshold values to jsonString
String getTresholdValues() {
  treshold_vals["pt1"] = treshold_values[0];
  treshold_vals["pt25"] = treshold_values[1];
  treshold_vals["pt4"] = treshold_values[2];
  treshold_vals["pt10"] = treshold_values[3];
  String jsonString = JSON.stringify(treshold_vals);
  return jsonString;
}

String getRelayStatus() {
  if (treshold_values[0] < 4.4 || treshold_values[1] < 4.5|| treshold_values[2] < 5.5 || treshold_values[3] < 8) {
    // digitalWrite(2, LOW);
    relay_status["relay"] = 0;
  } else {
    // digitalWrite(2, HIGH);
    relay_status["relay"] = 1;
  }
  return JSON.stringify(relay_status);
  // return JSON.stringify(1);
}

void sendJson(String jsonData)
{
  webSocket.broadcastTXT(jsonData);
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("Client " + String(num) + " connected");
      sendJson(getTresholdValues());
      sendJson(getReadings());
      sendJson(getRelayStatus());
      break;
    case WStype_TEXT:                                 // if a client has sent data, then type == WStype_TEXT
      // try to decipher the JSON string received
      StaticJsonDocument<200> doc;                    // create JSON container 
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
         Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        const char* l_type = doc["type"];
        const float l_value = doc["value"];

        if (String(l_type) == "pt1"){
          treshold_values[0] =  make_float_pretty(l_value,1);
          write_treshold_to_eeprom(0);
        } else if (String(l_type) == "pt25"){
          treshold_values[1] =  make_float_pretty(l_value,1);
          write_treshold_to_eeprom(1);
        } else if (String(l_type) == "pt4"){
          treshold_values[2] =  make_float_pretty(l_value,1);
          write_treshold_to_eeprom(2);
        } else if (String(l_type) == "pt10"){
          treshold_values[3] =  make_float_pretty(l_value,1);
          write_treshold_to_eeprom(3);
        }
        sendJson(getTresholdValues());
        sendJson(getReadings());
        sendJson(getRelayStatus());
      }
      break;
  }
}

void initBME()
{
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

void init_WiFi()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.onEvent(Wifi_connected,WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(Get_IPAddress, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.begin(ssid, password);
  Serial.println("Waiting for WIFI network...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  WiFi.onEvent(Wifi_disconnected,WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
}

void setup() {
  int16_t ret;
  uint8_t auto_clean_days = 4;
  uint32_t auto_clean;
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  Serial.begin(115200);
  initBME();
  init_WiFi();

  sensirion_i2c_init();
  
  ret = sps30_set_fan_auto_cleaning_interval_days(auto_clean_days);
  ret = sps30_start_measurement();
  
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  server.serveStatic("/", SPIFFS, "/");
  Serial.println("SPIFFS mounted successfully");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {    // define here wat the webserver needs to do
    request->send(SPIFFS, "/index.html", "text/html");           
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "File not found");
  });

  server.serveStatic("/", SPIFFS, "/");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  server.begin();
  read_tresholds_from_eeprom();
  sendJson(getReadings());
  sendJson(getRelayStatus());              
}

void loop() {
  webSocket.loop();

  if ((millis() - previous_time) > Delay) {
    sendJson(getReadings());
    sendJson(getRelayStatus());
    previous_time = millis();
  }
}
