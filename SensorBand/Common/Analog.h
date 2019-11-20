// Analogue to digital conversion for nRF51
// Karim Ladha 2014

#ifndef ANALOG_H
#define ANALOG_H

// Useful definitions
#define BattPercent() (AdcBattToPercent(GetBatt()))

// Global result
extern volatile uint16_t battRaw;
extern volatile uint8_t battPercent;
extern volatile uint8_t battLow;
extern volatile uint8_t battMin;
extern volatile int16_t tempRaw;
extern volatile int16_t tempx10C;
extern volatile int8_t tempCelcius;

// ADC Sampling
uint16_t GetBatt(void);
int16_t GetTemp(void);

// Conversion functions
uint8_t AdcBattToPercent(uint16_t adcVal);
uint16_t AdcToMillivolt(uint16_t adcVal);
int16_t TempTo10C(int16_t tempVal);
int8_t TempCelcius(int16_t tempVal);
const char* TempFloat(int16_t tempVal);

#endif

