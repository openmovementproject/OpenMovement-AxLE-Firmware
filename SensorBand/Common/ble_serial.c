#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf_nvic.h"
#include "nrf_soc.h"
#include "nrf.h"
#include "ble.h"
#include "softdevice_handler.h"
#include "ble_nus.h"
#include "app_util_platform.h"
#include "ble_serial.h"
#include "utils/Queue.h"
#include "Config.h"
#include "HardwareProfile.h"

// Definitions

// Instance of the nordic uart service and other variables
ble_nus_t m_nus;	
void (*ble_serial_user_cb)(void) = NULL;
volatile bool serial_in_queue_flag = false;
volatile bool serial_connected = false;
// The serial fifo queues
queue_t serial_in_queue;
queue_t serial_out_queue;
// The serial data buffers 
uint8_t serial_in_buffer[BLE_SERIAL_IN_QUEUE_LEN];
uint8_t serial_out_buffer[BLE_SERIAL_OUT_QUEUE_LEN];

// Driver internal prototypes
extern void ble_serial_receive_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length);
extern void ble_serial_transmit_handler(void);
extern void ble_serial_init_buffers(void);
extern void ble_serial_radio_evt_enable(void);
extern void SWI1_IRQHandler(void); // Radio events

// Driver functions source
// Call once from services initialize and enable serial service operation
void ble_serial_service_init(void)
{
	uint32_t err_code;
	ble_nus_init_t nus_init;
	// Called before any connections, reset the state to disconnected and dump any data
	ble_serial_init_buffers();
	// Clear pending packet and connected flags
	serial_in_queue_flag = false;
	serial_connected = false;
	// Serial service initialization for Nordic UART emulator 
	memset(&nus_init, 0, sizeof(nus_init));
	nus_init.data_handler = ble_serial_receive_handler;
	err_code = ble_nus_init(&m_nus, &nus_init);
	APP_ERROR_CHECK(err_code);
	// Start radio event handler. Checks for new output data after each radio event
	ble_serial_radio_evt_enable();
}

// Add data to the outgoing serial buffer - returns the added segment length (or max available space for NULL pointer)
uint16_t ble_serial_service_send(const uint8_t* data_buffer, uint16_t data_len)
{
	uint32_t length;
	// Check if connected
	if(serial_connected == false)
		return 0;
	// Check source buffer parameter
	if(data_buffer == NULL)
	{
		// No data pointer given, return maximum available buffer space instead
		length = QueueFree(&serial_out_queue);
	}
	else
	{
		// Copy data to queue and return the count copied successfully
		length = QueuePush(&serial_out_queue, (const void*) data_buffer, data_len);
		// If data was added, try sending it out to client
// Called every connection event
//		if(length != 0)	ble_serial_transmit_handler();
	}
	// Return size value
	return (uint16_t)length;
}
// Check input serial buffer for input data and/or extract it - returns copied length (or count received for NULL pointer)
uint16_t ble_serial_service_receive(uint8_t* data_buffer, uint16_t data_len)
{
	uint32_t length;
	// Clear reception flag
	serial_in_queue_flag = false;
	// Verify destination parameter
	if(data_buffer == NULL)
	{
		// No data pointer given, return maximum available buffered input available
		length = QueueLength(&serial_in_queue);
	}
	else
	{
		// Copy data to queue and return the count copied successfully
		length = QueuePop(&serial_in_queue, data_buffer, data_len);
	}
	// Return size value
	return (uint16_t)length;
}
// SoftDevice event handler, called on BLE events, checks for relevant events and maintains state, keeps data flowing efficiently
void ble_serial_event_handler(ble_nus_t * p_nus, ble_evt_t * p_ble_evt)
{
	uint32_t err_code;
	// Check parameters are valid first
	if ((p_nus == NULL) || (p_ble_evt == NULL))
	{
		// Odd event interception, unexpected early exit
		return;
	}
	// Check for relevant events to handle. Call the service handlers and data transfer tasks
	switch (p_ble_evt->header.evt_id)
	{
		case BLE_GAP_EVT_CONNECTED:
			// Dump any data, reset buffer state and calculate the queue settings from packet queue length
			ble_serial_init_buffers();
			// Update connected flag
			serial_connected = true;
			// Pass the connect event to the ble nus service 
			ble_nus_on_ble_evt(&m_nus, p_ble_evt);
			break;

		case BLE_GAP_EVT_DISCONNECTED:
			// Clear the serial buffers ready for next connection
			ble_serial_init_buffers();
			// Update connected flag
			serial_connected = false;			
			// Pass the event to the ble disconnect nus service 
			ble_nus_on_ble_evt(&m_nus, p_ble_evt);
			break;

		case BLE_GATTS_EVT_WRITE:
			// Pass the data write event to the ble nus service, data in handler called or notify/indicate state set
			ble_nus_on_ble_evt(&m_nus, p_ble_evt);
			// Call user serial handler if set
			if(ble_serial_user_cb != NULL)
				ble_serial_user_cb();
			// Indicate new data input
			serial_in_queue_flag = true;
			break;

		// All these events will result in a packet send attempt if connected
		// Indication confirmation event, send next indication - indication send method compatible
		case BLE_GATTS_EVT_HVC:
		// BLE packet(s) sent event or connection event completed(internally made), try sending more out data - one or more notifications is sent
		case BLE_EVT_TX_COMPLETE:

			// Try to add outgoing packets to queue and clear sent data from buffer
			ble_serial_transmit_handler();
			break;
		// Other events irrelevant to this module
		default:
			break;
	}
}


// Internal private routines
// Data handler for remote->local data flow and reply
void ble_serial_receive_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
	// Parameter checks with early exit and error if unexpected
	if(length == 0)
	{
		// No data
		return;
	}
	if(p_data == NULL)
	{
		// Invalid call params, shouldn't ever occurr 
		app_error_fault_handler(0xBEEFB00B + __LINE__, 0, (uint32_t)NULL);
	}			
	// Try moving input data into receive fifo
	else
	{
		length -= QueuePush(&serial_in_queue, p_data, length);
		if(length != 0)
		{
			// Lost rx'ed data - assert in debug only or just dump lost data
#ifdef __DEBUG
			app_error_fault_handler(0xBEEFB00B + __LINE__, 0, (uint32_t)NULL);
#endif
		}	
	}
	return;
}

// Serial handler to send data out
void ble_serial_transmit_handler(void)
{	
	uint32_t return_code;
	uint8_t* queue_ptr;

	// Check if connected
	if(serial_connected == false)
		return;
	// Check for outgoing data, queue packet(s) for transmit to client until max packets pending is reached
	for(;;)
	{
		int32_t length;
		// Check available output data contiguous length, sent from queue directly
		length = QueueContiguousEntries(&serial_out_queue, (void**)&queue_ptr);
		// Exit early if no more out data present in buffer to send
		if(length <= 0)
			break;
		// Clamp length to maximum serial packet length
		if(length > BLE_NUS_MAX_DATA_LEN)
			length = BLE_NUS_MAX_DATA_LEN;
		// Try adding data to the outgoing radio's packet send queue
		return_code = ble_nus_string_send(&m_nus, queue_ptr, length);
		// Check result of transmit packet routine
		switch(return_code) {
			case NRF_SUCCESS: 
			{
				// If packet queued for send then update the sent packet size record array 
				QueueExternallyRemoved(&serial_out_queue, length);	
				// Try to enqueue another packet
				continue;
			}	
			// Other expected errors
			case BLE_ERROR_NO_TX_PACKETS:
			case NRF_ERROR_INVALID_STATE:
			{
				// The last packet did not get added, no change and exit
				break;
			}
			default :
			{
				// Check for other unexpected errors
#ifdef __DEBUG
				APP_ERROR_CHECK(return_code);		
#endif
				break;
			}		
		} // Switch...
		// Stop looping on first send failure;
		break;
	} // For...
}

// Dump any remaining data from the buffers and clear buffer queue states
void ble_serial_init_buffers(void)
{
	// Initialize and reset the serial buffer queues
	QueueInit(&serial_in_queue, sizeof(uint8_t), BLE_SERIAL_IN_QUEUE_LEN, serial_in_buffer);	
	QueueInit(&serial_out_queue, sizeof(uint8_t), BLE_SERIAL_OUT_QUEUE_LEN, serial_out_buffer);	
}

// The event created on radio on and off events and the event handler
//void ble_serial_radio_evt_handler(bool radio_active)
void SWI1_IRQHandler(void)
{
	// Check if connected
	if(serial_connected == false)
		return;	
	// Create a new event to pass to the local serial BLE event handler
	ble_evt_t p_ble_evt = {.header.evt_id = BLE_EVT_TX_COMPLETE, .header.evt_len = 0};
	ble_serial_event_handler(&m_nus, &p_ble_evt);	
}
void ble_serial_radio_evt_enable(void)
{
	uint32_t err_code;
	// Minimize latency by checking for data after every radio connection event (low priority)
	err_code = sd_nvic_ClearPendingIRQ(RADIO_NOTIFICATION_IRQn);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_SetPriority(RADIO_NOTIFICATION_IRQn, APP_IRQ_PRIORITY_LOW);
	APP_ERROR_CHECK(err_code);
	err_code = sd_nvic_EnableIRQ(RADIO_NOTIFICATION_IRQn);
	APP_ERROR_CHECK(err_code);
	err_code = sd_radio_notification_cfg_set(NRF_RADIO_NOTIFICATION_TYPE_INT_ON_INACTIVE, NRF_RADIO_NOTIFICATION_DISTANCE_NONE);
	APP_ERROR_CHECK(err_code);	
}


// EOF

