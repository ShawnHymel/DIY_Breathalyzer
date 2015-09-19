/**
 * BAC Display
 * Author: Shawn Hymel
 * Date: September 18, 2015
 *
 * Calculate BAC from MQ-3 and display on 7-Segment Shield
 *
 * There's really only one appropriate license for this code:
 * https://en.wikipedia.org/wiki/Beerware
 */

#include <Wire.h>
#include "BAC_Lookup.h"

// Constants
#define DEBUG       0
#define SENSOR_PIN  A1
#define BRIGHTNESS  80
#define S7_ADDR     0x71
#define DEC_MASK    0b00000001
#define BAC_START   410     // Beginning ADC value of BAC chart
#define BAC_END     859     // Lsat ADC value in BAC chart

// Global variables
char temp_str[10];
int sensor_read;
uint8_t bac;

void setup() {
  
  // Set up debugging
#if DEBUG
  Serial.begin(9600);
  Serial.println("BAC Display");
#endif

  // Initialize pins
  pinMode(SENSOR_PIN, INPUT);

  // Set up I2C and clear display
  Wire.begin();
  
  // Set brightness and clear display
  setBrightnessI2C(BRIGHTNESS);
  clearDisplayI2C();
}

void loop() {
  
  // Read voltage
  sensor_read = analogRead(SENSOR_PIN);
#if DEBUG
  Serial.print("ADC: ");
  Serial.println(sensor_read);
#endif

  // Calculate ppm. Regression fitting from MQ-3 datasheet.
  // Equation using 5V max ADC and RL = 4.7k. "v" is voltage.
  // PPM = 150.4351049*v^5 - 2244.75988*v^4 + 13308.5139*v^3 - 
  //       39136.08594*v^2 + 57082.6258*v - 32982.05333
  // Calculate BAC. See BAC/ppm chart from page 2 of:
  // http://sgx.cdistore.com/datasheets/sgx/AN4-Using-MiCS-Sensors-for-Alcohol-Detection1.pdf
  // All of this was put into the lookup table in BAC_Lookup.h
  if ( sensor_read < BAC_START ) {
      bac = 0;
      sprintf(temp_str, "0000");
  } else if ( sensor_read > BAC_END ) {
      sprintf(temp_str, "EEEE");
  } else {
      sensor_read = sensor_read - BAC_START;
      bac = bac_chart[sensor_read];
      if ( bac < 10 ) {
        sprintf(temp_str, "000%1d", bac);
      } else if ( bac < 100 ) {
        sprintf(temp_str, "00%2d", bac);
      } else {
        sprintf(temp_str, "0%3d", bac);
      }
  }
  
  // Send out to display
  setDecimalsI2C(DEC_MASK);
  s7sSendStringI2C(temp_str);

  // Delay before next reading
  delay(100);
}

/****************************************************************
 * Display Functions
 ***************************************************************/
 
// This custom function works somewhat like a serial.print.
//  You can send it an array of chars (string) and it'll print
//  the first 4 characters in the array.
void s7sSendStringI2C(String toSend)
{
  Wire.beginTransmission(S7_ADDR);
  for (int i=0; i<4; i++)
  {
    Wire.write(toSend[i]);
  }
  Wire.endTransmission();
}
 
// Send the clear display command (0x76)
//  This will clear the display and reset the cursor
void clearDisplayI2C()
{
  Wire.beginTransmission(S7_ADDR);
  Wire.write(0x76);  // Clear display command
  Wire.endTransmission();
}

// Set the displays brightness. Should receive byte with the value
//  to set the brightness to
//  dimmest------------->brightest
//     0--------127--------255
void setBrightnessI2C(byte value)
{
  Wire.beginTransmission(S7_ADDR);
  Wire.write(0x7A);  // Set brightness command byte
  Wire.write(value);  // brightness data byte
  Wire.endTransmission();
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void setDecimalsI2C(byte decimals)
{
  Wire.beginTransmission(S7_ADDR);
  Wire.write(0x77);
  Wire.write(decimals);
  Wire.endTransmission();
}
