#ifndef __MQTT_H__
#define __MQTT_H__


#include <ArduinoJson.h>
#include <Bridge.h>
#include <BridgeClient.h>
#include <PubSubClient.h>

#define MQTT_BUFFER_SIZE 100

class MQTT {
private:
  IPAddress server;
  int port;
  BridgeClient yun;
  String hostname;
  String debug_topic;

  void reconnect();

  unsigned long lastHeartbeat;

  void recv_set();

  void (*func_subscribe_callback)(void);

public:
  MQTT(IPAddress server, int port);
  void init();
  void loop();
  void set_subscribe_callback(void (*callback)(void));
  void debug(char* str) { client.publish(debug_topic.c_str(), str); }
  PubSubClient client;
  char stringBuffer[200];
};

static void callback(char* topic, byte* payload, unsigned int length);

extern MQTT mqtt;

#endif
