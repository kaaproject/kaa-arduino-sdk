#ifndef KAA_IOT_PLATFORM_LIB_H
#define KAA_IOT_PLATFORM_LIB_H

#include <PubSubClient.h>
#include <ArduinoJson.h>

#define PRINT_DBG(...) printMsg(__VA_ARGS__)

#define KPC_INSTANCE_NAME "kp1"
#define EPMX_INSTANCE_NAME "epmx"
#define CMX_INSTANCE_NAME "cmx"
#define DCX_INSTANCE_NAME "dcx"
#define CEX_INSTANCE_NAME "cex"

class Kaa {
private:
  PubSubClient* pub_sub_client_ptr;

  char* kaa_token;
  char* kaa_application_version;

  char metadata_update_topic[100];
  char config_topic[100];
  char data_topic[100];
  char command_topic[100];
  char command_result_topic[100];

  int (*commandCallback)(char* command_type, char* payload, unsigned int len);

  #ifndef EXTERNAL_DEBUG_IMPLEMENTATION
  void printMsg(const char * msg, ...) {
    char buff[256];
    va_list args;
    va_start(args, msg);
    vsnprintf(buff, sizeof(buff) - 2, msg, args);
    buff[sizeof(buff) - 1] = '\0';
    Serial.print(buff);
  }
  #endif

  void cexMessageHandler(char* topic_part, char* payload, unsigned int len) {
    char command_type[50];
    char* rest = topic_part;
    char* token = strtok_r(rest, "/", &rest);
    if (!strcmp(token, kaa_token)) {
      token = strtok_r(rest, "/", &rest);
      if (!strcmp(token, "command")) {
        token = strtok_r(rest, "/", &rest);
        //command_type = token;
        strncpy(command_type, token, sizeof(command_type) - 1);
        command_type[sizeof(command_type) - 1] = '\0';
        token = strtok_r(rest, "/", &rest);
        if (!strcmp(token, "status")) {
          PRINT_DBG("\nCommand %s received\n", command_type);
          //commandProcess(command_type, payload, len);
          commandCallback(command_type, payload, len);
        }
        else if (!strcmp(token, "error")) {
          PRINT_DBG("\nCommand %s error\n", command_type);
          PRINT_DBG("%s\n", payload);
        }
      }
    }
  }

public:
  Kaa(PubSubClient* _pub_sub_client_ptr, char* _kaa_token, char* _kaa_application_version){
      pub_sub_client_ptr = _pub_sub_client_ptr;
      kaa_token = _kaa_token;
      kaa_application_version = _kaa_application_version;
      composeTopics();
  }

  void setCommandCallback(int (*_commandCallback)(char* command_type, char* payload, unsigned int len)){
      commandCallback = _commandCallback;
  }

  void setToken(char* _kaa_token){
      kaa_token = _kaa_token;
  }

  void setAppVersion(char* _kaa_application_version){
      kaa_application_version = _kaa_application_version;
  }

  void composeTopics() {
    sprintf(metadata_update_topic, "%s/%s/%s/%s/update/keys", KPC_INSTANCE_NAME, kaa_application_version, EPMX_INSTANCE_NAME, kaa_token);
    sprintf(config_topic, "%s/%s/%s/%s/config/json", KPC_INSTANCE_NAME, kaa_application_version, CMX_INSTANCE_NAME, kaa_token);
    sprintf(data_topic, "%s/%s/%s/%s/json", KPC_INSTANCE_NAME, kaa_application_version, DCX_INSTANCE_NAME, kaa_token);
    sprintf(command_topic, "%s/%s/%s/%s/command", KPC_INSTANCE_NAME, kaa_application_version, CEX_INSTANCE_NAME, kaa_token);
    sprintf(command_result_topic, "%s/%s/%s/%s/result", KPC_INSTANCE_NAME, kaa_application_version, CEX_INSTANCE_NAME, kaa_token);
  }

  void sendMetadata(const char* metadata_str) {
    char topic_buffer[200];
    sprintf( topic_buffer, "%s/%d", metadata_update_topic, random(1, 100000) );

    PRINT_DBG("Publishing: %s %s\n", topic_buffer, metadata_str);
    pub_sub_client_ptr->publish(topic_buffer, metadata_str);
  }

  void connect() {
    pub_sub_client_ptr->subscribe(config_topic);
  }

  void messageArrivedCallback(const char* topic_recv, const char* payload_recv, unsigned int len) {
    char topic[512];
    char payload[1023];
    strncpy(topic, topic_recv, sizeof(topic) - 1);
    topic[sizeof(topic) - 1] = '\0';
    if (len < 1023) {
      strncpy(payload, payload_recv, len);
      payload[len] = '\0'; //Add terminator after last payload byte
    }
    else {
      PRINT_DBG("Error. payload len >= 1023\n");
      return;
    }
    char* rest = topic;
    char* token = strtok_r(rest, "/", &rest);
    if (!strcmp(token, KPC_INSTANCE_NAME)) {
      token = strtok_r(rest, "/", &rest);
      if (!strcmp(token, kaa_application_version)) {
        token = strtok_r(rest, "/", &rest);
        //EPMX
        if (!strcmp(token, EPMX_INSTANCE_NAME)) {
          PRINT_DBG("Message for EPMX\n");
          //Do something
        }
        //DCX
        else if (!strcmp(token, DCX_INSTANCE_NAME)) {
          PRINT_DBG("Message for DCX\n");
          //Do something
        }
        //CMX
        else if (!strcmp(token, CMX_INSTANCE_NAME)) {
          PRINT_DBG("Message for CMX\n");
          //Do something
        }
        //CEX
        else if (!strcmp(token, CEX_INSTANCE_NAME)) {
          PRINT_DBG("Message for CEX\n");
          //PRINT_DBG("%s\n", rest);
          cexMessageHandler(rest, payload, len);
        }
      }
    }
  }

  void sendCommandResultAllIds(char* command_type, JsonVariant json_var){
      //Sending result to all command ids
      PRINT_DBG("number of ids = %d\n", json_var.size());
      DynamicJsonDocument doc_result(1023);
      //StaticJsonDocument<255> doc_result;
      for (int i = 0; i < json_var.size(); i++) {
        unsigned int command_id = json_var[i]["id"].as<unsigned int>();
        PRINT_DBG("command_id = %d\n", command_id);
        doc_result.createNestedObject();
        doc_result[i]["id"] = command_id;
        doc_result[i]["statusCode"] = 200;
        doc_result[i]["payload"] = "done";
      }
      PRINT_DBG("command_type = %s\n", command_type);
      sendCommandResultRaw(command_type, doc_result.as<String>().c_str());
  }

  void sendCommandResult(const char* command_type, int command_id) {
    char topic_buffer[200];
    char command_result_payload_buffer[200];

    sprintf( topic_buffer, "%s/%s", command_result_topic, command_type );
    sprintf( command_result_payload_buffer, "[{\"id\": %d, \"statusCode\": 200, \"payload\": \"done\"}]", command_id);
    PRINT_DBG("Publishing: %s %s\n", topic_buffer, command_result_payload_buffer);
    pub_sub_client_ptr->publish(topic_buffer, command_result_payload_buffer);
  }

  void sendCommandResultRaw(const char* command_type, const char* json_payload) {
    char topic_buffer[200];
    sprintf( topic_buffer, "%s/%s", command_result_topic, command_type );
    PRINT_DBG("Publishing: %s %s\n", topic_buffer, json_payload);
    pub_sub_client_ptr->publish(topic_buffer, json_payload);
  }

  void sendDataRaw(const char* payload) {
    char topic_buffer[200];
    sprintf( topic_buffer, "%s/%d", data_topic, random(1, 100000) );
    PRINT_DBG("Publishing: %s %s\n", topic_buffer, payload);
    pub_sub_client_ptr->publish(topic_buffer, payload);
  }

  void requestConfig() {
    char topic_buffer[200];
    char request_config_payload[] = "{\"observe\": true}";
    sprintf( topic_buffer, "%s/%d", config_topic, random(1, 100000) );
    PRINT_DBG("Publishing: %s %s\n", topic_buffer, request_config_payload);
    pub_sub_client_ptr->publish(topic_buffer, request_config_payload);
  }
};

#endif