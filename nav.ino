#include <ArduinoJson.h>
#include "mqtt.h"

//TODO: code for finding velocity by measuring time between single tapes

int ir_pin = 2; //might have to change pin to interrupt, should be 2/3 to be same as Uno
volatile int numHits = 0; //_MUST_ be volatile so compiler doesn't fuck with this/optimize it out
const unsigned long thresholdTime = 100; //number of microseconds to track hits in
unsigned long lastReset = micros();
char* topic = "sensors/left/nav";

// set up the mqtt client
IPAddress server(192, 168, 0, 100);
MQTT mqtt = MQTT(server, 1883);

StaticJsonBuffer<200> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

//function called by interrupt
void incrementHits()
{
    numHits++;
}

void setup()
{
    pinMode(ir_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(1), incrementHits, FALLING);     //have to map the pin to the actual interrupt number

    //set up message ahead of time to reduce delay in sending message
    //will change the message later to be whatever it should be when braking should be on
    root["tape"] = "DETECTED";
    root.printTo(mqtt.stringBuffer, sizeof(mqtt.stringBuffer));

    mqtt.init();
    mqtt.loop();

    mqtt.client.subscribe(topic);
}

void loop()
{
    if(numHits > 1) //send the message if more than one hit in the time interval specified
    {
        mqtt.client.publish(topic, mqtt.stringBuffer);
    }

    if((micros() - lastReset) > thresholdTime)
    {
        numHits = 0;
        lastReset = micros();
    }

}
