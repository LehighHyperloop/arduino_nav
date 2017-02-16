#include <ArduinoJson.h>
#include "mqtt.h"

// set up the mqtt client
IPAddress server(192, 168, 0, 100);
MQTT mqtt = MQTT(server, 1883);

const int ir_pin = 2;

#define TICKS_PER_CALC (10)
volatile int ticks = 0; //_MUST_ be volatile so compiler doesn't fuck with this/optimize it out
volatile unsigned long ticks_total = 0;
volatile unsigned long last_t = 0;
volatile unsigned long last_count_delta = 0;

//function called by interrupt
void intr()
{
  ticks += 1;
  ticks_total += 1;
  if (ticks == TICKS_PER_CALC) {
    unsigned long t = millis();
    last_count_delta = t - last_t;
    ticks = 0;
    last_t = t;
  }
}

void setup()
{
  Serial.begin(9600);

  /*flash onboard LED for debug*/
  Serial.println();
  Serial.println();
  Serial.println();
  for (int i = 0; i < 10; i++)
  {
    digitalWrite(13, HIGH);
    Serial.print(F("."));
    if (i % 10 == 0)
    {
      Serial.println();
    }
    delay(10);
    digitalWrite(13, LOW);
    delay(100);
  }
  Serial.println();
  delay(50);

  pinMode(ir_pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(ir_pin), intr, FALLING);     //have to map the pin to the actual interrupt number

  mqtt.init();

  mqtt.loop();
  mqtt.debug("nav (" __DATE__ " " __TIME__ ")");
  Serial.println("nav (" __DATE__ " " __TIME__ ")");
  mqtt.loop();
}

#define INTERVAL (100)
unsigned long last_loop = 0;
float rps = 0;
void loop()
{
  mqtt.loop();

  unsigned long t = millis();
  int delta = t - last_loop;
  if (delta >= INTERVAL) {
    rps = TICKS_PER_CALC / 2 / float(last_count_delta / 1000.0);
		unsigned int l_ticks_total = ticks_total;
		unsigned int l_last_t = last_t;
    if (t - last_t >= 1000) {
      // Speed timed out, probably 0
      rps = 0;
    }

    Serial.print(rps);
    Serial.print(" ");
    Serial.println(rps * 60);

    StaticJsonBuffer<MQTT_BUFFER_SIZE> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["rps"] = rps;
		root["last_t"] = l_last_t;
		root["ticks"] = l_ticks_total;
    root["t"] = t;

    root.printTo(mqtt.stringBuffer, sizeof(mqtt.stringBuffer));
    mqtt.client.publish("sensor/right/rotation", mqtt.stringBuffer);

    last_loop = t;
  }
}

