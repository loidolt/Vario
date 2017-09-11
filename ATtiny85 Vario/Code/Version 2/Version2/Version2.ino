#include <Tone.h>

#include <BMP180.h>

#include <TinyWireM.h>
#include <USI_TWI_Master.h>

int button = 4;
int speaker = 3;
int led = 1;

Tone tone_out1;
Tone tone_out2;

BMP180 barometer;

void setup()
{
	TinyWireM.begin();	//initialize I2C lib
	
	TinyWireM.beginTransmission(9600);	//setup slave's address (7 bit address - same as Wire)

	pinMode(led, OUTPUT);
	
	barometer = BMP180();	//create instance of BMP180 sensor
	
	if(barometer.EnsureConnected())		//check to see if we can connect to the sensor
	{
		analogWrite(led,255);	//indicate connected
		delay(500);
		
		barometer.SoftReset();	//when connected, reset to ensure clean start
		
		barometer.Initialize();		//initialize the sensor and pull calibration data
	}
}
