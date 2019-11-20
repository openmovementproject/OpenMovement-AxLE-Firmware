// Karim Ladha 2016
// Hardware configuration file for Mi band clone
#ifndef HARDWARE_PROFILE_SENSOR_BAND_H
#define HARDWARE_PROFILE_SENSOR_BAND_H

#include <stdint.h>
#include "config.h"

/*
IDE set include paths:
$(TargetsDir)/nRF51/CMSIS
$(TargetsDir)/CMSIS_3/CMSIS/Include
*/
/*
Original OM band hardware:
iB001W IO PINS:
Vibrator:P0.13
LED right:P0.15
LED middle:P0.14
VBAT_ADC:P0.06/AN7
ACC_CS: P0.02
ACC_SCK: P0.05
ACC_SDO: P0.03
ACC_SDI: P0.04
ACC_INT1: P0.00
ACC_INT2: P0.01
Tied to Gnd: P0.26 ? maybe ?
Tied to Gnd: P0.27 ? maybe ?

ACC_VDDIO : P0.30 ?

Tied to Vdd: P0.31 ? maybe ?
P0.31 not present, input

New ON band hardware:
Vibrator:P0.12
LED blue:P0.15
LED green:P0.13
VBAT_ADC:P0.06/AN7
ACC_CS: P0.02
ACC_SCK: P0.05
ACC_SDO: P0.03
ACC_SDI: P0.04
ACC_INT1: P0.00
ACC_INT2: P0.01

ACC_VDDIO: P0.30
Tied to Gnd: -
Tied to Vdd: -
*/

#ifdef OLD_OM_BAND_HW
#define OUTPUT_PIN_MASK			0x0000E034
#define OUTPUT_LOW_PIN_MASK		0x0000E030
#define OUTPUT_HIGH_PIN_MASK	0x00000004
#define UNCONNECTED_PINS		0xBFFF1F80
#else
#define OUTPUT_PIN_MASK			0x0000B034
#define OUTPUT_LOW_PIN_MASK		0x0000B034
#define OUTPUT_HIGH_PIN_MASK	0x00000004
#define UNCONNECTED_PINS		0xBFFF4F80
#endif

#define InitIO()	{int32_t i; for(i=0;i<32;i++){NRF_GPIO->PIN_CNF[i] = 0x00000002;}/* Special pin cfg, inputs, no in-buffer	*/\
					/*Bit numbers												  3322 2222 2222 1111 1111 11				*/\
					/*															  1098 7654 3210 9876 5432 1098 7654 3210	*/\
					NRF_GPIO->OUTCLR = 0xFFFFFFFF; /*							0b1111 1111 1111 1111 1111 1111 1111 1111;	* Clear output regs */\
					NRF_GPIO->DIRCLR = 0xFFFFFFFF; /*							0b1111 1111 1111 1111 1111 1111 1111 1111;	* Default as inputs */\
					NRF_GPIO->DIRCLR = 0x0000004B; /*							0b1100 1100 0000 0000 0000 0000 0100 1011;	* Set input pins */\
					NRF_GPIO->DIRSET = OUTPUT_PIN_MASK; /*						0b0000 0000 0000 0000 1110 0000 0011 0100;	* Set output pins */\
					NRF_GPIO->OUTCLR = OUTPUT_LOW_PIN_MASK; /*					0b0000 0000 0000 0000 1110 0000 0011 0000;	* Set all output low pins */\
					NRF_GPIO->OUTSET = OUTPUT_HIGH_PIN_MASK; /*					0b0000 0000 0000 0000 0000 0000 0000 0100;	* Set some output high pins */\
					NRF_GPIO->DIRSET = UNCONNECTED_PINS;						/* Unconnected pins to outputs */\
					NRF_GPIO->OUTCLR = UNCONNECTED_PINS;						/* Unconnected pins driven low */\
					NRF_POWER->DCDCEN = 0;										/* DCDC converter enabled */\
					ACCEL_SHUTDOWN();											/* Accelerometer GPIO initialization*/}


#define GPIO_SENSE_NETS(pLow, pHigh) {	uint32_t input = (GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos) | (GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos);	\
										volatile uint32_t pins = ~NRF_GPIO->DIR, *cfg = NRF_GPIO->PIN_CNF; /* Pull inputs up, sense pins not high e.g. Gnd net */	\
										while(pins){if(pins & 0x1){*cfg = input;} cfg++; pins >>=1; } SysTimeDelayMs(10); *pLow = ~(NRF_GPIO->DIR | NRF_GPIO->IN);	\
										input = (GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos) | (GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos);		\
										pins = ~NRF_GPIO->DIR; cfg = NRF_GPIO->PIN_CNF; /* Pull inputs down and sense high. Detect pins held high e.g. Vdd net*/	\
										while(pins){if(pins & 0x1){*cfg = input;} cfg++; pins >>=1; } SysTimeDelayMs(10); *pHigh = (~NRF_GPIO->DIR) & NRF_GPIO->IN;	\
										input = (GPIO_PIN_CNF_SENSE_High << GPIO_PIN_CNF_SENSE_Pos); pins = ~NRF_GPIO->DIR; cfg = NRF_GPIO->PIN_CNF;				\
										while(pins){if(pins & 0x1){*cfg = input;} cfg++; pins >>=1; } } /* Turn off pull settings before exit */ 

#define USB_BUS_SENSE	Not available
#define BATT_PIN		6
#define BAT_ANALOGUE_CH	ADC_CONFIG_PSEL_AnalogInput7

// Remove default LEDs
#define LEDs_NUMBER 0

#ifdef OLD_OM_BAND_HW
#define LED_1			Not available
#define LED_2			14
#define LED_3			15
#else
#define LED_1			Not available
#define LED_2			13
#define LED_3			15
#endif
#define BSP_LED_0		LED_2
#define BSP_LED_1		LED_3
#define BSP_LED_0_MASK	(1UL<<LED_2)
#define BSP_LED_1_MASK	(1UL<<LED_3)
#define LEDS_MASK		(BSP_LED_0_MASK | BSP_LED_1_MASK) // Can use LEDS_ON(LEDS_MASK)
#define LEDS_INV_MASK	(0)	 

#define LED_ON(_LED_PIN_NUM)	{nrf_gpio_pin_set(_LED_PIN_NUM);}
#define LED_OFF(_LED_PIN_NUM)	{nrf_gpio_pin_clear(_LED_PIN_NUM);}
#define LED_TOGGLE(_LED_PIN_NUM)	{nrf_gpio_pin_toggle(_LED_PIN_NUM);}
#define LED_STATE(_LED_PIN_NUM)		(NRF_GPIO->OUT & (1UL<<_LED_PIN_NUM))

// Vibration motor
#ifdef OLD_OM_BAND_HW
#define MOTOR_PIN		13
#else
#define MOTOR_PIN		12
#endif

#define MOTOR_PIN_MASK	(1ul << MOTOR_PIN)
#define MOTOR_STATE()	(NRF_GPIO->OUT & MOTOR_PIN_MASK)
#define MOTOR(_x)		{if(_x != 0){nrf_gpio_pin_set(MOTOR_PIN);}else{nrf_gpio_pin_clear(MOTOR_PIN);}}

// Hardware control variables and macros
typedef struct hw_ctrl_tag {
	uint8_t Led2;
	uint8_t Led3;
	uint8_t Motor;
} hw_ctrl_t;

extern volatile hw_ctrl_t hw_ctrl;
#define HW_SET_MODE(_on, _pulse, _time)	( ((_on)?0x01:0x00) | ((_pulse)?0x02:0x00) | (_time << 2) )
#define HW_CTRL_FORCE_ON	0xFF	/* Enables LEDs to be set on continuously */

// Not using buttons (no buttons present)
#define BUTTONS_NUMBER 0

// DFU bootloader space increased in size 
// Set to: FLASH (rx) : ORIGIN = 0x3A000, LENGTH = 0x6000 
// Previously: FLASH (rx) : ORIGIN = 0x3C000, LENGTH = 0x3C00

// Soft-device specific: Low frequency clock source to be used by the SoftDevice
#define NRF_CLOCK_LFCLKSRC	  {  .source		= NRF_CLOCK_LF_SRC_XTAL,			\
								 .rc_ctiv		= 0,								 \
								 .rc_temp_ctiv  = 0,								 \
								 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM}

// Accelerometer connection definitions (SPI and GPIO) 
// Pin configuration - (example pin out)
#define ACCEL_CS	2
#define ACCEL_SCK	5
#define ACCEL_SDO	3
#define ACCEL_SDI	4
#define ACCEL_INT1	0
#define ACCEL_INT2	1
#define ACCEL_VDD	30
#define ACCEL_SPI	NRF_SPI0
// Pin initialisation
#define INIT_ACCEL_PINS()	{											/* Setup the GPIO pins for SPI for master mode + interrupts */\
							NRF_GPIO->PIN_CNF[ACCEL_VDD] =	0x00000003; /*Sense-off, Drive S0S1, Pull-off, In-Off, Output */\
							NRF_GPIO->PIN_CNF[ACCEL_CS] =	0x00000003; /*Sense-off, Drive S0S1, Pull-off, In-Off, Output */\
							NRF_GPIO->PIN_CNF[ACCEL_SCK] =	0x00000001; /*Sense-off, Drive S0S1, Pull-off, In-On, Output */\
							NRF_GPIO->PIN_CNF[ACCEL_SDI] =	0x00000003; /*Sense-off, Drive S0S1, Pull-off, In-Off, Output */\
							NRF_GPIO->OUTSET = (1 << ACCEL_VDD)|(1 << ACCEL_CS)|(1 << ACCEL_SCK)|(1 << ACCEL_SDI);/* Drive SPI outputs high */\
							NRF_GPIO->PIN_CNF[ACCEL_SDO] =	0x00000000; /*Sense-off, Drive S0S1, Pull-off, In-On, Input */\
							NRF_GPIO->PIN_CNF[ACCEL_INT1] = 0x00020000; /*Sense-on-high, Drive S0S1, Pull-off, In-On, Input */\
							NRF_GPIO->PIN_CNF[ACCEL_INT2] = 0x00020000;}/*Sense-on-high, Drive S0S1, Pull-off, In-On, Input */
#define ACCEL_SHUTDOWN()	{											/* Setup the GPIO pins  to turn off accelerometer*/\
							NRF_GPIO->PIN_CNF[ACCEL_VDD] =	0x00000003; /*Sense-off, Drive S0S1, Pull-off, In-Off, Output */\
							NRF_GPIO->PIN_CNF[ACCEL_CS] =	0x00000003; /*Sense-off, Drive S0S1, Pull-off, In-Off, Output */\
							NRF_GPIO->PIN_CNF[ACCEL_SCK] =	0x00000001; /*Sense-off, Drive S0S1, Pull-off, In-On, Output */\
							NRF_GPIO->PIN_CNF[ACCEL_SDI] =	0x00000003; /*Sense-off, Drive S0S1, Pull-off, In-Off, Output */\
							NRF_GPIO->OUTCLR = (1 << ACCEL_VDD)|(1 << ACCEL_CS)|(1 << ACCEL_SCK)|(1 << ACCEL_SDI);}/* Drive SPI outputs high */
// SPI configuration
#define ACCEL_OPEN_SPI()	{	/* Configure and enable the SPI module in master mode, select the device */\
							ACCEL_SPI->POWER		= SPI_POWER_POWER_Enabled;\
							ACCEL_SPI->ENABLE		= SPI_ENABLE_ENABLE_Disabled;\
							ACCEL_SPI->INTENCLR		= SPI_INTENCLR_READY_Msk;\
							ACCEL_SPI->PSELSCK		= ACCEL_SCK;\
							ACCEL_SPI->PSELMOSI		= ACCEL_SDI;\
							ACCEL_SPI->PSELMISO		= ACCEL_SDO;\
							ACCEL_SPI->FREQUENCY	= SPI_FREQUENCY_FREQUENCY_M8;\
							ACCEL_SPI->CONFIG		= 6UL;	/* MSB 1st, Active low, Sample 2nd */\
							ACCEL_SPI->EVENTS_READY	= 0UL;	/* Clear ready event if set */\
							ACCEL_SPI->ENABLE		= SPI_ENABLE_ENABLE_Enabled;}
// SPI off/shut down							
#define ACCEL_CLOSE_SPI()	{	/* Disable the SPI module, de-select device, float outputs with pull-ups to save power */\
							ACCEL_SPI->ENABLE		= SPI_ENABLE_ENABLE_Disabled;}
// SPI driver macros
extern uint32_t g_spi_rx_last;
#define ACCEL_SPI_WRITE(_x)	{ACCEL_SPI->TXD = (uint32_t)_x; while(!ACCEL_SPI->EVENTS_READY); g_spi_rx_last = ACCEL_SPI->RXD; ACCEL_SPI->EVENTS_READY = 0UL;}
#define ACCEL_SPI_READ_REG	((uint8_t)g_spi_rx_last)	

#define Nop()	{\
asm volatile("mov r0, r0");}


// Use for debugging where needed
#if 0
#include "nrf_delay.h"

void DebugFlash(uint32_t count)
{
	LED_ON(LED_3);	
	nrf_delay_ms(300);
	LED_OFF(LED_3);
	nrf_delay_ms(200);

	while(count >= 10)
	{
		count -= 10;
		LED_ON(LED_3);	
		nrf_delay_ms(20);
		LED_OFF(LED_3);
		nrf_delay_ms(500);
	}
	while(count > 0)
	{
		count--;
		LED_ON(LED_2);	
		nrf_delay_ms(20);
		LED_OFF(LED_2);
		nrf_delay_ms(500);
	}
}

// Debug hardware pin connectivity detect
while(1)
{
	static volatile uint32_t low, high;
	static volatile uint32_t count, pin, mask;
	// Sense pulled pins high or low external to MCU
	GPIO_SENSE_NETS(&low, &high);
	// Strobe pins to detect connectivity
	for(pin = 0; pin < 32; pin++)
	{
		mask = 1UL << pin;
		// Set to output and strobe pin
		NRF_GPIO->DIRSET = mask;
		for(count = 5; count > 0; count--)
		{
			NRF_GPIO->OUTSET = mask;
			nrf_delay_ms(50);
			NRF_GPIO->OUTCLR = mask;
			nrf_delay_ms(50);
		}
		NRF_GPIO->DIRCLR = mask;
	} // Next pin
	// Prevent variable elimination
	PreventVariableElimination(&low + (high + count + pin + mask));
}
#endif
#endif

