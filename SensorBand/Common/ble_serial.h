#ifndef _BLE_SERIAL_H_
#define _BLE_SERIAL_H_

#include <stdint.h>
#include "utils/Queue.h"

// Global variables, mainly for debug 
// Instance of the nordic uart service and other variables
extern ble_nus_t m_nus;	
extern void (*ble_serial_user_cb)(void);
extern volatile bool serial_in_queue_flag;
extern uint8_t serial_out_send_record[];
extern uint8_t ble_tx_queue_max_len, serial_out_send_record_tail_index, serial_out_pending_count;
// The serial data buffers 
extern uint8_t serial_in_buffer[];
extern uint8_t serial_out_buffer[];
// The serial fifo queues
extern queue_t serial_in_queue;
extern queue_t serial_out_queue;

// Function prototypes
// Call from services initialise to setup buffered serial service operation
void ble_serial_service_init(void);
// Add data to the outgoing serial buffer - returns the added segment length (or max available space for NULL pointer)
uint16_t ble_serial_service_send(const uint8_t* data_buffer, uint16_t data_len);
// Check input serial buffer for input data and/or extract it - returns copied length (or count received for NULL pointer)
uint16_t ble_serial_service_receive(uint8_t* data_buffer, uint16_t data_len);
// SoftDevice event handler, called for all BLE events to handle connection changes and serial tasks
void ble_serial_event_handler(ble_nus_t * p_nus, ble_evt_t * p_ble_evt);
// Called to check output queue for new data
void ble_serial_transmit_handler(void);
	
#endif