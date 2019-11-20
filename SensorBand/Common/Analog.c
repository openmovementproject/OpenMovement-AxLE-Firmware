// Analogue to digital conversion for nRF51/52
// Karim Ladha 2014

// Include
#include <stdlib.h>
#include <stdint.h>
#include "nrf.h"
#include "nrf_soc.h"
#include "nrf_sdm.h"
#include "nrf_delay.h"
#include "HardwareProfile.h"
#include "app_config.h"


// Definitions
#define ADC_REFERENCE_VOLTAGE_MV	1200 /* Note: For NRF52 AxLE we use 600mz but div-6 for same performance*/
#define ADC_SCALING_FACTOR			3
#define BATT_TABLE_OS 471																
#define BATT_TABLE_MAX 590	

// Globals
volatile uint16_t battRaw = 0;
volatile uint8_t battPercent = 0;
volatile int16_t tempRaw = 0;
volatile int16_t tempx10C = 0;
volatile int8_t tempCelcius = 0;
const uint8_t battCapacity[]; /* Battery capacity look-up data */																
volatile uint8_t battLow = 0;
volatile uint8_t battMin = BATTERY_LOW_THRESHOLD;

// Get battery raw adc val
uint16_t GetBatt(void)
{
	#ifdef NRF51
	// Configure the ADC
	NRF_ADC->CONFIG	 =	(ADC_CONFIG_RES_10bit								<< ADC_CONFIG_RES_Pos)		|
						(ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling	<< ADC_CONFIG_INPSEL_Pos)	|
						(ADC_CONFIG_REFSEL_VBG								<< ADC_CONFIG_REFSEL_Pos)	|
						(BAT_ANALOGUE_CH									<< ADC_CONFIG_PSEL_Pos)		|
						(ADC_CONFIG_EXTREFSEL_None							<< ADC_CONFIG_EXTREFSEL_Pos);
	// Enable and stop the ADC
	NRF_ADC->ENABLE	= ADC_ENABLE_ENABLE_Enabled;
	NRF_ADC->TASKS_STOP	 = 1;
	// Clear the interrupt and event flags
	NRF_ADC->INTENCLR	= ADC_INTENSET_END_Msk;
	NRF_ADC->EVENTS_END	 = 0;
	// Start the conversion and await completion. Clear done event and read result
	NRF_ADC->TASKS_START	= 1;
	while(!NRF_ADC->EVENTS_END);
	NRF_ADC->EVENTS_END	 = 0;
	battRaw					= NRF_ADC->RESULT;
	// Ensure stopped and disable ADC
	NRF_ADC->TASKS_STOP	 = 1;
	NRF_ADC->ENABLE			= ADC_ENABLE_ENABLE_Disabled;
	
	#elif defined NRF52
	// Interrupts off, setup channel and adc configuration
	NRF_SAADC->INTENCLR = 0xFFFFFFFF;
	NRF_SAADC->CH[0].PSELP = SAADC_CH_PSELP_PSELP_AnalogInput5;
	NRF_SAADC->CH[0].PSELN = SAADC_CH_PSELN_PSELN_NC;
	NRF_SAADC->CH[0].CONFIG = (SAADC_CH_CONFIG_GAIN_Gain1_6 << SAADC_CH_CONFIG_GAIN_Pos); /*One shot, SE mode, Tacq 3us, Reference 0.6v, Gain 1/6, no resitors*/
	NRF_SAADC->CH[0].LIMIT = 0x7FFF8000;
	NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_10bit << SAADC_RESOLUTION_VAL_Pos;
	NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Bypass << SAADC_OVERSAMPLE_OVERSAMPLE_Pos;
	NRF_SAADC->SAMPLERATE = SAADC_SAMPLERATE_MODE_Task << SAADC_SAMPLERATE_MODE_Pos;
	NRF_SAADC->RESULT.MAXCNT = 1;
	NRF_SAADC->RESULT.PTR = (uint32_t)&battRaw;
	// Enable adc
	NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos;
	// Start up adc
	NRF_SAADC->EVENTS_STARTED = 0;
	NRF_SAADC->TASKS_START = 1;
	while(!NRF_SAADC->EVENTS_STARTED);
	// Initiate a single sample
	NRF_SAADC->EVENTS_END = 0;
	NRF_SAADC->TASKS_SAMPLE = 1;
	while(!NRF_SAADC->EVENTS_END);
	// Turn off the adc
	NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos;
	NRF_SAADC->TASKS_STOP = 1;	
	#endif
	
	// Return converted value
	return battRaw;
}

int16_t GetTemp(void)
{
	// Check if SD is enabled
#ifdef SOFTDEVICE_PRESENT
	uint8_t p_softdevice_enabled;
	if(sd_softdevice_is_enabled(&p_softdevice_enabled) == NRF_SUCCESS)
	{
		int32_t sd_die_temp;
		// Use this if SD is enabled: uint32_t	sd_temp_get (int32_t *p_temp)
		if(sd_temp_get(&sd_die_temp) != NRF_SUCCESS)
		{
			// Error: Not actually handled as app error
			sd_die_temp = 0;
		}
		tempRaw = sd_die_temp;
	}
	else
#endif
	{
		#ifdef NRF51
		NRF_TEMP->POWER = TEMP_POWER_POWER_Enabled;
		#endif
		NRF_TEMP->TASKS_STOP = 1;
		NRF_TEMP->INTENCLR = TEMP_INTENCLR_DATARDY_Msk;
		NRF_TEMP->EVENTS_DATARDY = 0;
		NRF_TEMP->TASKS_START = 1;
		while(!NRF_TEMP->EVENTS_DATARDY);
		tempRaw = (int16_t)NRF_TEMP->TEMP;
		NRF_TEMP->TASKS_STOP = 1;
		#ifdef NRF51
		NRF_TEMP->POWER = TEMP_POWER_POWER_Disabled;
		#endif
	}
	return tempRaw;
}

const uint8_t battCapacity[120] = { /* Capacity = Table[(sample - 471)], for sample range 471 up to 590 */																
	2,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,	3,   
	3,	4,	4,	4,	4,	4,	4,	4,	4,	4,	4,	4,	5,	5,	5,	5,   
	5,	5,	5,	5,	6,	6,	6,	6,	6,	6,	7,	7,	7,	7,	8,	8,   
	10,	12,	13,	14,	16,	17,	18,	20,	22,	24,	27,	30,	33,	37,	40,	43,   
	45,	48,	50,	52,	54,	56,	57,	59,	60,	62,	63,	64,	65,	66,	68,	69,   
	70,	71,	72,	73,	74,	75,	76,	77,	78,	79,	80,	81,	82,	83,	84,	85,   
	86,	87,	87,	88,	89,	90,	90,	91,	92,	93,	94,	94,	95,	96,	96,	97,   
	97,	98,	98,	98,	99,	99,	99,	99,
}; 

/* Battery capacity lookup table, 10bit ADC, 50% input divider, 33% scaler, and reference of 1200 mV */															
uint8_t AdcBattToPercent(uint16_t sample)
{
	int8_t calculated, delta;
	// Calculate percentage from table
	if (sample > BATT_TABLE_MAX) calculated = 100;
	else if (sample < BATT_TABLE_OS) calculated = 0;	
	else calculated = battCapacity[(sample - BATT_TABLE_OS)];
	// Reduce oscillations and transients on global measure
	// First sample, accept calculation
	if(battPercent == 0) 
		battPercent = calculated; 
	// Future samples, increment slowly
	else if(calculated > battPercent)
		battPercent++;
	else if(calculated < battPercent)
		battPercent--;
	// Update low battery counter
	if((calculated < battMin) && (battLow < 0xFF))
		battLow++;
	else
		battLow = 0;
	// Return smoothed value
	return battPercent;
}

// Convert reading to units of 0.001 V
uint16_t AdcToMillivolt(uint16_t adcVal)
{
	// Conversion to millivolts (32bit prescision, 10bit in, 11bit ref, <11bit ScaleM)
	uint32_t temp = (uint32_t)adcVal * ADC_REFERENCE_VOLTAGE_MV * ADC_SCALING_FACTOR;
	temp >>= 10;
	return (uint16_t)temp;
}

int16_t TempTo10C(int16_t tempVal)
{
	tempx10C = (tempVal*5)>>1; // x 2.5
	return tempx10C;
}

int8_t TempCelcius(int16_t tempVal)
{
	tempCelcius = tempVal>>2; // x 0.25
	return tempCelcius;
}

// Use: TempFloat(tempRaw);
const char* TempFloat(int16_t tempVal)
{
	// Raw temp module value is in 0.25 steps - 10bit
	static char output[8]; // "-999.99\0"
	div_t result = {.rem = (tempVal >> 2) & 0x00FF, .quot = 0};
	char *ptr = output;

	// Invert value and set sign flag based on sign bit
	if(tempVal & 0x200){ tempVal = (~tempVal) + 1; *ptr++ = '-'; }

	// Integer parts of value
	result.rem = (tempVal >> 2) & 0x00FF;
	result = div(result.rem,100);
	if(result.quot)	*ptr++ = '0' + result.quot;
	result = div(result.rem,10);
	if(result.quot)	*ptr++ = '0' + result.quot;
	*ptr++ = '0' + result.rem;

	// Decimal part of result
	*ptr++ = '.';
	switch(tempVal & 0x0003){
		case 0 : *ptr++ = '0';*ptr++ = '0';break;
		case 1 : *ptr++ = '2';*ptr++ = '5';break;
		case 2 : *ptr++ = '5';*ptr++ = '0';break;
		case 3 : *ptr++ = '7';*ptr++ = '5';break;
	}

	// Terminate string with null
	*ptr = '\0';
	// Return
	return output;



}
