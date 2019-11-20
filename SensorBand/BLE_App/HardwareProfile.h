// Defining "BOARD_CUSTOM" includes "custom_board.h" which includes the local "HardwareProfile.h"
#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

// Hardware specific includes
#include "nrf.h"
#include "nrf_gpio.h"
#include "Config.h"

// Old or new OM band hardware selection
//#define OLD_OM_BAND_HW

// Hardware selection
#ifdef BOARD_DEV_KIT_PCA10028
// Debugging of sensor band firmware using dev kit PCA10028 but using local hardware profile
#warning "Compiling for dev kit, not band hardware - check memory defs"
#include "HardwareProfile-pca10028.h"
#else
// Actual header file for the sensor band hardware
#include "HardwareProfile-SensorBand.h"
#endif

#endif