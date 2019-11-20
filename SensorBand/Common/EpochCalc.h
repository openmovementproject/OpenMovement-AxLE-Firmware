// Calculate energy expenditure over fixed time period
#ifndef _EPOCH_CALC_H_
#define _EPOCH_CALC_H_
// Include
#include <stdint.h>
#include "Peripherals/LIS3DH.h"
#include "Config.h"

// Types
// Pedometer phase state
typedef enum {
	PED_NO_STATE = 0,
	PED_HIGH_DETECTED,
	PED_LOW_DETECTED,
	PED_STEP_DETECTED
}PedPhase_t;
// Pedometer status variable
typedef struct {
	uint32_t total;
	uint32_t steps;
	uint16_t interval;
	int16_t min;
	int16_t max;
	int16_t Tmin;
	int16_t Tmax;
	int16_t level;
	PedPhase_t phase;
} PedState_t;

// Globals
// Eepoch state variables
extern int32_t x_dc, y_dc, z_dc;
// Variable outputs
extern uint32_t sample_count;	// Samples included in integration
extern uint64_t eepoch_sum;	// Integrated SVM value
// Pedometer variables
extern PedState_t pedState;

void EpochInit(accel_t* current);

uint32_t SquareRootRounded(uint32_t a_nInput);

uint32_t CalcSvm(accel_t* data);

uint32_t EpochAdd(accel_t* data);

void PedInit(int16_t initialiser);

void PedTask(int16_t amplitude);

uint16_t PedResetSteps(void);

#endif
//EOF
