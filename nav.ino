#include <ArduinoJson.h>
#include "mqtt.h"

const int ir_pin = 2; //might have to change pin to interrupt, should be 2/3 to be same as Uno
const char* topic = "sensors/left/nav";
const unsigned long thresholdTime = 100000; //number of microseconds to track hits in
const double tapeDistance = 100; //100 feet between single tapes
volatile int numHits = 0; //_MUST_ be volatile so compiler doesn't fuck with this/optimize it out
unsigned long lastReset = micros();
unsigned long lastHit = micros();
double roughVelocity = 0.0; //velocity based on time between tapes, using known distance between tapes
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
        if(distanceRemaining == ">1000")
        {
            roughVelocity = 6.333333 / (micros() - lastHit);
            distanceRemaining = "<1000";
            root["distanceRemaining"] = distanceRemaining;
        }
        else
        {
            roughVelocity = 3.00 / (micros() - lastHit);
            distanceRemaining = "<500";
            root["distanceRemaining"] = distanceRemaining;
        }
        //don't need to publish here since the next instruction is the publish after the else
    }
    else //if in range of normally spaced tape, report velocity based on 100ft tape distance
    {
        roughVelocity = tapeDistance / (micros() - lastHit); //velocity in ft/microsecond
    }

    // publish velocity on each loop
    root["velocity"] = roughVelocity;
    root["distanceRemaining"] = distanceRemaining;
    sendMessage(root);

    if((micros() - lastReset) > thresholdTime) //reset timer every *thresholdTime* microseconds
    {
        numHits = 0;
        lastReset = micros();
    }
}
