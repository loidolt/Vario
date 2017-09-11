int SPEAKER = 8;    // The speaker is on pin 8

int freq = 50;      // Starting frequency

void setup()
{
    pinMode(SPEAKER, OUTPUT);
}

void loop()
{

    tone(SPEAKER, 2500);

    delay(500);
    
    noTone(SPEAKER);
    
    delay(500);
}
