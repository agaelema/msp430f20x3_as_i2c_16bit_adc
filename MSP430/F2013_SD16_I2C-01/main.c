/******************************************************************************
 * SD16_A as a I2C ADC
 * - configured as Slave address 0X0B
 ******************************************************************************
 *
 *                MSP430F20x3
 *             ------------------
 *         /|\|              XIN|-
 *          | |                 |
 *          --|RST          XOUT|-
 *            |                 |
 *    Vin+ -->|A0+ P1.0         |
 *    Vin- -->|A0- P1.1         |
 *    Vin+ -->|A1+ P1.2     P1.7|- <-> 	I2C SDA
 *    Vin- -->|A1- P1.3     P1.6|- <-	I2C CLK
 *    Vin+ -->|A2+ P1.4         |
 *    Vin- -->|A2- P1.5         |
 *            |                 |
 *
 * Haroldo Amaral - 2019
 * https://github.com/agaelema/msp430f20x3_as_i2c_16bit_adc
 ******************************************************************************/

#include <msp430.h>
#include <stdint.h>
#include "sd16_header.h"


/******************************************************************************
 * Definitions and macros
 ******************************************************************************/
#define     false               (0)
#define     true                (1)

/* I2C */
#define     I2C_READ_STATUS     (1)             // Read bit - master read from slave
#define     I2C_WRITE_STATUS    (0)             // write bit - master write to slave
#define     I2C_ACK             (0x00)
#define     I2C_NACK            (0xFF)
#define     I2C_DUMMY_BYTE      (0xFF)

#define     SLAVE_ADDR          (0x0B)          // slave address
#define     RX_MAX_BYTES        (2)             // **** How many bytes?? ****
#define     TX_MAX_BYTES        (2)             // **** How many bytes?? ****


/******************************************************************************
 * Variables
 ******************************************************************************/
/* I2C */
const uint8_t     SLV_Addr = SLAVE_ADDR << 1;   // Address is 0x48<<1 for R/W
uint8_t     i2c_State = 0;                      // machine state variable
uint8_t     i2c_Direction = 0;                  // State variables
uint8_t     rxByteCounter = 0;
uint8_t     txByteCounter = 0;
uint8_t     receivedData[RX_MAX_BYTES] = {0};   // store received data


/* SD16 */
volatile static int16_t    ADC_Read = 0;                        // store conversion result

enum _outputSelection
{
    out_MEM0 = 0,                               // SD16MEM0
};
typedef enum _outputSelection outputSelection;

outputSelection TXdataSelection = out_MEM0;
int16_t     *transmittedValue;                           // pointer to transmitted result - era dataPTR


/******************************************************************************
 * Prototype of functions
 ******************************************************************************/
void Data_RX(void);
void TX_Data(uint8_t data);
void Setup_USI_Slave(void);



/******************************************************************************
 * main code
 ******************************************************************************/
int main(void)
{
    WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog

    DCOCTL = 0;
    BCSCTL1 = CALBC1_16MHZ;
    DCOCTL = CALDCO_16MHZ;

    Setup_USI_Slave();

    /******************************************************************************
     * SD16 default configuration
     * - SD16 clock to 1MHz - SMCLK
     * - 2's format, OSR 1024x, single conversion, differential mode
     * - Channel 1, gain 1x, 4 conversion delay
     ******************************************************************************/
    /* 1.2V ref, SMCLK, div 1x, xdiv 16x */
    SD16CTL = SD16REFON | SD16SSEL_1 | SD16DIV_0 | SD16XDIV_2;
    /* 2's format, 1024 OSR, single conv, differential */
    SD16CCTL0 =  SD16DF | SD16OSR_1024 | SD16SNGL;
    /* A1 +, gain 1x, interrupt after fourth sample */
    SD16INCTL0 = SD16INCH_1 | SD16GAIN_1 | SD16INTDLY_0;
    /* A1+ = P1.2, A1- = P1.3 */
    SD16AE = SD16AE2|SD16AE3;

    P1SEL |= BIT3;              // pin function as Ref output - if not connected to a channel
    SD16CTL |= SD16VMIDON;      // turn on Vref buffer

    P2SEL = 0x00;               // pins 6-7 as in/out - primary function
    P2DIR = 0x00;               // configure as input

    transmittedValue = (int16_t *)&ADC_Read;     // point to ADC_Read variable

    _BIS_SR(LPM1_bits + GIE);           // enable LPM and General Interruption
    __no_operation();
}


/******************************************************************************
 *
 ******************************************************************************/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = SD16_VECTOR
__interrupt void SD16_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(SD16_VECTOR))) SD16_ISR (void)
#else
#error Compiler not supported!
#endif
{
    ADC_Read = SD16MEM0;
}


/******************************************************************************
 * USI interrupt service routine
 * - Rx bytes from master: State 2->4->6->8
 * - Tx bytes to Master: State 2->4->10->12->14
 ******************************************************************************/
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USI_VECTOR
__interrupt void USI_TXRX_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USI_VECTOR))) USI_TXRX_ISR (void)
#else
#error Compiler not supported!
#endif
{
    uint16_t I2C_RW_Bit = 0;                // store i2c direction - read/write
    int16_t txData;                         // temporary save tx value

    /******************************************************************************
     * start condition received?
     ******************************************************************************/
    if (USICTL1 & USISTTIFG)                // Start entry?
    {
        P1OUT |= 0x01;                      // LED on: sequence start
        i2c_State = 2;                      // Enter 1st state on start

        rxByteCounter = 0;
        txByteCounter = 0;
    }

    /******************************************************************************
     * state machine - control rx/tx process
     ******************************************************************************/
    switch( __even_in_range(i2c_State, 16) )
    {
    case 0:                                 // Idle, should not get here
        break;

    case 2:
        USICNT = (USICNT & 0xE0) + 0x08;    // Bit counter = 8, RX address -  bits to be received
        USICTL1 &= ~USISTTIFG;              // Clear start flag
        i2c_State = 4;                      // Go to next state: check address
        break;

    case 4:
        I2C_RW_Bit = (USISRL & 0x01);       // save i2c direction - rx/tx

        if (I2C_RW_Bit == I2C_READ_STATUS)
        {
            i2c_Direction = I2C_READ_STATUS;
        }
        else
        {
            i2c_Direction = I2C_WRITE_STATUS;
        }

        USICTL0 |= USIOE;                   // SDA = output

        /* check if slave address match */
        if ( USISRL == (SLV_Addr|I2C_RW_Bit) )
        {
            USISRL = I2C_ACK;               // load Ack on shift register

            /*
             * Select next state based on i2c direction (rx/tx)
             */
            if (i2c_Direction == I2C_WRITE_STATUS)
            {
                i2c_State = 6;
            }
            if (i2c_Direction == I2C_READ_STATUS)
            {
                i2c_State = 10;
            }
        }
        else                                // if address not match
        {
            USISRL = I2C_NACK;              // load Nack on shift register

            i2c_State = 16;                 // next state: prep for next Start
        }

        USICNT |= 0x01;                     // update USI counter - send (N)Ack bit
        break;

    case 6:
        Data_RX();                          // Receive data byte
        i2c_State = 8;                      // next state: Test data and (N)Ack
        break;

    case 8:
        USICTL0 |= USIOE;                   // SDA = output

        /*
         * receive data and save on buffer
         */
        if (rxByteCounter < RX_MAX_BYTES)   // check rx counter limit
        {
            receivedData[rxByteCounter] = USISRL;   // save byte on buffer
        }
        else;

        rxByteCounter++;

        /*
         * if all bytes were received, process data
         */
        if (rxByteCounter >= RX_MAX_BYTES)
        {
            uint8_t configuration_changed = false;

            /*
             * compare first byte - function identifier
             */
            if (receivedData[0] == SD16_CHCTRL_LOW)         // 0xA0 -> SD16CCTL0 Low byte - 0 to 7
            {
                if ( (SD16CCTL0 & BIT4) != (receivedData[1] & BIT4) )
                {
                    SD16CCTL0 &= ~BIT4;                         // clear data format bit - BIT4
                    SD16CCTL0 |= (receivedData[1] & BIT4);      // save data format value
                    configuration_changed = true;
                }
                else;
            }
            else if (receivedData[0] == SD16_CHCTRL_HIGH)   // 0xA1 -> SD16CCTL0 High byte - 8 to 15
            {
                if ( ((SD16CCTL0>>8) & (BIT0+BIT1+BIT2+BIT3+BIT4)) != (receivedData[1] & (BIT0+BIT1+BIT2+BIT3+BIT4)) )
                {
                    SD16CCTL0 &= ~((BIT0+BIT1+BIT2+BIT3+BIT4) << 8);    // clear bits 8-12
                    SD16CCTL0 |= (receivedData[1] & (BIT0+BIT1+BIT2+BIT3+BIT4)) << 8;     // save received values
                    configuration_changed = true;

                    if ( (SD16CCTL0 & SD16SNGL) )       // if single conversion
                    {
                        SD16CCTL0 &= ~SD16IE;           // disable interruption
                    }
                    else                                // if continuous conversion
                    {
                        SD16CCTL0 |= SD16IE;            // enable interruption
                    }
                }
            }
            else if (receivedData[0] == SD16_IN_CTRL)       // 0xB0 -> SD16INCTL0
            {
                if ( SD16INCTL0 != receivedData[1])
                {
                    SD16INCTL0 = 0x00;
                    SD16INCTL0 = (receivedData[1] & 0xFF);
                    configuration_changed = true;
                }
            }
            else if (receivedData[0] == SD16_CONVERSION)        // 0xFF -> SD16CCTL0 Low byte
            {
                if (receivedData[1] & SD16_START_CONVERSION)    // if start conversion received
                {
                    if ( (SD16CCTL0 & SD16SNGL) )               // if single conversion mode
                    {
                        SD16CCTL0 |= SD16SC;                    // set bit to start the conversion
                        while ( (SD16CCTL0 & SD16IFG) != SD16IFG ); // pooling the flag

                        ADC_Read = SD16MEM0;                    // store conversion
                        transmittedValue = (int16_t *)&ADC_Read;
                    }
                    else                                        // if continuous mode
                    {
                        SD16CCTL0 |= SD16SC;                    // set bit to start the conversion

                        /* reading process inside SD16 ISR */

                        transmittedValue = (int16_t *)&ADC_Read;
                    }
                }
                else                                            // if stop condition
                {
                    SD16CCTL0 &= ~SD16SC;                       // clear bit to stop conversion
                }
            }
            else;

            /*
             * configure SD16AE based on the last changes
             */
            if (configuration_changed)
            {
                /*
                 * check the selected channel and connect to external pins
                 */
                if ( (SD16INCTL0 & 0x07) == SD16INCH_0 )        // mask and compare - channel 0
                {
                    if ( SD16CCTL0 & SD16UNI )                  // if single-ended input
                    {
                        SD16AE = SD16AE0;                       // connect + input to pin
                    }
                    else                                        // if differential input
                    {
                        SD16AE = SD16AE0|SD16AE1;               // connect + and - to pin
                    }
                }
                else if ( (SD16INCTL0 & 0x07) == SD16INCH_1 )
                {
                    if ( SD16CCTL0 & SD16UNI )
                    {
                        SD16AE = SD16AE2;
                    }
                    else
                    {
                        SD16AE = SD16AE2|SD16AE3;
                    }
                }
                else if ( (SD16INCTL0 & 0x07) == SD16INCH_2 )
                {
                    if ( SD16CCTL0 & SD16UNI )
                    {
                        SD16AE = SD16AE4;
                    }
                    else
                    {
                        SD16AE = SD16AE4|SD16AE5;
                    }
                }
                else
                {
                    SD16AE = 0x00;
                }

            }

            /* clear buffer */
            receivedData[0] = 0;
            receivedData[1] = 0;
        }

        /*
         * check number of received bytes
         * - return Ack or Nack
         */
        if (rxByteCounter < (RX_MAX_BYTES))         // If inside the limit
        {
            USISRL = I2C_ACK;                       // load Ack on shift register
            USICNT |= 0x01;                         // load USI counter - send (N)Ack bit

            i2c_State = 6;                          // Rcv another byte
        }
        else                                        // if all bytes received
        {
            USISRL = I2C_NACK;                      // load NAck on shift register
            USICTL0 &= ~USIOE;                      // SDA = input

            i2c_State = 0;                          // Reset state machine
        }

        break;

    case 10:
        /* goto label - return point after send a byte */
        i2cTxLabel:

        if (txByteCounter < TX_MAX_BYTES)       // verify if tx counter is within the limits
        {
            txData = *transmittedValue;
            txData = (txData >> (8 * txByteCounter)) & 0xFF;
            TX_Data(txData);

            txByteCounter++;
        }
        else                                    // if all bytes have been sent
        {
            TX_Data( I2C_DUMMY_BYTE);           // send dummy data "0xFF"
        }

    i2c_State = 12;                         // Go to next state: receive (N)Ack from master
    break;

    case 12:                                    // check (N)Ack from master
        USICTL0 &= ~USIOE;                      // SDA = input
        USICNT |= 0x01;                         // load USI counter, receive (N)Ack
        i2c_State = 14;                         // Go to next state: check (N)Ack
        break;

    case 14:                                    // Process (N)Ack
        if ( USISRL & (I2C_NACK & 0x01) )       // If Nack received from master - stop transmission
        {
            USICTL0 &= ~USIOE;                  // SDA = input

            i2c_State = 0;                      // Reset state machine
        }
        else                                    // Ack received
        {
            goto    i2cTxLabel;                 // tx another byte
        }

        break;

    case 16:                                    // Prep for Start condition
        USICTL0 &= ~USIOE;                      // SDA = input
        i2c_State = 0;                          // Reset state machine

        rxByteCounter = 0;
        txByteCounter = 0;
        break;

    }

    USICTL1 &= ~USIIFG;                         // Clear pending flags
}


void Data_RX(void)
{
    USICTL0 &= ~USIOE;                      // SDA = input
    USICNT |=  0x08;                        // load USI counter, RX byte
}


void TX_Data(uint8_t data)
{
    USICTL0 |= USIOE;                       // SDA = output
    USISRL = data;                          // put data on buffer;
    USICNT |=  0x08;                        // load USI counter, TX byte
}


void Setup_USI_Slave(void)
{
    P1OUT = BIT6 + BIT7;                    // P1.6 & P1.7 Pullups
    P1REN |= BIT6 + BIT7;                   // P1.6 & P1.7 Pullups

    USICTL0 = USIPE6|USIPE7|USISWRST;       // Port & USI mode setup - SCL/SDA enabled
    USICTL1 = USII2C|USIIE|USISTTIE;        // Enable I2C mode & USI interrupts - i2c enabled / interrupt enabled / start condition interrupt enable
    USICKCTL = USICKPL;                     // Setup clock polarity - inactive in High
    USICNT |= USIIFGCC;                     // Disable automatic clear control
    USICTL0 &= ~USISWRST;                   // Enable USI
    USICTL1 &= ~USIIFG;                     // Clear pending flag

    i2c_Direction = 0;
}
