#include <ArduinoJson.h>
#include "mqtt.h"

//TODO: combine both velocity and 1000ft status into one publish without losing speed
//TODO: publish the combined message on every loop (don't just alert when you hit the last 1000)

int ir_pin = 2; //might have to change pin to interrupt, should be 2/3 to be same as Uno
volatile int numHits = 0; //_MUST_ be volatile so compiler doesn't fuck with this/optimize it out
const unsigned long thresholdTime = 100; //number of microseconds to track hits in
unsigned long lastReset = micros();
unsigned long lastHit = micros();
char* topic = "sensors/left/nav";
double roughVelocity = 0.0; //velocity based on time between tapes, using known distance between tapes
const double tapeDistance = 100; //100 feet between single tapes
bool past1000 = false;

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

    //set up message ahead of time to reduce delay in sending message
    //will change the message later to be whatever it should be when
    //we hit the last 1000 feet
    root["IR_Trigger"] = "YES";
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

        past1000 = true;

        //bit of a hack to try to get v when in the regions of close tape
        if(!past1000)
            roughVelocity = 6.333333 / (micros() - lastHit);
        else
            roughVelocity = 3.0 / (micros() - lastHit);
    }
    else //if in range of normally spaced tape, report v based on 100ft tape distance
    {
        roughVelocity = tapeDistance / (micros() - lastHit); //velocity in ft/microsecond
    }

    //publish velocity on each loop
    //mqtt.client.publish(topic, mqtt.stringBuffer)

    if((micros() - lastReset) > thresholdTime)
    {
        numHits = 0;
        lastReset = micros();
    }
}
