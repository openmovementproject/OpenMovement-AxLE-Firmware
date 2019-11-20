// Karim Ladha 2016
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "stdbool.h"

// Device settings structure
typedef struct Settings_tag {
	uint8_t address[6];		// Device mac address copy from FICR
	char serialNumber[13];	// Device serial number string
	char masterKey[7];		// Default password
	char securityKey[7];	// User set password
	int32_t	 epochOffset;	// Adjustment for epoch from RTC
	uint32_t epochPeriod;	// Epoch window length setting
	uint32_t epochStop;		// Stop time of epoch logger
	uint32_t goalTimeOffset;// Offset to apply to epoch time
	uint32_t goalPeriod;	// Period used to reset goal count
	uint32_t goalStepCount;	// The step count goal
	uint32_t cueingPeriod;	// Periodic vibration cue setting 
	uint32_t cyclesBattery;	// Battery charge/discharge cycles
	uint32_t cyclesReset;	// Power-on or controlled reset counter
	uint32_t cyclesErase;	// Number of completed erase operations
} Settings_t;

// Current device status
typedef struct Status_tag {
	uint32_t epochCloseTime;// Next epoch window close time
	uint32_t epochSpanLenth;// Current epoch window length
	uint32_t epochReadIndex;// Epoch block index next to read
	uint32_t cueingNextTime;// The next scheduled queue time
	uint32_t cueingCount;	// Periodic vibration cue count down 
	int32_t  appCounter;	// Counter used for timed app exit
	int8_t  appState;		// Current application state
	uint8_t goalComplete;	// Goal met flag
	uint8_t authenticated;	// Authenticated state
	uint8_t streamMode;		// Stream setting
	// FW1.6
	uint16_t accelRate;
	uint8_t accelRange;
} Status_t;

typedef enum {
	APP_STATE_EXIT,
	APP_STATE_LOW_BATT,
	APP_STATE_READY,
	APP_STATE_LOGGING
} AppState_t;

// Global instance
extern Settings_t settings;
extern Status_t status;

// Application definitions
#define CODE_REGION_1_START			0x0001B000
#define BOOTLOADER_REGION_START		0x0003C000

// Flash constraints
//#define ERASE_BLOCK_SIZE			0x400
//#define WRITE_PAGE_SIZE			0x4

// RTC counter and external oscillator
#define SYS_TIME_VARS_EXTERNAL		
#define SYSTIME_NUM_SECOND_CBS		0
#define SYSTIME_NUM_PERIODIC_CBS	0
#define SYSTIME_RTC					NRF_RTC1
#define SYS_TIME_EPOCH_ONLY			
#define SysTimeDelayMs(_x)			nrf_delay_ms(_x)
#define RTC_CLOCK_FREQ	32768ul
/**< Value of the RTC1 PRESCALER register. Scheduler tick rate is 32768 Hz */
#define APP_TIMER_PRESCALER			 0		

// Bootloader precharge behaviour
#define PRECHARGE_THRESHOLD		10									// 10% / 3.65v, Percentage threshold to exit bootloader
#define PRECHARGE_FLASH_RATE	HARDWARE_TASK_RATE					// Flash pre-charge LED at this rate
#define PRECHARGE_TIMEOUT		(24ul*60*60*PRECHARGE_FLASH_RATE)	// 24h, Prevent infinite pre-charge
#define PRECHARGE_FLASH_PERIOD	(RTC_CLOCK_FREQ/PRECHARGE_FLASH_RATE)

// Bootloader timeouts
// KL: Timeout before bootloader trys to run application
extern uint32_t m_dfu_timeout;
#define DFU_TIMEOUT_INTERVAL_INACTIVE	APP_TIMER_TICKS(10000UL, APP_TIMER_PRESCALER)	/**< DFU initial timeout interval in units of timer ticks. Short period, wait for update server */	 
#define DFU_TIMEOUT_INTERVAL_ACTIVE		APP_TIMER_TICKS(120000UL, APP_TIMER_PRESCALER)	/**< DFU timeout interval when active in units of timer ticks. Update server connected, DFU active */	 
#define DFU_TIMEOUT_INTERVAL_NO_APP		APP_TIMER_TICKS(3600000UL, APP_TIMER_PRESCALER)	/**< DFU timeout interval when no app was found i.e. Prevent DFU cycling on/off rapidly  */	 
#define BOOTLOADER_DFU_START 0xB1	/* Set NRF_POWER->GPREGRET to this to invoke longer bootloader timeout, from SD use sd_power_gpregret_set(BOOTLOADER_DFU_START) */

// Advertising interval to use for discovery. Latched on startup, range 20-10,000m
#define APP_ADV_FAST_INTERVAL			MSEC_TO_UNITS(100, UNIT_0_625_MS)	
#define APP_ADV_FAST_TIMEOUT_IN_SECONDS	10
#define APP_ADV_SLOW_INTERVAL			MSEC_TO_UNITS(1000, UNIT_0_625_MS)	
#define APP_ADV_SLOW_TIMEOUT_IN_SECONDS	BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED

// Device information
#define DEVICE_NAME					"axLE-Band"	
#define DIS_MANUFACTURER_NAME		"OpenMovement"
#define DIS_MODEL_NUMBER			"ib001"
#define DIS_HARDWARE_REVISION		"1.1"

//KL:	1.3 09-10-2017
//		1.4 DC offset fix 
//		1.5 minor fixes
//		1.6 lockup prevention
//		1.8 goal function disabled by default
//		1.9 Adding rate/range changes
//		1.10 Adding alternate serial command and name extension
#define DIS_FIRMWARE_REVISION		"1.10" 

#define DIS_SOFTWARE_REVISION		"\0"
#define DIS_SERIAL_NUMBER			settings.serialNumber	// Device address as 12 ascii hex chars

// Application
#define NUS_SERVICE_UUID_TYPE		BLE_UUID_TYPE_VENDOR_BEGIN	
#define DEAD_BEEF					0xDEADBEEF	
#define BLE_CON_INT_LOW_POWER		500
#define BLE_CON_INT_HIGH_SPEED		40
#define BLE_CONN_INT_RANGE_PERCENT	25	// +/- 25% used for connection interval range
#define BLE_CONN_INTERVAL_DISCONN	0xFFFF

#define BATTERY_LOW_THRESHOLD		5	// Percentage. Stop logger after battery is depleted @ 5%
#define BATTERY_LOW_THRESHOLD_START	10	// Percentage. Restart logger after battery is recharged @ 10%
#define LOW_BATT_THRESHOLD_COUNT	5	// Battery must be low for 5 seconds before app state change
#define LOW_BATT_FLASH_INTERVAL		10	// Check/flash after/every N seconds if low battery

#define LOG_WAIT_FLASH_INTERVAL		10	// Check/flash after/every N seconds if low battery

// Gesture settings
#define GESTURE_DISABLE
#define GESTURE_PDQ_ENABLE
#define GESTURE_GOAL_ENABLE

// Cue reminder mode settings
#define CUE_INTERVAL_DEFAULT	60ul	/* 1 minute */
#define CUE_INTERVAL_MIN		5ul		/* 5 sec */
#define CUE_INTERVAL_MAX		300ul	/* 5 minute */

// Timer and scheduler settings
#define APP_TIMER_PRESCALER			0
#define APP_TIMER_OP_QUEUE_SIZE		2
#define APP_TIMER_CONFIG_USE_SCHEDULER	true
#define SCHED_MAX_EVENT_DATA_SIZE	MAX(APP_TIMER_SCHED_EVT_SIZE, 0)
#define SCHED_QUEUE_SIZE			20

// Serial in/out service settings
#define BLE_SERIAL_IN_QUEUE_LEN		64
#define BLE_SERIAL_OUT_QUEUE_LEN	1200

// Hardware task rate Hz
#define HARDWARE_TASK_RATE			8

// Watch dog timer (also in nRF5_SDK\components\drivers_nrf\hal\nrf_wdt.h)
#define NRF_WDT_CHANNEL_NUMBER		0x8UL
#define NRF_WDT_RR_VALUE			0x6E524635UL 
extern void ClrWdt(void);

// OM epoch service UUID base
#define OM_BAND_BASE_UUID			{{0x38, 0xBC, 0x6A, 0x39, 0x94, 0x55, 0x49, 0x0C, 0xBF, 0x39, 0xA8, 0x74, 0x00, 0x00, 0x0B, 0x81}} /* UUID base for OM data service */

// Epoch logging settings
#define EPOCH_LENGTH_DEFAULT		(60ul * 1)		// 1 minute epoch interval
#define EPOCH_NVM_SIZE_TOTAL		(64ul * 1024)	// Size of program flash used for data 

// Accelerometer
// Default device settings
#define ACCEL_DEFAULT_RANGE		8
#define ACCEL_DEFAULT_RATE		50
#define ACCEL_FIFO_WATERMARK	25
#define ACCEL_DYNAMIC_6D_ORIENTATION_DETECT

// Epoch and pedometer energy calculation settings
#define EE_LPF_SHIFT			6										/* Average 2^6 or 64 samples for DC subtraction */
#define PED_ONE_G_VALUE			4096lu									/*Required to scale pedometer calculations - 8g value */
#define PED_MIN_ACTIVITY_LEVEL	((int)(0.25F * PED_ONE_G_VALUE))		/*Min amplitude of peak to peak activity 0.25g */
#define PED_MIN_STEP_INTERVAL	((int)(0.3F * ACCEL_DEFAULT_RATE))		/*In samples, 300ms */
#define PED_MAX_STEP_INTERVAL	((int)(2.0F * ACCEL_DEFAULT_RATE))		/*To lower spurious detection, 2s */

// Register settings for hardware detect modules
#define ACCEL_ORIENTATION_THRESHOLD	((uint8_t)((0.9F * 128.0F)/ACCEL_DEFAULT_RANGE))	// Contents of INT1_THS, 0.9g
#define ACCEL_ORIENTATION_DURATION	((uint8_t)(1.0F * ACCEL_DEFAULT_RATE))				// Contents of INT1_DURATION, 1 second
#define ACCEL_CLICK_THRESHOLD		((uint8_t)((2.0F * 128.0F)/ACCEL_DEFAULT_RANGE))	// Contents of CLICK_THS, 2g
#define ACCEL_CLICK_TIME_LIMIT		((uint8_t)(0.1 * ACCEL_DEFAULT_RATE))				// Contents of TIME_LIMIT, 100 ms
#define ACCEL_CLICK_TIME_LATENCY	((uint8_t)(0.1 * ACCEL_DEFAULT_RATE))				// Contents of TIME_LATENCY, 100 ms
#define ACCEL_CLICK_WINDOW			((uint8_t)(0.5 * ACCEL_DEFAULT_RATE))				// Contents of TIME_WINDOW, 500 ms

// Gesture recognition algorithm
extern void GestureDetectTasks(void);

// Initializer for the device status variable
#define ACCEL_LIS3DH_DEFAULT_REGS {\
	.ctrl_reg0	= 0x10,			/* Set nominal value, SDO pull up off									*/\
	.temp_cfg	= 0x00,			/* Aux/Temp off															*/\
	.ctrl_reg1	= 0x47,			/* 50Hz, XYZ on, normal power (ctrl1.lp=0)								*/\
	.ctrl_reg2	= 0x04,			/* HPF normal, Freq: ODR/50, HPF for tap on								*/\
	.ctrl_reg3	= 0x06,			/* Enable INT1: Watermark, overrun										*/\
	.ctrl_reg4	= 0xA0,			/* RW collision prot, LSB first, +/-8g, HiRes off (ctrl4.hr=1)			*/\
	.ctrl_reg5	= 0x4A,			/* FIFO on, both interrupts latch										*/\
	.ctrl_reg6	= 0xC0,			/* INT2: Tap & INT2_enable, Int pins I/O active high					*/\
	.reference	= 0x00,			/* Unused static reference												*/\
	.fifo_ctrl	= 0x80 + ACCEL_FIFO_WATERMARK,	/* FIFO stream mode, threshold: setting (default = 25)	*/\
	.int1_cfg	= 0xFF,			/* 6D dynamic orientation detection on in all axis						*/\
	.int1_ths	= ACCEL_ORIENTATION_THRESHOLD,	/* Orientation threshold, (Threshold*128)/Range			*/\
	.int1_dur	= ACCEL_ORIENTATION_DURATION,	/* Minimum duration (1/ODR)								*/\
	.int2_cfg	= 0xFF,			/* 6D dynamic orientation detection on in all axis						*/\
	.int2_ths	= ACCEL_ORIENTATION_THRESHOLD,	/* Orientation threshold, (Threshold*128)/Range			*/\
	.int2_dur	= ACCEL_ORIENTATION_DURATION,	/* Minimum duration (1/ODR)								*/\
	.click_cfg	= 0x2A,			/* Double tap only only													*/\
	.click_ths	= 0x80 | ACCEL_CLICK_THRESHOLD,	/* Tap threshold (Threshold*128)/Range, Latch enabled	*/\
	.time_limit	= ACCEL_CLICK_TIME_LIMIT,	/* Time for tap to fall below threshold	(1/ODR)				*/\
	.time_latency = ACCEL_CLICK_TIME_LATENCY,/* De-bounce time between taps if double tap used			*/\
	.time_window = ACCEL_CLICK_WINDOW};		/* Window for second tap if double tap used					*/

	//.click_cfg	= 0x15 - Single only, = 0x3F - Single AND double tap on all axis			


#endif
