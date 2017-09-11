/*
 Arduino Vario by Jaros, 2012 (dedicated to atmega328 based arduinos)
 Part of the "GoFly" project
 https://sites.google.com/site/jarosrwebsite/para-nav
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 Arduino board creates NMEA like protocol with variometer output and beeping sound.
 LK8000 EXTERNAL INSTRUMENT SERIES 1 - NMEA SENTENCE: LK8EX1
 VERSION A, 110217
 
 $LK8EX1,pressure,altitude,vario,temperature,battery,*checksum
 
 Field 0, raw pressure in hPascal:hPA*100 (example for 1013.25 becomes 101325)
 no padding (987.25 becomes 98725, NOT 098725)
 If no pressure available, send 999999 (6 times 9)
 If pressure is available, field 1 altitude will be ignored
 Field 1, altitude in meters, relative to QNH 1013.25
 If raw pressure is available, this value will be IGNORED (you can set it to 99999
 but not really needed)!(if you want to use this value, set raw pressure to 999999)
 This value is relative to sea level (QNE). We are assuming that currently at 0m
 altitude pressure is standard 1013.25.If you cannot send raw altitude, then send
 what you have but then you must NOT adjust it from Basic Setting in LK.
 Altitude can be negative. If altitude not available, and Pressure not available, set Altitude
 to 99999. LK will say "Baro altitude available" if one of fields 0 and 1 is available.
 Field 2, vario in cm/s
 If vario not available, send 9999. Value can also be negative.
 Field 3, temperature in C , can be also negative.If not available, send 99
 Field 4, battery voltage or charge percentage.Cannot be negative.If not available, send 999.
 Voltage is sent as float value like: 0.1 1.4 2.3 11.2. To send percentage, add 1000.
 Example 0% = 1000. 14% = 1014 . Do not send float values for percentages.
 Percentage should be 0 to 100, with no decimals, added by 1000!
 Credits:
 (1) http://code.google.com/p/bmp085driver/                             //bmp085 library
 (2) http://mbed.org/users/tkreyche/notebook/bmp085-pressure-sensor/    //more about bmp085 and average filter
 (3) http://code.google.com/p/rogue-code/                               //helpfull tone library to make nice beeping without using delay
 (4) http://www.daqq.eu/index.php?show=prj_sanity_nullifier             //how to make loud piezo speaker
 (5) http://lk8000.it                                                   //everything because of that
 (6) http://taturno.com/2011/10/30/variometro-la-rivincita/             //huge thanks for Vario algorithm
 (7) http://code.google.com/p/tinkerit/wiki/SecretVoltmeter             //how to measure battery level using AVR ucontroller
 (8) https://www.sparkfun.com/tutorials/246                             // 3 wires Serial LCD quickstart
 */

#include <Wire.h>                      //i2c library
#include "BMP085.h"                    //bmp085 library, download from url link (1)
#include <Tone.h>                      //tone library, download from url link (3)
#include<stdlib.h>                     //we need that to use dtostrf() and convert float to string
#include <SoftwareSerial.h>            // we need that to comunicate with 3 wires LCD Seral

// Attach the serial display's RX line to digital pin 2
SoftwareSerial mySerial(2,3); // pin 2 = TX, pin 3 = RX (unused)

/////////////////////////////////////////
///////////////////////////////////////// variables that You can test and try
short speaker_pin1 = 8;   //    9        //arduino speaker output -
short speaker_pin2 = 9;    //10          //arduino speaker output +
float vario_climb_rate_start = 0.5;      //minimum climb beeping value(ex. start climbing beeping at 0.4m/s)
float vario_sink_rate_start = -1.1;      //maximum sink beeping value (ex. start sink beep at -1.1m/s)
#define SAMPLES_ARR 25                   //define moving average filter array size (2->30), more means vario is less sensitive and slower, NMEA output
#define UART_SPEED 9600                  //define serial transmision speed (9600,19200, etc...)
#define PROTOCOL 3                       //define NMEA output: 1 = $LK8EX1, 2 = FlymasterF1, 3 = Off-Line Test with Serial Monitor (Arduino) 
#define NMEA_LED_pin 6 //13              // LED NMEA out pin (remember pin Arduino digital pin 6 is not Atmega 328 pin 6)
#define NMEA_OUT_per_SEC 3               // NMEA output string samples per second (1 to 20)
#define VOLUME 2                         // volume setting 0-no sound, 1-low sound volume, 2-high sound volume
/////////////////////////////////////////
/////////////////////////////////////////
BMP085   bmp085 = BMP085();              //set up bmp085 sensor
Tone     tone_out1;
Tone     tone_out2;
long     Temperature = 0;
long     Pressure = 101325;
float    Altitude=0;
float    Battery_Vcc_mV = 0;             //variable to hold the value of Vcc from battery
float    Battery_Vcc_V = 0;              //variable to hold the value of Vcc from battery
const float p0 = 101325;                 //Pressure at sea level (Pa)
unsigned long get_time1 = millis();
unsigned long get_time2 = millis();
unsigned long get_time3 = millis();
int      my_temperature = 1;
char     altitude_arr[6];            //wee need this array to translate float to string 
char     vario_arr[6];               //wee need this array to translate float to string
char     batt_arr[6];
int      samples=40;
int      maxsamples=50;
float    alt[51];
float    tim[51];
float    beep;
float    Beep_period;
static long k[SAMPLES_ARR];

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

void play_welcome_beep()                 //play only once welcome beep after turning on arduino vario
{
  
     
  for (int aa=300;aa<=1500;aa=aa+100)
  {
    tone_out1.play(aa,200);             // play beep on pin (note,duration)
   if (VOLUME==2){ tone_out2.play(aa+5,200);}             // play beep on pin (note,duration)
    delay(100);
  }
  for (int aa=1500;aa>=100;aa=aa-100)
  {
    tone_out1.play(aa,200);             // play beep on pin (note,duration)
   if (VOLUME==2){ tone_out2.play(aa+5,200);}             // play beep on pin (note,duration)
    delay(100);
  }
}

//long readVcc()                         // function to read battery value - still in developing phase
//{ 
//  long result;
//  // Read 1.1V reference against AVcc
//  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
//  delay(2); // Wait for Vref to settle
//  ADCSRA |= _BV(ADSC); // Convert
//  while (bit_is_set(ADCSRA,ADSC));
//  result = ADCL;
//  result |= ADCH<<8;
//  result = 1126400L / result; // Back-calculate AVcc in mV
//  return result;
//}

void setup()                // setup() function to setup all necessary parameters before we go to endless loop() function
{
  Serial.begin(UART_SPEED);       // set up arduino serial port
  Wire.begin();                   // lets init i2c protocol
 
  
  if (VOLUME==1){
  tone_out1.begin(speaker_pin1);       // piezo speaker output pin8 -
  pinMode(speaker_pin2, OUTPUT);      // set pin for output NMEA LED blink;
  digitalWrite(speaker_pin2, LOW); 
  } 
  else if (VOLUME==2){
  tone_out1.begin(speaker_pin1);       // piezo speaker output pin8 -
  tone_out2.begin(speaker_pin2);       // piezo speaker output pin9 +
  }
  bmp085.init(MODE_ULTRA_HIGHRES, p0, false); 
                            // BMP085 ultra-high-res mode, 101325Pa = 1013.25hPa, false = using Pa units
                            // this initialization is useful for normalizing pressure to specific datum.
                            // OR setting current local hPa information from a weather station/local airport (QNH).
  
  
  

  mySerial.begin(9600); // set up serial port for 9600 baud
  delay(2000); // wait for display to boot up
    
  mySerial.write(254); // cursor to beginning of first line
  mySerial.write(128);
  mySerial.write("  INITIALIZING  "); // clear display + legends
  
  delay(500);
  mySerial.write(254); // cursor to beginning of second line
  mySerial.write(192);
  mySerial.write("  ARDUINO VARIO ");      
  
  delay(2000); // 
  mySerial.write(254); // cursor to beginning of first line
  mySerial.write(128);
  mySerial.write("VARIO:      CM/S"); // clear display + legends
  
  mySerial.write(254); // cursor to beginning of second line
  mySerial.write(192);
  mySerial.write("ALTIM:       MTS");
  
  //delay(2000); // wait for display to boot up
  play_welcome_beep();      //everything is ready, play "welcome" sound
  pinMode(NMEA_LED_pin, OUTPUT);      // set pin for output NMEA LED blink;

}
float nmea_vario_cms =0;
float nmea_time_s=0;
float nmea_alt_m=0;
float nmea_old_alt_m=0;


char variostring[6], altstring[6]; // create string arrays

//************************************************************************************
//******************************* MAIN LOOP ******************************************
//************************************************************************************
void loop(void)
{
  float time=millis();              //take time, look into arduino millis() function
  float vario=0;
  float N1=0;
  float N2=0;
  float N3=0;
  float D1=0;
  float D2=0;
  bmp085.calcTruePressure(&Pressure);                                   //get one sample from BMP085 in every loop
  long average_pressure = Averaging_Filter(Pressure);                   //put it in filter and take average, this averaging is for NMEA output
  Altitude = (float)44330 * (1 - pow(((float)Pressure/p0), 0.190295));  //take new altitude in meters from pressure sample, not from average pressure
 //
 nmea_alt_m = (float)44330 * (1 - pow(((float)average_pressure/p0), 0.190295));
 if ((millis() >= (nmea_time_s+1000))){
 nmea_vario_cms = ((nmea_alt_m-nmea_old_alt_m))*100; 
 nmea_old_alt_m = nmea_alt_m;
 nmea_time_s = millis();
 }
 //
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
 if ((time-beep)>Beep_period)                          // make some beep
  {
    beep=time;
    if (vario>vario_climb_rate_start && vario<=10 )
    {
      switch (VOLUME) 
      {
        case 0: 
          break;
        case 1:
          Beep_period=550-(vario*(30+vario));
          tone_out1.play((1400+(200*vario)),420-(vario*(20+vario))); //when climbing make faster and shorter beeps
        case 2:
          Beep_period=550-(vario*(30+vario));
          tone_out1.play((1400+(200*vario)),420-(vario*(20+vario))); //when climbing make faster and shorter beeps
          tone_out2.play((1406+(200*vario)),420-(vario*(20+vario)));
      }               
    } else if (vario >10) 
    {
      switch (VOLUME) 
      {
        case 0: 
          break;
        case 1:
          Beep_period=160;
          tone_out1.play(3450,120);                          //spike climb rate
        case 2:
          Beep_period=160;
          tone_out1.play(3450,120);                          //spike climb rate
          tone_out2.play(3456,120);
      }               
    } else if (vario< vario_sink_rate_start){           //if you have high performace glider you can change sink beep to -0.95m/s ;)
      switch (VOLUME) 
      {
        case 0: 
          break;
        case 1:
          Beep_period=200;
          tone_out1.play(300,340);
        case 2:
          Beep_period=200;
          tone_out1.play(300,340);
          tone_out2.play(320,340);
      }               
    }
  }

  if (millis() >= (get_time2+1000))      //every second get temperature and battery level
  {
    bmp085.getTemperature(&Temperature); // get temperature in celsius from time to time, we have to divide that by 10 to get XY.Z
    my_temperature = Temperature/10;
 //   Battery_Vcc_mV = readVcc();                      //get voltage in milivolts
 //   Battery_Vcc_V =(float(Battery_Vcc_mV)/1000);     // get voltage in volts
    get_time2 = millis();
  }

 if ((millis() >= (get_time3+(1000/NMEA_OUT_per_SEC))) && (PROTOCOL == 1))       //every NMEA_OUT/second send NMEA output over serial port (LK8000 protocol)
  {
    int battery_percentage = map(Battery_Vcc_mV, 2500, 4300, 1, 100);  
    String str_out =                                                                 //combine all values and create part of NMEA data string output
      String("LK8EX1"+String(",")+String(average_pressure,DEC)+String(",")+String(dtostrf(Altitude,0,0,altitude_arr))+String(",")+
      String(dtostrf(nmea_vario_cms,0,0,vario_arr))+String(",")+String(my_temperature,DEC)+String(",")+String(battery_percentage+1000)+String(","));
    unsigned int checksum_end,ai,bi;                                                 // Calculating checksum for data string
    for (checksum_end = 0, ai = 0; ai < str_out.length(); ai++)
    {
      bi = (unsigned char)str_out[ai];
      checksum_end ^= bi;
    }
    //creating now NMEA serial output for LK8000. LK8EX1 protocol format:
    //$LK8EX1,pressure,altitude,vario,temperature,battery,*checksum
    Serial.print("$");                     //print first sign of NMEA protocol
    Serial.print(str_out);                 // print data string
    Serial.print("*");                     //end of protocol string
    Serial.println(checksum_end,HEX);      //print calculated checksum on the end of the string in HEX
    get_time3 = millis();
    if (digitalRead(NMEA_LED_pin)== HIGH) {digitalWrite(NMEA_LED_pin, LOW);} else {digitalWrite(NMEA_LED_pin, HIGH);}
   }
   
  if ((millis() >= (get_time3+(1000/NMEA_OUT_per_SEC))) && (PROTOCOL == 2))       //every NMEA_OUT/second send NMEA output over serial port (Flymaster F1 Protocol)
  {
    String str_out =
    String("VARIO,"+String(average_pressure,DEC)+ String(",")+String(dtostrf((nmea_vario_cms/10),0,0,vario_arr))+
    String(",100,0,1")+String(",")+String(my_temperature,DEC)+String(",0"));
    unsigned int checksum_end,ai,bi;                                                 // Calculating checksum for data string
    for (checksum_end = 0, ai = 0; ai < str_out.length(); ai++)
    {
      bi = (unsigned char)str_out[ai];
      checksum_end ^= bi;
    }
    //creating now NMEA serial output for LK8000. Flymaster F1 protocol format:
    //// $VARIO,fPressure,fVario,Bat1Volts,Bat2Volts,BatBank,TempSensor1,TempSensor2*Checksum
    Serial.print("$");                     //print first sign of NMEA protocol
    Serial.print(str_out);                 // print data string
    Serial.print("*");                     //end of protocol string
    Serial.println(checksum_end,HEX);      //print calculated checksum on the end of the string in HEX
    get_time3 = millis();
   if (digitalRead(NMEA_LED_pin)== HIGH) {digitalWrite(NMEA_LED_pin, LOW);} else {digitalWrite(NMEA_LED_pin, HIGH);}
 }
 
 //************************************************************************************************************
  if ((millis() >= (get_time3+(3000/NMEA_OUT_per_SEC))) && (PROTOCOL == 3))       //every NMEA_OUT/second send NMEA output over serial port (Off-Line test)
  {
    //Note: dtostrf(floatVar, minStringWidthIncDecimalPoint, numVarsAfterDecimal, charBuf);
    
    
    String str_out =                                                                 //combine all values and create part of NMEA data string output
      String(String(millis())+String(",")+String(average_pressure,DEC)+String(",")+String(dtostrf(Altitude,0,0,altitude_arr))+String(",")+
      String(dtostrf(nmea_vario_cms,0,0,vario_arr))+String(",")+String(my_temperature,DEC));
    
    //**********************************************************************
    //creating now NMEA serial output for Off-line Test. Protocol output format:
    // 
    //     TIME (s)*1000, PRESSURE (hPa),ALTITUDE(m), VARIO(cm/s), TEMPERATURE (gC)
    //
    //**********************************************************************
     
    Serial.println(str_out);                 // print data string to Arduino Serial 
    //lcd.print((String) dtostrf(nmea_vario_cms,0,0,vario_arr));
    //vario_lcd  = String(dtostrf(nmea_vario_cms,0,0,vario_arr)); 
    //alt_lcd = String(dtostrf(Altitude,0,0,altitude_arr));
    
     //vario_lcd  = 5; 
     //alt_lcd = 1251; 
    
    //stringOne.toCharArray(charBuf, 50)
    
     //variostring  = String( String(dtostrf(nmea_vario_cms,0,0,vario_arr))); 
     //altstring  = String( String(dtostrf(Altitude,0,0,altitude_arr)));
     
    String vario_lcd = String( String(dtostrf(nmea_vario_cms,0,0,vario_arr)));
    String alt_lcd = String( String(dtostrf(Altitude,0,0,altitude_arr)));
    
    vario_lcd.toCharArray(variostring, 6);
    alt_lcd.toCharArray(altstring, 6);
    
    //sprintf(variostring,"%4d", variostring);  // create strings from the numbers right-justify to 4 spaces
    //sprintf(altstring,"%4d",altstring);       // create strings from the numbers right-justify to 4 spaces
    
    ////////////////////////only for simulation
    //vario_lcd = random(10000); // make some fake data
    //alt_lcd = random(1000);
      
    //sprintf(variostring,"%4d",vario_lcd); // create strings from the numbers
    //sprintf(altstring,"%4d",alt_lcd); // right-justify to 4 spaces
    ////////////////////////////////////////////////////
    
    
    //sprintf(variostring,"%4d",dtostrf(nmea_vario_cms*1000,0,0,vario_arr)); // create strings from the numbers
    //sprintf(altstring,"%4d",dtostrf(Altitude*100,0,0,altitude_arr)); // right-justify to 4 spaces
    
    //sprintf(variostring,"%4d",nmea_vario_cms*100); // create strings from the numbers
    
    //  position	 1	 2	 3	 4	 5	 6	 7	 8	 9	 10	 11	 12	 13	 14	 15	 16
    //  line 1	       128	 129	 130	 131	 132	 133	 134	 135	 136	 137	 138	 139	 140	 141	 142	 143
    //  line 2	       192	 193	 194	 195	 196	 197	 198	 199	 200	 201	 202	 203	 204	 205	 206	 207

    mySerial.write(254); // cursor to 8th position on first line
    mySerial.write(135);
    mySerial.write("     "); // from 8th clear 5 positions 
    
    mySerial.write(254); // cursor to 8th position on first line
    mySerial.write(135);
    mySerial.write(variostring); // write out the Vario value
    //mySerial.write("1"); // write out the RPM value

    mySerial.write(254); // cursor to 8th position on second line
    mySerial.write(199);
    mySerial.write("      "); // from 8th position clear 6 positions 
    
    mySerial.write(254); // cursor to 8th position on second line
    mySerial.write(199);
    mySerial.write(altstring); // write out the Altitude value
    

    get_time3 = millis();
    if (digitalRead(NMEA_LED_pin)== HIGH) {digitalWrite(NMEA_LED_pin, LOW);} else {digitalWrite(NMEA_LED_pin, HIGH);}
   }
  //************************************************************************************************************
}
//The End
