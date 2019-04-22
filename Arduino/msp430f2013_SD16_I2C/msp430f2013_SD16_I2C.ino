/******************************************************************************
 * control SD16_A of MSP430F2013 as a I2C ADC
 * - MSP430 address: 0X0B
 ******************************************************************************
 *
 *              Arduino Uno/Nano
 *             -----------------
 *            |                 |
 *   I2C SDA -|A4               |
 *   I2C SCL -|A5               |
 *            |                 |
 *            |                 |
 *
 * check voltage levels between master and slave (iso converter if needed)
 *
 * Haroldo Amaral - 2019
 * https://github.com/agaelema/msp430f20x3_as_i2c_16bit_adc
 ******************************************************************************/

#include "Arduino.h"
#include <Wire.h>


/******************************************************************************
 * SD16 definitions and header
 ******************************************************************************/
#include "sd16_header.h"
#define 		SLV_Addr		0x0B			// Address is 0x0B<<1 for R/W

/* Select operation mode - uncomment desired mode - comment all others */

//#define			I2C_ADC_START_CONVERSION		// start with current configuration
//#define			I2C_ADC_STOP_CONVERSION
#define			I2C_ADC_START_SING_CONVERSION_CH1_GAIN1x_BIPOLAR_1024_2S
//#define			I2C_ADC_START_CONT_CONVERSION_CH1_GAIN1x_BIPOLAR_1024_2S


/******************************************************************************
 * ADS1115 configuration
 ******************************************************************************/
#define		ENABLE_ADS1115		// enable/disable ADS1115

#if defined ( ENABLE_ADS1115 )
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads(0x48);
#endif



/******************************************************************************
 * setup
 ******************************************************************************/
void setup()
{
	Serial.begin (9600);

	while (!Serial);

#if		defined (ENABLE_ADS1115)
	ads.setGain(GAIN_FOUR);  // 4x gain   +/- 1.024V  1 bit = 0.03125mV
	ads.begin();
#endif
}


/******************************************************************************
 * main program
 ******************************************************************************/
void loop()
{
	int var = 0;
	uint8_t counter = 0;
	Wire.begin();

	while(1)
	{

#if defined	(I2C_ADC_START_CONVERSION)
		/*
		 * set slave address - configuration register - register value - end transmission
		 */
		Wire.beginTransmission (SLV_Addr);
		var = Wire.write(SD16_CONVERSION);
		var = Wire.write(SD16_START_CONVERSION);
		var = Wire.endTransmission();
#endif
#if defined	(I2C_ADC_STOP_CONVERSION)
		/*
		 * set slave address - configuration register - register value - end transmission
		 */
		Wire.beginTransmission (SLV_Addr);
		var = Wire.write(SD16_CONVERSION);
		var = Wire.write(SD16_STOP_CONVERSION);
		var = Wire.endTransmission();
#endif
#if defined	(I2C_ADC_START_SING_CONVERSION_CH1_GAIN1x_BIPOLAR_1024_2S)
		/*
		 * set slave address - configuration register - register value - end transmission
		 */
		Wire.beginTransmission (SLV_Addr);
		var = Wire.write(SD16_CHCTRL_LOW);
		var = Wire.write(SD16_DF_2S_COMP);
		var = Wire.endTransmission();

		Wire.beginTransmission (SLV_Addr);
		var = Wire.write(SD16_CHCTRL_HIGH);
		var = Wire.write(SD16_OSR_1024x|SD16_SNG_CONV|SD16_BIPOLAR);
		var = Wire.endTransmission();

		Wire.beginTransmission (SLV_Addr);
		var = Wire.write(SD16_IN_CTRL);
		var = Wire.write(SD16_CH1|SD16_GAIN1x);
		var = Wire.endTransmission();

		Wire.beginTransmission (SLV_Addr);
		var = Wire.write(SD16_CONVERSION);
		var = Wire.write(SD16_START_CONVERSION);
		var = Wire.endTransmission();
#endif
#if defined	(I2C_ADC_START_CONT_CONVERSION_CH1_GAIN1x_BIPOLAR_1024_2S)
		if ( counter == 0 )
		{
			/*
			 * set slave address - configuration register - register value - end transmission
			 */
			Wire.beginTransmission (SLV_Addr);
			var = Wire.write(SD16_CHCTRL_LOW);
			var = Wire.write(SD16_DF_2S_COMP);
			var = Wire.endTransmission();

			Wire.beginTransmission (SLV_Addr);
			var = Wire.write(SD16_CHCTRL_HIGH);
			var = Wire.write(SD16_OSR_1024x|SD16_CONT_CONV|SD16_BIPOLAR);
			var = Wire.endTransmission();

			Wire.beginTransmission (SLV_Addr);
			var = Wire.write(SD16_IN_CTRL);
			var = Wire.write(SD16_CH1|SD16_GAIN1x);
			var = Wire.endTransmission();

			Wire.beginTransmission (SLV_Addr);
			var = Wire.write(SD16_CONVERSION);
			var = Wire.write(SD16_START_CONVERSION);
			var = Wire.endTransmission();
		}
		counter++;
#endif

		volatile int c = 0;
		int counter = 0;
		int16_t bufferReceived[2] = {0};


		Wire.requestFrom(SLV_Addr, 2);					// read 2 bytes from i2c
		while (Wire.available())
		{
			c = Wire.read();
			bufferReceived[counter] = (int16_t)c;		// save on buffer
			counter++;
		}

		/*
		 * convert 2 bytes in 1 16-bit integer variable
		 */
		int16_t received = 0;
		received |= bufferReceived[0] & 0xFF;
		received |= (bufferReceived[1] & 0xFF) << 8;


#if		defined (ENABLE_ADS1115)
		int16_t adc0 = 0;
		adc0 = ads.readADC_Differential_0_1();

		Serial.print((int16_t)(adc0 & 0xFFFF)); Serial.print(' ');
#endif
		Serial.println((int16_t)(received & 0xFFFF));

	}
}
