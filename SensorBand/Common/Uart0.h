#ifndef _U0_HEADER_
#define _U0_HEADER_

#include <stdint.h>

extern unsigned char uart0_rx_buffer[];
extern unsigned char uart0_tx_buffer[];
extern unsigned short uart0_rx_head, uart0_rx_tail;
extern unsigned short uart0_tx_head, uart0_tx_tail;

void Uart0Clear(void);
	
void Uart0Init(int tx, int rx, int cts, int rts, int hwfc, int baud);

void Uart0EnableIrq(void);
void Uart0DisableIrq(void);

uint32_t Uart0MeasureBaud(uint32_t pin, uint32_t edgecount, uint32_t timeout_ms);

int Uart0Putc(char c);
int Uart0Puts(const char* str);

int Uart0Busy(void);

int Uart0Getc(void);


#endif
