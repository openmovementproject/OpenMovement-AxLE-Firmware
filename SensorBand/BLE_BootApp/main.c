/* Copyright (c) 2013 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/**@file
 *
 * @defgroup ble_sdk_app_bootloader_main main.c
 * @{
 * @ingroup dfu_bootloader_api
 * @brief Bootloader project main file.
 *
 * -# Receive start data packet. 
 * -# Based on start packet, prepare NVM area to store received data. 
 * -# Receive data packet. 
 * -# Validate data packet.
 * -# Write Data packet to NVM.
 * -# If not finished - Wait for next packet.
 * -# Receive stop data packet.
 * -# Activate Image, boot application.
 *
 */

#include "dfu_transport.h"
#include "bootloader.h"
#include "bootloader_util.h"
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_soc.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "ble.h"
#include "nrf.h"
#include "ble_hci.h"
#include "app_scheduler.h"
#include "app_timer_appsh.h"
#include "nrf_error.h"
#include "bsp.h"
#include "softdevice_handler_appsh.h"
#include "pstorage_platform.h"
#include "nrf_mbr.h"
#include "nrf_log.h"
#include "boards.h"
#include "app_util_platform.h"	/* KL: Added for ISR priorities */
#include "dfu_bank_internal.h"	/* KL: Added for connection visibility */
#include "app_util.h"			/* KL: Needed for definitions */
#include "Common/Analog.h"
#include "Peripherals/SysTime.h"
#include "dfu_types.h"
#include "nrf_delay.h"
//#include "Peripherals/LIS3DH.h" /* KL: No space for accel functions */

// KL: Timeout before bootloader tries to run application
uint32_t m_dfu_timeout = DFU_TIMEOUT_INTERVAL_INACTIVE;
// Advertising interval to use for discovery, was 25ms
uint32_t m_adv_interval = APP_ADV_INTV_NORMAL;
// KL: RTC time epoch - persistent section
volatile EpochTime_t __attribute__((section(".rtc_state"))) rtcEpochTriplicate[3];
// LED settings
volatile int8_t gleds = -1;
// Other variables
extern uint8_t m_boot_settings[];

// UICR registers to be added to hex file - Bootload process protected using device/hardware versions. Image verified by CRC16
// The above is accomplished using a data array
#ifndef REBOOT_IMAGE_BUILD
// CODE_REGION_1_START_CONST	0x0001B000
// BOOTLOADER_REGION_START		0x0003C000
struct UICR_REGISTERS_START_tag
{
	uint32_t start_regs[32];
	dfu_device_info_t;
	uint32_t end_regs[31];
} volatile  __attribute__ ((section(".uicr_start"))) UICR_REGISTERS_START
= {
	{
		CODE_REGION_1_START_CONST,						// 0x10001000	UICR_CLENR0
		0xFFFFFFFF,										// 0x10001004	UICR_RBPCONF
		0xFFFFFFFF,										// 0x10001008	UICR_XTALFREQ
		0xFFFFFFFF,										// 0x1000100C
		0xFFFFFFFF,										// 0x10001010	UICR_FWID
		BOOTLOADER_REGION_START,						// 0x10001014	UICR_BOOTLOADER
		0xFFFFFFFF,0xFFFFFFFF,							// 0x10001018 - 0x10001020
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x10001020 - 0x10001030
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x10001030 - 0x10001040
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x10001040 - 0x10001050
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x10001050 - 0x10001060
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x10001060 - 0x10001070
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF		// 0x10001070 - 0x10001080
	},
	{
		.device_type = 0xB001,							// 0x10001080 - 0x10001082
		.device_rev = 0xB001							// 0x10001082 - 0x10001084
	},
	{
				   0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	//	0x10001084	-	0x10001090
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x10001090 - 0x100010A0
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x100010A0 - 0x100010B0
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x100010B0 - 0x100010C0
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x100010C0 - 0x100010D0
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x100010D0 - 0x100010E0
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,	// 0x100010E0 - 0x100010F0
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF		// 0x100010F0 - 0x10001100
	}
};
#endif

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze 
 *		   how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num	 Line number of the failing ASSERT call.
 * @param[in] file_name	File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
	app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack
 *		   event has been received.
 *
 * @param[in]	p_ble_evt	Bluetooth stack event.
 */
static inline __attribute__((always_inline)) void sys_evt_dispatch(uint32_t event)
{
	pstorage_sys_event_handler(event);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 *
 * @param[in] init_softdevice  true if SoftDevice should be initialized. The SoftDevice must only 
 *							  be initialized if a chip reset has occured. Soft reset from 
 *							  application must not reinitialize the SoftDevice.
 */
static inline __attribute__((always_inline)) void ble_stack_init(bool init_softdevice)
{
	uint32_t		 err_code;
	sd_mbr_command_t com = {SD_MBR_COMMAND_INIT_SD, };
	nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;

	if (init_softdevice)
	{
		err_code = sd_mbr_command(&com);
		APP_ERROR_CHECK(err_code);
	}
	
	err_code = sd_softdevice_vector_table_base_set(BOOTLOADER_REGION_START);
	APP_ERROR_CHECK(err_code);
   
	SOFTDEVICE_HANDLER_APPSH_INIT(&clock_lf_cfg, true);

	// Enable BLE stack.
	ble_enable_params_t ble_enable_params;
	// Only one connection as a central is used when performing dfu.
	err_code = softdevice_enable_get_default_config(1, 1, &ble_enable_params);
	APP_ERROR_CHECK(err_code);

	ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
	err_code = softdevice_enable(&ble_enable_params);
	APP_ERROR_CHECK(err_code);
	
	err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
	APP_ERROR_CHECK(err_code);
}

// While battery is too low, wait until voltage reaches sufficient level to run app safely without brownout
static inline __attribute__((always_inline))void WaitForPrecharge(void)
{
	uint32_t timeout; 
	// LED tasks is running at 8Hz in back ground - set it to always turn off the LEDs
	gleds = -1;
	// Change minimum battery threshold to hold in pre-charge loop
	battMin = BATTERY_LOW_THRESHOLD;
	// Update battery level - sets battLow
	BattPercent();
	// Wait while battery is below threshold
	while( (battLow) && (++timeout < PRECHARGE_TIMEOUT) )
	{
		// Wait for event. Clear Event Register. Wait for event.
		__WFE();__SEV();__WFE();
		// One LED to flash/blip at very low duty and power(just visible)
#ifdef __DEBUG
		LED_ON(LED_3);
#endif
		// Scheduled tasks - Battery sample, LED tasks will turn LEDs off
		app_sched_execute();
	}
	// Return to running DFU process now battery has enough charge
	return;	
}

// LED connection indicator settings - once per second from background tasks 
static inline __attribute__((always_inline))void indicate_connection_state(void)
{
	// Exit early if off i.e. in pre-charge loop
	if(m_adv_interval == APP_ADV_INTV_OFF)
		return;

	// Connected - flash both LEDs. If in DFU mode, alternate flashes
	if(IS_CONNECTED())
	{
		// DFU connection, flash together out of phase
		if(m_dfu_state >= DFU_STATE_RDY)
			gleds = 0x31; 
		// Non-DFU initialized connection, flash together
		else
			gleds = 0x30;
	}
	else
	{
		// Waiting for a connection. If app is present & using the short exit timeout
		if(m_dfu_timeout == DFU_TIMEOUT_INTERVAL_INACTIVE)
		{
			// Flash LED2 for short timeout only
			gleds = 0x11; 
		}
		else
		{
			// No app - Blip LED2
			gleds = -1;
			// Blip one LED while waiting to connect DFU in low power mode
			LED_ON(LED_2);
		}
		// Unconnected + low battery, reset to wait in pre-charge loop
		if(battLow > 100)
		{
			LED_OFF(LED_2); 
			LED_OFF(LED_3);
			NVIC_SystemReset();
		}
	}
	return;
}

// LED flash timer and other tasks handler instance at 8Hz
APP_TIMER_DEF(background_tasks_timer);

// LED setting update function - called at 8Hz
void BackgroundTasksHandler(void* unused)
{
	// LED flash state variable
	static uint8_t flashPhase = 0;
	uint8_t leds;
	flashPhase++;
	// Reset hardware WDT continuously in case enabled by application
	ClrWdt(); 
	// Low frequency tasks each second (pre-scale flash phase count, assume power of 2)
	if((flashPhase & (HARDWARE_TASK_RATE - 1)) == 0)
	{
		// Indicate current connection state on LEDs
		indicate_connection_state();
		// Update battery level value each second
		BattPercent();
	}
	// LED indication setting
	if(gleds == -1)
		leds = 0;
	// Calculate new state
	else
	{
		// See if any LEDs are toggling and set correct toggle phase
		if(flashPhase & 0x01)	leds = (gleds ^ 0x0F) & (gleds >> 4);	// 0 = In phase with flash->b0
		else					leds = (gleds) & (gleds >> 4);			// 1 = Inverted phase to flash->b0
		// Override to on any LEDs not toggling (top bit 0) but set to On (bottom bit 1)
		leds |= ((~gleds) >> 4) & (gleds);
	}
	// Update actual LED setting
	if(leds & 0x01)	{LED_ON(LED_2);}
	else			{LED_OFF(LED_2);}
	if(leds & 0x02)	{LED_ON(LED_3);}
	else			{LED_OFF(LED_3);}
}
// LED flashing start
static inline __attribute__((always_inline)) void BackgroundTasksHandlerInit(void)
{
	uint32_t err_code;
	gleds = -1;
	err_code = app_timer_create(&background_tasks_timer, APP_TIMER_MODE_REPEATED, BackgroundTasksHandler);
	APP_ERROR_CHECK(err_code);
	// Restart timer... Schedules event after timeout
	err_code = app_timer_start(background_tasks_timer, (APP_TIMER_CLOCK_FREQ / HARDWARE_TASK_RATE), NULL);
	APP_ERROR_CHECK(err_code);
}
// LED flashing off
static inline __attribute__((always_inline))void BackgroundTasksHandlerStop(void)
{
	uint32_t err_code;
	err_code = app_timer_stop(background_tasks_timer);
	APP_ERROR_CHECK(err_code);
	LED_OFF(LED_2); 
	LED_OFF(LED_3);
}
// Needed for variable elimination inhibit cod
static inline __attribute__((always_inline)) void PreventVariableElimination(volatile uint32_t* arbitrary)
{
#ifndef REBOOT_IMAGE_BUILD // Remove UICR from output .hex file - for replacing bootloader via dfu
	*arbitrary += UICR_REGISTERS_START.start_regs[0];
	*arbitrary += (uint32_t)m_boot_settings;
#endif
}

/**@brief Function for bootloader main entry.
 */
int main(void)
{
	uint32_t err_code;
	bool dfu_invoked = false;

	// If user has flagged bootloader to start with special retention value flag
	if(NRF_POWER->GPREGRET == BOOTLOADER_DFU_START)
		dfu_invoked = true;
	NRF_POWER->GPREGRET = 0;
	// Set advert interval to indicate not discoverable and clear retention register
	m_adv_interval = APP_ADV_INTV_OFF;

	// KL: Variable elimination inhibit - never called but required for optimizer use
	PreventVariableElimination(&err_code);

	// Nordic specific hardware initialize
	SystemInit();	

	// Initialize hardware
	InitIO();

	// Check LF clock is on and using the crystal
	if((!NRF_CLOCK->LFCLKRUN) || (NRF_CLOCK->LFCLKSRC != CLOCK_LFCLKSRCCOPY_SRC_Xtal))
	{
		if(NRF_CLOCK->LFCLKRUN)							/* If already running */
			NRF_CLOCK->TASKS_LFCLKSTOP = 1;				/* Turn off osc */
		NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRCCOPY_SRC_Xtal;
		NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;				/* Clear event */
		NRF_CLOCK->TASKS_LFCLKSTART = 1;				/* Turn on osc */
		while(!NRF_CLOCK->EVENTS_LFCLKSTARTED);			/* Wait start */
	}

	// Enable RTC, epoch update task and Nordic timer/scheduler module
	SysTimeInit();

	// Start the LED indicator and other tasks handler - runs at 8Hz
	BackgroundTasksHandlerInit();

	// Pre-charge routine. Prevents application continuing if battery is flat
	if(!dfu_invoked)
		WaitForPrecharge();

	// If DFU invoked deliberately use long timeout and normal rate
	if(dfu_invoked)
	{
		m_dfu_timeout = DFU_TIMEOUT_INTERVAL_ACTIVE;	// Longer timeout
		m_adv_interval = APP_ADV_INTV_NORMAL;			// Fast adverts 
	}
	// If no app is available to execute, use very long timeout and low power advert rate
	else if(!bootloader_app_is_valid(DFU_BANK_0_REGION_START))
	{
		 m_dfu_timeout = DFU_TIMEOUT_INTERVAL_NO_APP;	// Infinite timeout
		 m_adv_interval = APP_ADV_INTV_LOW_POWER;		// Very low rate
	}
	// Normal condition. Fast initial advert rate and shorter DFU timeout
	else
	{
		m_dfu_timeout = DFU_TIMEOUT_INTERVAL_INACTIVE;	// Short timeout
		m_adv_interval = APP_ADV_INTV_NORMAL;			// Fast adverts
	}

	// This check ensures that the defined fields in the bootloader corresponds with actual setting in the chip.
	APP_ERROR_CHECK_BOOL(*((uint32_t *)NRF_UICR_BOOT_START_ADDRESS) == BOOTLOADER_REGION_START);
	APP_ERROR_CHECK_BOOL(NRF_FICR->CODEPAGESIZE == CODE_PAGE_SIZE);

	// Load bootloader settings and set interrupt vector mapping
	(void)bootloader_init();

	// Complete any pending SD or BL update copy operations
	if (bootloader_dfu_sd_in_progress())
	{
		// Complete copy of SD or BL 
		err_code = bootloader_dfu_sd_update_continue();
		APP_ERROR_CHECK(err_code);
		// Init the stack unless app invoked dfu and already running
		ble_stack_init(true);//!dfu_invoked); // KL: Fixed boot entry from cmd bug
		// Clear the update SD+BL nvm setting state  
		err_code = bootloader_dfu_sd_update_finalize();
		APP_ERROR_CHECK(err_code);
	}
	else
	{
		// No pending SD or BL update required, init the stack and scheduler
		ble_stack_init(true);//!dfu_invoked); // KL: Fixed boot entry from cmd bug
	}

	// Run bootloader to allow firmware updates
	err_code = bootloader_dfu_start();
	APP_ERROR_CHECK(err_code);

	// Clean up code before exit
	BackgroundTasksHandlerStop();

	// This launches the application if present and bootloader has not received a SD or BL image
	if (bootloader_app_is_valid(DFU_BANK_0_REGION_START) && !bootloader_dfu_sd_in_progress())
	{
		bootloader_app_start(DFU_BANK_0_REGION_START);
	}
	
	// No valid app present or DFU initiated reset
	NVIC_SystemReset();
}


// Fault handler - KL: Will hang here in debug, otherwise will reset
void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
	// On assert, the system can only recover with a reset.
#if defined(DEBUG) || defined(_DEBUG) || defined(__DEBUG)
	// All LEDs on
	LED_ON(BSP_LED_0);
	LED_ON(BSP_LED_1);		

	// KL: This overrides the weak function - will reset if not in debug
	app_error_save_and_stop(id, pc, info);

	// If printing is disrupted, remove the irq calls, or set the loop variable to 0 in the debugger.
	__disable_irq();
	while(1);
	__enable_irq();

#endif
	// NON-DEBUG -> Reset
	NVIC_SystemReset();
}