// Karim Ladha 2016
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "stdbool.h"

// Application definitions
//#define CODE_REGION_1_START			0x0001B000
#if defined(DEBUG) || defined(_DEBUG) || defined(__DEBUG) 
#define BOOTLOADER_REGION_START			0x00034000				 
#else
#define BOOTLOADER_REGION_START			0x0003C000				 
#endif
//#define REBOOT_IMAGE_BUILD	// Remove UICR from output - for replacing bootloader via dfu
#define CODE_REGION_1_START_CONST		0x0001B000

// Flash constraints
//#define ERASE_BLOCK_SIZE		0x400
//#define WRITE_PAGE_SIZE			0x4

// RTC counter and external oscillator
#define SYS_TIME_VARS_EXTERNAL		
#define SYSTIME_NUM_SECOND_CBS		0
#define SYSTIME_NUM_PERIODIC_CBS	0
#define SYSTIME_RTC					NRF_RTC1
#define SYS_TIME_EPOCH_ONLY			

// Timer and scheduler
#define APP_TIMER_PRESCALER			0													/**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_OP_QUEUE_SIZE		6 /* 4 KL: +2 for LED+RTC */						/**< Size of timer operation queues. */
#define APP_TIMER_CONFIG_USE_SCHEDULER	true
#define SCHED_MAX_EVENT_DATA_SIZE	MAX(APP_TIMER_SCHED_EVT_SIZE, 0)					/**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE			25 /* 20 KL: Added 5 to allow timer events */		/**< Maximum number of events in the scheduler queue. */
#define HARDWARE_TASK_RATE			8ul

#define RTC_CLOCK_FREQ	32768ul
/**< Value of the RTC1 PRESCALER register. Scheduler tick rate is 32768 Hz */
#define APP_TIMER_PRESCALER			 0		

// Bootloader precharge behaviour
#define BATTERY_LOW_THRESHOLD	1										// 1% / 3.32v, 
#define PRECHARGE_FLASH_RATE	HARDWARE_TASK_RATE						// Flash pre-charge LED at this rate
#define PRECHARGE_TIMEOUT		((PRECHARGE_FLASH_RATE)*60ul*60ul*1ul)	// 1h, Prevent infinite pre-charge
#define PRECHARGE_FLASH_PERIOD	(RTC_CLOCK_FREQ/PRECHARGE_FLASH_RATE)


// Bootloader timeouts
// KL: Timeout before bootloader trys to run application
extern uint32_t m_dfu_timeout;
#define DFU_TIMEOUT_INTERVAL_INACTIVE	APP_TIMER_TICKS(30000UL, APP_TIMER_PRESCALER)	/**< DFU initial timeout interval in units of timer ticks. Short period, wait for update server, app present */	 
#define DFU_TIMEOUT_INTERVAL_ACTIVE		APP_TIMER_TICKS(120000UL, APP_TIMER_PRESCALER)	/**< DFU timeout interval when active in units of timer ticks. Update server was connected, DFU active */	 
#define DFU_TIMEOUT_INTERVAL_NO_APP		APP_TIMER_TICKS(3600000UL, APP_TIMER_PRESCALER)	/**< DFU timeout interval when no app was found i.e. Prevent DFU cycling on/off rapidly  */	 

// Advertising interval to use for discovery. Latched on startup, was 25ms
extern uint32_t m_adv_interval;
#define APP_ADV_INTERVAL				MSEC_TO_UNITS(m_adv_interval, UNIT_0_625_MS)	/**< The advertising interval */
#define APP_ADV_INTV_OFF				(0)												/* Not advertising. Used to indicate pre-charge state */
#define APP_ADV_INTV_NORMAL				(100)											/* Default DFU app interval ~200uA drain 10Hz  */
#define APP_ADV_INTV_LOW_POWER			(1000)											/* For low power mode when no app found ~40uA drain 2Hz, and ~22uA @ 1Hz */

// Advert and connection power
#define DFU_ADVERT_TX_POWER		RADIO_TXPOWER_TXPOWER_Neg20dBm							/* Measured to work over ~1m line of sight */
#define DFU_CONNECTED_TX_POWER	RADIO_TXPOWER_TXPOWER_0dBm								/* Measured to work over ~10m line of sight */

// DFU device settings
#define DEVICE_NAME						"OM-DFU"												/**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME				"OpenLab"												/**< Manufacturer. Will be passed to Device Information Service. */
#define MIN_CONN_INTERVAL				(uint16_t)(MSEC_TO_UNITS(15, UNIT_1_25_MS))				/**< Minimum acceptable connection interval (15 milliseconds). */
#define MAX_CONN_INTERVAL				(uint16_t)(MSEC_TO_UNITS(40, UNIT_1_25_MS))				/**< Maximum acceptable connection interval (40 milliseconds). */
#define SLAVE_LATENCY					0													   /**< Slave latency. */
#define CONN_SUP_TIMEOUT				(4 * 100)											   /**< Connection supervisory timeout (4 seconds). */

// Bootloader settings
#define IS_SRVC_CHANGED_CHARACT_PRESENT 0												/**< Include the service_changed characteristic. For DFU this should normally be the case. */ 
/* KL: Not really required unless the device connection is preserved through the App->DFU->App sequence. We do not intend to do this. */


// Accelerometer settings
#define ACCEL_FIFO_WATERMARK 25

// Watch dog timer (also in nRF5_SDK\components\drivers_nrf\hal\nrf_wdt.h)
#define NRF_WDT_CHANNEL_NUMBER 0x8UL
#define NRF_WDT_RR_VALUE	   0x6E524635UL 
extern void ClrWdt(void);

#endif
