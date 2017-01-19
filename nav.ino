#include <ArduinoJson.h>

#include "mqtt.h"

int ir_pin = 1; //idk what pin it really is
volatile int numHits = 0; //_MUST_ be volatile so compiler doesn't fuck with it
const unsigned long thresholdTime = 100; //number of milliseconds to track hits in
unsigned long lastReset = micros();

// set up the mqtt client
IPAddress server(192, 168, 0, 100);
MQTT mqtt = MQTT(server, 1883);

StaticJsonBuffer<200> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();


void incrementHits()
{
    numHits++;
}

void setup()
{
    pinMode(ir_pin, INPUT);
    attachInterrupt(1, incrementHits, FALLING);

    //set up message ahead of time to reduce delay in sending message
    root["tape"] = "DETECTED";
    root.printTo(mqtt.stringBuffer, sizeof(mqtt.stringBuffer));

    mqtt.init();
    mqtt.loop();

    //nothing for now but might subscribe to something later idk
    //mqtt.client.subscribe();
}

void loop()
{
    if(numHits > 1) //send the message if more than one hit in the time interval specified
    {
        mqtt.client.publish("sensors/left/nav", mqtt.stringBuffer);
    }

    if((micros() - lastReset) > thresholdTime)
    {
        numHits = 0;
        lastReset = micros();
    }

}
