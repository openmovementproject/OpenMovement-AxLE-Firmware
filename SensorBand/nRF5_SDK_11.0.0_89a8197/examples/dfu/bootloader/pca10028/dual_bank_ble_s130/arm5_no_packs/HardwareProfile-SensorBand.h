// Karim Ladha 2016
// Hardware configuration file for Mi band clone
#ifndef _HARDWARE_PROFILE_H_
#define _HARDWARE_PROFILE_H_

#include <string.h>
#include "nrf.h"
#include "config.h"

/*
IDE set include paths:
$(TargetsDir)/nRF51/CMSIS
$(TargetsDir)/CMSIS_3/CMSIS/Include
*/
/*
iB001W IO PINS:
Vibrator:P0.13
LED right:P0.15
LED middle:P0.14
VBAT_ADC:P0.06
ACC_CS: P0.02
ACC_SCK: P0.05
ACC_SDO: P0.03
ACC_SDI: P0.04
ACC_INT1: P0.00
ACC_INT2: P0.01
Tied to Vdd: P0.30
Tied to Gnd: P0.27
Tied to Gnd: P0.26
*/

//#define InitIO()	{memset((void*)&NRF_GPIO->PIN_CNF[0],0,sizeof(NRF_GPIO->PIN_CNF)); /* Special pin cfg */\
//					/*Bit numbers		 3322222222221111111111			 */\
//					/*					 10987654321098765432109876543210*/\
//					NRF_GPIO->DIRCLR = 0b11111111111111111111111111111111;	/* Default as inputs */\
//					NRF_GPIO->DIRCLR = 0b01001100000000000000000000001011;	/* Set input pins */\
//					NRF_GPIO->DIRSET = 0b00000000000000001110000000110100;	/* Set output pins */\
//					NRF_GPIO->DIRSET = 0b10110011111111110001111110000000;	/* Drive floating pins */\
//					NRF_GPIO->OUTCLR = 0b10110011111111111111111110110000;	/* Set output low pins */\
//					NRF_GPIO->OUTSET = 0b00000000000000000000000000000100;	/* Set output high pins */\
//					NRF_POWER->DCDCEN = 1;									/* DCDC converter enabled */\
//					/*SystemClock(STARTUP_CLOCK_ARG);	*/						/* Startup cpu clocks */}

#define InitIO()	{memset((void*)&NRF_GPIO->PIN_CNF[0],0,sizeof(NRF_GPIO->PIN_CNF)); /* Special pin cfg */\
																				/*Bit numbers		 	3322 2222 2222 1111 1111 11			 */\
																				/*					 			1098 7654 3210 9876 5432 1098 7654 3210*/\
					NRF_GPIO->DIRCLR = 0xFFFFFFFF; /*							0b1111 1111 1111 1111 1111 1111 1111 1111;	* Default as inputs */\
					NRF_GPIO->DIRCLR = 0x4C00000B; /*							0b0100 1100 0000 0000 0000 0000 0000 1011;	* Set input pins */\
					NRF_GPIO->DIRSET = 0x0000E034; /*							0b0000 0000 0000 0000 1110 0000 0011 0100;	* Set output pins */\
					NRF_GPIO->DIRSET = 0xB3FF1F80; /*							0b1011 0011 1111 1111 0001 1111 1000 0000;	* Drive floating pins */\
					NRF_GPIO->OUTCLR = 0xB3FFFFBF; /*							0b1011 0011 1111 1111 1111 1111 1011 0000;	* Set output low pins */\
					NRF_GPIO->OUTSET = 0x00000004; /*							0b0000 0000 0000 0000 0000 0000 0000 0100;	* Set output high pins */\
					NRF_POWER->DCDCEN = 1;									/* DCDC converter enabled */\
					/*SystemClock(STARTUP_CLOCK_ARG);	*/						/* Startup cpu clocks */}

#define GPIO_SENSE_NETS(pLow, pHigh) {	uint32_t input = (GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos) | (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos);	\
										volatile uint32_t pins = ~NRF_GPIO->DIR, *cfg = NRF_GPIO->PIN_CNF; /* Pull inputs up, sense pins not high e.g. Gnd net */	\
										while(pins){if(pins & 0x1){*cfg = input;} cfg++; pins >>=1; } SysTimeDelayMs(10); *pLow = ~(NRF_GPIO->DIR | NRF_GPIO->IN);	\
										input = (GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos) | (GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos);		\
										pins = ~NRF_GPIO->DIR; cfg = NRF_GPIO->PIN_CNF; /* Pull inputs down and sense high. Detect pins held high e.g. Vdd net*/	\
										while(pins){if(pins & 0x1){*cfg = input;} cfg++; pins >>=1; } SysTimeDelayMs(10); *pHigh = (~NRF_GPIO->DIR) & NRF_GPIO->IN;	\
										input = (GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos); pins = ~NRF_GPIO->DIR; cfg = NRF_GPIO->PIN_CNF;				\
										while(pins){if(pins & 0x1){*cfg = input;} cfg++; pins >>=1; } } /* Turn off pull settings before exit */ 

#define USB_BUS_SENSE	Not available

#define LED_1		  Not available
#define LED_2		  14
#define LED_3		  15

// DFU bootloader space increased in size 
// Set to: FLASH (rx) : ORIGIN = 0x3A000, LENGTH = 0x6000 
// Previously: FLASH (rx) : ORIGIN = 0x3C000, LENGTH = 0x3C00

#define BSP_LED_0	 LED_2
#define BSP_LED_1	 LED_3
#define LEDS_MASK	 ((1<<LED_2) | (1<<LED_3))
#define LEDS_INV_MASK	 ((1<<LED_2) | (1<<LED_3))	 
#define ACC_CS		2
#define ACC_SCK	   5
#define ACC_SDO	   3
#define ACC_SDI	   4
#define ACC_INT1	  0
#define ACC_INT2	  1


// Low frequency clock source to be used by the SoftDevice
#define NRF_CLOCK_LFCLKSRC	  {.source		= NRF_CLOCK_LF_SRC_XTAL,			\
								 .rc_ctiv		= 0,								 \
								 .rc_temp_ctiv  = 0,								 \
								 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM}

#endif

