// Karim Ladha 2015
// NRF51822 optional system initialisation, non user, invoked in crt0.s
// Sets internal special function registers to known/default values
// Includes optional rtc setup for applications requiring simple
// rtc timing functions; Enable in config.

#include <stdint.h>
#include "Compiler.h"
#include "Config.h"

/* System globals */
volatile uint32_t system_tick_seconds = 0;
volatile uint32_t image_offset __attribute__((used, section(".image_offset")));

/* System tick functions */
/*	
	Use the following defines in config.h to enable.
	#define SYS_RTC				NRF_RTC1
	#define SYS_RTC_VECTOR		RTC1_IRQHandler
	#define SYS_RTC_VECNUM		RTC1_IRQn
	#define SYS_RTC_PRIORITY	2
	Prototypes
	extern volatile uint32_t system_tick_seconds;
	extern uint32_t SystemTick(void);
	extern void SystemTickInit(void);
*/
#ifdef SYS_RTC
#ifdef SYS_RTC_VECTOR
void __attribute__ ((interrupt ("IRQ"))) SYS_RTC_VECTOR(void)
{
	NVIC_ClearPendingIRQ(SYS_RTC_VECNUM); /* Clear flag */
	while(SYS_RTC->EVENTS_COMPARE[SYS_RTC_INDEX])
	{
		uint32_t new_compare_val = SYS_RTC->CC[SYS_RTC_INDEX] + SYS_RTC_PERIOD;
		SYS_RTC->CC[SYS_RTC_INDEX] = new_compare_val;
		/* If the isr is delayed, this stops the tick failing */
		if(new_compare_val > SYS_RTC->COUNTER)
			SYS_RTC->EVENTS_COMPARE[SYS_RTC_INDEX] = 0;
		system_tick_seconds++;
	}
}
#endif
/* Simple get tick function */
uint32_t SystemTick(void)
{
	uint32_t top_bits, rtc_counter;
	__disable_irq();
	/* Get 24bit rtc count value and seconds */
	do{
		SYS_RTC->EVENTS_OVRFLW = 0;
		rtc_counter = SYS_RTC->COUNTER;		/* 24 bit val */
		top_bits = (system_tick_seconds>>9);/* Top 8 bits */
	}while(SYS_RTC->EVENTS_OVRFLW);
	__enable_irq();
	return ((top_bits<<24) + rtc_counter);
}
/* Configures the system tick timer */
void SystemTickInit(void)
{
	// System real time clock init
	SYS_RTC->POWER = 1;					/* Power on */
	SYS_RTC->TASKS_STOP = 1;			/* Stop rtc */
	SYS_RTC->TASKS_CLEAR = 1;			/* Clear it, loses fractional count */
	SYS_RTC->PRESCALER = 0;				/* Set prescaler div by 1 */
	SYS_RTC->EVTENCLR = 0x000F0003;		/* Stop all events */
	SYS_RTC->INTENCLR = 0x000F0003;		/* Stop all rtc interrupts */
	SYS_RTC->CC[SYS_RTC_INDEX] = SYS_RTC_PERIOD;/* Set first second CC value*/
	SYS_RTC->EVENTS_COMPARE[SYS_RTC_INDEX] = 0;	/* Clear seconds event */
	/* Enable one second interrupt using compare */
	SYS_RTC->INTENSET = (1ul<<(RTC_EVTENSET_COMPARE0_Pos + SYS_RTC_INDEX));
	NVIC_SetPriority(SYS_RTC_VECNUM, SYS_RTC_PRIORITY);/* Priority */
	NVIC_ClearPendingIRQ(SYS_RTC_VECNUM);	/* Clear flag */
	NVIC_EnableIRQ(SYS_RTC_VECNUM);		/* Enable int */
	SYS_RTC->TASKS_START = 1;			/* Start timer */
}
#endif

/* LPRC calibration ISR if using LPRC source*/
void __attribute__ ((weak, interrupt ("IRQ"))) POWER_CLOCK_IRQHandler(void)
{
	NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);
	/* Power off warning event! Pre-empt brownout? */
	if(NRF_POWER->EVENTS_POFWARN)NRF_POWER->EVENTS_POFWARN = 0;
	/* Check events for calibrating LFRC */
	if(NRF_CLOCK->EVENTS_DONE)			/* Calibration complete */
	{
		NRF_CLOCK->EVENTS_DONE = 0;		/* Clear event cal done	*/
		NRF_CLOCK->TASKS_CTSTART = 1;	/* Start calibration timer */
	} 
	else if (NRF_CLOCK->EVENTS_CTTO)	/* Calibration timed out */
	{
		NRF_CLOCK->EVENTS_CTTO = 0;		/* Clear event cal timed out */
		NRF_CLOCK->TASKS_CAL = 1;		/* Start calibration event */
	}
}

/* Attempts to call to executable image section */
/* Returns if valid vector table not found */
/* Not a very stringent check is used... */
void SystemExecImage(uint32_t address)
{
	uint32_t endOfStackVal, resetEntry, i;
	void(*vector)(void);
	// Check address
	if((address & 0x3) != 0) 
		return;		/* Address not word aligned */	
	if(	(address < START_OF_RAM || address > END_OF_RAM) &&
		(address < START_OF_FLASH || address > END_OF_FLASH) )
		return;		/* Invalid address */
	// Retrieve start of expected vector table 
	endOfStackVal = *(uint32_t*)address;
	resetEntry = *(uint32_t*)(address + 4);
	// Do sanity check on entry values
	if(endOfStackVal < START_OF_RAM || endOfStackVal > END_OF_RAM)
		return;		/* Stack limit value impossible */
	if(	(resetEntry < START_OF_RAM || resetEntry > END_OF_RAM) &&
		(resetEntry < START_OF_FLASH || resetEntry > END_OF_FLASH) )
		return;		/* Reset vector outside executable range */
	// Call reset entry section ptr in image
	/* Turn off interrupts */
	for(i=0;i<32;i++){NVIC_DisableIRQ(i);}
	vector = (void*)resetEntry;
	vector();	/* Call vector */
	Reset();	/* Return state unknown, reset if it returns */
}

/* User provided clock switching function */
void SystemClock(uint32_t setting)
{
	/* Clock settings arg */
	union clock_{
	uint32_t val;
	struct {
		uint8_t lf_clk_on;
		uint8_t lf_clk_src;
		uint8_t rc_cal_tmr;
		uint8_t hf_xtal_on;
	};}clock;
	clock.val = setting;

	/* HF XTAL SETTING */
	if(clock.hf_xtal_on) 
	{
		/* Requesting HF xtal source */
		if(NRF_CLOCK->HFCLKSTAT != 0x00010001)
		{
			/* Not using xtal or not running yet */
			NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;			/* Event clear */
			NRF_CLOCK->TASKS_HFCLKSTART = 1;			/* Start xtal osc */
			while(!NRF_CLOCK->EVENTS_HFCLKSTARTED);		/* Wait start */
		}
	}
	else
	{
		/* Xtal source stop */
		NRF_CLOCK->TASKS_HFCLKSTOP = 1;
	}

	/* LF CLOCK SETTING */
	if(clock.lf_clk_on) 
	{
		/* LF clock requested */
		/* Set source register */
		NRF_CLOCK->LFCLKSRC = clock.lf_clk_src;
		/* Start it running */
		NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;				/* Clear event */
		NRF_CLOCK->TASKS_LFCLKSTART = 1;				/* Turn on osc */
		while(!NRF_CLOCK->EVENTS_LFCLKSTARTED);			/* Wait start */
		if(clock.lf_clk_src == 0)						/* LFRC selected */
		{	
			if(clock.rc_cal_tmr)
			{
				/* Requesting periodic calibration */
				NRF_CLOCK->CTIV = clock.rc_cal_tmr;
				/* Begin periodic ISR */
				NRF_CLOCK->INTENCLR = 0x1B;
				NRF_CLOCK->INTENSET = 0x18;
				NVIC_SetPriority(POWER_CLOCK_IRQn, 3);	/* Lowest priority*/
				NVIC_ClearPendingIRQ(POWER_CLOCK_IRQn);	/* Clear flag */
				NVIC_EnableIRQ(POWER_CLOCK_IRQn);		/* Enable int */
			}
			/* Calibration of LFRC */
			NRF_CLOCK->EVENTS_DONE = 0;					/* Clear event */
			NRF_CLOCK->TASKS_CAL = 1;					/* Trigger task */
			while(!NRF_CLOCK->EVENTS_DONE);				/* Wait for done */
		}
	}
	else
	{
		/*LF source stop */
		NRF_CLOCK->TASKS_LFCLKSTOP;		
	}
}

// Non-user pre-initialisation routine for safe entry
//void SystemInit(void)
//{
//	// NVMC (non-volatile memory controller)
//	NRF_NVMC->CONFIG = 0;				/* Stop any NVMC operations*/
//	while(!NRF_NVMC->READY);			/* Wait for it to stop */
//
//	// MPU (code protection)
//	NRF_MPU->PERR0 = 0;					/* No peripherals protected */
//	NRF_MPU->RLENR0	= 0;				/* No ram is protected */ 
//	NRF_MPU->PROTENSET0 = 0;			/* Read protection disabled */
//	NRF_MPU->PROTENSET1 = 0;			/* Read protection disabled */
//	NRF_MPU->DISABLEINDEBUG = 0;		/* Memory protection active in debug */
//	NRF_MPU->PROTBLOCKSIZE = 0;			/* 4kB protection block size */
//
//	// Retained registers
//	NRF_GPIO->DIRCLR = 0xFFFFFFFF;		/* All pins inputs */
//	NRF_POWER->RESET = 0;				/* Stop reset from debug */
//	NRF_POWER->RAMON	= 0x00030003;	/* RAM blocks 1+2 on, retained on off*/
//	NRF_POWER->RAMONB	= 0x00030003;	/* RAM blocks 2+3 on, retained on off*/
//
//	// Brownout comparator
//	NRF_POWER->POFCON = 0;				/* No brownout detect */
//
//	// Startup cpu clocks 
//	SystemClock(STARTUP_CLOCK_ARG);
//
//	// Return to crt0.s sequence
//	return;
//}
// Some form of simple delay is needed at startup for gpio to be valid
// This is a quick blocking delay for such purposes using inline assembler
void DelayUs(uint32_t us)
{
	asm volatile(
	"loop:			\n\t" // do {loop start
	"mov %0, %0		\n\t" // Nop(), 62.5ns
	"mov %0, %0		\n\t" // Nop(), 62.5ns
	"mov %0, %0		\n\t" // Nop(), 62.5ns
	"mov %0, %0		\n\t" // Nop(), 62.5ns
	"mov %0, %0		\n\t" // Nop(), 62.5ns
	"mov %0, %0		\n\t" // Nop(), 62.5ns
	"mov %0, %0		\n\t" // Nop(), 62.5ns
	"mov %0, %0		\n\t" // Nop(), 62.5ns
	"sub %0, #1		\n\t" // r--
	"bne loop		\n\t" // } while(r)
	: /*Out*/ : /*In*/ "r" (us)
	: /*Clobbers*/ "cc");
}


void system_reset(void)
{
	/* CMSIS reset function*/
	NVIC_SystemReset(); 
}
// Replace the following if required
void __attribute__ ((weak)) default_handler(void)
{
	/* Unimplemented isr handler should reset */
	system_reset();
}
