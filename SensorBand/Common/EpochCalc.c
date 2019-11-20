// Calculate energy expenditure over fixed time period
// Include
#include <stdint.h>
#include <string.h>
#include "Peripherals/LIS3DH.h"
#include "EpochCalc.h"
#include "Config.h"

// Types
typedef union {
	uint16_t var16[4];
	uint32_t var32[2]; 
	uint64_t var64;
} m64_t;	

// Globals
int32_t x_dc, y_dc, z_dc;
uint32_t sample_count;
uint64_t eepoch_sum;
PedState_t pedState;									

void EpochInit(accel_t* current)
{
	x_dc = ((uint32_t)current->x) << EE_LPF_SHIFT;
	y_dc = ((uint32_t)current->y) << EE_LPF_SHIFT;
	z_dc = ((uint32_t)current->z) << EE_LPF_SHIFT;
	sample_count = 0;
	eepoch_sum = 0;
}

/** Integer sqrt+round: stackoverflow.com/questions/1100090/looking-for-an-efficient-integer-square-root-algorithm-for-arm-thumb2
 *	   - SquareRootRounded(6) --> 2
 *	   - SquareRootRounded(7) --> 3
 */
uint32_t SquareRootRounded(uint32_t a_nInput)
{
	uint32_t op  = a_nInput;
	uint32_t res = 0;
	uint32_t one = 1uL << 30; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type
	// "one" starts at the highest power of four <= than the argument.
	while(one > op){one >>= 2;}
	while(one != 0)
	{
		if(op >= res + one)
		{
			op = op - (res + one);
			res = res +  2 * one;
		}
		res >>= 1;
		one >>= 2;
	}
	/* Do arithmetic rounding to nearest integer */
	if(op > res){res++;}
	return res;
}

uint32_t CalcSvm(accel_t* data)
{
	// Calc vector magnitude 
	int32_t x_sqr, y_sqr, z_sqr;
	uint32_t temp;
	// Update DC value accumulators for HPF
	x_dc = x_dc - (x_dc >> EE_LPF_SHIFT) + data->x;
	y_dc = y_dc - (y_dc >> EE_LPF_SHIFT) + data->y;
	z_dc = z_dc - (z_dc >> EE_LPF_SHIFT) + data->z;
// KL: Fixed...
	// Apply high pass filter
    x_sqr = data->x - (x_dc >> EE_LPF_SHIFT);
    y_sqr = data->y - (y_dc >> EE_LPF_SHIFT);
    z_sqr = data->z - (z_dc >> EE_LPF_SHIFT);
	// Square vals
	x_sqr *= x_sqr;
	y_sqr *= y_sqr;
	z_sqr *= z_sqr;
/*
	// Apply high pass filter
	data->x -= x_dc >> EE_LPF_SHIFT;
	data->y -= y_dc >> EE_LPF_SHIFT;
	data->z -= z_dc >> EE_LPF_SHIFT;	
	// Square vals
	x_sqr =  (int32_t)data->x * data->x;
	y_sqr =  (int32_t)data->y * data->y;
	z_sqr =  (int32_t)data->z * data->z;
*/
	// Sum vals with overflow check and square root result
	temp = (uint32_t)x_sqr + (uint32_t)y_sqr + (uint32_t)z_sqr;
	// Calculate SVM, always positive
	return (SquareRootRounded(temp));
}

uint32_t EpochAdd(accel_t* data)
{
	uint32_t svm;
	// Get sample svm
	svm = CalcSvm(data);
	// Accumulate and add to sample count
	eepoch_sum += svm;
	sample_count++;
	// Apply pedometer calculation (w'clamp)
	if(svm > 0x00007FFF)
		svm = 0x7FFF;
	PedTask(svm);
	// Return size of epoch
	return sample_count;
}

// Initialize variables
void PedInit(int16_t initialiser)
{
	// Clear pedometer status
	memset(&pedState, 0, sizeof(PedState_t));
	pedState.min = pedState.max = initialiser;
}

// Tasks required per sample
void PedTask(int16_t amplitude)
{
	// Track Max/Min (fast attack and slow decay tracking)
	pedState.max = pedState.max - (1 + pedState.level >> 3);
	pedState.min = pedState.min + (1 + pedState.level >> 3);
	if (pedState.max <= amplitude)	pedState.max = amplitude;	
	if (pedState.min >= amplitude)	pedState.min = amplitude;

	// Calculate peak-to-peak activity level
	pedState.level = pedState.max - pedState.min;

	// Pedometer state machine to track step thresholds
	if(pedState.level > PED_MIN_ACTIVITY_LEVEL)
	{
		// Bottom level threshold calculate and check, 25% of range
		pedState.Tmin = pedState.min + (pedState.level >> 2);		
		// While level is below the low threshold
		if(amplitude < pedState.Tmin)
		{
			// Set low-detected state
			pedState.phase = PED_LOW_DETECTED;
		}
		else if(pedState.phase == PED_LOW_DETECTED)
		{
			// After low detected, calculate and check for upper threshold, 75% of range
			pedState.Tmax = pedState.max - (pedState.level >> 2);	
			if(amplitude > pedState.Tmax)
			{
				// Low/High detected, check step interval
				if(pedState.interval > PED_MIN_STEP_INTERVAL)
				{
					pedState.phase = PED_STEP_DETECTED;
					// Reset state machine variables
					pedState.interval = 0;
					pedState.phase = PED_NO_STATE;
					pedState.total++;
					pedState.steps++;
				}
			}
		}
		// Pedometer state machine to measure and validate step intervals
		if(++pedState.interval > PED_MAX_STEP_INTERVAL)
		{
			pedState.interval = PED_MIN_STEP_INTERVAL;
			// Begin looking for low level state again
			pedState.phase = PED_NO_STATE;
		}
	}
}

uint16_t PedResetSteps(void)
{
	uint16_t steps = pedState.steps;
	pedState.steps = 0;
	return steps;
}

//EOF
