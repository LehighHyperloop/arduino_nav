int ir_pin = 1; //idk what pin it really is
int notification_pin = 5; //pin to signal on once you knnow you should break
int numHits = 0;
const unsigned long thresholdTime = 100; //number of milliseconds to track hits in
unsigned long lastReset = micros();


void incrementHits()
{
    numHits++;
}

void setup()
{
    pinMode(ir_pin, INPUT);
    pinMode(notification_pin, OUTPUT);
    digitalWrite(notification_pin, LOW);
    attachInterrupt(1, incrementHits, FALLING);
}

void loop()
{
    if(numHits > 1)
    {
        digitalWrite(notification_pin, HIGH);
    }
    if((micros() - lastReset) > thresholdTime)
    {
        numHits = 0;
        lastReset = micros();
    }

}
