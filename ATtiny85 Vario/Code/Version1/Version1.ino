//ATtiny Vario
//Version 1

//Include libraries

#include <USI_TWI_Master.h>
#include <tinyBMP085.h>                                 //lib for barometer
#include <TinyWireM.h>					//I2C Master lib for ATtinys which use USI

//Set pins

int button = 4;
int speaker = 3;
int led = 1;

//Tones

const int Note_A = 142;

//Barometer specific objects

tinyBMP085 bmp;

//Variables

float vario_climb_rate_start = 0.5;		//minimum climb beeping value (ex. start climbing beeping at 0.5m/s)
float vario_sink_rate_start = -1.1;		//maximum sink beeping value (ex. start sink beep at -1.1m/s)
#define SAMPLES_ARR 25					//define moving average filter array size (2->30), more means vario is less sensitive ans slower, NMEA output
int      maxsamples=50;
int      samples=0;
float    alt[51];
float    tim[51];
float    beep;
float    Beep_period;
static long k[SAMPLES_ARR];
double     PressureNum = 101325;
double     PressureNum2=0;
float    Altitude=0;


static long Averaging_Filter(long input);
static long Averaging_Filter(long input) // moving average filter function
{
  long sum = 0;
  for (int i = 0; i < SAMPLES_ARR; i++) {
    k[i] = k[i+1];
  }
  k[SAMPLES_ARR - 1] = input;
  for (int i = 0; i < SAMPLES_ARR; i++) {
    sum += k[i];
  }
  return ( sum / SAMPLES_ARR ) ;
}

void play_welcome_beep()				//play only one welcome beep after turning on vario
{
	for (int aa=300;aa<=1500;aa=aa+100)
	{
		tone(aa,4,200);			//play beep on pin (note,duration)
	}
	for (int aa=1500;aa>=100;aa=aa-100)
	{
		tone(aa,4,200);		//play beep on pin (note,duration)
	}
}

void setup()
{
        //Initialize pin modes
	
	pinMode(button,INPUT);
	pinMode(speaker,OUTPUT);
	pinMode(led,OUTPUT);
	
	analogWrite(led,225);						//show it's alive
        delay(500);
        analogWrite(led,255);
        delay(500);
	
	//Wake-up pressure sensor
        bmp.begin();
}

float nmea_vario_cms =0;
float nmea_time_s=0;
float nmea_alt_m=0;
float nmea_old_alt_m=0;

void loop()
{
  int pressure = bmp.readPressure();                                      //pressure callout for barometer
  float time=millis();                                                    //take time, look into arduino millis() function
  float vario=0;
  float N1=0;
  float N2=0;
  float N3=0;
  float D1=0;
  float D2=0;
  float p0=0;
  pressure=(PressureNum, PressureNum2);                         //get one sample from BMP180 in every loop
  long average_pressure = Averaging_Filter(PressureNum);                   //put it in filter and take average, this averaging is for NMEA output
  Altitude = (float)44330 * (1 - pow(((float)PressureNum/p0), 0.190295));  //take new altitude in meters from pressure sample, not from average pressure
  
 nmea_alt_m = (float)44330 * (1 - pow(((float)average_pressure/p0), 0.190295));
 if ((millis() >= (nmea_time_s+1000))){
 nmea_vario_cms = ((nmea_alt_m-nmea_old_alt_m))*100; 
 nmea_old_alt_m = nmea_alt_m;
 nmea_time_s = millis();
 }
 
  for(int cc=1;cc<=maxsamples;cc++){                                    //vario algorithm
    alt[(cc-1)]=alt[cc];                                                //move "altitude" value in every loop inside table
    tim[(cc-1)]=tim[cc];                                                //move "time" value in every loop inside table
  };                                                                    //now we have altitude-time tables
  alt[maxsamples]=Altitude;                                             //put current "altitude" value on the end of the table
  tim[maxsamples]=time;                                                 //put current "time" value on the end of the table
  float stime=tim[maxsamples-samples];
  for(int cc=(maxsamples-samples);cc<maxsamples;cc++){
    N1+=(tim[cc]-stime)*alt[cc];
    N2+=(tim[cc]-stime);
    N3+=(alt[cc]);
    D1+=(tim[cc]-stime)*(tim[cc]-stime);
    D2+=(tim[cc]-stime);
  };

 vario=1000*((samples*N1)-N2*N3)/(samples*D1-D2*D2);
 if ((time-beep)>Beep_period)                          	// make some beep
{
    beep=time;
    if (vario>vario_climb_rate_start && vario<=10 )
    {
          Beep_period=550-(vario*(30+vario));
          tone((1400+(200*vario)),420-(vario*(20+vario))); //when climbing make faster and shorter beeps
    }               
    else if (vario >10) 
    {
          Beep_period=160;
          tone(3450,120);                          //spike climb rate
    }               
    else if (vario< vario_sink_rate_start){           //if you have high performance glider you can change sink beep to -0.95m/s ;)
    {
          Beep_period=200;
          tone(300,340);
    }               
  }
}
}
