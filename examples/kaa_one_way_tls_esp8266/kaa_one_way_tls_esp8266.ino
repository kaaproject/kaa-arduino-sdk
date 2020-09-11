#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "kaa.h"

#define KAA_SERVER "mqtt.cloud.kaaiot.com"
#define KAA_PORT 8883
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

// Root certificate
const char trustRoot[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";
X509List cert(trustRoot);

WiFiClientSecure espClient;
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
  doc_data["serial"] = String(ESP.getChipId());

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

void getTime() {
  // Set time via NTP, as required for x.509 validation
  Serial.println("Setting time using SNTP");
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

void setupCert() {
  Serial.printf("Using certificate: %s\n", trustRoot);
  espClient.setTrustAnchors(&cert);
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
  PRINT_DBG("Attempting MQTT connection to %s:%u ... ", mqtt_host, mqtt_port);
  // Create client ID
  String clientId = "ESP8266Client-";
  clientId += String(ESP.getChipId());
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
  getTime();
  setupCert();
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
