// KL: Compiler.h
#ifndef _COMPILER_H_
#define _COMPILER_H_
/* Compiler.h header is to provide:
	- Inclusion of files required for general code production (if such is required)
	- Inclusion of headers required for the macros used in hardware profile and configuration headers
	- Include headers containing generic processor specific information and usage e.g. CMSIS
*/
#include "nrf.h"
#ifdef NRF51
#include "../nRF5_SDK_11.0.0_89a8197/components/toolchain/CMSIS/Include/core_cm0.h"
#include "../nRF5_SDK_11.0.0_89a8197/components/toolchain/CMSIS/Include/core_cmFunc.h"
#include "../nRF5_SDK_11.0.0_89a8197/components/toolchain/CMSIS/Include/core_cmInstr.h"
#elif defined(NRF52)
#include "../nRF5_SDK_15.0.0_a53641a/components/toolchain/cmsis/include/core_cm4.h"
#include "../nRF5_SDK_11.0.0_89a8197/components/toolchain/CMSIS/Include/core_cmFunc.h"
#include "../nRF5_SDK_11.0.0_89a8197/components/toolchain/CMSIS/Include/core_cmInstr.h"
#endif

#endif
