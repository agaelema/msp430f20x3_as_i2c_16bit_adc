# msp430f20x3_as_i2c_16bit_adc

Project using the SD16A of MSP430F2013 as an I2C ADC.

The current firmware of MSP430 enables to select between channels 0, 1 and 2, in differential or single-ended mode using continuos os single conversion.

----

Example: Arduino Uno/Nano controlling/reading SD16 converter

Basic Circuit:
![alt text](https://github.com/agaelema/msp430f20x3_as_i2c_16bit_adc/blob/master/_img/34-sch-sd16-basic.png?raw=true "Basic circuit")

### Test 01

The Arduino Nano was connected to the MSP430F2013 and to an ADS1115 to compare the results.

![alt text](https://github.com/agaelema/msp430f20x3_as_i2c_16bit_adc/blob/master/_img/34-sch-sd16-test.png?raw=true "MSP430 as I2C ADC connected to Arudino Nano")

A reference voltage was connected to the conditioning circuit (voltage divider) and adjusted to generate the same code in both converters, as in the following figure.

![alt text](https://github.com/agaelema/msp430f20x3_as_i2c_16bit_adc/blob/master/_img/34-sch-sd16-test-conditioning.png?raw=true "Conditioning circuit")

The result of this test is shown below:

![alt text](https://github.com/agaelema/msp430f20x3_as_i2c_16bit_adc/blob/master/_img/34-teste-01-01-msp430-ads1115.png?raw=true "Test 01 Result")

### Test 02

Using the same circuit of Test 01, but with a signal generator connected to the analog input. On top one signal with bigger voltage, at bottom one signal with small amplitude (more susceptible to noise).

![alt text](https://github.com/agaelema/msp430f20x3_as_i2c_16bit_adc/blob/master/_img/34-teste-02.png?raw=true "Test 02 Result")


___
### DISCLAIMER

This project (software and hardware) is provided 'as is' with no explicit or implied warranties in respect of its properties, including, but not limited to, correctness and/or fitness for purpose.
