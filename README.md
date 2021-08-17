# Kaa IoT Platform Arduino library

## Overview: 

This library provides a client functionality for communicating with Kaa IoT Platform. 

As MQTT client this library uses [PubSubClient](https://github.com/knolleary/pubsubclient).

The library comes with example sketches for ESP8266, ESP32 and STM32. More examples will be available in future.  

Currently supported functions:
 - Send metadata
 - Send data
 - Receive commands and send response

## Reference: 

To use this library:
```c++
#include "kaa.h"
```

Create Kaa object:  
Pass pointer to PubSubClient object, kaa token and kaa app version to the constructor.
```c++
Kaa kaa(pointer_to_pubsubclient_object, kaa_token, kaa_app_version);
```

### Functions:

**messageArrivedCallback()**

```Topic```, ```payload``` and ```payload length``` of the received MQTT message need to be passed to this function.
  
It should be called at the end of PubSubClient message arrived callback.
  
```c++
void messageArrivedCallback(const char* topic_recv, const char* payload_recv, unsigned int len)
//Usage
kaa.messageArrivedCallback(topic, payload, payload_length);
```

**sendMetadata()**

Receives metadata as JSON string and sends it to Kaa.
  
```c++
void sendMetadata(const char* metadata_str)
//Usage
kaa.sendMetadata(metadata_json_str);
```

**sendDataRaw()**

Receives data as JSON string and sends it [reliably](https://docs.kaaiot.io/KAA/docs/v1.3.0/Features/Data-collection/#reliable-data-collection) to Kaa.
  
```c++
void sendDataRaw(const char* payload)
//Usage
kaa.sendDataRaw(data_json_str);
```

**sendDataRawUnreliably()**

Receives data as JSON string and sends it [unreliably](https://docs.kaaiot.io/KAA/docs/v1.3.0/Features/Data-collection/#unreliable-data-collection) to Kaa.
  
```c++
void sendDataRawUnreliably(const char* payload)
//Usage
kaa.sendDataRawUnreliably(data_json_str);
```

**requestConfig()**

Requests configuration from Kaa.
Response will be passed to `configResponseCallback` that must be set in order to invoke `requestConfig`, otherwise `requestConfig` invocation will have no effect.
  
```c++
int requestConfig()
//Usage
int ok = kaa.requestConfig();
```

**setConfigResponseCallback()**

The callback is called when device receives configuration response from the Kaa platform.

```c++
void (*configResponseCallback)(char* request_status, char* payload, unsigned int len);
//Usage
kaa.setConfigResponseCallback(&configResponseCallback);
```

**setConfigPushCallback()**

The callback is called when device receives configuration push from the Kaa platform.

```c++
void (*configPushCallback)(char* payload, unsigned int len);
//Usage
kaa.setConfigPushCallback(&configPushCallback);
```

**setCommandCallback()**

The callback is called when device receives command(s) from the Kaa platform.

```c++
void setCommandCallback(int (*commandCallback)(char* command_type, char* payload, unsigned int len))
//Usage
kaa.setCommandCallback(&commandCallback);
```

**sendCommandResultAllIds()**

After parsing a command payload, this function must be called in your ```commandCallback``` function to send a response to the platform about successful command execution.  
Function receives command type and Arduino JSON JsonVariant as parsed command payload.

```c++
void sendCommandResultAllIds(char* command_type, JsonVariant json_var)
//Usage
kaa.sendCommandResultAllIds(command_type, json_var);
```

Check examples to learn more about the usage of the functions of this library.
