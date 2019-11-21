// Karim Ladha 2014
#ifndef _CONFIG_H_
#define _CONFIG_H_

// Hardware etc.
#include <string.h>
#include "custom_board.h"

// Revision strings
#define BOOTLOADER_REVISION	"1.0"
#define BOOTLOADER_REVISION_VAL	0x10
#define HARDWARE_NAME		"NRF_DEMO"
#define HARDWARE_REVISION	"0.1"
#define FIRMWARE_REVISION	"0.1"

// Debug helpers
#define DEBUG_PIN_TOGGLE()		nrf_gpio_pin_toggle(LED_1)

// Flash constraints
#define ERASE_BLOCK_SIZE		0x400
#define WRITE_PAGE_SIZE			0x4

// Memory regions, to match linker / placement file
#define START_OF_RAM			0x20000000
#define END_OF_RAM				0x20004000
#define START_OF_FLASH			0x00000000
#define END_OF_FLASH			0x00040000
#define APP_ADDRESS				0x00004000
#define SPARE_RAM_ADDRESS		0x20001000
#define FICR_ADDRESS			0x10000000
#define FICR_LENGTH				0x100
#define UICR_ADDRESS			0x10001000
#define UICR_LENGTH				0x100

// Startup clock behaviour - see startup code struct
// (B[3]=HF CLK XTAL, B[2]=LF CLK CAL, B[1]=LF CLK SRC, B[0]=LF CLK RUN)
//#define STARTUP_CLOCK_ARG	0x01100001	/* 16MHz xtal, 4s cal, 32 kHz LFRC, LF run */
//#define STARTUP_CLOCK_ARG	0x01000101	/* 16MHz xtal, no-cal, 32 kHz xtal, LF run */
#define STARTUP_CLOCK_ARG	0x00000101	/* 16MHz HFRC, no-cal, 32 kHz xtal, LF run */

// Bootloader precharge behavior
#define PRECHARGE_THRESHOLD	0
#define PRECHARGE_TIMEOUT	1000

// Bootloader transport
#define BOOTLOADER_WAIT_TIMEOUT	(0xFFFFFFFF)
#define BOOTLOADER_DATA_MTU		(ERASE_BLOCK_SIZE)
#define MAX_BOOTLOADER_CMD_LEN	(BOOTLOADER_DATA_MTU + 9)
#define MAX_BOOTLOADER_RSP_LEN	(BOOTLOADER_DATA_MTU + 6)
#define BOOT_RX_PIN_NUMBER	6
#define BOOT_TX_PIN_NUMBER	4
#define BOOT_BAUD_RATE		UART_BAUDRATE_BAUDRATE_Baud115200
#define TRANSPORT_PRIORITY	1

// System RTC setup
#define SYS_TICK_RATE				32768
#define RTC_INT_PRIORITY			2
#define T1_INT_PRIORITY				RTC_INT_PRIORITY
#define SYSTIME_RTC					NRF_RTC0
#define SYSTIME_RTC_VECNUM			RTC0_IRQn
#define SYSTIME_RTC_VECTOR			RTC0_IRQHandler
#define SYSTIME_NUM_SECOND_CBS		4	/* User set */
#define SYSTIME_NUM_PERIODIC_CBS	2	/* HW limited value */
#define SYSTIME_SAVE_ISR_CONTEXT
#define GetSystemMIPS() 16
#define DELAY_US(_count)  DelayUs(_count)
extern void DelayUs(uint32_t us);
#define SYS_TIME_VARS_EXTERNAL

// User provided system functions
void SystemClock(uint32_t setting);

// Macros
#define Reset()	{NVIC_SystemReset();}

// Debug visibility only
// The ram located image offset, visibility is for debug (ram address of start of vector table)
extern volatile uint32_t __attribute__((section(".image_offset"))) image_offset;

#endif
