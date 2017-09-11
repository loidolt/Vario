#include <TinyWireM.h>
#include <LiquidCrystal_I2C.h>
#include <DHT22.h>
#define DHT22_PIN 1
DHT22 myDHT22(DHT22_PIN);
#define BMP085_ADDRESS 0x77  // I2C address of BMP085   
LiquidCrystal_I2C lcd(0x27,16,2);
char _buffer[16];
    void setup()
    {
      lcd.init();                      // initialize the lcd 
      lcd.backlight();
      bmp085Calibration();
     }

    /* Main program loop */
    void loop()
    {
      DHT22_ERROR_t errorCode;
      short temperature;
      long pressure;
  // The sensor can only be read from every 1-2s, and requires a minimum
  // 2s warm-up after power-on.
  delay(2000);
  errorCode = myDHT22.readData();
  sprintf(_buffer, "Temp. %hi.%01hi C",
                   myDHT22.getTemperatureCInt()/10, abs(myDHT22.getTemperatureCInt()%10));
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(_buffer);
  sprintf(_buffer, "Um %i.%01i %% RH",
          myDHT22.getHumidityInt()/10, myDHT22.getHumidityInt()%10);
  lcd.setCursor(0,0);
  lcd.print(_buffer);
  delay(2000);
     
  temperature = bmp085GetTemperature(bmp085ReadUT());
  pressure = bmp085GetPressure(bmp085ReadUP());
  sprintf(_buffer,"TempBMP  %hi.%01hi C",
             temperature/10,abs(temperature)%10);
  lcd.clear();
  
  lcd.setCursor(0,1);
  lcd.print(_buffer);
  sprintf(_buffer,"Pre %lu.%02lu hPa" ,pressure/100, abs(pressure) % 100);
  lcd.setCursor(0,0);
 
  lcd.print( _buffer);
 }
    
const unsigned char OSS = 0;  // Oversampling Setting
int ac1;
int ac2; 
int ac3; 
unsigned int ac4;
unsigned int ac5;
unsigned int ac6;
int b1; 
int b2;
int mb;
int mc;
int md;
long b5; 
void bmp085Calibration()
{
  ac1 = bmp085ReadInt(0xAA);
  ac2 = bmp085ReadInt(0xAC);
  ac3 = bmp085ReadInt(0xAE);
  ac4 = bmp085ReadInt(0xB0);
  ac5 = bmp085ReadInt(0xB2);
  ac6 = bmp085ReadInt(0xB4);
  b1 = bmp085ReadInt(0xB6);
  b2 = bmp085ReadInt(0xB8);
  mb = bmp085ReadInt(0xBA);
  mc = bmp085ReadInt(0xBC);
  md = bmp085ReadInt(0xBE);
}

// Calculate temperature given ut.
// Value returned will be in units of 0.1 deg C
double bmp085GetTemperature(unsigned int ut)
{
  long x1, x2;
  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;
  return (((b5 + 8)>>4));  
}

// Calculate pressure given up
// calibration values must be known
// b5 is also required so bmp085GetTemperature(...) must be called first.
// Value returned will be pressure in units of Pa.
double bmp085GetPressure(unsigned long up)
{
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;
  
  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;
  
  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;
  
  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;
    
  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;
  
  return p;
}

// Read 1 byte from the BMP085 at 'address'
char bmp085Read(unsigned char address)
{
  unsigned char data;
  
  TinyWireM.beginTransmission(BMP085_ADDRESS);
  TinyWireM.send(address);
  TinyWireM.endTransmission();
  
  TinyWireM.requestFrom(BMP085_ADDRESS, 1);
  while(!TinyWireM.available())
    ;
    
  return TinyWireM.receive();
}

// Read 2 bytes from the BMP085
// First byte will be from 'address'
// Second byte will be from 'address'+1
int bmp085ReadInt(unsigned char address)
{
  unsigned char msb, lsb;
  
  TinyWireM.beginTransmission(BMP085_ADDRESS);
  TinyWireM.send(address);
  TinyWireM.endTransmission();
  
  TinyWireM.requestFrom(BMP085_ADDRESS, 2);
  while(TinyWireM.available()<2)
    ;
  msb = TinyWireM.receive();
  lsb = TinyWireM.receive();
  
  return (int) msb<<8 | lsb;
}

// Read the uncompensated temperature value
unsigned int bmp085ReadUT()
{
  unsigned int ut;
  
  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  TinyWireM.beginTransmission(BMP085_ADDRESS);
  TinyWireM.send(0xF4);
  TinyWireM.send(0x2E);
  TinyWireM.endTransmission();
  
  // Wait at least 4.5ms
  delay(5);
  
  // Read two bytes from registers 0xF6 and 0xF7
  ut = bmp085ReadInt(0xF6);
  return ut;
}

// Read the uncompensated pressure value
unsigned long bmp085ReadUP()
{
  unsigned char msb, lsb, xlsb;
  unsigned long up = 0;
  
  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  TinyWireM.beginTransmission(BMP085_ADDRESS);
  TinyWireM.send(0xF4);
  TinyWireM.send(0x34 + (OSS<<6));
  TinyWireM.endTransmission();
  
  // Wait for conversion, delay time dependent on OSS
  delay(2 + (3<<OSS));
  
  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  TinyWireM.beginTransmission(BMP085_ADDRESS);
  TinyWireM.send(0xF6);
  TinyWireM.endTransmission();
  TinyWireM.requestFrom(BMP085_ADDRESS, 3);
  
  // Wait for data to become available
  while(TinyWireM.available() < 3)
    ;
  msb = TinyWireM.receive();
  lsb = TinyWireM.receive();
  xlsb = TinyWireM.receive();
  
  up = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8-OSS);
  
  return up;
}
