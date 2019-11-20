#include <stdint.h>
#include "nrf51.h"
#include "Uart0.h"
#include "nordic_common.h"
#include "nrf_delay.h"
#include "nrf_gpiote.h"
#include "nrf51_bitfields.h"
#include "boards.h"

unsigned char uart0_rx_buffer[U0_BUFF_SIZE_RX];
unsigned char uart0_tx_buffer[U0_BUFF_SIZE_TX];
unsigned short uart0_rx_head, uart0_rx_tail;
unsigned short uart0_tx_head, uart0_tx_tail;

#define Uart0EnableIrq()	NVIC_EnableIRQ(UART0_IRQn)
#define Uart0DisableIrq()   NVIC_DisableIRQ(UART0_IRQn)

void __attribute__ ((interrupt ("IRQ"))) UART0_IRQHandler(void)
{ 
	// Clear flag
	NVIC->ICPR[0] = (1lu<<UART0_IRQn);
	
	if(NRF_UART0->EVENTS_TXDRDY)
	{
		NRF_UART0->EVENTS_TXDRDY = 0;
		uart0_tx_head++;
		uart0_tx_head %= U0_BUFF_SIZE_TX;
		if(uart0_tx_head != uart0_tx_tail)
		{
			NRF_UART0->TXD = uart0_tx_buffer[uart0_tx_head];
		}
	}
	
	if(NRF_UART0->EVENTS_RXDRDY)
	{
		NRF_UART0->EVENTS_RXDRDY = 0;
		uart0_rx_buffer[uart0_rx_tail++] = (uint8_t)NRF_UART0->RXD;
		uart0_rx_tail %= U0_BUFF_SIZE_RX;
	}
}

void Uart0Init(int tx, int rx, int cts, int rts, int hwfc, int baud)
{
	Uart0Clear();

	NVIC_DisableIRQ(UART0_IRQn);
	if(NRF_UART0->ENABLE)
		nrf_delay_us(100); // Incase we may have to allow stop bit to finish
	
	NRF_UART0->ENABLE		= (UART_ENABLE_ENABLE_Disabled << UART_ENABLE_ENABLE_Pos);
	NRF_UART0->TASKS_STARTTX = 0;
	NRF_UART0->TASKS_STARTRX = 0;

	nrf_gpio_cfg_output(tx);
	nrf_gpio_pin_set(tx);
	nrf_gpio_cfg_input(rx, NRF_GPIO_PIN_NOPULL);
	NRF_UART0->PSELTXD = tx;
	NRF_UART0->PSELRXD = rx;

	if (hwfc)
	{
		nrf_gpio_cfg_output(rts);
		nrf_gpio_cfg_input(cts, NRF_GPIO_PIN_NOPULL);
		NRF_UART0->PSELCTS = cts;
		NRF_UART0->PSELRTS = rts;
		NRF_UART0->CONFIG  = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
	}

	switch (baud) {
		case (1200) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud1200	<< UART_BAUDRATE_BAUDRATE_Pos); break;
		case (2400) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud2400	<< UART_BAUDRATE_BAUDRATE_Pos); break;
		case (4800) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud4800	<< UART_BAUDRATE_BAUDRATE_Pos); break;
		case (9600) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud9600	<< UART_BAUDRATE_BAUDRATE_Pos); break;
		case (1440) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud14400	<< UART_BAUDRATE_BAUDRATE_Pos); break;	
		case (19200) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud19200 << UART_BAUDRATE_BAUDRATE_Pos); break;
		case (28800) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud28800 << UART_BAUDRATE_BAUDRATE_Pos); break;
		case (38400) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud38400 << UART_BAUDRATE_BAUDRATE_Pos); break;	
		case (57600) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud57600 << UART_BAUDRATE_BAUDRATE_Pos); break;
		case (76800) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud76800 << UART_BAUDRATE_BAUDRATE_Pos); break;
		case (115200) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud115200 << UART_BAUDRATE_BAUDRATE_Pos); break;
		case (230400) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud230400 << UART_BAUDRATE_BAUDRATE_Pos); break;
		case (250000) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud250000 << UART_BAUDRATE_BAUDRATE_Pos); break;
		case (460800) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud460800 << UART_BAUDRATE_BAUDRATE_Pos); break;
		case (921600) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud921600 << UART_BAUDRATE_BAUDRATE_Pos); break;
		case (1000000) :	NRF_UART0->BAUDRATE = (UART_BAUDRATE_BAUDRATE_Baud1M  << UART_BAUDRATE_BAUDRATE_Pos); break;
		default : break;
	}	

	NRF_UART0->ENABLE		= (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
	NRF_UART0->TASKS_STARTTX = 1;
	NRF_UART0->TASKS_STARTRX = 1;
	NRF_UART0->EVENTS_RXDRDY = 0;
	NRF_UART0->EVENTS_TXDRDY = 1;
	NRF_UART0->INTENSET = UART_INTENSET_TXDRDY_Msk;	
	NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Msk;
	NVIC_SetPriority(UART0_IRQn, U0_PRIORITY);
	NVIC_EnableIRQ(UART0_IRQn);
}

void Uart0Clear(void)
{
	uart0_rx_head = uart0_rx_tail = 0;
	uart0_tx_head = uart0_tx_tail = 0;
}

uint32_t Uart0MeasureBaud(uint32_t pin, uint32_t edgecount, uint32_t timeout_ms)
{
	// Init vars
	uint32_t timeout = timeout_ms/4;	// 4ms timer period
	uint32_t baudInterval = 0xffffffff;
	uint32_t pintime = 0;
	uint32_t baud;

		// First determine baud using edge intervals
	nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_NOPULL);
	
	// Init timer 1 to measure edges
	NRF_TIMER1->POWER = TIMER_POWER_POWER_Enabled;
	NRF_TIMER1->SHORTS = 0;
	NRF_TIMER1->PRESCALER = 0;
	NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;
	NRF_TIMER1->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
	NRF_TIMER1->CC[1] = 0xffff;
	
	// Set GPIOTE to trigger timer 1 from Rx pin toggle
	NVIC_DisableIRQ(GPIOTE_IRQn);
	nrf_gpiote_event_config(0, pin, NRF_GPIOTE_POLARITY_HITOLO);
	
  // Configure PPI channel 0 to trigger capture event
  NRF_PPI->CH[0].TEP = (uint32_t)&NRF_TIMER1->TASKS_CAPTURE[0];
  NRF_PPI->CH[0].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[0];
  NRF_PPI->CHEN = (PPI_CHEN_CH0_Enabled << PPI_CHEN_CH0_Pos);
	
	// Start timer
	NRF_TIMER1->TASKS_CLEAR = 1;
	NRF_TIMER1->TASKS_START = 1;
	NRF_TIMER1->EVENTS_COMPARE[1] = 0;
	
	// Baud measure method using ccp on edges
	while(timeout)
	{
		if(NRF_GPIOTE->EVENTS_IN[0])
		{
			unsigned short pulseTime;
			pulseTime = NRF_TIMER1->CC[0] - pintime;
			pintime = NRF_TIMER1->CC[0];
			NRF_GPIOTE->EVENTS_IN[0] = 0;
			if(pulseTime < baudInterval)	
				baudInterval = pulseTime;
			if(edgecount-- <= 0)break; // After specified edge number
			continue;
		}
		if(NRF_TIMER1->EVENTS_COMPARE[1])
		{
			NRF_TIMER1->EVENTS_COMPARE[1] = 0;
			timeout--;
		}
	}

	// Turn off PPI
	NRF_PPI->CHEN = 0;
	NRF_TIMER1->TASKS_STOP = 1;
	NRF_TIMER1->POWER = TIMER_POWER_POWER_Disabled;
	nrf_gpiote_unconfig(0);
	
	if(baudInterval == 0 ) return 0;	// Failed, interval time of zero!
	
	// Find baud
	baud = (32000000/baudInterval);
	if (baud > ((uint32_t)(1.05f*921600)) ) return 0;
	else if ( (baud<((uint32_t)(1.05f*921600))) && (baud>((uint32_t)(0.95f*921600))))baud = 921600;
	else if ( (baud<((uint32_t)(1.05f*460800))) && (baud>((uint32_t)(0.95f*460800))))baud = 460800;
	else if ( (baud<((uint32_t)(1.05f*250000))) && (baud>((uint32_t)(0.95f*250000))))baud = 250000;
	else if ( (baud<((uint32_t)(1.05f*115200))) && (baud>((uint32_t)(0.95f*115200))))baud = 115200;
	else if ( (baud<((uint32_t)(1.05f*57600))) && (baud>((uint32_t)(0.95f*57600))))baud = 57600;
	else if ( (baud<((uint32_t)(1.05f*38400))) && (baud>((uint32_t)(0.95f*38400))))baud = 38400;
	else if ( (baud<((uint32_t)(1.05f*19200))) && (baud>((uint32_t)(0.95f*19200))))baud = 19200;
	else if ( (baud<((uint32_t)(1.05f*9600))) && (baud>((uint32_t)(0.95f*9600))))baud = 9600;
	else if ( (baud<((uint32_t)(1.05f*4800))) && (baud>((uint32_t)(0.95f*4800))))baud = 4800;	
	else return 0; // Unsupported rate	
	
	return baud;
}

int Uart0Putc(char c)
{
	NVIC_DisableIRQ(UART0_IRQn);
	if((uart0_tx_head - uart0_tx_tail) == 1)
	{
		NVIC_EnableIRQ(UART0_IRQn);
		return -1;
	}
	uart0_tx_buffer[uart0_tx_tail] = c;
	if(uart0_tx_head == uart0_tx_tail)
	{
		NRF_UART0->TXD = uart0_tx_buffer[uart0_tx_head];
	}
	uart0_tx_tail++;
	uart0_tx_tail %= U0_BUFF_SIZE_TX;
	NVIC_EnableIRQ(UART0_IRQn);
	return 0;
}

int Uart0Puts(const char* str)
{
	unsigned short first;
	// Ints off
	NVIC_DisableIRQ(UART0_IRQn);
	// If buffer full
	if((uart0_tx_head - uart0_tx_tail) == 1)
	{
		NVIC_EnableIRQ(UART0_IRQn);
		return -1;
	}
	// Remember first char location 
	first = uart0_tx_tail;
	// Puts rest of string to buffer
	while(*str != '\0')
	{
		uart0_tx_buffer[uart0_tx_tail] = *str++;
		uart0_tx_tail++;
		uart0_tx_tail %= U0_BUFF_SIZE_TX;
		if((uart0_tx_head - uart0_tx_tail) == 1)
			break;
	}
	// First char only, check if uart is idleing + load
	if(uart0_tx_head == first)
	{
		NRF_UART0->TXD = uart0_tx_buffer[uart0_tx_head];
	}
	// Enable ints
	NVIC_EnableIRQ(UART0_IRQn);
	return 0;
}

int Uart0Busy(void)
{
	return ((uart0_tx_tail != uart0_tx_head)?1:0);
}

int Uart0Getc(void)
{
	int ch;
	if(uart0_rx_head != uart0_rx_tail)
	{
		ch = uart0_rx_buffer[uart0_rx_head++];
		uart0_rx_head %= U0_BUFF_SIZE_RX;
		return ch;
	}
	else
	{
		return -1;
	}
}


