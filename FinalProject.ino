// Hunter Bickel

#include <LiquidCrystal.h>
#include <DHT11.h>
#include <Stepper.h>
#include <Wire.h>
#include <I2C_RTC.h>

#define RDA  (1 << 7)
#define TBE  (1 << 5)

//UART
// UART Pointers
volatile unsigned char *myUCSR0A  = 0x00C0;
volatile unsigned char *myUCSR0B  = 0x00C1;
volatile unsigned char *myUCSR0C  = 0x00C2;
volatile unsigned int  *myUBRR0   = 0x00C4;
volatile unsigned char *myUDR0    = 0x00C6;

//EICRA
volatile unsigned char *myEICRA = 0x69;
volatile unsigned char *myEIMSK = 0x3D;
volatile unsigned char *myEIFR = 0x3C;

//Port A
volatile unsigned char *myPortA = 0x22;
volatile unsigned char *myDDRA = 0x21;
volatile unsigned char *myPinA = 0x20;

//Port B
volatile unsigned char *myPortB = 0x25;
volatile unsigned char *myDDRB = 0x24;
volatile unsigned char *myPinB = 0x23;

//Port E
volatile unsigned char *myPortE = 0x2E;
volatile unsigned char *myDDRE = 0x2D;
volatile unsigned char *myPinE = 0x2C;

//Port L
volatile unsigned char *myPortL = 0x10B;
volatile unsigned char *myDDRL = 0x10A;
volatile unsigned char *myPinL = 0x109;

//Analog Read
volatile unsigned char *my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char *my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char *my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int *my_ADC_DATA = (unsigned int*) 0x78;

//Timer
volatile unsigned char *myTCCR1A = (unsigned char *) 0x80;
volatile unsigned char *myTCCR1B = (unsigned char *) 0x81;
volatile unsigned char *myTCCR1C = (unsigned char *) 0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *) 0x6F;
volatile unsigned int  *myTCNT1  = (unsigned  int *) 0x84;
volatile unsigned char *myTIFR1 =  (unsigned char *) 0x36;

DS1307 RTC;

LiquidCrystal lcd{7, 8, 9, 10, 11, 12}; // LCD setup

DHT11 dht(13); // DHT11 setup
int temp;
int humid;

Stepper stepperName = Stepper(2048, 3, 4, 5, 6);

int waterLevel; //for water level detection

enum state {Idle, Running, Error, Disabled};

enum state currentState = Disabled;

volatile bool startPressed = false;
 
void setup() 
{ 

  U0Init(9600);
  adc_init();

  stepperName.setSpeed(10);
  
  lcd.begin(16, 2); // Set up LCD

  *myDDRB |= (1 << 0); // Red LED
  *myDDRB |= (1 << 2); // Yellow
  *myDDRL |= (1 << 0); // Blue
  *myDDRL |= (1 << 2); // Green

  *myDDRA &= ~(1 << 5); // Stop
  *myPortA |= (1 << 5);
  *myDDRE &= ~(1 << 4); // Start, set to input, turn on pull-up resistor
  *myPortE |= (1 << 4); 
  *myDDRA &= !(1 << 1); // Restart
  *myPortA |= (1 << 1);

  *myDDRB |= (1 << 7); // Fan

  RTC.begin();
  RTC.setHourMode(CLOCK_H12);

  *myEICRA |= (1 << 1); // Setup Interrupt
  *myEICRA &= ~(1 << 0);
  *myEIMSK |= (1 << 0);

  attachInterrupt(digitalPinToInterrupt(2), StartButton, FALLING);

} 
 
 
void loop() 
{ 
  

  if (currentState != Disabled) {

    int result = dht.readTemperatureHumidity(temp, humid);

    if (currentState != Error) {
      if (result == 0) { // Update LCD screen with temp and humidity
        lcd.clear();
        lcd.print("Temp: ");
        lcd.setCursor(5, 0);
        lcd.print(temp);
        lcd.setCursor(0, 1);
        lcd.print("Humid: ");
        lcd.setCursor(6, 1);
        lcd.print(humid);
        lcd.setCursor(0, 0);
      }
    }

    if (*myPinA & (1 << 5)) { // Check if off button gets clicked
      *myPortA &= ~(1 << 7); // Turn fan off
      currentState = Disabled; // Go into DISABLED mode
      *myPortL &= ~(1 << 2);
      *myPortB &= ~(1 << 0);
      *myPortL &= ~(1 << 0);
      stringStream("Transitioning to DISABLED\n");
      IntToString(RTC.getHours());
      stringStream(":");
      IntToString(RTC.getMinutes());
      stringStream(":");
      IntToString(RTC.getSeconds());
      stringStream("\n");
      lcd.clear();
    }
    
    //Control vent position using potentiometer
    int ventVal = adc_read(1);

    if (ventVal > 450) {
      stepperName.step(2048);
    } else {}

    int resVal = adc_read(0);

    if (currentState == Idle) {
      
      
      if (resVal < 500) { // If water level too low, state == error
        *myPortL &= ~(1 << 2); 
        currentState = Error;
        stringStream("Transitioning to ERROR\n");
        IntToString(RTC.getHours());
        stringStream(":");
        IntToString(RTC.getMinutes());
        stringStream(":");
        IntToString(RTC.getSeconds());
        stringStream("\n");
        *myPortB |= (1 << 0); // Red LED on (all others off)
      }

      if (temp >= 22) { // If temp too high, state = RUNNING
        *myPortL &= ~(1 << 2); 
        currentState = Running;
        stringStream("Transitioning to RUNNING\n");
        IntToString(RTC.getHours());
        stringStream(":");
        IntToString(RTC.getMinutes());
        stringStream(":");
        IntToString(RTC.getSeconds());
        stringStream("\n");
        *myPortL |= (1 << 0); // Blue LED on (all others off)
      }
      
    } else if (currentState == Error) {

      if ((*myPinA & (1 << 1)) && resVal >= 200) { // Reset button should change it back to Idle if water is normal
        *myPortB &= ~(1 << 0);
        currentState = Idle;
        lcd.clear();
        stringStream("Transitioning to IDLE\n");
        IntToString(RTC.getHours());
        stringStream(":");
        IntToString(RTC.getMinutes());
        stringStream(":");
        IntToString(RTC.getSeconds());
        stringStream("\n");
        *myPortL |= (1 << 2); // Green LED on
      }

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR: water low");
    } else if (currentState == Running) {
      *myPortA |= (1 << 7);

      if (resVal < 500) { // Transition to ERROR if water is too low
        *myPortL &= ~(1 << 0);
        currentState = Error;
        *myPortA &= ~(1 << 7);
        stringStream("Transitioning to ERROR\n");
        IntToString(RTC.getHours());
        stringStream(":");
        IntToString(RTC.getMinutes());
        stringStream(":");
        IntToString(RTC.getSeconds());
        stringStream("\n");
        *myPortB |= (1 << 0); // Red LED on (all others off)
      }
      
      if (temp < 22) { // Transition to IDLE if temperature drops below threshold
        *myPortL &= ~(1 << 0); 
        currentState = Idle;
        *myPortA &= ~(1 << 7);
        stringStream("Transitioning to IDLE\n");
        IntToString(RTC.getHours());
        stringStream(":");
        IntToString(RTC.getMinutes());
        stringStream(":");
        IntToString(RTC.getSeconds());
        stringStream("\n");
        *myPortL |= (1 << 2); // Green LED on
      }
      
    }

    my_delay(1000);

  } else { //Disabled

  *myPortB |= (1 << 2); // Yellow LED ON

  if (startPressed) {
    currentState = Idle;
    *myPortB &= ~(1 << 2);
    stringStream("Transitioning to IDLE\n");
    IntToString(RTC.getHours());
    stringStream(":");
    IntToString(RTC.getMinutes());
    stringStream(":");
    IntToString(RTC.getSeconds());
    stringStream("\n");
    startPressed = false;
    *myPortL |= (1 << 2); // Green LED on
  }

  }
}

// Interrupt for Start button
void StartButton() {
  startPressed = true;
}

/*
 * UART FUNCTIONS
 */
void U0Init(int U0baud)
{
 unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;
}
unsigned char kbhit()
{
  return (*myUCSR0A & RDA);
}
unsigned char getChar()
{
  return *myUDR0;
}
void putChar(unsigned char U0pdata)
{
  while((*myUCSR0A & TBE)==0);
  *myUDR0 = U0pdata;
}

void stringStream(const char *str) {
  while (*str != '\0') {
    putChar(*str);
    str++;
  }
}

void IntToString(int num) {
  char holder[10];

  int i = 0;
  if (num == 0) {
    holder[i++] = '0';
  }

  while (num != 0) {
    holder[i++] = '0' + (num % 10);
    num /= 10;
  }

  holder[i] = '\0';

  for (int j = 0, k = i - 1; j < k; j++, k --) {
    char temp = holder[j];
    holder[j] = holder[k];
    holder[k] = temp;
  }

  stringStream(holder);
}

void adc_init()
{
  // setup the A register
  *my_ADCSRA |= 0b10000000; // set bit   7 to 1 to enable the ADC
  *my_ADCSRA &= 0b11011111; // clear bit 6 to 0 to disable the ADC trigger mode
  *my_ADCSRA &= 0b11110111; // clear bit 5 to 0 to disable the ADC interrupt
  *my_ADCSRA &= 0b11111000; // clear bit 0-2 to 0 to set prescaler selection to slow reading
  // setup the B register
  *my_ADCSRB &= 0b11110111; // clear bit 3 to 0 to reset the channel and gain bits
  *my_ADCSRB &= 0b11111000; // clear bit 2-0 to 0 to set free running mode
  // setup the MUX Register
  *my_ADMUX  &= 0b01111111; // clear bit 7 to 0 for AVCC analog reference
  *my_ADMUX  |= 0b01000000; // set bit   6 to 1 for AVCC analog reference
  *my_ADMUX  &= 0b11011111; // clear bit 5 to 0 for right adjust result
  *my_ADMUX  &= 0b11100000; // clear bit 4-0 to 0 to reset the channel and gain bits
}

unsigned int adc_read(unsigned char adc_channel_num)
{
  // clear the channel selection bits (MUX 4:0)
  *my_ADMUX  &= 0b11100000;
  // clear the channel selection bits (MUX 5)
  *my_ADCSRB &= 0b11110111;
  // set the channel number
  if(adc_channel_num > 7)
  {
    // set the channel selection bits, but remove the most significant bit (bit 3)
    adc_channel_num -= 8;
    // set MUX bit 5
    *my_ADCSRB |= 0b00001000;
  }
  // set the channel selection bits
  *my_ADMUX  += adc_channel_num;
  // set bit 6 of ADCSRA to 1 to start a conversion
  *my_ADCSRA |= 0x40;
  // wait for the conversion to complete
  while((*my_ADCSRA & 0x40) != 0);
  // return the result in the ADC data register
  return *my_ADC_DATA;
}

void my_delay(unsigned int freq)
{
  *myTCCR1B &= 0xF8;
  *myTCNT1 = (unsigned int) (65536 - freq); 
  *myTCCR1B |= 0b00000101;
  while ((*myTIFR1 & 0x01) == 0);
  *myTCCR1B &= 0xF8;
  *myTIFR1 |= 0x01;
}