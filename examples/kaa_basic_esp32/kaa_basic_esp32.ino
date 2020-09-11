#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "kaa.h"

#define KAA_SERVER "mqtt.cloud.kaaiot.com"
#define KAA_PORT 1883
#define KAA_TOKEN "**********"
#define KAA_APP_VERSION "**********"

#define RECONNECT_TIME  5000 //ms
#define SEND_TIME       3000 //ms

#define COMMAND_TYPE "OUTPUT_SWITCH"
#define OUTPUT_1_NAME "output_1"

const char* ssid = "******";
const char* password = "********";

char mqtt_host[] = KAA_SERVER;
unsigned int mqtt_port = KAA_PORT;

unsigned long now = 0;
unsigned long last_reconnect = 0;
unsigned long last_msg = 0;

WiFiClient espClient;
PubSubClient client(espClient);
Kaa kaa(&client, KAA_TOKEN, KAA_APP_VERSION);

#define PRINT_DBG(...) printMsg(__VA_ARGS__)

void printMsg(const char * msg, ...) {
  char buff[256];
  va_list args;
  va_start(args, msg);
  vsnprintf(buff, sizeof(buff) - 2, msg, args);
  buff[sizeof(buff) - 1] = '\0';
  Serial.print(buff);
}

String getChipId(){
  char buf[20];
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(buf, "%04X%08X", (uint16_t)(chipid>>32), (uint32_t)chipid);
  return String(buf);
}

void composeAndSendMetadata() {
  StaticJsonDocument<255> doc_data;
  String ipstring = (
                      String(WiFi.localIP()[0]) + "." +
                      String(WiFi.localIP()[1]) + "." +
                      String(WiFi.localIP()[2]) + "." +
                      String(WiFi.localIP()[3])
                    );

  doc_data["name"] = "ESP8266";
  doc_data["model"] = "Wemos D1 mini";
  doc_data["location"] = "Kyiv";
  doc_data["longitude"] = 50.465647;
  doc_data["latitude"] = 30.515270;
  doc_data["ip"] = ipstring;
  doc_data["mac"] = String(WiFi.macAddress());
  doc_data["serial"] = String(getChipId());

  kaa.sendMetadata(doc_data.as<String>().c_str());
}

void sendOutputsState() {
  StaticJsonDocument<255> doc_data;

  doc_data.createNestedObject();
  doc_data[0][OUTPUT_1_NAME] = digitalRead(LED_BUILTIN);
  
  kaa.sendDataRaw(doc_data.as<String>().c_str());
}

void changeOutputState(int output_number, int output_state) {
  digitalWrite(LED_BUILTIN, output_state);
  sendOutputsState();
}

int commandCallback(char* command_type, char* payload, unsigned int len) {
  if (!strcmp(command_type, COMMAND_TYPE)) {
    DynamicJsonDocument doc(1023);
    //StaticJsonDocument<255> doc;
    deserializeJson(doc, payload, len);
    JsonVariant json_var = doc.as<JsonVariant>();

    PRINT_DBG("Used command_id = %d\n", json_var[0]["id"].as<unsigned int>());
    int output_number = json_var[0]["payload"]["number"].as<int>();
    int output_state = json_var[0]["payload"]["state"].as<int>();
    changeOutputState(output_number, output_state);

    kaa.sendCommandResultAllIds(command_type, json_var);
  }
  else {
    PRINT_DBG("Unknown command\n");
  }
  return 0;
}

void setupWifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  String ipstring = (
               String(WiFi.localIP()[0]) + "." +
               String(WiFi.localIP()[1]) + "." +
               String(WiFi.localIP()[2]) + "." +
               String(WiFi.localIP()[3])
             );
  Serial.println();
  PRINT_DBG("WiFi connected\n");
  PRINT_DBG("IP address: %s\n", ipstring.c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {
  PRINT_DBG("Message arrived [%s] ", topic);
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  kaa.messageArrivedCallback(topic, (char*)payload, length);
}

void reconnect() {
  PRINT_DBG("Attempting MQTT connection to %s:%u ...", mqtt_host, mqtt_port);
  // Create client ID
  String clientId = "ESP8266Client-";
  clientId += String(getChipId());
  // Attempt to connect
  if (client.connect(clientId.c_str()))
  {
    PRINT_DBG("connected\n");
    kaa.connect();
    composeAndSendMetadata();
  } else
  {
    PRINT_DBG("failed, rc=%d try again in %d milliseconds\n", client.state(), RECONNECT_TIME);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  setupWifi();
  client.setServer(mqtt_host, mqtt_port);
  client.setCallback(callback);
  kaa.setCommandCallback(&commandCallback);
}

void loop() {
  //Checking connection
  if (!client.connected())
  {
    now = millis();
    if ( ((now - last_reconnect) > RECONNECT_TIME) || (now < last_reconnect) )
    {
      last_reconnect = now;
      reconnect();
    }
    return;
  }
  client.loop();

  //Sending something
  now = millis();
  if ( ((now - last_msg) > SEND_TIME) || (now < last_msg) )
  {
    last_msg = now;
    //Send something here
    sendOutputsState();
  }
}
