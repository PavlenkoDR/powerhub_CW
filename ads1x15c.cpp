
#include "ads1x15c.hpp"
int Adafruit_ADS1015::fd = 0;

/**************************************************************************/
/*!
    @brief  Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/

/**************************************************************************/
/*!
    @brief  Writes 16-bits to the specified destination register
*/
/**************************************************************************/
static void writeRegister(uint8_t i2cAddress, uint8_t reg, uint16_t value) {
	uint8_t writeBuf[3];
	writeBuf[0] = reg; // config register is 1	//0 - чтение 1 - чтение\запись
	writeBuf[1] = value>>8; // 0xC2 single shot off
	writeBuf[2] = value & 0xFF; // bits 7-0  0x85

	// begin conversion
	if( write( Adafruit_ADS1015::fd, writeBuf, 3 ) != 3 ) {
		//perror( "Write to register 1" );
		throw "Write to register 1";
	}
}

/**************************************************************************/
/*!
    @brief  Writes 16-bits to the specified destination register
*/
/**************************************************************************/
static uint16_t readRegister(uint8_t i2cAddress, uint8_t reg) {
	uint8_t readBuf[2];
	// connect to ADS1115 as i2c slave
	if( ioctl( Adafruit_ADS1015::fd, I2C_SLAVE, i2cAddress ) < 0 ) {
		//printf( "Error: Couldn't find device on address!" );
		throw "Error: Couldn't find device on address!";
	}
	// set pointer to 0
	readBuf[0] = ADS1015_REG_POINTER_CONVERT;
	if( write( Adafruit_ADS1015::fd, readBuf, 1 ) != 1 ) {
		//perror( "Write register select" );
		throw "Write register select";
	}
	if( read( Adafruit_ADS1015::fd, readBuf, 2 ) != 2 ) {
		//perror( "Read conversion" );
		throw "Read conversion";
	}
  return ((readBuf[0] << 8) | readBuf[1]);  
}

/**************************************************************************/
/*!
    @brief  Instantiates a new ADS1015 class w/appropriate properties
*/
/**************************************************************************/
Adafruit_ADS1015::Adafruit_ADS1015(uint8_t i2cAddress, adsGain_t gain) 
{
   m_i2cAddress = i2cAddress;
   m_conversionDelay = ADS1015_CONVERSIONDELAY;
   m_bitShift = 4;
   m_gain = GAIN_TWOTHIRDS; /* +/- 6.144V range (limited to VDD +0.3V max!) */
	// open device on /dev/i2c-1 the default on Raspberry Pi B
	if( ( Adafruit_ADS1015::fd = open( "/dev/i2c-1", O_RDWR ) ) < 0 ) {
		//printf( "Error: Couldn't open device! %d", Adafruit_ADS1015::fd );
		throw "Error: Couldn't open device! " + std::to_string(Adafruit_ADS1015::fd);
	}
	// connect to ADS1115 as i2c slave
	if( ioctl( Adafruit_ADS1015::fd, I2C_SLAVE, i2cAddress ) < 0 ) {
		//printf( "Error: Couldn't find device on address!" );
		throw "Error: Couldn't find device on address!";
	}
	step = 2048.0;
	setGain(gain);
}
void Adafruit_ADS1015::Adafruit_ADS1015_init(uint8_t i2cAddress, adsGain_t gain) 
{
   m_i2cAddress = i2cAddress;
   m_conversionDelay = ADS1015_CONVERSIONDELAY;
   m_bitShift = 4;
   m_gain = GAIN_TWOTHIRDS; /* +/- 6.144V range (limited to VDD +0.3V max!) */
	// open device on /dev/i2c-1 the default on Raspberry Pi B
	if( ( Adafruit_ADS1015::fd = open( "/dev/i2c-1", O_RDWR ) ) < 0 ) {
		//printf( "Error: Couldn't open device! %d", Adafruit_ADS1015::fd );
		throw "Error: Couldn't open device! " + std::to_string(Adafruit_ADS1015::fd);
	}
	// connect to ADS1115 as i2c slave
	if( ioctl( Adafruit_ADS1015::fd, I2C_SLAVE, i2cAddress ) < 0 ) {
		//printf( "Error: Couldn't find device on address!" );
		throw "Error: Couldn't find device on address!";
	}
	step = 2048.0;
	setGain(gain);
}

/**************************************************************************/
/*!
    @brief  Instantiates a new ADS1115 class w/appropriate properties
*/
/**************************************************************************/
Adafruit_ADS1115::Adafruit_ADS1115(uint8_t i2cAddress, adsGain_t gain)
{
   m_i2cAddress = i2cAddress;
   m_conversionDelay = ADS1115_CONVERSIONDELAY;
   m_bitShift = 0;
   m_gain = GAIN_TWOTHIRDS; /* +/- 6.144V range (limited to VDD +0.3V max!) */
	// open device on /dev/i2c-1 the default on Raspberry Pi B
	if( ( Adafruit_ADS1015::fd = open( "/dev/i2c-1", O_RDWR ) ) < 0 ) {
		//printf( "Error: Couldn't open device! %d", Adafruit_ADS1015::fd );
		throw "Error: Couldn't open device! " + std::to_string(Adafruit_ADS1015::fd);
	}

	// connect to ADS1115 as i2c slave
	if( ioctl( Adafruit_ADS1015::fd, I2C_SLAVE, i2cAddress ) < 0 ) {
		//printf( "Error: Couldn't find device on address!" );
		throw "Error: Couldn't find device on address!";
	}
	step = 32768.0;
	setGain(gain);
}
void Adafruit_ADS1115::Adafruit_ADS1115_init(uint8_t i2cAddress, adsGain_t gain)
{
   m_i2cAddress = i2cAddress;
   m_conversionDelay = ADS1115_CONVERSIONDELAY;
   m_bitShift = 0;
   m_gain = GAIN_TWOTHIRDS; /* +/- 6.144V range (limited to VDD +0.3V max!) */
	// open device on /dev/i2c-1 the default on Raspberry Pi B
	if( ( Adafruit_ADS1015::fd = open( "/dev/i2c-1", O_RDWR ) ) < 0 ) {
		//printf( "Error: Couldn't open device! %d", Adafruit_ADS1015::fd );
		throw "Error: Couldn't open device! " + std::to_string(Adafruit_ADS1015::fd);
	}

	// connect to ADS1115 as i2c slave
	if( ioctl( Adafruit_ADS1015::fd, I2C_SLAVE, i2cAddress ) < 0 ) {
		//printf( "Error: Couldn't find device on address!" );
		throw "Error: Couldn't find device on address!";
	}
	step = 32768.0;
	setGain(gain);
}

/**************************************************************************/
/*!
    @brief  Sets the gain and input voltage range
*/
/**************************************************************************/
void Adafruit_ADS1015::setGain(adsGain_t gain)
{
  	m_gain = gain;
	switch(gain)
	{
	case(ADS1015_REG_CONFIG_PGA_6_144V):
		VPS = 6.144 / step;
		break;
	case(ADS1015_REG_CONFIG_PGA_4_096V):
		VPS = 4.096 / step;
		break;
	case(ADS1015_REG_CONFIG_PGA_2_048V):
		VPS = 2.048 / step;
		break;
	case(ADS1015_REG_CONFIG_PGA_1_024V):
		VPS = 1.024 / step;
		break;
	case(ADS1015_REG_CONFIG_PGA_0_512V):
		VPS = 0.512 / step;
		break;
	case(ADS1015_REG_CONFIG_PGA_0_256V):
		VPS = 0.256 / step;
		break;
	}
}

/**************************************************************************/
/*!
    @brief  Gets a gain and input voltage range
*/
/**************************************************************************/
adsGain_t Adafruit_ADS1015::getGain()
{
  return m_gain;
}

/**************************************************************************/
/*!
    @brief  Gets a single-ended ADC reading from the specified channel
*/
/**************************************************************************/
uint16_t Adafruit_ADS1015::readADC_SingleEnded(uint8_t channel) {
  if (channel > 3)
  {
    return 0;
  }
  
  // Start with default values
  uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
                    ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
                    ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;

  // Set single-ended input channel
  switch (channel)
  {
    case (0):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
      break;
    case (1):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
      break;
    case (2):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
      break;
    case (3):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
      break;
  }

  // Set 'start single-conversion' bit
  config |= ADS1015_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1015_REG_POINTER_CONFIG, config);

  // Wait for the conversion to complete
  usleep(m_conversionDelay);

  // Read the conversion results
  // Shift 12-bit results right 4 bits for the ADS1015
  return readRegister(m_i2cAddress, ADS1015_REG_POINTER_CONVERT) >> m_bitShift;  
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN0) and N (AIN1) input.  Generates
            a signed value since the difference can be either
            positive or negative.
*/
/**************************************************************************/
int16_t Adafruit_ADS1015::readADC_Differential_0_1() {
  // Start with default values
  uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
                    ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
                    ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;
                    
  // Set channels
  config |= ADS1015_REG_CONFIG_MUX_DIFF_0_1;          // AIN0 = P, AIN1 = N

  // Set 'start single-conversion' bit
  config |= ADS1015_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1015_REG_POINTER_CONFIG, config);

  // Wait for the conversion to complete
  usleep(m_conversionDelay);

  // Read the conversion results
  uint16_t res = readRegister(m_i2cAddress, ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
  if (m_bitShift == 0)
  {
    return (int16_t)res;
  }
  else
  {
    // Shift 12-bit results right 4 bits for the ADS1015,
    // making sure we keep the sign bit intact
    if (res > 0x07FF)
    {
      // negative number - extend the sign to 16th bit
      res |= 0xF000;
    }
    return (int16_t)res;
  }
}

/**************************************************************************/
/*! 
    @brief  Reads the conversion results, measuring the voltage
            difference between the P (AIN2) and N (AIN3) input.  Generates
            a signed value since the difference can be either
            positive or negative.
*/
/**************************************************************************/
int16_t Adafruit_ADS1015::readADC_Differential_2_3() {
  // Start with default values
  uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE    | // Disable the comparator (default val)
                    ADS1015_REG_CONFIG_CLAT_NONLAT  | // Non-latching (default val)
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
                    ADS1015_REG_CONFIG_MODE_SINGLE;   // Single-shot mode (default)

  // Set PGA/voltage range
  config |= m_gain;

  // Set channels
  config |= ADS1015_REG_CONFIG_MUX_DIFF_2_3;          // AIN2 = P, AIN3 = N

  // Set 'start single-conversion' bit
  config |= ADS1015_REG_CONFIG_OS_SINGLE;

  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1015_REG_POINTER_CONFIG, config);

  // Wait for the conversion to complete
  usleep(m_conversionDelay);

  // Read the conversion results
  uint16_t res = readRegister(m_i2cAddress, ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
  if (m_bitShift == 0)
  {
    return (int16_t)res;
  }
  else
  {
    // Shift 12-bit results right 4 bits for the ADS1015,
    // making sure we keep the sign bit intact
    if (res > 0x07FF)
    {
      // negative number - extend the sign to 16th bit
      res |= 0xF000;
    }
    return (int16_t)res;
  }
}

/**************************************************************************/
/*!
    @brief  Sets up the comparator to operate in basic mode, causing the
            ALERT/RDY pin to assert (go from high to low) when the ADC
            value exceeds the specified threshold.

            This will also set the ADC in continuous conversion mode.
*/
/**************************************************************************/
void Adafruit_ADS1015::startComparator_SingleEnded(uint8_t channel, int16_t threshold)
{
  // Start with default values
  uint16_t config = ADS1015_REG_CONFIG_CQUE_1CONV   | // Comparator enabled and asserts on 1 match
                    ADS1015_REG_CONFIG_CLAT_LATCH   | // Latching mode
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low   (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD   | // Traditional comparator (default val)
                    ADS1015_REG_CONFIG_DR_1600SPS   | // 1600 samples per second (default)
                    ADS1015_REG_CONFIG_MODE_CONTIN  | // Continuous conversion mode
                    ADS1015_REG_CONFIG_MODE_CONTIN;   // Continuous conversion mode

  // Set PGA/voltage range
  config |= m_gain;
                    
  // Set single-ended input channel
  switch (channel)
  {
    case (0):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_0;
      break;
    case (1):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_1;
      break;
    case (2):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_2;
      break;
    case (3):
      config |= ADS1015_REG_CONFIG_MUX_SINGLE_3;
      break;
  }

  // Set the high threshold register
  // Shift 12-bit results left 4 bits for the ADS1015
  //printf("@@@@@@");
  writeRegister(m_i2cAddress, ADS1015_REG_POINTER_HITHRESH, threshold << m_bitShift);

  //printf("@@@@@@");
  // Write config register to the ADC
  writeRegister(m_i2cAddress, ADS1015_REG_POINTER_CONFIG, config);
}

/**************************************************************************/
/*!
    @brief  In order to clear the comparator, we need to read the
            conversion results.  This function reads the last conversion
            results without changing the config value.
*/
/**************************************************************************/
int16_t Adafruit_ADS1015::getLastConversionResults()
{
  // Wait for the conversion to complete
  usleep(m_conversionDelay);

  // Read the conversion results
  uint16_t res = readRegister(m_i2cAddress, ADS1015_REG_POINTER_CONVERT) >> m_bitShift;
  if (m_bitShift == 0)
  {
    return (int16_t)res;
  }
  else
  {
    // Shift 12-bit results right 4 bits for the ADS1015,
    // making sure we keep the sign bit intact
    if (res > 0x07FF)
    {
      // negative number - extend the sign to 16th bit
      res |= 0xF000;
    }
    return (int16_t)res;
  }
}

