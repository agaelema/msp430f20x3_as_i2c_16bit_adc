/******************************************************************************
 * sd16_header.h
 *
 *  Created on: 21 de abr de 2019
 *      Author: Haroldo Amaral
 *      v0.1
 ******************************************************************************/

#ifndef SD16_HEADER_H_
#define SD16_HEADER_H_


/* DS16 */
#define     ADC_GAIN            (1)                             // pga gain
#define     VREF                (double)(1.2)                   // reference voltage
#define     VFSR                (double)((VREF/2.0)/ADC_GAIN)   // full scale range
#define     ADC_BITS            ((1UL << 15) - 1)               // 2^15-1 - bipolar max count - "lost" one bit to signal
#define     ADC_VBIT            (float)(VFSR/ADC_BITS)          // voltage per bit

/*
 * Configuration registers
 */
#define     SD16_CHCTRL_LOW             (0xA0)          // SD16CCTL0 (low byte)
#define     SD16_CHCTRL_HIGH            (0xA1)          // SD16CCTL0 (high byte)
#define     SD16_IN_CTRL                (0xB0)          // SD16INCTL0
#define     SD16_CONVERSION             (0xFF)          // used just to start/stop conversion


/* SD16_CTRL_LOW */
#define     SD16_DF_OFFSET              (0 << 4)        // low level on bit 4 - offset format
#define     SD16_DF_2S_COMP             (1 << 4)        // low level on bit 4 - offset format
/* SD16_CTRL_HIGH */
#define     SD16_OSR_32x                (0x03)
#define     SD16_OSR_64x                (0x02)
#define     SD16_OSR_128x               (0x01)
#define     SD16_OSR_256x               (0x00)
#define     SD16_OSR_512x               (0x08)
#define     SD16_OSR_1024x              (0x09)
#define     SD16_SNG_CONV               (1 << 2)        // single conversion bit (in a byte)
#define     SD16_CONT_CONV              (0 << 2)        // continuous conversion bit (in a byte)
#define     SD16_UNIPOLAR               (1 << 4)        // unipolar input
#define     SD16_BIPOLAR                (0 << 4)        // bipolar/differential input
/* SD16_IN_CTRL */
#define     SD16_CH0                    (0x00)
#define     SD16_CH1                    (0x01)
#define     SD16_CH2                    (0x02)
#define     SD16_CH3                    (0x03)
#define     SD16_CH4                    (0x04)
#define     SD16_CH5_VCC_VSS            (0x05)          // measure supply voltage - see datasheet
#define     SD16_CH6_Temperature        (0x06)          // temperature sensor
#define     SD16_CH7_ShortCircuit       (0x07)          // used to measure internal offset
#define     SD16_GAIN1x                 (0x00 << 3)
#define     SD16_GAIN2x                 (0x01 << 3)
#define     SD16_GAIN4x                 (0x02 << 3)
#define     SD16_GAIN8x                 (0x03 << 3)
#define     SD16_GAIN16x                (0x04 << 3)
#define     SD16_GAIN32x                (0x05 << 3)
/* SD16_CONVERSION */
#define     SD16_STOP_CONVERSION        (0x00)          // start bit in low level
#define     SD16_START_CONVERSION       (0x01)          // start bit in high level



#endif /* SD16_HEADER_H_ */
