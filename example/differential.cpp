#include "Adafruit_ADS1015.h"
#include <unistd.h>
#include <iostream>
#include <cstdint>

using namespace std;

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//Adafruit_ADS1015 ads;     /* Use thi for the 12-bit version */
const float VPS = 4.096 / 32768.0; // volts per step	4.096/(2^15)

void setup()
{
	cout << "Hello!" << endl;
	cout << "Getting differential reading from AIN0 (P) and AIN1 (N)" << endl;
	cout << "ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)" << endl;
  
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  
  //ads.begin();
  ads.startComparator_SingleEnded(0, 1000);
}

void loop()
{
  	int16_t adc0;
	while(true)
	{
		
  		adc0 = ads.getLastConversionResults();
		cout << "AIN0: " << adc0*VPS << endl;
	  	usleep(1000);
	}
}

int main()
{
	setup();
	loop();
	return 0;
}
