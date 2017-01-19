#include <ArduinoJson.h>
#include "mqtt.h"

int ir_pin = 2; //might have to change pin to interrupt, should be 2/3 to be same as Uno
volatile int numHits = 0; //_MUST_ be volatile so compiler doesn't fuck with this/optimize it out
const unsigned long thresholdTime = 100; //number of microseconds to track hits in
unsigned long lastReset = micros();
unsigned long lastHit = micros();
char* topic = "sensors/left/nav";
double roughVelocity = 0.0; //velocity based on time between tapes, using known distance between tapes
const double tapeDistance = 100; //100 feet between single tapes
bool past1000 = true;
char* distanceRemaining = ">1000";

// set up the mqtt client
IPAddress server(192, 168, 0, 100);
MQTT mqtt = MQTT(server, 1883);

StaticJsonBuffer<200> jsonBuffer;
JsonObject& root = jsonBuffer.createObject();

//function called by interrupt
void incrementHits()
{
    numHits++;
    lastHit = micros();
}

void setup()
{
    pinMode(ir_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(1), incrementHits, FALLING);     //have to map the pin to the actual interrupt number

    //set up message ahead of time to reduce overhead of building message
    //will change the message later to be whatever it should be when
    //we hit the last 1000 feet

    mqtt.init();
    mqtt.loop();

    mqtt.client.subscribe(topic);
}

//given a filled JsonObject, publishes it to our topic
void sendMessage(JsonObject &root)
{
    root.printTo(mqtt.stringBuffer, sizeof(mqtt.stringBuffer));
    mqtt.client.publish(topic, mqtt.stringBuffer);
}

void loop()
{
    if(numHits > 1) //send the message if more than one hit in the time interval specified
    {
        //bit of a hack to try to get v when in the regions of close tape
        if(!past1000)
        {
            roughVelocity = 6.333333 / (micros() - lastHit);
            distanceRemaining = "<1000";
            root["distanceRemaining"] = distanceRemaining;
            past1000 = true;
        }
        else
        {
            roughVelocity = 3.0 / (micros() - lastHit);
            distanceRemaining = "<500";
            root["distanceRemaining"] = distanceRemaining;
        }

        root["velocity"] = roughVelocity;
        sendMessage(root);
    }
    else //if in range of normally spaced tape, report v based on 100ft tape distance
    {
        roughVelocity = tapeDistance / (micros() - lastHit); //velocity in ft/microsecond
    }

    // publish velocity on each loop
    root["velocity"] = roughVelocity;
    root["distanceRemaining"] = distanceRemaining;
    sendMessage(root);

    if((micros() - lastReset) > thresholdTime)
    {
        numHits = 0;
        lastReset = micros();
    }
}
