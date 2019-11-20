/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
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

/** @file
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief	 UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "softdevice_handler.h"
#include "device_manager.h"
#include "pstorage.h"
#include "app_button.h"
#include "ble_nus.h"
#include "ble_bas.h"
#include "ble_dis.h"
#include "app_util_platform.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "app_scheduler.h"
#include "app_timer_appsh.h"
#include "app_timer.h"
#include "nrf_mbr.h"
#include "nrf_delay.h"
#include "Analog.h"
#include "Peripherals/LIS3DH.h"
#include "Peripherals/SysTime.h"
#include "nrf_drv_gpiote.h"
#include "ble_serial.h"
#include "acc_tasks.h"
#include "AsciiHex.h"
#include "HardwareProfile.h"

// Flash variable address checking variable parameter
const uint32_t __attribute__((section(".nvm_flash_data"), aligned(0x400)))start_of_nvm_data_range = 0xFFFFFFFF; // This value has no effect on the NVM data memory
const uint32_t __attribute__((section(".nvm_settings_data"), aligned(0x400)))start_of_nvm_settings_range = 0xFFFFFFFF; // This invalidates the settings causing defaults to load

// Prototypes
void ble_stack_off(void);
void conn_params_error_handler(uint32_t nrf_error);
void conn_param_update_interval(uint16_t newIntervalMillisec);
void on_conn_params_evt(ble_conn_params_evt_t * p_evt);
uint32_t device_manager_evt_handler(dm_handle_t const *p_handle, dm_event_t const *p_event, ret_code_t event_result);
void DebugSerialDump(uint8_t* buffer, uint16_t bufferLen, uint8_t* source, uint16_t length);

// Global constants and settings
// The User Information and Control Registers (UICR)
#ifdef DEBUG
// UICR registers to be added to hex file - app pretends to be the bootloader in debug
volatile uint32_t __attribute__ ((section(".uicr_start"))) UICR_REGISTERS_START[] = {
	CODE_REGION_1_START,		// 0x10001000	UICR_CLENR0
	0xFFFFFFFF,					// 0x10001004	UICR_RBPCONF
	0xFFFFFFFF,					// 0x10001008	UICR_XTALFREQ
	0xFFFFFFFF,					// 0x1000100C
	0xFFFFFFFF,					// 0x10001010	UICR_FWID
	CODE_REGION_1_START,		// 0x10001014	UICR_BOOTLOADER
};
#else
	//	Not required: Bootloader sets this variable
#endif

// Default GAP connection parameters value - fast 40ms +/-25%
const ble_gap_conn_params_t ble_gap_conn_params_default = {
	.min_conn_interval	= MSEC_TO_UNITS((BLE_CON_INT_HIGH_SPEED - (BLE_CON_INT_HIGH_SPEED / 4)), UNIT_1_25_MS),
	.max_conn_interval	= MSEC_TO_UNITS((BLE_CON_INT_HIGH_SPEED + (BLE_CON_INT_HIGH_SPEED / 4)), UNIT_1_25_MS),
	.slave_latency		= 0,
	.conn_sup_timeout	= MSEC_TO_UNITS(4000, UNIT_10_MS)
};

// Connection parameter negotiation settings
const ble_conn_params_init_t ble_conn_params_init_defaults = {
	.p_conn_params					= NULL,				
	.first_conn_params_update_delay = APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER),
	.next_conn_params_update_delay	= APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER),
	.max_conn_params_update_count	= 3,
	.start_on_notify_cccd_handle	= BLE_GATT_HANDLE_INVALID,
	.disconnect_on_fail				= true, /*KL: Disconnect on failed parameter negotiation*/
	.evt_handler					= NULL, /*KL: No handler used, disconnects on negotiation fail*/
	.error_handler					= conn_params_error_handler		
};

// Advertising options
ble_adv_modes_config_t advertising_options = {
	.ble_adv_whitelist_enabled		= false,
	.ble_adv_directed_enabled		= false,
	.ble_adv_directed_slow_enabled	= false,
	.ble_adv_directed_slow_interval	= 0,
	.ble_adv_directed_slow_timeout	= 0,
	.ble_adv_fast_enabled			= true,
	.ble_adv_fast_interval			= APP_ADV_FAST_INTERVAL,
	.ble_adv_fast_timeout			= APP_ADV_FAST_TIMEOUT_IN_SECONDS,
	.ble_adv_slow_enabled			= true,
	.ble_adv_slow_interval			= APP_ADV_SLOW_INTERVAL,
	.ble_adv_slow_timeout			= APP_ADV_SLOW_TIMEOUT_IN_SECONDS
};

// Device manager security default settings
const dm_application_param_t dm_application_param_defaults = {
	.evt_handler					= device_manager_evt_handler,
	.service_type					= DM_PROTOCOL_CNTXT_GATT_SRVR_ID,
	.sec_param.bond					= 1,	/* Allow bonding */
	.sec_param.mitm					= 0,	/* No MITM required */
	.sec_param.lesc					= 0,	/* Secure connections disabled */
	.sec_param.keypress				= 0,	/* No key press events */
	.sec_param.io_caps				= BLE_GAP_IO_CAPS_NONE, /* No i/o */
	.sec_param.oob					= 0, /* No out of bound data */
	.sec_param.min_key_size			= 7,	/* Security key min length: 7 */
	.sec_param.max_key_size			= 16,	/* Security key max length: 16 */
	.sec_param.kdist_own			= 0,	/* Key distribution map: Off */
	.sec_param.kdist_peer			= 0		/* Key distribution map: Off */
};

// Services list for advertising
static ble_uuid_t m_adv_uuids[] = {	{BLE_UUID_NUS_SERVICE,		NUS_SERVICE_UUID_TYPE},  
									{BLE_UUID_BATTERY_SERVICE,	BLE_UUID_TYPE_BLE},
									{BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE} };

// Instance of the battery service
static ble_bas_t m_bas;	
// Instance of the single current connection
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
// The connection interval of the client connection
uint16_t m_conn_interval = BLE_CONN_INTERVAL_DISCONN;
// Instance of the device manager for connection bonding
static dm_application_instance_t m_app_handle;	
// RTC time epoch - persistent section
volatile EpochTime_t __attribute__((section(".rtc_state"))) rtcEpochTriplicate[3];
// Peripherals etc.
volatile hw_ctrl_t hw_ctrl = {0};

// Device setttings, pstorage handle and application state
pstorage_handle_t	settings_pstorage_handle;	
Settings_t			settings = {0};
Status_t			status = {0};

// Prototypes for settings function routines
bool SettingsInitialise(void);
bool SettingsPstorageSave(void);
bool SettingsDefaults(void);
void SettingsPstorageHandler(pstorage_handle_t * p_handle, uint8_t op_code, uint32_t result, uint8_t* p_data, uint32_t data_len);

// FL:1.9
void StopStreamingAndRestartLogger(void);
void StopLoggingStartStream(void);

bool SettingsInitialise(void)
{
	pstorage_module_param_t param;
	pstorage_handle_t block_handle;
	ret_code_t err_code;	

	// Initialize status 
	memset(&status, 0, sizeof(Status_t));
	status.goalComplete		= false;						// Clear goal flag
	status.cueingCount		= 0;							// Zero count down
	status.epochReadIndex	= EPOCH_BLOCK_INDEX_INVALID;	// Epoch block index next to read
	status.epochSpanLenth	= EPOCH_LENGTH_DEFAULT;			// Length of current window
	status.authenticated	= false;						// Authenticated state
	status.streamMode		= 0;							// Stream accelerometer data

	// FW:1.9
	status.accelRange		= 8;
	status.accelRate		= 50;

	status.appState			= APP_STATE_READY;				// Application state - not started
	status.appCounter		= 0;							// Counter used for timing app mode
	status.epochCloseTime	= SYSTIME_VALUE_INVALID;		// Set to indicate logger is not started

	// Initialise the pstorage module
	param.block_size  = PSTORAGE_FLASH_PAGE_SIZE;
	param.block_count = 1;
	param.cb		  = SettingsPstorageHandler;
	// Register with the pstorage module
	err_code = pstorage_register(&param, &settings_pstorage_handle);	
	APP_ERROR_CHECK(err_code);	
	if(err_code != NRF_SUCCESS)
	{
		// Failed to initialise NVM
		app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
		return false;
	}	

	// Set module ID (may not be needed) to read saved settings
	block_handle.module_id = settings_pstorage_handle.module_id;
	// Request to read from indexed block (at offset of zero)
	pstorage_block_identifier_get(&settings_pstorage_handle, 0, &block_handle);	
	// Ensure data was read into destination pointer variable and check if address is valid
	if(pstorage_load((uint8_t*)&settings, &block_handle, sizeof(Settings_t), 0) != NRF_SUCCESS)
		app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
	// Should only occur on newly programmed device
	if(memcmp((void*)NRF_FICR->DEVICEADDR, settings.address, 6) != 0)
	{
		// Restore defaults
		SettingsDefaults();
		status.authenticated = true;
		return false;
	}
	// Set to authenticated if no password set
	if(memcmp(settings.masterKey, settings.securityKey, 6) == 0)
	{
		status.authenticated = true;
	}
	// Settings loaded successfully
	return true;
}

bool SettingsPstorageSave(void)
{
	uint32_t err_code;
	pstorage_handle_t block_handle;
	// Set module ID (may not be needed)
	block_handle.module_id = settings_pstorage_handle.module_id;
	// Request to read from indexed block (at offset of zero)
	pstorage_block_identifier_get(&settings_pstorage_handle, 0, &block_handle);	
	// Save settings
	err_code = pstorage_update(&block_handle, (uint8_t*)&settings, sizeof(Settings_t), 0);	
	if(err_code != NRF_SUCCESS)
	{
		app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
		return false;		
	}	
	return true;
}

bool SettingsDefaults(void)
{
	pstorage_module_param_t param;
	pstorage_handle_t block_handle;
	ret_code_t err_code;	
	// Clear settings
	memset(&settings, 0, sizeof(Settings_t));
	// Mis-match. Write 6 byte mac address to NVM
	memcpy(settings.address, (void*)NRF_FICR->DEVICEADDR, 6);		
	// Reset default setting serial number and passwords
	// Set master key and clear security key
	sprintf(settings.serialNumber,"%02X%02X%02X%02X%02X%02X",
		settings.address[5] | 0xC0, // KL: Fix BT address top bits compliance
		settings.address[4],
		settings.address[3],
		settings.address[2],
		settings.address[1],
		settings.address[0]);
	// Set master key and clear security key to last 6 digits of MAC/Serial
	memcpy(settings.masterKey, &settings.serialNumber[6], 7);
	memcpy(settings.securityKey, &settings.serialNumber[6], 7);
	// Other/non-zero settings to defaults
	settings.epochOffset	= 0;							// Adjustment for epoch from RTC
	settings.epochPeriod	= EPOCH_LENGTH_DEFAULT;			// Epoch window length setting
	settings.epochStop		= 0xFFFFFFFF;					// Set stop time to end
	settings.goalTimeOffset = 0;							// Offset to apply to epoch time
	settings.goalPeriod		= 24ul*60ul*60ul;				// Period used to reset goal count
	settings.goalStepCount	= 0xffffffff;					// The step count goal - disabled
	settings.cueingPeriod	= CUE_INTERVAL_DEFAULT;			// Periodic vibration cue setting 
	// Reset the usage counters
	settings.cyclesBattery	= 0;							// Zero battery charge/discharge cycles
	settings.cyclesReset	= 0;							// Zero power-on or controlled reset counter
	settings.cyclesErase	= 0;							// Reset number of completed erase operations
	// Default to authenticated on reset to defaults
	status.authenticated = true;
	// Write back to NVM as well - callback triggered
	return SettingsPstorageSave();
}

void SettingsPstorageHandler(pstorage_handle_t * p_handle, uint8_t op_code, uint32_t result, uint8_t* p_data, uint32_t data_len)
{
	switch(op_code)
	{
		case PSTORAGE_UPDATE_OP_CODE:
			// After update (NVM write), check result
			if(result != NRF_SUCCESS)
			{
				app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
			}			
			break;
		case PSTORAGE_CLEAR_OP_CODE:
		case PSTORAGE_STORE_OP_CODE:
		case PSTORAGE_LOAD_OP_CODE:			
		default:
			// Unhandled and un-needed events
			break;
	}		
}

// Clear hardware variables
void HardwareOutputsClear(void)
{
	// No connection - indicators off
	hw_ctrl.Led2 = 0;
	hw_ctrl.Led3 = 0;
	hw_ctrl.Motor = 0;
	battMin = BATTERY_LOW_THRESHOLD;
	// Hardware off
	LED_OFF(LED_2); 
	LED_OFF(LED_3);
	MOTOR(0);
}
/**@brief Function for assert macro callback.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyse 
 *		   how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num	 Line number of the failing ASSERT call.
 * @param[in] p_file_name File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
	app_error_handler(DEAD_BEEF, line_num, p_file_name);
}
// Function for the GAP initialization.
static void gap_params_init(void)
{
	uint32_t				err_code;
	ble_gap_conn_params_t   gap_conn_params;
	ble_gap_conn_sec_mode_t sec_mode;

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

	// KL: FW10, adding last 6 characters of cluetooth address to name
	int length = strlen(DEVICE_NAME) + 1 + 6 + 1;
	static char name[32] = {0};
	char *pos = name;

	pos = memcpy(pos, DEVICE_NAME, strlen(DEVICE_NAME));
	pos += strlen(DEVICE_NAME);
	*pos++ = '-';
	memcpy(pos, &settings.serialNumber[6], 6);
	
	if(strlen(name) <= 31) pos = name;
	else pos = DEVICE_NAME "-err";

	err_code = sd_ble_gap_device_name_set(&sec_mode,
										  (const uint8_t *)pos,
										  strlen(pos));
	APP_ERROR_CHECK(err_code);

	//err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_UNKNOWN);
	err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_GENERIC_WATCH);
	APP_ERROR_CHECK(err_code);

	memcpy(&gap_conn_params, &ble_gap_conn_params_default, sizeof(ble_gap_conn_params_t));

	err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
	APP_ERROR_CHECK(err_code);
}
// Change the connection parameters
void conn_param_update_interval(uint16_t newIntervalMillisec)
{
	uint32_t retVal;
	ble_gap_conn_params_t ble_gap_conn_params_new;
	memcpy(&ble_gap_conn_params_new, &ble_gap_conn_params_default, sizeof(ble_gap_conn_params_t));

	ble_gap_conn_params_new.min_conn_interval	= ((((uint32_t)newIntervalMillisec) * (1000ul - (10* BLE_CONN_INT_RANGE_PERCENT ))) / (UNIT_1_25_MS));
	ble_gap_conn_params_new.max_conn_interval	= ((((uint32_t)newIntervalMillisec) * (1000ul + (10* BLE_CONN_INT_RANGE_PERCENT ))) / (UNIT_1_25_MS));
	
	retVal = ble_conn_params_change_conn_params(&ble_gap_conn_params_new);	
	if(retVal != NRF_SUCCESS)
	{
		// No update scheduled - error
		app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
	}
	return;
}
// Data handler for remote->local data flow and reply
void serial_tasks(void)
{
	#define SERIAL_CMD_LEN		(64)					// Maximum receive/transmit serial text packet length (working buffer)
	#define WRITE_SEGMENT_SIZE	(SERIAL_CMD_LEN / 2)	// 512/32 = 16 segments of 64 bytes in read block sequence
	const char* reply = NULL;
	uint16_t length, result;
	uint8_t buffer[SERIAL_CMD_LEN + 1];

	// Stop streaming
	if(status.streamMode != 0)
	{
		// Re-initialise the logger
		StopStreamingAndRestartLogger();
	}

	// Command input handler
	result = ble_serial_service_receive(buffer, SERIAL_CMD_LEN);
	// For non-zero length strings
	if(result == 0)
		return;
	// Handle password/authentication commands
	switch(buffer[0])
	{
		// Read the serial number
		case '#':
		{
			// Print response to user
			sprintf(buffer, "#:%s\r\n", settings.serialNumber);
			reply = buffer;
			length = strlen(reply);
			break; 
		}
		// Unlock device. Enter the device password
		case 'U':
		case 'u':
		{
			// Compare password to master and device passwords
			status.authenticated = false;
			if(memcmp(settings.securityKey, &buffer[1], 6) == 0)
				status.authenticated = true;
			// Print response to user
			if(status.authenticated == true)
				reply = "Authenticated\r\n";
			else
				reply = "!\r\n";
			length = strlen(reply);
			break; 
		}
		// Change the device password 
		case 'P':
		case 'p':
		{
			// User must be authenticated and have 6 extra characters in command
			if((status.authenticated != true) || (result < 7))
			{
				reply = "!\r\n";
				length = strlen(reply);
				break; 
			}				
			// Check and copy 6 chars of input to settings password: "Pxxxxxx"
			for(length = 1; length < 7; length++)
			{
				char ch = buffer[length];			// Chars: "0123456789ABCDEF" only
#if 0
// Option for HEX excoded 6 character (24 bit) password
				if((ch >= '0') && (ch <= '9'))continue;
				if((ch >= 'A') && (ch <= 'F'))continue;
#elif 1
// Option for any valid alphanumeric characters
				if((ch >= '0') && (ch <= '9'))continue;
				if((ch >= 'A') && (ch <= 'Z'))continue;
				if((ch >= 'a') && (ch <= 'z'))continue;
#else
// Option for any valid ascii character
				if((ch >= ' ') && (ch <= '~'))continue;
#endif
				break; // Out of range
			}
			if(length != 7)
			{
				reply = "Error?\r\n";
				length = strlen(reply);
				break; 
			}
			// Copy to settings variable and save
			memcpy(settings.securityKey, &buffer[1], 6);
			if(SettingsPstorageSave())
			{
				// Frame response "P:xxxxxx"
				buffer[0] = 'P';buffer[1] = ':';
				memcpy(&buffer[2], settings.securityKey, 6);
				buffer[8] = '\r';
				buffer[9] = '\n';
				buffer[10] = '\0';
				reply = buffer;
			}
			else
			{
				// Save settings failed
				reply = "Error\r\n";
			}
			length = strlen(reply);
			break; 
		}
		// Epoch block erase command for all blocks - check password (serial number last 6 chars)		
		case 'E': 
		case 'e':
		{
			// If not authenticated 
			if(status.authenticated == false)
			{
				// Allow authentication by master password for erase all
				if(	(result >= 7) && (memcmp(settings.masterKey, &buffer[1], 6) == 0) )
				{
					// On 'master key erase all', allow authentication
					status.authenticated = true;
					// Edit input command to cause erase all
					buffer[1] = '!';
				}
				else
				{
					// Not authenticated, reply '!'
					reply = "!\r\n";
					length = strlen(reply);
					break;
				}
			}	

			// User must be authenticated or have 6 extra master key chars after command matching the master key
			if(status.authenticated == true)
			{
				// If the command is a query
				if(buffer[1] == '?')
				{
					// Settings and data erased
					length = sprintf(buffer,"B:%lu\r\nR:%lu\r\nE:%lu\r\n",
						(unsigned long) settings.cyclesBattery,
						(unsigned long) settings.cyclesReset,
						(unsigned long) settings.cyclesErase);
					// Reply length set, send reply
					reply = buffer;
					break; 
				}

				// Reset and save the security key to equal the master key
				memcpy(settings.securityKey, settings.masterKey, 6);
				// Increment erase count
				settings.cyclesErase++;

				// If the command was for factory reset
				if(buffer[1] == '!')
				{
					// Restore setting defaults and wipe all NVM - Reset on error
					if(!SettingsDefaults())
						app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
					// Settings and data erased
					reply = "Erase all\r\n";
				}
				else
				{
					// Check save result - on application NVM save failure - better to reset
					if(!SettingsPstorageSave())
						app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
					// Data erased, settings preserved
					reply = "Erase data\r\n";
				}

				// If logging state - restart logger
				if(status.appState == APP_STATE_LOGGING)
				{
					if(!AccelEpochLoggerStop())
						app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
				}
				// Set reply chars based on results of erase and settings save
				if(!AccelEpochBlockClearAll()) 
					app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
				// If logging state - restart logger
				if(status.appState == APP_STATE_LOGGING)
				{
					status.appState = APP_STATE_READY;		
					status.appCounter = 5; // Re-start logger in a few seconds				
				}
				// Erased device is now clear, default password set, authenticated
			}			
			// Set reply length
			length = strlen(reply);
			break; 
		}	
		case '0': 
		case 'O': 
		case 'o': 
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Shut off hardware
			reply = "OFF\r\n";
			length = strlen(reply);
			// Clear variables and state
			HardwareOutputsClear();
			break; 
		}
		case '1': 
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Buzz vibration motor
			reply = "MOT\r\n";
			length = strlen(reply);
			hw_ctrl.Motor = HW_SET_MODE(1,0,16);
			break; 
		}
		case '2': 
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Turn on LED for 1 second
			reply = "LED2\r\n";
			length = strlen(reply);
			hw_ctrl.Led2 = HW_SET_MODE(1,0,8);
			break; 
		}
		case '3': 
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Turn on LED for 1 second
			reply = "LED3\r\n";
			length = strlen(reply);
			hw_ctrl.Led3 = HW_SET_MODE(1,0,8);
			break; 
		}
		case 'M': 
		case 'm':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Buzz vibration motor
			reply = "MOT\r\n";
			length = strlen(reply);
			hw_ctrl.Motor = HW_SET_MODE(1,1,8);
			break; 
		}
		case 'B': 
		case 'b':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Sample battery
			sprintf(buffer, "B:%d%%\r\n",(int)battPercent); 
			reply = buffer; 
			length = strlen(reply);
			break; 
		}
		case 'A': 
		case 'a':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Accelerometer sensor sample
			accel_t* ax = (accel_t*)&accel_regs.xl;
			sprintf(buffer, "A:%d,%d,%d,%02X\r\n",ax->x,ax->y,ax->z, accel_regs.int1_src); 
			reply = buffer; 
			length = strlen(reply);
			break; 
		}
		case 'T': 
		case 't':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// If other chars are in the read buffer, change the time value
			if((result > 1) && (buffer[1] != '?' )) 
			{
				uint32_t newTime = 0;
				// Read the input into a start index - check for none zero read (invalid input)
				if(ReadHexToBinary((uint8_t*)&newTime, &buffer[1], (2 * sizeof(uint32_t))) > 0)
				{
					SysTimeSetEpoch(newTime);
					// Fix scheduled tasks like epoch and cueing
					status.cueingCount = 0;
					AccelCalcEpochWindow();
				}
			}
			// Read the current time
			sprintf(buffer, "T:%lu\r\n", (uint32_t)rtcEpochTriplicate[0]); 
			reply = buffer;
			length = strlen(reply);
			break; 
		}
		case 'H': 
		case 'h':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// If other chars are in the read buffer, change the time value
			if((result > 1) && (buffer[1] != '?' )) 
			{
				uint32_t epochStop = 0;
				// Read the input into a temporary variable
				if(ReadHexToBinary((uint8_t*)&epochStop, &buffer[1], (2 * sizeof(uint32_t))) > 0)
				{
					// Set logger stop time
					settings.epochStop = epochStop;
					// Save settings
					SettingsPstorageSave();
				}
			}
			// Read the current time
			sprintf(buffer, "H:%lu\r\n", (long)settings.epochStop); 
			reply = buffer;
			length = strlen(reply);
			break; 
		}
		case 'n':
		case 'N':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Change period of epoch logger
			if((result > 1) && (buffer[1] != '?' )) 
			{
				uint32_t period = 0;
				// Read the input into a start index - check for none zero read (invalid input)
				if(ReadHexToBinary((uint8_t*)&period, &buffer[1], (2 * sizeof(uint32_t))) > 0)
				{
					// Clamp within limits 15 sec - 2 hours
					if((period < 15) || (period > 7200))
						period = EPOCH_LENGTH_DEFAULT;
					// Alter settings
					settings.epochPeriod = period;
					// Reset logger using pause count
					status.appCounter = 5;
					// Save settings
					SettingsPstorageSave();
				}
			}
			// Print the epoch logger period
			length = sprintf(buffer, "N:%lu\r\n", (unsigned long)settings.epochPeriod); 
			reply = buffer;
			break; 
		}

		case 'L': 
		case 'l':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Request client to increase (slower) connection interval
			conn_param_update_interval(BLE_CON_INT_LOW_POWER);
			reply = "LP\r\n";
			length = strlen(reply);
			break; 
		}
		case 'F': 
		case 'f':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Request client to reduce (faster) connection interval
			conn_param_update_interval(BLE_CON_INT_HIGH_SPEED);
			reply = "HP\r\n";
			length = strlen(reply);
			break; 
		}	
		case 'v':
		case 'V':
		{
			uint32_t value;
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// If other chars are in the read buffer. Restart logger - XXXXX
			if((result > 2) && (buffer[1] != '?' )) 
			{
				value = 0;
				// Read the input first before interpreting it
				if(ReadHexToBinary((uint8_t*)&value, &buffer[1], (2 * sizeof(uint32_t))) > 0)
				{
					// Limit connection interval
					if( (value < (BLE_CON_INT_HIGH_SPEED >> 2)) || (value > (BLE_CON_INT_LOW_POWER << 2)) )
					{
						// Force interval value to high
						value = BLE_CON_INT_HIGH_SPEED;
					}
					// Request new connection interval value
					conn_param_update_interval(value);
				}
			}
			// Get the connection interval to print - unless connection has terminated
			value = 0;
			if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
				value = (m_conn_interval * UNIT_1_25_MS) / 1000;
			// Print the connection interval
			length = sprintf(buffer, "V:%lums\r\n",(unsigned long)value );
			reply = buffer;
			break;
		}

		case 'C': 
		case 'c':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Check for query symbol
			if( (result > 1)  && (buffer[1] == '?'))
			{
				// Skip below setup steps - just print out values
			}
			else  // If not a cue query
			{
				// If other chars are in the read buffer, change the cueing period 
				if(result > 1)
				{
					uint16_t interval = 0;
					// Read the input into a start index - check for none zero read (invalid input)
					if(ReadHexToBinary((uint8_t*)&interval, &buffer[1], (2 * sizeof(uint16_t))) > 0)
						settings.cueingPeriod = (uint32_t)interval;
					// Clamp period range and set global setting
					if( (settings.cueingPeriod < CUE_INTERVAL_MIN)	||	(settings.cueingPeriod > CUE_INTERVAL_MAX) )	
					{
						// Invalid or out of range value
						settings.cueingPeriod = CUE_INTERVAL_DEFAULT;
					}
					// Save settings
					SettingsPstorageSave();
					// Setup count to restart below
					status.cueingCount = 0;
				}
				// If cue count is none zero, toggle it off
				if(status.cueingCount == 0)	status.cueingCount = 3600ul / settings.cueingPeriod;
				else						status.cueingCount = 0; // Off	
			}

			// Set interval to next cue output
			if(status.cueingCount > 0)
			{
				// Set cue time
				status.cueingNextTime = rtcEpochTriplicate[0] + settings.cueingPeriod;
			}
			// Set reply text to indicate setting, "Q:xxxx C:xxxx"
			sprintf(buffer, "Q:%lu\r\nC:%lu\r\n",
				(unsigned long)settings.cueingPeriod,
				(unsigned long)status.cueingCount); 
			reply = buffer; 
			length = strlen(reply);
			break; 
		}	
		case 'Y': 
		case 'y':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Empty battery - LEDs on all the time
			if(hw_ctrl.Led2 != HW_CTRL_FORCE_ON)hw_ctrl.Led2 = HW_CTRL_FORCE_ON;
			else								hw_ctrl.Led2 = 0;
			if(hw_ctrl.Led3 != HW_CTRL_FORCE_ON)hw_ctrl.Led3 = HW_CTRL_FORCE_ON;
			else								hw_ctrl.Led3 = 0;
			// Disable battery low threshold
			battMin = 0;
			// Buzz motor every 5 seconds
			settings.cueingPeriod = 5;
			status.cueingCount = (uint32_t)(-1ul);
			// Set cue time
			status.cueingNextTime = rtcEpochTriplicate[0] + settings.cueingPeriod;
			// Set response
			reply = "Drain\r\n";
			length = strlen(reply);
			break; 
		}	
		// Set the command variables e.g. read index
		case 'W': 
		case 'w':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// If other chars are in the read buffer after the 'W' (none null)
			if((result > 1) && (buffer[1] != '?' )) 
			{
				// Set the current block to invalid first 
				status.epochReadIndex = EPOCH_BLOCK_INDEX_INVALID;
				// Read the input into a start index - check for none zero read (invalid input)
				uint16_t block_index_setting = activeIndex;
				if(ReadHexToBinary((uint8_t*)&block_index_setting, &buffer[1], (2 * sizeof(uint16_t))) > 0)
					status.epochReadIndex = block_index_setting;
			}
			// Fall through to query case to reply with changed variables
		}				
		// Query status: time-now, time-last-epoch-block, num-last-epoch-block, num-blocks-total, nvm-block-indexed
		case 'Q': 
		case 'q':
		{
			uint8_t* output = buffer;
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Logger query response
#if 0
// Binary only, ascii hex output
			output += WriteBinaryToHex(output, (uint8_t*)&rtcEpochTriplicate[0], sizeof(uint32_t), false);			/* time-now, 8 chars */
			output += WriteBinaryToHex(output, (uint8_t*)&activeEpochBlock.info, sizeof(EpochBlockInfo_t), false);	/* number-active-epoch-block, samples-active-epoch-block, time-active-epoch-block, 16 chars*/
			output += WriteBinaryToHex(output, (uint8_t*)&epockBlockCount, sizeof(uint16_t), false);				/* number of epoch blocks total, 4 chars */
			output += WriteBinaryToHex(output, (uint8_t*)&status.epochReadIndex, sizeof(uint16_t), false);			/* currently-indexed-block, 4 chars */
			*output++ = '\r';*output++ = '\n'; // 34 Chars total
			// Set reply variable
			reply = buffer;
			length = output - buffer;
			// Check buffer did not overflow
			if(length > SERIAL_CMD_LEN)
			{
				// Adding data to queue failed. Not fully sent - error
				app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
			}

#else
// Logger query info string - 3x13=39 chars max x 2
			length = sprintf(buffer, "T:%lu\r\nB:%lu\r\nN:%lu\r\n",
				(unsigned long)rtcEpochTriplicate[0],
				(unsigned long)activeEpochBlock.info.block_number,
				(unsigned long)activeEpochBlock.info.data_length);
			QueuePush(&serial_out_queue, buffer, length);

			length = sprintf(buffer, "E:%lu\r\nC:%lu\r\nI:%lu",
				(unsigned long)activeEpochBlock.info.time_stamp,	
				(unsigned long)epockBlockCount,
				(unsigned long)status.epochReadIndex);
			QueuePush(&serial_out_queue, buffer, length);

			// Terminate packet to send the queue contents
			reply = "\r\n";
			length = strlen(reply);
#endif
			break; 
		}	

		// Synchronise the eepoch logger timing with a +/- offset
		case 'S': 
		case 's':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// If other chars are in the read buffer after the 'W' (none null)
			if((result > 1) && (buffer[1] != '?' )) 
			{
				// Read the input into a start index - check for none zero read (invalid input)
				int32_t offset = 0;
				if(ReadHexToBinary((uint8_t*)&offset, &buffer[1], (2 * sizeof(int32_t))) > 0)
				{
					if(offset != 0)
					{
						// Get the signed offset value to apply
						int8_t signed_offset = offset & 0xFF;
						// Apply timing correction
						settings.epochOffset = offset;
						// Save new sync offset settings 
						SettingsPstorageSave();
						// Recalculate next window close time
						AccelCalcEpochWindow();
					}
				}
			}
			// Set reply text to indicate sync value
			sprintf(buffer, "S:%ld\r\n",(long)settings.epochOffset); 
			reply = buffer; 
			length = strlen(reply);
			break;
		}	
		// Epoch block read command for indexed block	
		case 'R': 
		case 'r':
		{
			uint8_t *queueBuffer;
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Check the queue has enough room to accommodate the full block
			if(QueueFree(&serial_out_queue) >= (2 + 2*EPOCH_NVM_BLOCK_SIZE))
			{
				uint16_t index = 0;
				// Fix block number if invalid or wrapped (set to start of NVM)
				if(status.epochReadIndex >= EPOCH_NVM_BLOCK_COUNT) 
				{
					status.epochReadIndex = activeIndex;
					if(status.epochReadIndex != 0) 
						status.epochReadIndex--;
				}
				// Write out the encoded block in short sections to the queue
				while(index < EPOCH_NVM_BLOCK_SIZE)
				{
					// Early exit on read data fail
					if( !AccelEpochBlockRead((buffer + WRITE_SEGMENT_SIZE), index, WRITE_SEGMENT_SIZE, status.epochReadIndex) )
						break;
					// Encode into ascii hex in place. Early out if encoded is not expected length
					if( (WriteBinaryToHex(buffer, (buffer + WRITE_SEGMENT_SIZE), WRITE_SEGMENT_SIZE, false)) !=  (2 * WRITE_SEGMENT_SIZE) )
						break;
					// Add the segment to the queue. Early exit if push fails to write all output
					if( (QueuePush(&serial_out_queue, buffer, (2 * WRITE_SEGMENT_SIZE))) != (2 * WRITE_SEGMENT_SIZE) )
						break;
					// Increment the write offset
					index += WRITE_SEGMENT_SIZE;
				}
				// Check whole block was added
				if(index < EPOCH_NVM_BLOCK_SIZE)
				{
					// Adding data to queue failed. Not fully sent - error in debug, user read too fast
#ifdef __DEBUG
					app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
#endif
				}
				// Increment the block index pointer on successful read
				if(++status.epochReadIndex >= epockBlockCount)
					status.epochReadIndex = 0;

				// Terminate packet to send the queue contents
				reply = "\r\n";
				length = strlen(reply);
			}
			break; 
		}	
		// Stream IMU data command
		case 'I':
		case 'i':
		// Stream debug data command
		case 'D':
		case 'd':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Set output mode - stream
			if((buffer[0] == 'I') || (buffer[0] == 'i'))
			{
				status.streamMode = 1;
				// Now check for extra rate range settings
				if((result > 1) && (buffer[1] == ' ' ))
				{
					uint32_t i;
					char* ptr = &buffer[2];
					// Terminate for possible full input packet
					buffer[result] = '\0';
					status.accelRate = atoi(ptr);
					while(*ptr >= '0' && *ptr <= '9')ptr++;
					if(*ptr == ' ') status.accelRange = atoi(ptr);
					// Check values are valid
					// Done in accel driver...
					// Stop logging, start streaming new rate/range
					status.streamMode = 3;
				}
				StopLoggingStartStream();
			}
			// Set output mode - debug
			else if((buffer[0] == 'D') || (buffer[0] == 'd'))
				status.streamMode = 2;
			else
				status.streamMode = 0;
			// Print mode and exit
			sprintf(buffer, "OP:%02X, %d, %d\r\n",status.streamMode, status.accelRate, status.accelRange); 
			reply = buffer; 
			length = strlen(reply);
			break; 
		}

		// Query all states
		case '?':
		{
			// Read current states for logger/status/settings etc.
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// Dump states
			DebugSerialDump(buffer, SERIAL_CMD_LEN, (uint8_t*)&settings, sizeof(Settings_t));
			DebugSerialDump(buffer, SERIAL_CMD_LEN, (uint8_t*)&status, sizeof(Status_t));
			break;
		}

		// Goal step count functionality
		case 'G':
		case 'g':
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// If other chars are in the read buffer, change the time value
			if((result > 2) && (buffer[1] != '?' )) 
			{
				uint32_t value = 0;
				// Read the input first before interpreting it
				if(ReadHexToBinary((uint8_t*)&value, &buffer[2], (2 * sizeof(uint32_t))) > 0)
				{
					// Value read from input
					switch(buffer[1]) {
						case 'O' : 
						case 'o' : {	// Offset
							settings.goalTimeOffset = value;
							break;
						}
						case 'P' : 
						case 'p' : {	// Goal count reset period
							settings.goalPeriod = value;
							break;
						}
						case 'G' : 
						case 'g' : {	// Step goal
							settings.goalStepCount = value;
							break;
						}
						default : break;
					}
					// Update the goal function timing
					if(pedState.total >= settings.goalStepCount) 
						status.goalComplete = true;
					else
						status.goalComplete = false;
				}
			}
			// Read the goal settings - <= 39 chars
			sprintf(buffer, "O:%lu\r\nP:%lu\r\nG:%ld\r\n",
				(unsigned long)settings.goalTimeOffset,
				(unsigned long)settings.goalPeriod,
				(unsigned long)settings.goalStepCount); 
			reply = buffer;
			length = strlen(reply);
			break; 
		}

		// Reset device to run DFU app (soft and hard resets are possible)
		case 'X': 
		case 'x': 
		{
			// If not authenticated, do not handle. Reply "!"
			if(status.authenticated != true)
			{
				reply = "!\r\n";
				length = strlen(reply);
				break;
			}
			// If command is "x!", reset immediately
			if(buffer[1] == '!')
			{
				// Set non-volatile reg to indicate bootloader invocation
                ble_stack_off();
				// Indicate on LEDs
                LED_ON(LED_2);LED_ON(LED_3);
				nrf_delay_ms(1000);
                sd_softdevice_disable(); 
				// Will not be able to print this to user
				reply = "Reset\r\n";
				// Hard reset, immediate
				NVIC_SystemReset();
			}
			// Reset to boot loader
			else if((buffer[1] == 'b') || (buffer[1] == 'B'))
			{
				// Set retention regs for boot entry
				// Soft device is faulty - these commands cause a lockup
                //sd_power_gpregret_clr(POWER_GPREGRET_GPREGRET_Msk); 
				//sd_power_gpregret_set(BOOTLOADER_DFU_START); 
				// Soft reset... Exit app in 5 seconds
				status.appState	= APP_STATE_EXIT;	
				status.appCounter = 5;
				// Print response
				reply = "DFU\r\n";
			}
			// If other chars are in the read buffer. Restart logger - XXXXX
			else if(result > 2) 
			{
				uint32_t value = 0;
				// Read the input first before interpreting it
				if(ReadHexToBinary((uint8_t*)&value, &buffer[1], (2 * sizeof(uint32_t))) > 0)
				{
					// The logger will pause, go to ready state, while counter expires
					status.appCounter = value;
					// Unknown arguments to 'x' exit command
					sprintf(buffer, "X:%lu\r\n",(unsigned long)value);
					reply = buffer;
				}
				else
				{
					// Invalid pause length/argument
					reply = "X?\r\n";	
				}
			}
			else
			{
				// Unknown arguments to 'x' exit command
				reply = "?\r\n";				
			}
			length = strlen(reply);
			break; 
		}
		default : {
			// Unknown command
			reply = "?\r\n";
			length = strlen(reply);
			break;
		}
	}

	// Send reply output
	if(reply != NULL)
	{
		result = ble_serial_service_send(reply, length);
		if(result != length)
		{
			// Not sent - error (ignore if in release - don't reset)
#ifdef __DEBUG
			app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
#endif
		}
	}
}

void DebugSerialDump(uint8_t* buffer, uint16_t bufferLen, uint8_t* source, uint16_t length)
{
	// Check the queue has enough room for the data dump
	if(QueueFree(&serial_out_queue) >= (2 + (2 * length)))
	{
		uint32_t offset = 0;
		// Write out the encoded data in short sections to the queue
		while(offset < length)
		{
			uint32_t written, toWrite;
			// Get propper length to write each pass
			toWrite = length - offset;
			if(toWrite > (bufferLen >> 1))
				toWrite = (bufferLen >> 1);
			// Encode into ascii hex in place. Early out if encoded is not expected length
			written = WriteBinaryToHex(buffer, source + offset, toWrite, false);
			// Add the segment to the queue. Early exit if push fails to write all output
			if(QueuePush(&serial_out_queue, buffer, written) != written) break;
			// Increment offset
			offset += toWrite;

		}
		// Terminate packet to send the queue contents
        QueuePush(&serial_out_queue, "\r\n", 2);
	}
}


static void battery_level_update(void)
{
	uint32_t err_code;
	err_code = ble_bas_battery_level_update(&m_bas, battPercent);
	// Check error code
	if ((err_code != NRF_SUCCESS) &&
		(err_code != NRF_ERROR_INVALID_STATE) &&
		(err_code != BLE_ERROR_NO_TX_PACKETS) &&
		(err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
		)
	{
		// Not successful, error handler
		APP_ERROR_HANDLER(err_code);
	}
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
	uint32_t	   err_code;
	ble_nus_init_t nus_init;
	ble_bas_init_t bas_init;
	ble_dis_init_t dis_init;

	// Initialize Device Information Service.
	memset(&dis_init, 0, sizeof(dis_init));
	ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, DIS_MANUFACTURER_NAME);	/**< Manufacturer Name String. */
	ble_srv_ascii_to_utf8(&dis_init.serial_num_str, DIS_MODEL_NUMBER);			/**< Model Number String. */
	ble_srv_ascii_to_utf8(&dis_init.serial_num_str, DIS_SERIAL_NUMBER);			/**< Serial Number String. */
	ble_srv_ascii_to_utf8(&dis_init.hw_rev_str, DIS_HARDWARE_REVISION);			/**< Hardware Revision String. */
	ble_srv_ascii_to_utf8(&dis_init.fw_rev_str, DIS_FIRMWARE_REVISION);			/**< Firmware Revision String. */
	ble_srv_ascii_to_utf8(&dis_init.sw_rev_str, DIS_SOFTWARE_REVISION);			/**< Software Revision String. */
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);
	err_code = ble_dis_init(&dis_init);
	APP_ERROR_CHECK(err_code);

	// Battery service initialization
	memset(&bas_init, 0, sizeof(bas_init));	
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.cccd_write_perm);
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.read_perm);
	BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init.battery_level_char_attr_md.write_perm);	
	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_report_read_perm);	
	bas_init.evt_handler		  = NULL;
	bas_init.support_notification = true;
	bas_init.p_report_ref		 = NULL;
	bas_init.initial_batt_level   = 100;	
	err_code = ble_bas_init(&m_bas, &bas_init);
	APP_ERROR_CHECK(err_code);

	// BLE serial service startup 
	ble_serial_service_init();
}

/* Handler for connection errors to get error code */
void conn_params_error_handler(uint32_t nrf_error)
{
	APP_ERROR_HANDLER(nrf_error);
}

/* Function for initializing the Connection Parameters module. */
static void conn_params_init(void)
{
	uint32_t			   err_code;
	ble_conn_params_init_t cp_init;

	memcpy(&cp_init, &ble_conn_params_init_defaults, sizeof(cp_init));
	err_code = ble_conn_params_init(&cp_init);
	APP_ERROR_CHECK(err_code);
}

/* Function for handling advertising events. */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
	uint32_t err_code;

	switch (ble_adv_evt)
	{
		case BLE_ADV_EVT_FAST:
			// KL: No visual outputs/display
			//err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
			//APP_ERROR_CHECK(err_code);
			break;
		case BLE_ADV_EVT_IDLE: // Advertising shouldn't stop unless connected - failure
			// KL: Reset the device...
			NVIC_SystemReset();
			break;
		default:
			break;
	}
}

/* Function for the application's SoftDevice event handler */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
	uint32_t err_code;

	switch (p_ble_evt->header.evt_id)
	{
		case BLE_GAP_EVT_CONNECTED:
			// Reset the authentication/streaming state
#ifdef __DEBUG
			status.authenticated = true;
#else
			// Set to authenticated if no password set
			if(memcmp(settings.masterKey, settings.securityKey, 6) == 0)
			{
				status.authenticated = true;
			}
			else
			{
				status.authenticated = false;
			}
#endif
			// Stop streaming
			if(status.streamMode != 0)
			{
				// Re-initialise the logger
				StopStreamingAndRestartLogger();
			}

			// Update connection handle for new connection
			m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
			// Connection interval
			m_conn_interval = p_ble_evt->evt.gap_evt.params.connected.conn_params.max_conn_interval;
			break;
			
		case BLE_GAP_EVT_DISCONNECTED:
			// Reset the authentication/streaming state
			// Stop streaming
			if(status.streamMode != 0)
			{
				// Re-initialise the logger
				StopStreamingAndRestartLogger();
			}
			
			// Reset gap parameters for next connection
			gap_params_init();
			conn_params_init();
			// Update connection handle for disconnection
			m_conn_handle = BLE_CONN_HANDLE_INVALID;
			// Change in connection parameters
			m_conn_interval = BLE_CONN_INTERVAL_DISCONN;
			break;

		case BLE_GAP_EVT_CONN_PARAM_UPDATE :
			// Change in connection parameters
			m_conn_interval = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.max_conn_interval;
			break;

		case BLE_GATTS_EVT_SYS_ATTR_MISSING:
			// No system attribute cccds are stored.
			err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
			APP_ERROR_CHECK(err_code);
			break;

		default:
			// No implementation needed.
			break;
	}
}

/* Function for dispatching a SoftDevice event to all modules with a SoftDevice event handler. */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
	dm_ble_evt_handler(p_ble_evt);

	ble_conn_params_on_ble_evt(p_ble_evt);
	
	// BLE serial. Calls "ble_nus_on_ble_evt(&m_nus, p_ble_evt);" externally
	ble_serial_event_handler(&m_nus, p_ble_evt);	

	ble_bas_on_ble_evt(&m_bas, p_ble_evt);
		
	on_ble_evt(p_ble_evt);
	
	ble_advertising_on_ble_evt(p_ble_evt);
}

/* Function for dispatching a system event to interested modules. */
static void sys_evt_dispatch(uint32_t sys_evt)
{
	pstorage_sys_event_handler(sys_evt);
	ble_advertising_on_sys_evt(sys_evt);
}

/* Function for the SoftDevice initialization. */
static void ble_stack_init(void)
{
	uint32_t err_code;
	
	nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
	
	// Initialize SoftDevice.
	SOFTDEVICE_HANDLER_INIT(&clock_lf_cfg, NULL);// SOC/SYS/BLE events not handled with scheduler
	
	ble_enable_params_t ble_enable_params;
	err_code = softdevice_enable_get_default_config(0, /* Centrals links count */
													1, /* Peripherals links count */
													&ble_enable_params);
	APP_ERROR_CHECK(err_code);
		
	//Check the ram settings against the used number of links
	CHECK_RAM_START_ADDR(0,1); // Centrals:0, Peripherals:1
	
	// Enable BLE stack.
	err_code = softdevice_enable(&ble_enable_params);
	APP_ERROR_CHECK(err_code);
	
	// Subscribe for BLE events.
	err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
	APP_ERROR_CHECK(err_code);
	
	// Register with the SoftDevice handler module for BLE events.
	err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
	APP_ERROR_CHECK(err_code);	
}

void ble_stack_off(void)
{
	uint32_t err_code;
	// If connected, disconnect
	if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
	{
		err_code = sd_ble_gap_disconnect(m_conn_handle,  BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		m_conn_handle = BLE_CONN_HANDLE_INVALID;
		APP_ERROR_CHECK(err_code);	
	}
	// Note, only advertising if not connected (exclusive)
	else
	{
		// Stop advertising
		err_code = sd_ble_gap_adv_stop();
		APP_ERROR_CHECK(err_code);	
	}
}

/* Function for handling the Device Manager events. */
uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
										   dm_event_t	const	 *	p_event,
										   ret_code_t		   event_result)
{
	APP_ERROR_CHECK(event_result);
	return NRF_SUCCESS;
}

/* Function for the Device Manager initialization. */
static void device_manager_init(bool erase_bonds)
{
	uint32_t			   err_code;
	dm_init_param_t		init_param = {.clear_persistent_data = erase_bonds};
	dm_application_param_t register_param = {0};

//	// Initialize persistent storage module.
//	err_code = pstorage_init();
//	APP_ERROR_CHECK(err_code);

	err_code = dm_init(&init_param);
	APP_ERROR_CHECK(err_code);

	memcpy(&register_param, &dm_application_param_defaults, sizeof(dm_application_param_t));

	err_code = dm_register(&m_app_handle, &register_param);
	APP_ERROR_CHECK(err_code);
}

static void dm_context_load(dm_handle_t const * p_handle)
{
	uint32_t				 err_code;
	static uint32_t		  context_data;
	dm_application_context_t context;

	context.len	= sizeof(context_data);
	context.p_data = (uint8_t *)&context_data;

	err_code = dm_application_context_get(p_handle, &context);
	if (err_code == NRF_SUCCESS)
	{
		err_code = dm_application_context_delete(p_handle);
		APP_ERROR_CHECK(err_code);
	}
	else if (err_code == DM_NO_APP_CONTEXT)
	{
		// No context available. Ignore.
	}
	else
	{
		APP_ERROR_HANDLER(err_code);
	}
}

/* Function for initializing the Advertising functionality. */
static void advertising_init(void)
{
	uint32_t	  err_code;
	ble_advdata_t advdata;
	ble_advdata_t scanrsp;

	// Build advertising data structures to pass into ble_advertising_init.
	memset(&advdata, 0, sizeof(advdata));
	advdata.name_type		  = BLE_ADVDATA_FULL_NAME;
	advdata.include_appearance = false;
	advdata.flags			  = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE; //KL: Required if advertising indefinitely
	//advdata.flags			  = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

	// Scan response contains service uuids
	memset(&scanrsp, 0, sizeof(scanrsp));
	scanrsp.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
	scanrsp.uuids_complete.p_uuids  = m_adv_uuids;

	// Set the advertising parameters
	err_code = ble_advertising_init(&advdata, &scanrsp, &advertising_options, on_adv_evt, NULL);
	APP_ERROR_CHECK(err_code);
}


/** 
	Hardware control using periodic timer (at ~8Hz)
 */
// Control timer instance
APP_TIMER_DEF(hardware_tasks_timer);
// Hardware setting update function
void HardwareTasks(void* unused)
{
	static uint8_t phaseCount = 0;
	uint8_t state, counts;
	// Update flash phase
	phaseCount++;
	// Indicate connection state, If connected - flash both LEDs
	//if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
	// If connection interval is in 'fast' mode
//	if(m_conn_interval <= ( MSEC_TO_UNITS((BLE_CON_INT_HIGH_SPEED + (BLE_CON_INT_HIGH_SPEED / 4)),UNIT_1_25_MS) ) )
//	{
//		// Connected at high power - brief flash (very dim)
//		LED_ON(LED_2);
//	}

	// LED2 - setting and count
	if(hw_ctrl.Led2 < 0x04)
	{
		// Count over, set off
		hw_ctrl.Led2 = 0;
		LED_OFF(LED_2); 
	}
	else
	{
		if(hw_ctrl.Led2 == HW_CTRL_FORCE_ON)
		{
			// On continuous
			state = 1;
		}
		else
		{
			// Decrement count timer
			hw_ctrl.Led2 -= 4; 
			state = ((hw_ctrl.Led2 >> 1) & phaseCount) ^ hw_ctrl.Led2;
		}
		// Update setting
		if(state & 0x01)	{LED_ON(LED_2);}
		else				{LED_OFF(LED_2);}
	}
	// LED3 - setting and count
	if(hw_ctrl.Led3 < 0x04)
	{
		// Count over, set off
		hw_ctrl.Led3 = 0;
		LED_OFF(LED_3); 
	}
	else
	{
		if(hw_ctrl.Led3 == HW_CTRL_FORCE_ON)
		{
			// On continuous
			state = 1;
		}
		else
		{
			// Decrement count timer
			hw_ctrl.Led3 -= 4; 
			state = ((hw_ctrl.Led3 >> 1) & phaseCount) ^ hw_ctrl.Led3;
		}
		// Update setting
		if(state & 0x01)	{LED_ON(LED_3);}
		else				{LED_OFF(LED_3);}
	}
	// Motor - setting and count
	if(hw_ctrl.Motor < 0x04)
	{
		// Count over, set off
		hw_ctrl.Motor = 0;
		MOTOR(0);  
	}
	else
	{
		// Decrement count timer
		hw_ctrl.Motor -= 4; 
		// Update setting
		state = ((hw_ctrl.Motor >> 1) & phaseCount) ^ hw_ctrl.Motor;
		if(state & 0x01)	{MOTOR(1);}
		else				{MOTOR(0);}
	}

	// Once per second (assume HARDWARE_TASK_RATE == power of 2)
	if((phaseCount & (HARDWARE_TASK_RATE - 1)) == 0)
	{
		// Motor buzz cuing output functionality. Check if cue is due
		if( (status.cueingCount > 0) && (rtcEpochTriplicate[0] >= status.cueingNextTime) )
		{
			// Decrement count
			status.cueingCount--;
			// Update time of next cue
			status.cueingNextTime = rtcEpochTriplicate[0] + settings.cueingPeriod;
			// Vibrate motor for cue output
			hw_ctrl.Motor = HW_SET_MODE(1,1,8);
		}

		// Step goal check and reset timing
		if((status.goalComplete == false) && (pedState.total > settings.goalStepCount))
		{
			// Prevent further triggering
			status.goalComplete = true;
			// Buzz motor and flash green LED
			hw_ctrl.Motor = HW_SET_MODE(1,1,8);
			hw_ctrl.Led2 = HW_SET_MODE(1,1,8);
		}
		// End of period - restart count and flag
		if(((rtcEpochTriplicate[0] + settings.goalTimeOffset) % settings.goalPeriod) == 0ul)
		{
			// Reset the total
			pedState.total = 0;
			// Reset the goal value positive
			status.goalComplete = false;
		}

		// Alternate battery/temperature update
		if(phaseCount & (HARDWARE_TASK_RATE))
		{
			// Read temperature to global raw value and celcius calc
			GetTemp();
			TempCelcius(tempRaw);
		}
		else 
		{
			// Read battery level and calculate percent
			GetBatt();
			AdcBattToPercent(battRaw);
			
			// Update level in battery service
			battery_level_update();
		}

		// Application state counter decrement
		if(status.appCounter > 0)
		{
			// Used to time other app tasks
			status.appCounter--;
		}
	}// Each second
}

// Hardware control start
static void HardwareTasksInit(void)
{
	uint32_t err_code;
	// Clear variables and state
	HardwareOutputsClear();
	// Get initial battery level
	BattPercent();
	// Fast timer to schedule hardware control task
	err_code = app_timer_create(&hardware_tasks_timer, APP_TIMER_MODE_REPEATED, HardwareTasks);
	APP_ERROR_CHECK(err_code);
	// Restart timer... Schedules event after timeout
	err_code = app_timer_start(hardware_tasks_timer,(APP_TIMER_CLOCK_FREQ / HARDWARE_TASK_RATE), NULL);
	APP_ERROR_CHECK(err_code);
}
// Hardware control off
void HardwareTasksStop(void)
{
	uint32_t err_code;
	err_code = app_timer_stop(hardware_tasks_timer);
	APP_ERROR_CHECK(err_code);
	// Clear variables and state
	HardwareOutputsClear();
}

#ifndef GESTURE_DISABLE
void GestureDetectTasks(void)
{
	// State machines positions
	typedef enum {
		GESTURE_STATE_START,
		GESTURE_STATE_UP_WAIT,
		GESTURE_STATE_DOWN_WAIT,
		GESTURE_STATE_UP_SELECT,
		GESTURE_STATE_DOWN_SELECT,
		GESTURE_STATE_CUE_WAIT,
		GESTURE_STATE_GOAL_WAIT
	} GestureState_t;

	// Gesture definitions
	#define GESTURE_POINT_UP		ACCEL_POS_X_UP
	#define GESTURE_POINT_DOWN		ACCEL_POS_X_DOWN
	#define GESTURE_SELECT_CUE		ACCEL_POS_Z_UP
	#define GESTURE_SELECT_GOAL		ACCEL_POS_Y_UP

	// Gesture detect state
	static uint8_t gestState = GESTURE_STATE_START;
	static uint8_t gestCount = 0;
	uint8_t orientation = accel_regs.int1_src & 0x3F;

	// State machine for gesture recognition
	switch (gestState) {
		// Start of state machine detection
		case GESTURE_STATE_START :  {	  
			if(orientation == GESTURE_POINT_UP)
				gestState = GESTURE_STATE_UP_WAIT;
			else if(orientation == GESTURE_POINT_DOWN)
				gestState = GESTURE_STATE_DOWN_WAIT;
			gestCount = 0;
			break;
		}
		// Gesture was 'up', check for down
		case GESTURE_STATE_UP_WAIT : {	
			if(orientation == GESTURE_POINT_DOWN)
				gestState = GESTURE_STATE_DOWN_SELECT;
			gestCount = 0;
			break;
		}
		// Gesture was 'down', check for up
		case GESTURE_STATE_DOWN_WAIT : {	
			if(orientation == GESTURE_POINT_UP)
				gestState = GESTURE_STATE_UP_SELECT;
			gestCount = 0;
			break;
		}
		// Wait for gesture selection from up/down point
		case GESTURE_STATE_UP_SELECT : {
			if(orientation == GESTURE_SELECT_CUE)
				gestState = GESTURE_STATE_CUE_WAIT;
			else if(orientation == GESTURE_SELECT_GOAL)
				gestState = GESTURE_STATE_GOAL_WAIT;
			else if(orientation == ACCEL_POS_UNSTABLE)
				break; // Ignore
			else if(orientation != GESTURE_POINT_UP)
				gestState = GESTURE_STATE_START;
			break;
		}
		case GESTURE_STATE_DOWN_SELECT : {
			if(orientation == GESTURE_SELECT_CUE)
				gestState = GESTURE_STATE_CUE_WAIT;
			else if(orientation == GESTURE_SELECT_GOAL)
				gestState = GESTURE_STATE_GOAL_WAIT;
			else if(orientation == ACCEL_POS_UNSTABLE)
				break; // Ignore
			else if(orientation != GESTURE_POINT_DOWN)
				gestState = GESTURE_STATE_START;
			break;
		}
		// Cueing: Wait for hold time in select, then gesture is selected
		case GESTURE_STATE_CUE_WAIT : {
#ifdef GESTURE_PDQ_ENABLE	
			if((orientation != GESTURE_SELECT_CUE)&&(orientation != ACCEL_POS_UNSTABLE))
				gestState = GESTURE_STATE_START;
			else if(++gestCount > 1)
			{
				// Clear state
				gestState = GESTURE_STATE_START;
				// Fix period if not valid
				if(status.cueingCount != 0)
				{
					// Clear count
					status.cueingCount = 0;
					// Short blue LED flash
					hw_ctrl.Led3 = HW_SET_MODE(1,0,2);
				}
				else
				{
					// Toggle cuing on - set one hour of cueing
					status.cueingCount = 3600ul / settings.cueingPeriod;
					// Long blue LED flashing
					hw_ctrl.Led3 = HW_SET_MODE(1,1,8);
				}			
			}
			break;
#else
			// Reset gesture detection method
			gestState = GESTURE_STATE_START;
			break;
#endif
		}
		// Goal check: Wait for hold time in select, then gesture is selected
		case GESTURE_STATE_GOAL_WAIT : {
#ifdef GESTURE_GOAL_ENABLE	
			if((orientation != GESTURE_SELECT_GOAL)&&(orientation != ACCEL_POS_UNSTABLE))
				gestState = GESTURE_STATE_START;
			else if(++gestCount > 1)
			{
				// Clear state
				gestState = GESTURE_STATE_START;
				// Blue flash for no goal, else green flash
				if(status.goalComplete == true)
					hw_ctrl.Led2 = HW_SET_MODE(1,1,8); // Long green LED flashing
				else	
					hw_ctrl.Led2 = HW_SET_MODE(1,0,2); // Short green LED flash
			}
			break;
#else
			// Reset gesture detection method
			gestState = GESTURE_STATE_START;
			break;
#endif
		}
		// All unknown states reset orientation detection state
		default : {		
			gestState = GESTURE_STATE_START;
			gestCount = 0;
			break;
		}

	} // State machine switch
	
	// Debug output
	#if 0 //def __DEBUG
	{
		char buffer[16];
		int length = sprintf(buffer,"G:%02X,%02X\r\n",orientation,gestState);
		ble_serial_service_send(buffer, length);
	}
	#endif
	return;
}
#endif

void PreventVariableElimination(const void* arbitrary)
{
	static uint8_t unused;
	if(*(uint8_t*)arbitrary != 0)
	{
		unused++;
	}
}

void InitSystem(void)
{
	uint32_t err_code;
	
	// Nordic specific hardware initialize
	SystemInit();
	// Initialize the device hardware 
	InitIO();

	// Ensure the LF clock is on and using the crystal
	if((!NRF_CLOCK->LFCLKRUN) || (NRF_CLOCK->LFCLKSRC != CLOCK_LFCLKSRCCOPY_SRC_Xtal))
	{
		if(NRF_CLOCK->LFCLKRUN)							/* If already running */
			NRF_CLOCK->TASKS_LFCLKSTOP = 1;				/* Turn off osc */
		NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRCCOPY_SRC_Xtal;
		NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;				/* Clear event */
		NRF_CLOCK->TASKS_LFCLKSTART = 1;				/* Turn on osc */
		while(!NRF_CLOCK->EVENTS_LFCLKSTARTED);			/* Wait start */
	}

	// Required if debugging to emulate entry by bootloader
	#ifdef DEBUG
	{
		sd_mbr_command_t com = {SD_MBR_COMMAND_INIT_SD, };

		err_code = sd_mbr_command(&com);
		APP_ERROR_CHECK(err_code);
	
		err_code = sd_softdevice_vector_table_base_set(CODE_REGION_1_START);
		APP_ERROR_CHECK(err_code);
	}
	#endif

	// Power up accelerometer and wait for stability before shutting off - lowest power
	INIT_ACCEL_PINS();
	nrf_delay_ms(5);
	if(AccelPresent())
		AccelShutdown();

	// Start the RTC timer
	SysTimeInit();
	// Initialise hardware control task
	HardwareTasksInit();

	// Check NVM pstorage setting - checks correct alignment of linker script region
	if(	( ((uint32_t)PSTORAGE_DATA_START_ADDR) != ((uint32_t)&start_of_nvm_data_range) ) ||
		( ((uint32_t)PSTORAGE_SETTINGS_START_ADDR) != ((uint32_t)&start_of_nvm_settings_range) ) )
	{
		// Not aligned as expected with memory
		app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
	}

	// KL: Variable elimination inhibit - required for optimizer use
	PreventVariableElimination(&err_code);
	PreventVariableElimination(&start_of_nvm_settings_range);
	#ifdef DEBUG
	PreventVariableElimination((void*)UICR_REGISTERS_START);
	#endif
}

/**@brief Application main function.
 */
int main(void)
{
	uint32_t err_code, count;
	bool erase_bonds;

	// Init all basic low level system components, hardware tasks
	InitSystem();

	// BLE stack initialize - required for pstorage to function
	ble_stack_init();

	// Pstorage initialization and the application sections using pstorage
	err_code = pstorage_init();
	APP_ERROR_CHECK(err_code);

	// Start epoch logger application - uses pstorage for data
	if(!AccelEpochLoggerInit()) 
		app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);

	// Initialize settings - uses pstorage second last page
	if(SettingsInitialise())
	{
		// Settings loaded OK on startup, increment counter
		settings.cyclesReset++;
		// Save settings
		SettingsPstorageSave();
	}

	// Setup remaining BLE systems
	gap_params_init();
	services_init();			// Starts up serial service internally
	advertising_init();
	conn_params_init();

	// Setup device manager - uses end pstorage space
	device_manager_init(false); 

	// Operate application loop (either low-batt or run app)
	while(status.appState != APP_STATE_EXIT)
	{
		// Evaluate battery to determine required state
		if((battPercent > battMin) && (battLow == 0)) 
		{
			// Change state to logging on low-batt exit
			status.appState = APP_STATE_READY;		
			status.appCounter = 5; // Start in 5 seconds
		}
		else
		{
			// Change to low battery app state
			status.appState = APP_STATE_LOW_BATT;
			status.appCounter = 0;
		}

		// If app shouldn't run due to low battery
		if(status.appState == APP_STATE_LOW_BATT)
		{
			// Battery low mode entry, increment counter
			settings.cyclesBattery++;
			// Save settings
			SettingsPstorageSave();

			// Update battery low threshold - loop till sufficient battery
			battMin = BATTERY_LOW_THRESHOLD_START;
			// Set application counter value
			status.appCounter = LOW_BATT_FLASH_INTERVAL;

			// Enter main loop. App runs in ISR context
			while(1)
			{
				// Low power wait for event
				err_code = sd_app_evt_wait();
				APP_ERROR_CHECK(err_code);
				// Scheduled tasks
				app_sched_execute();
				// Timed app tasks
				if(status.appCounter == 0)
				{
					// Low battery state - If battery is still low, flash blue LED and reset counter
					hw_ctrl.Led3 = HW_SET_MODE(1,0,1);
					// Restart app counter - low battery flash
					status.appCounter = LOW_BATT_FLASH_INTERVAL;
					// Exit loop if battery recovers
					if((battPercent > battMin) && (battLow == 0))
					{
						// Battery has recovered to start threshold
						break;
					}
				}
			}
			// Re-evaluate state
			continue;			
		}// Low battery app loop...

		// If app should run and sufficient battery - start advertising
		if((status.appState == APP_STATE_LOGGING) || (status.appState == APP_STATE_READY))
		{
			// Update battery low threshold - Log until battery low
			battMin = BATTERY_LOW_THRESHOLD;
			// Set application counter value
			status.appCounter = 5;

			// Ensure pins are setup
			INIT_ACCEL_PINS();
			nrf_delay_ms(50);
			// Wait for any pstorage accesses to complete - or advertising fails
			do { err_code = pstorage_access_status_get(&count);
			} while( (err_code == NRF_SUCCESS) && (count > 0) );
			// Begin advertising
			err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
			APP_ERROR_CHECK(err_code);

			// Enter main loop. App runs in ISR context except commands
			while(1)
			{
				// Low power wait for event
				err_code = sd_app_evt_wait();
				APP_ERROR_CHECK(err_code);
				// Scheduled tasks
				app_sched_execute();
				// If characters are received
				if(serial_in_queue_flag)
					serial_tasks();
				// Logging state - counter is used to pause logger
				if(status.appState == APP_STATE_LOGGING) 
				{
					// Handle end of epoch periods
                    AccelEpochWriteTasks();
					// If logging paused or a stop limit is set
					if(	(status.appCounter > 0) || 
						( (settings.epochStop != 0) && (settings.epochStop < rtcEpochTriplicate[0]) ))	
					{
						// Exit routines. Stop BLE stack and logger
						AccelEpochLoggerStop();
						// Device is now waiting for counter to expire
						status.appState = APP_STATE_READY;
					}
				}
				else if(status.appCounter != 0)
				{
					// No action until count finishes for other two modes - wait and stop
					
				}
				// Timer used to time logging pauses
				else if(status.appState == APP_STATE_READY)
				{
// KL: Firmware 1.9 bug, for early custom stream command before state transition
if(status.streamMode == 3)
{
	continue; // Stop checking for logging start
}
// KL: Testing in FW1.6
					// Flash LED when in wait mode
					if(status.appCounter % LOG_WAIT_FLASH_INTERVAL == 0)
					{
						// Flash green LED and reset counter
						hw_ctrl.Led2 = HW_SET_MODE(1,0,1);					
					}
					// If not in stop mode, start logging
					if((settings.epochStop == 0) || (settings.epochStop > rtcEpochTriplicate[0]))
					{
						// Start epoch logger application - uses pstorage for data
						if(!AccelEpochLoggerStart())
							app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
						// Device is now logging data
						status.appState = APP_STATE_LOGGING;
					}
				}
				// Exit app after count stops
				else if(status.appState == APP_STATE_EXIT)
				{
					// Exit application invoked
					break;
				}

				// Exit into low batt mode if battery depleted from any state
				if((battPercent < battMin) && (battLow > LOW_BATT_THRESHOLD_COUNT)) 
				{
					// Battery has become depleted
					break;
				}
			}

			// Exit routines. Stop BLE stack and logger if running
			if(status.epochCloseTime != SYSTIME_VALUE_INVALID)
				AccelEpochLoggerStop();
			// Even lower power...
			ACCEL_SHUTDOWN();	
			nrf_delay_ms(50);
			// Wait for any pstorage accesses to complete - or advertising call fails
			do { err_code = pstorage_access_status_get(&count);
			} while( (err_code == NRF_SUCCESS) && (count > 0) );
			// Stop the BLE stack (advertising and connections)
			ble_stack_off();
			// Re-evaluate state
			continue;		
		}// Logging app loop...

	}// While state is not 'exit'

	// Shut down hardware control tasks before resetting
	HardwareTasksStop();
	// Device allowed to reset to bootloader
	sd_softdevice_disable(); 
	return 0;
}

void StopLoggingStartStream(void)
{
	if((status.streamMode == 1) && (status.appState == APP_STATE_READY))
	{
		// Edge case - do nothing but will not actually stream
	}
	else if(status.streamMode == 3) 
	{
		// Stop accelerometer and ISR
		AccelEpochLoggerStop();
		// Edit setting registers and restart accelerometer
		AccelEpochLoggerStart();
		// Prevent current epoch ever closing
		status.epochCloseTime = SYSTIME_VALUE_INVALID;
		// Data interrupt should fire on fifo full
		if(status.appState == APP_STATE_READY)
		{
			// Change state - prevents close time being updated
			status.appState = APP_STATE_LOGGING;
            status.appCounter = 0;
		}
	}
}

void StopStreamingAndRestartLogger(void)
{
	// Stop streaming - re-init accel to 50Hz/8g mode if required
	if(status.streamMode == 3)
	{
		// Reset default 'pedometer' settings
       	status.accelRange		= 8;
		status.accelRate		= 50;
		// Stop accelerometer and ISR
        AccelEpochLoggerStop();
		// Edit setting registers and restart accelerometer
        AccelEpochLoggerStart();
		// Data interrupt should fire on fifo full
	}
	
	status.streamMode = 0;
	// Streaming should stop
	return;
}


// Replace default ISR handler in startup.s with this to avoid infinite loop
void Default_ISRHandler(void)
{
	app_error_fault_handler(0xDEADBEEF + __LINE__, 0, (uint32_t)NULL);
}

/* Callback function for failed assertions and fails */
// Handle all application errors here. Override default error handler
#ifdef DEBUG
static uint32_t visible_id, visible_pc, visible_info;
error_info_t* visible_error_info;
#endif
void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
	// On assert, the system can only recover with a reset.
#ifndef DEBUG
	NVIC_SystemReset();
#else
	visible_error_info = (error_info_t*)info;
	visible_id = id; 
	visible_pc = pc; 
	visible_info = info;
	// Flash indefinitely in debug
	while(1)
	{
		LEDS_ON(LEDS_MASK);
		nrf_delay_ms(50);
		LEDS_OFF(LEDS_MASK);
		nrf_delay_ms(50);
	}
#endif // DEBUG
}


