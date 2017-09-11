const int dirpin = 13;
const int steppin = 12;

void setup(){
  
  pinMode(dirpin, OUTPUT);
  pinMode(steppin, OUTPUT);
}

void loop(){
  
  //stepper feed 6 inches
	int i;
			
	digitalWrite(dirpin, LOW);// Set the direction.
	delay(100);

	for (i = 0; i<1600; i++){ // Iterate for 1600 micro steps
		digitalWrite(steppin, LOW); // This LOW to HIGH change is what creates the
		digitalWrite(steppin, HIGH); // "Rising Edge" so the EasyDriver knows to when to step.
		delayMicroseconds(500);} // Set motor speed
}
