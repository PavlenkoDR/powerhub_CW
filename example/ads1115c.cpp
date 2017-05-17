
// asd1115.c read TMP37 temperature sensor ANC0
// operates in continuous mode
// pull up resistors in module caused problems
// used level translator - operated ADS1115 at 5V
// by Lewis Loflin lewis@bvu.net
// www.bristolwatch.com
// http://www.bristolwatch.com/rpi/ads1115.html

#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>    // read/write usleep
#include <stdlib.h>    // exit function
#include <inttypes.h>  // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions
#include <limits.h>

// #define __USE_MISC
#include <math.h>

// Connect ADDR to GRD.
// Setup to use ADC0 single ended

#include <bitset>
#include "ads1x15c.h"
/*=========================================================================
    CONFIG REGISTER
    -----------------------------------------------------------------------*/
/*
    #define ADS1015_REG_CONFIG_OS_MASK      (0x8000)
    #define ADS1015_REG_CONFIG_OS_SINGLE    (0x8000)  // Write: Set to start a single-conversion
    #define ADS1015_REG_CONFIG_OS_BUSY      (0x0000)  // Read: Bit = 0 when conversion is in progress
    #define ADS1015_REG_CONFIG_OS_NOTBUSY   (0x8000)  // Read: Bit = 1 when device is not performing a conversion

    #define ADS1015_REG_CONFIG_MUX_MASK     (0x7000)
    #define ADS1015_REG_CONFIG_MUX_DIFF_0_1 (0x0000)  // Differential P = AIN0, N = AIN1 (default)
    #define ADS1015_REG_CONFIG_MUX_DIFF_0_3 (0x1000)  // Differential P = AIN0, N = AIN3
    #define ADS1015_REG_CONFIG_MUX_DIFF_1_3 (0x2000)  // Differential P = AIN1, N = AIN3
    #define ADS1015_REG_CONFIG_MUX_DIFF_2_3 (0x3000)  // Differential P = AIN2, N = AIN3
    #define ADS1015_REG_CONFIG_MUX_SINGLE_0 (0x4000)  // Single-ended AIN0
    #define ADS1015_REG_CONFIG_MUX_SINGLE_1 (0x5000)  // Single-ended AIN1
    #define ADS1015_REG_CONFIG_MUX_SINGLE_2 (0x6000)  // Single-ended AIN2
    #define ADS1015_REG_CONFIG_MUX_SINGLE_3 (0x7000)  // Single-ended AIN3

    #define ADS1015_REG_CONFIG_PGA_MASK     (0x0E00)
    #define ADS1015_REG_CONFIG_PGA_6_144V   (0x0000)  // +/-6.144V range = Gain 2/3
    #define ADS1015_REG_CONFIG_PGA_4_096V   (0x0200)  // +/-4.096V range = Gain 1
    #define ADS1015_REG_CONFIG_PGA_2_048V   (0x0400)  // +/-2.048V range = Gain 2 (default)
    #define ADS1015_REG_CONFIG_PGA_1_024V   (0x0600)  // +/-1.024V range = Gain 4
    #define ADS1015_REG_CONFIG_PGA_0_512V   (0x0800)  // +/-0.512V range = Gain 8
    #define ADS1015_REG_CONFIG_PGA_0_256V   (0x0A00)  // +/-0.256V range = Gain 16

    #define ADS1015_REG_CONFIG_MODE_MASK    (0x0100)
    #define ADS1015_REG_CONFIG_MODE_CONTIN  (0x0000)  // Continuous conversion mode
    #define ADS1015_REG_CONFIG_MODE_SINGLE  (0x0100)  // Power-down single-shot mode (default)

    #define ADS1015_REG_CONFIG_DR_MASK      (0x00E0)  
    #define ADS1015_REG_CONFIG_DR_128SPS    (0x0000)  // 128 samples per second
    #define ADS1015_REG_CONFIG_DR_250SPS    (0x0020)  // 250 samples per second
    #define ADS1015_REG_CONFIG_DR_490SPS    (0x0040)  // 490 samples per second
    #define ADS1015_REG_CONFIG_DR_920SPS    (0x0060)  // 920 samples per second
    #define ADS1015_REG_CONFIG_DR_1600SPS   (0x0080)  // 1600 samples per second (default)
    #define ADS1015_REG_CONFIG_DR_2400SPS   (0x00A0)  // 2400 samples per second
    #define ADS1015_REG_CONFIG_DR_3300SPS   (0x00C0)  // 3300 samples per second

    #define ADS1015_REG_CONFIG_CMODE_MASK   (0x0010)
    #define ADS1015_REG_CONFIG_CMODE_TRAD   (0x0000)  // Traditional comparator with hysteresis (default)
    #define ADS1015_REG_CONFIG_CMODE_WINDOW (0x0010)  // Window comparator

    #define ADS1015_REG_CONFIG_CPOL_MASK    (0x0008)
    #define ADS1015_REG_CONFIG_CPOL_ACTVLOW (0x0000)  // ALERT/RDY pin is low when active (default)
    #define ADS1015_REG_CONFIG_CPOL_ACTVHI  (0x0008)  // ALERT/RDY pin is high when active

    #define ADS1015_REG_CONFIG_CLAT_MASK    (0x0004)  // Determines if ALERT/RDY pin latches once asserted
    #define ADS1015_REG_CONFIG_CLAT_NONLAT  (0x0000)  // Non-latching comparator (default)
    #define ADS1015_REG_CONFIG_CLAT_LATCH   (0x0004)  // Latching comparator

    #define ADS1015_REG_CONFIG_CQUE_MASK    (0x0003)
    #define ADS1015_REG_CONFIG_CQUE_1CONV   (0x0000)  // Assert ALERT/RDY after one conversions
    #define ADS1015_REG_CONFIG_CQUE_2CONV   (0x0001)  // Assert ALERT/RDY after two conversions
    #define ADS1015_REG_CONFIG_CQUE_4CONV   (0x0002)  // Assert ALERT/RDY after four conversions
    #define ADS1015_REG_CONFIG_CQUE_NONE    (0x0003)  // Disable the comparator and put ALERT/RDY in high state (default)
/*=========================================================================*/
int fd;
// Note PCF8591 defaults to 0x48!
int asd_address = 0x48;
int16_t val;
uint8_t writeBuf[3];
uint8_t readBuf[2];
float myfloat;

const float VPS = 4.096 / 32768.0; // volts per step	4.096/(2^15)
// const float VPS = 6.144 / 32768.0; //volts per step

#define N_READS 1000
int16_t read_arr[N_READS];
#define T_SLEEP 1000

int compare( const void* a, const void* b ) {
	return ( *( int16_t* )a - * ( int16_t* )b );
}

/*
The resolution of the ADC in single ended mode
we have 15 bit rather than 16 bit resolution,
the 16th bit being the sign of the differential
reading.
*/

using namespace std;

int main() {
	/*
	// open device on /dev/i2c-1 the default on Raspberry Pi B
	if( ( fd = open( "/dev/i2c-1", O_RDWR ) ) < 0 ) {
		printf( "Error: Couldn't open device! %d\n", fd );
		exit( 1 );
	}

	// connect to ADS1115 as i2c slave
	if( ioctl( fd, I2C_SLAVE, asd_address ) < 0 ) {
		printf( "Error: Couldn't find device on address!\n" );
		exit( 1 );
	}

	// set config register and start conversion
	// AIN0 and GND, 4.096v, 128s/s
	// Refer to page 19 area of spec sheet
	writeBuf[0] = 1; // config register is 1		//0 - чтение 1 - чтение\запись

	/*
	uint16_t config =   ADS1015_REG_CONFIG_CQUE_2CONV   | // Disable the comparator (default val)
		            ADS1015_REG_CONFIG_CLAT_LATCH   | // Non-latching (default val)
		            ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
		            ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
		            ADS1015_REG_CONFIG_DR_2400SPS   | // 1600 samples per second (default)
			    ADS1015_REG_CONFIG_MUX_SINGLE_0 |
			    ADS1015_REG_CONFIG_OS_SINGLE    |
			    ADS1015_REG_CONFIG_PGA_4_096V   |
		            ADS1015_REG_CONFIG_MODE_CONTIN;   // Single-shot mode (default)

	cout << std::bitset<sizeof(ADS1015_REG_CONFIG_CQUE_2CONV)   * CHAR_BIT>(ADS1015_REG_CONFIG_CQUE_2CONV) << endl;
	cout << std::bitset<sizeof(ADS1015_REG_CONFIG_CLAT_LATCH)   * CHAR_BIT>(ADS1015_REG_CONFIG_CLAT_LATCH) << endl;
	cout << std::bitset<sizeof(ADS1015_REG_CONFIG_CPOL_ACTVLOW) * CHAR_BIT>(ADS1015_REG_CONFIG_CPOL_ACTVLOW) << endl;
	cout << std::bitset<sizeof(ADS1015_REG_CONFIG_CMODE_TRAD)   * CHAR_BIT>(ADS1015_REG_CONFIG_CMODE_TRAD) << endl;
	cout << std::bitset<sizeof(ADS1015_REG_CONFIG_DR_2400SPS)   * CHAR_BIT>(ADS1015_REG_CONFIG_DR_2400SPS) << endl;
	cout << std::bitset<sizeof(ADS1015_REG_CONFIG_MODE_SINGLE)  * CHAR_BIT>(ADS1015_REG_CONFIG_MODE_SINGLE) << endl;
	cout << std::bitset<sizeof(ADS1015_REG_CONFIG_OS_SINGLE)    * CHAR_BIT>(ADS1015_REG_CONFIG_OS_SINGLE) << endl;
	cout << std::bitset<sizeof(ADS1015_REG_CONFIG_PGA_4_096V)   * CHAR_BIT>(ADS1015_REG_CONFIG_PGA_4_096V) << endl;
	cout << std::bitset<sizeof(ADS1015_REG_CONFIG_MUX_SINGLE_0) * CHAR_BIT>(ADS1015_REG_CONFIG_MUX_SINGLE_0) << endl;
	cout << endl;
	writeBuf[1] = (config>>8); // 0xC2 single shot off
	// bit 15 flag bit for single shot not used here
	// Bits 14-12 input selection:
	// 100 ANC0; 101 ANC1; 110 ANC2; 111 ANC3
/*
	000: AINP = AIN0 и AINN = AIN1 100: AINP = AIN0 и AINN = GND
	001: AINP = AIN0 и AINN = AIN3 101: AINP = AIN1 и AINN = GND
	010: AINP = AIN1 и AINN = AIN3 110: AINP = AIN2 и AINN = GND
	011: AINP = AIN2 и AINN = AIN3 111: AINP = AIN3 и AINN = GND
*/
	// Bits 11-9 Amp gain. Default to 010 here 001 P19
/*
ПГ: 000 бит для выбора коэффициента усиления входного усилителя: FS = ± 6.144V (2/3 раза) 100: FS = ± 0.512V ( 8 - кратного)
001: FS = ± 4.096V (1 ×) 101: FS = ± 0.256V (16-кратное)
010: FS = ± 2.048V (2 раза) 110: FS = ± 0.256V (16-кратное)
011: FS = ± 1.024V (4 раза) 111: FS = ± 0.256V (16-кратное)
Значение по умолчанию «010». FS = полномасштабная 0-6.144V Если значение счетчика (0-2047), и составляет ± 1.024V (-2047 ~ 0 ~ 2047 ).
*/
	// Bit 8 Operational mode of the ADS1115.
	// 0 : Continuous conversion mode
	// 1 : Power-down single-shot mode (default)
	/*
	writeBuf[2] = config & 0xFF; // bits 7-0  0x85
	// Bits 7-5 data rate default to 100 for 1600SPS	
/*
	000 : 128SPS　 100 : 1600SPS
　　　　　	001 : 250SPS　 101 : 2400SPS
　　　　　	010 : 490SPS　 110 : 3300SPS
　　　　　	011 : 920SPS　 111 : 3300SPS
*/
	// Bits 4-0  comparator functions see spec sheet.

	// begin conversion
/*
	cout << std::bitset<sizeof(config) * CHAR_BIT>(config) << endl;
	cout << std::bitset<sizeof(writeBuf[1]) * CHAR_BIT>(writeBuf[1]) << endl;
	cout << std::bitset<sizeof(writeBuf[2]) * CHAR_BIT>(writeBuf[2]) << endl;
	if( write( fd, writeBuf, 3 ) != 3 ) {
		perror( "Write to register 1" );
		exit( 1 );
	}
	
	sleep( 1 );
	*/
	printf( "ASD1115 Demo will take %d readings.\n", N_READS );

	Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
  	ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  	ads.startComparator_SingleEnded(0, 1000);

	// set pointer to 0
	/*
	readBuf[0] = 0;

	if( write( fd, readBuf, 1 ) != 1 ) {
		perror( "Write register select" );
		exit( -1 );
	}

	// take N_READS readings:
	*/
	int idx = 0;

	while( 1 )   {

		// read conversion register
		/*
		if( read( fd, readBuf, 2 ) != 2 ) {
			perror( "Read conversion" );
			exit( -1 );
		}
		/*

		if( read( fd, &readBuf[0], 1 ) != 1 ) {
			perror( "Read conversion" );
			exit( -1 );
		}
		if( read( fd, &readBuf[1], 1 ) != 1 ) {
			perror( "Read conversion" );
			exit( -1 );
		}
		*/

		// could also multiply by 256 then add readBuf[1]
		//val = readBuf[0] << 8 | readBuf[1];
		val = ads.getLastConversionResults();
		// with +- LSB sometimes generates very low neg number.
		if( val < 0 )   val = 0;

		read_arr[idx] = val;

		myfloat = val * VPS; // convert to voltage

		//printf("Conversion number %d HEX 0x%02x DEC %d %4.3f volts.\n",
		//       count, val, val, myfloat);
		// TMP37 20mV per deg C
		// printf("Temp. C = %4.2f \n", myfloat / 0.02);
		// printf("Temp. F = %4.2f \n", myfloat / 0.02 * 9 / 5 + 32);

		/* Output:
		 Conversion number 1 HEX 0x1113 DEC 4371 0.546 volts.
		 Temp. C = 27.32
		 Temp. F = 81.17
		 */

		usleep( 1000 );

		idx++; // inc count

		if( idx == N_READS )   break;

	} // end while loop

	// power down ASD1115
	/*
	writeBuf[0] = 1;    // config register is 1
	writeBuf[1] = 0b11000011; // bit 15-8 0xC3 single shot on
	writeBuf[2] = 0b10000101; // bits 7-0  0x85

	if( write( fd, writeBuf, 3 ) != 3 ) {
		perror( "Write to register 1" );
		exit( 1 );
	}

	close( fd );
	*/
	int16_t min = SHRT_MAX, max = SHRT_MIN;

	for( idx = 0; idx < N_READS; idx++ ) {
		val = read_arr[idx];

		if( val > max ) max = val;

		if( val < min ) min = val;

		myfloat = val * VPS; // convert to voltage
		printf(
			"%d | 0x%02x | %d | %6.5f V \n",
			idx, val, val, myfloat
		);
	}

	printf( "min value: 0x%02x | %d | %4.3f \n", min, min, min * VPS );
	printf( "max value: 0x%02x | %d | %4.3f \n", max, max, max * VPS );

	// calculate median
	qsort( read_arr, N_READS, sizeof( int16_t ), compare );
	int16_t med = 0;

	if( N_READS % 2 == 0 ) {
		int m0 = read_arr[ N_READS / 2 ];
		int m1 = read_arr[ N_READS / 2 - 1 ];
		med = ( int16_t )( ( m0 + m1 ) / 2 );
	} else med = read_arr[ N_READS / 2 ];

	printf( "med value: 0x%02x | %d | %4.3f \n", med, med, med * VPS );

	printf( "powerload: %4.3f\n", 220.0f * ( ( ( max - min ) * VPS ) / 5.0f * 30.0f ) * M_SQRT1_2 );

	return 0;
}
