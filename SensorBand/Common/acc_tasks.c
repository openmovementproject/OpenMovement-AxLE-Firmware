

// Include
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nordic_common.h"
#include "pstorage.h"
#include "ble_nus.h"
#include "app_scheduler.h"
#include "app_timer_appsh.h"
#include "app_timer.h"
#include "nrf_drv_gpiote.h"
#include "Peripherals/LIS3DH.h"
#include "Peripherals/SysTime.h"
#include "Analog.h"
#include "EpochCalc.h"
#include "acc_tasks.h"
#include "ble_serial.h"
#include "AsciiHex.h"

// Definitions

// Types 

// Globals
pstorage_handle_t	epoch_pstorage_handle;						// Persistent storage handle for blocks requested by the module
Epoch_block_t		activeEpochBlock;							// 512 byte buffer currently active (includes block, size and time)
uint16_t			activeIndex = EPOCH_BLOCK_INDEX_INVALID;	// Position of active block in NVM
const uint16_t		epockBlockCount = EPOCH_NVM_BLOCK_COUNT;	// Total number of epoch blocks

// External variables
extern EpochTime_t rtcEpochTriplicate[3];
extern bool gStreamDataSetting;

// Prototypes
void AccelDeviceEventHandler(void * p_event_data, uint16_t event_size);
void AccelDeviceEventCheck(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
void AccelDeviceInterruptSetup(bool enable);
void AccelPstorageStoreActiveBlock(void);
void AccelPstorageEventHandler(pstorage_handle_t * p_handle, uint8_t op_code, uint32_t result, uint8_t* p_data, uint32_t data_len);

// Source
bool AccelEpochLoggerInit(void)
{
	pstorage_module_param_t param;
	ret_code_t err_code;
	EpochBlockInfo_t block_info;
	uint32_t max_block_num, index_start;
	uint32_t index;
	
	// Clear global variables to indicate invalid
	activeEpochBlock.info.block_number = 0;
	
	// The pstorage module must be initialised ONCE, done externally externally (main.c) i.e.
	//err_code = pstorage_init();	
	//APP_ERROR_CHECK(err_code);	

	// Verify the block of data is the expected size - should be 512 bytes
	if(sizeof(Epoch_block_t) != EPOCH_NVM_BLOCK_SIZE)
	{
		app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
		return false;
	}

	// Initialise the pstorage module
	param.block_size  = EPOCH_NVM_BLOCK_SIZE;
	param.block_count = EPOCH_NVM_BLOCK_COUNT;
	param.cb		  = AccelPstorageEventHandler;
	err_code = pstorage_register(&param, &epoch_pstorage_handle);	
	APP_ERROR_CHECK(err_code);	
	if(err_code != NRF_SUCCESS)
	{
		// Failed to initialise NVM
		app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
		return false;
	}

	// Set up the read position variables
	index_start = EPOCH_BLOCK_INDEX_INVALID;
	max_block_num = 0;
	
	// Read the NVM data record to find the write position
	for(index = 0; index < EPOCH_NVM_BLOCK_COUNT; index++)
	{
		// Read info section of each block. Ensure data was read into block info variable
		if(AccelEpochBlockRead((uint8_t*)&block_info, 0, sizeof(EpochBlockInfo_t), index) == false)
		{
			// Failed to load, take corrective action. Start at index 0 (any/current block number)
			app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
			index_start = EPOCH_NVM_BLOCK_COUNT;
			break;
		}			
		// Check for valid data block number - skip invalid/empty blocks
		if(block_info.block_number > EPOCH_BLOCK_NUMBER_LAST)
			continue;
		// Track max block number index
		if(block_info.block_number > max_block_num)
		{
			index_start = index;
			max_block_num = block_info.block_number;
		}
	}// for every block

	// Work out the start index for the next logging position
	if(		(index_start < EPOCH_NVM_BLOCK_COUNT) &&
			(max_block_num < EPOCH_BLOCK_NUMBER_LAST) )
	{
		activeIndex = index_start + 1;
		block_info.block_number = max_block_num + 1;
	}
	else
	{
		// Invalid or startup state - zeroed NVM -> index 0/block 1, 0xff NVM -> index 0/block 0
		activeIndex = 0;
		block_info.block_number = 0;		
	}

	// Clamp index and block number to wrap withing ranges
	if(activeIndex >= EPOCH_NVM_BLOCK_COUNT)				activeIndex = 0;
	if(block_info.block_number > EPOCH_BLOCK_NUMBER_LAST)	block_info.block_number = 0;
	
	// Clear current logging block data count and other variables
	//block_info.data_length = 0;
	memset(&activeEpochBlock, 0, EPOCH_NVM_BLOCK_SIZE); 
	
	// On first epoch write the other active block variables will be initialized

	// Set active block info
	memcpy(&activeEpochBlock.info,  &block_info, sizeof(EpochBlockInfo_t));
	// Set the current epoch window to not end - not logging
	status.epochCloseTime = SYSTIME_VALUE_INVALID;
	// Ready to log epoch data
	return true;
}

void AccelCalcEpochWindow(void)
{
	uint32_t timeNow, offset;
	int32_t remaining, delta;
	// Get current rtc time
	timeNow = rtcEpochTriplicate[0];
	// Calculate current required offset within current window
	offset = (timeNow + settings.epochOffset) % settings.epochPeriod;
	// Calculate actual remaining time in window
	remaining = status.epochCloseTime - timeNow;
	// Calculate the error between the windows
	delta = offset - remaining;
	// If current epoch close time is invalid
	if(remaining > settings.epochPeriod)
	{
		// Set close time one epoch period from now
		status.epochCloseTime = timeNow + (settings.epochPeriod - offset);
		// The current sample is invalid anyway, set fixed period
		status.epochSpanLenth = (settings.epochPeriod - offset);		
	}
	// Check remaining and offset agree
	else if(delta == 0)
	{
		// If zero, set the next window limit. Otherwise, no action needed
		if(remaining <= 0)
		{
			// Reset the window limit
			status.epochCloseTime = timeNow + settings.epochPeriod;
			status.epochSpanLenth = settings.epochPeriod;
		}
		else
		{
			// No changes required (update not necessary)
		}
	}
	else
	{
		// Adjust the current window limit to coincide on a period multiple
		status.epochCloseTime = timeNow + (settings.epochPeriod - offset);
		// Adjust window size to fit adjustment
		status.epochSpanLenth += delta;
	}
	return;
}

bool AccelEpochLoggerStart(void)
{
	accel_t current = {0};
	// Set current time and clear data length
	activeEpochBlock.info.data_length = 0;
	// Check accelerometer is present first
	if(!AccelPresent())
	{
		// Clear epoch vars 
		EpochInit(&current);
		// Failed to initialise accelerometer
		app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
		return false;
	}
	// Calculate *first* epoch window end time
	AccelCalcEpochWindow();
	// Initialize global settings variable with modified defaults
	AccelSetting(NULL, status.accelRange, status.accelRate);
	// Start the accelerometer using global settings variable
	AccelStartup(NULL);
	// Wait for sensor before reading a sample
	nrf_delay_ms(25);
	// Get the current sensor value to initialize filter
	AccelReadSample(&current);
	// Clear epoch vars 
	EpochInit(&current);
	// Clear pending interrupts
	AccelReadEvents();
	// Setup interrupt pins - Fifo/other interrupts will begin triggering
	AccelDeviceInterruptSetup(true);
	// Reset pedometer
	PedInit(PED_ONE_G_VALUE);
	return true;
}

bool AccelEpochLoggerStop(void)
{
	bool retval = false;
	// Check if accel present before stopping
	if(AccelPresent())
	{
		// Stop accelerometer interrupts
		AccelDeviceInterruptSetup(false);
		// Turn off accel - lowest power
		AccelShutdown();
		// Accel was present and stopped
		retval = true;
	}
	// If active logging and data stored, finalize
	if(activeEpochBlock.info.data_length > 0)
	{
		// NVM will be written and logger stopped on event
		AccelPstorageStoreActiveBlock();
	}
	// Set the current epoch window to not end - not logging
	status.epochCloseTime = SYSTIME_VALUE_INVALID;
	return retval;
}

bool AccelEpochBlockRead(uint8_t* destination, uint16_t offset, uint16_t length, uint16_t index)
{
	// If the index is for the active block then read from RAM
	if(index == activeIndex)
	{
		// Make fake CRC for now. TODO: Clear old epoch data and calculate CRC
		activeEpochBlock.check = 0xFFFF;
		// Copy the active block to destination
		memcpy(destination, ((uint8_t*)&activeEpochBlock) + offset, length);
		return true;
	}
	else
	{
		pstorage_handle_t block_handle;
		// Set module ID (may not be needed)
		block_handle.module_id = epoch_pstorage_handle.module_id;
		// Request to read from indexed block (at offset of zero)
		pstorage_block_identifier_get(&epoch_pstorage_handle, index, &block_handle);	
		// Ensure data was read into destination pointer variable
		if(((pstorage_load(destination, &block_handle, length, offset))) != NRF_SUCCESS)
		{
			// Failed to load, take corrective action. Start at index 0 (any/current block number)
			app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
			return false;
		}	
	}
	// Loaded NVM block successfully
	return true;
}

bool AccelEpochBlockClearAll(void)
{
	pstorage_handle_t block_handle;

	// Clear the logger state to begin at block 0
	activeIndex = 0; // Clear active index to start again at sector 0
	memset(&activeEpochBlock, 0, EPOCH_NVM_BLOCK_SIZE); // Wipe ram as well
	status.epochCloseTime = SYSTIME_VALUE_INVALID; // No end to current epoch

	// Set module ID (may not be needed)
	block_handle.module_id = epoch_pstorage_handle.module_id;
	// Request to read from block 0 (at offset of zero)
	pstorage_block_identifier_get(&epoch_pstorage_handle, 0, &block_handle);	
	// Ensure data was read into destination pointer variable
	if((pstorage_clear(&block_handle, (EPOCH_NVM_SIZE_TOTAL))) != NRF_SUCCESS)
	{
		// Failed to load, take corrective action. Start at index 0 (any/current block number)
		app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
		return false;
	}	
	// Cleared all NVM blocks queued
	return true;
}

// Called from the pstorage background module on events
void AccelPstorageEventHandler(pstorage_handle_t * p_handle, uint8_t op_code, uint32_t result, uint8_t* p_data, uint32_t data_len)
{
	switch(op_code)
	{
		case PSTORAGE_STORE_OP_CODE:
		case PSTORAGE_UPDATE_OP_CODE:

			// After update (NVM write), start the next block
			if (result == NRF_SUCCESS)
			{
				uint16_t nextBlockNum = activeEpochBlock.info.block_number + 1;
				// Either clear active block info completely
				memset(&activeEpochBlock.info,  0, sizeof(EpochBlockInfo_t));
				// Or just set write position of next point to zero
				// activeEpochBlock.info.data_length = 0;
				// Update index the next block number
				activeEpochBlock.info.block_number = nextBlockNum;
				// Update the NVM index position
				if(++activeIndex >= EPOCH_NVM_BLOCK_COUNT)
					activeIndex = 0;
			}

			break;

		case PSTORAGE_CLEAR_OP_CODE:
			// After clearing the block, get the handle to the active one and store (unless no data to store -> erase all case)
			if((result == NRF_SUCCESS) && (activeEpochBlock.info.data_length != 0))
			{
				// Commit the active epoch data block to NVM
				pstorage_handle_t nvm_handle;
				//Get the block handle being written, store to global
				result = pstorage_block_identifier_get(&epoch_pstorage_handle, activeIndex, &nvm_handle);
				if(result != NRF_SUCCESS)
				{
					app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
					return;		
				}	
				// Write the active block to NVM and check result
				result = pstorage_store(&nvm_handle, (uint8_t *)&activeEpochBlock, EPOCH_NVM_BLOCK_SIZE, 0);
				if(result != NRF_SUCCESS)
				{
					app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
					return;		
				}					
			}
			break;

		case PSTORAGE_LOAD_OP_CODE:			
		default:
			// Unhandled and un-needed events
			break;
	}			
}

void AccelPstorageStoreActiveBlock(void)
{
	uint32_t err_code, i;
	uint16_t *checkPtr, check;
	// Commit the active epoch data block to NVM
	pstorage_handle_t nvm_handle;
	// Check a valid block is being written
	if(activeIndex >= EPOCH_NVM_BLOCK_COUNT)
		return;

	// Calculate and write the checksum. Initialize variables
	checkPtr = (uint16_t*)&activeEpochBlock; check = 0;
	// Add 510 of the 512 bytes in the block as 16 bit words
	for(i = 0; i < ((EPOCH_NVM_BLOCK_SIZE / 2) - 1); i++)
	{
		check += *checkPtr++;
	}
	// Make the full block checksum equal zero (negate it)
	activeEpochBlock.check = (~check) + 1;

	//Get the block handle being written, store to global
	err_code = pstorage_block_identifier_get(&epoch_pstorage_handle, activeIndex, &nvm_handle);
	if(err_code != NRF_SUCCESS)
	{
		app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
		return;		
	}
#if 0
	//Request clearing of the block.
	err_code = pstorage_clear(&nvm_handle, EPOCH_NVM_BLOCK_SIZE);
	if(err_code != NRF_SUCCESS)
	{
		app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
		return;		
	}	
#else
	//Store GATTS information.
	err_code = pstorage_update(&nvm_handle, (uint8_t *)&activeEpochBlock, EPOCH_NVM_BLOCK_SIZE, 0);	
	if(err_code != NRF_SUCCESS)
	{
		app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
		return;		
	}							
#endif
	// The erase complete event triggers the store command
	return;
}	

void AccelPstorageAddEpoch(Epoch_sample_t* data)
{
	// Check a valid block is being written
	if(activeIndex >= EPOCH_NVM_BLOCK_COUNT)
		return;
	// Current block is full (busy writing error, shouldn't occurr)
	if(activeEpochBlock.info.data_length >= EPOCH_BLOCK_DATA_COUNT)
	{
		app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
		return;		
	}
	// Add to active epoch block, check for first data entry
	if(activeEpochBlock.info.data_length == 0)
	{
		// Write time stamp of first epoch entry to the block info
		activeEpochBlock.info.time_stamp = status.epochCloseTime;
		// Set the data block format
		activeEpochBlock.blockFormat = BLOCK_FORMAT_EPOCH_DATAv2;
		// Add epoch period into old meta data region
		activeEpochBlock.blockEpochPeriod = settings.epochPeriod; 
		// TODO: Update block meta data if required
		memset(activeEpochBlock.meta_data, 0, sizeof(activeEpochBlock.meta_data));
	}	
	// Add data point of new epoch
	memcpy(&activeEpochBlock.epoch_data[activeEpochBlock.info.data_length], data, sizeof(Epoch_sample_t));
	// Update length variable
	activeEpochBlock.info.data_length++;
	// Check for final entry in a full block
	if(activeEpochBlock.info.data_length >= EPOCH_BLOCK_DATA_COUNT)
	{
		// Save the block to memory, success will reset the length
		AccelPstorageStoreActiveBlock();
	}
	// The next epoch should occur after the data has been saved to NVM
	return;	
}
			
void AccelEpochWriteTasks(void)
{
	// Check if the sample window has finished
	if(rtcEpochTriplicate[0] >= status.epochCloseTime)
	{
		uint16_t steps;
		Epoch_sample_t epoch;

		// Verify the accelerometer is producing data
		if(sample_count == 0)
		{
			// Accel is broken, restart device
			app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
		}

		// Set the data part values
		epoch.part.batt = battPercent;
		epoch.part.temp = tempCelcius;
		epoch.part.accel = accel_regs.int1_src;// Orientation

		// Extended max step count by moving extra bits into accel unused bit space
		steps = PedResetSteps();
		epoch.part.steps = steps;
		epoch.part.accel &= 0x3f;
		epoch.part.accel |= (steps >> 2) & 0xC0;

		// Normalize the epoch integration by dividing by window length (i.e. Set to 'sum per second')
		eepoch_sum /= settings.epochPeriod;
		// Add to data point in block		
		memcpy(epoch.part.epoch, &eepoch_sum, sizeof(uint32_t));
		// Add result to the global buffer and reset
		AccelPstorageAddEpoch(&epoch);
		// Clear the eepoch variables - restart integrator
		eepoch_sum = 0;
		sample_count = 0;
		// Calculate next window end time
		AccelCalcEpochWindow();

        // Check pins to ensure no events are missed
		AccelDeviceEventCheck((nrf_drv_gpiote_pin_t) 0, (nrf_gpiote_polarity_t) 0);
	}
}	
					   
void AccelDeviceEventHandler(void * p_event_data, uint16_t event_size)
{
	uint8_t event = *(const uint8_t*)p_event_data;
	// Handle INT1 and INT2 high level pin events

	// Fifo event pin
	if(event == ACCEL_INT1)
	{
		uint8_t count, index;
		accel_t samples[ACCEL_FIFO_WATERMARK];

		// Read interrupt sources to clear pending interrupts
		count = AccelReadFifo(&samples[0],ACCEL_FIFO_WATERMARK);

		// FW:1.9 edit
		if(status.streamMode != 3)
		{
			// Calaulate epoch/pedometer values
			for(index = 0; index < count; index++)
			{
				// Add data to the epoch
				EpochAdd(&samples[index]);
			}
		}

		// Check for over-run error flags set
		if(accel_regs.fifo_src & 0x40) 
		{
			// Not much to do. Better to skip data and maybe log an error?
#ifdef __DEBUG
			app_error_fault_handler(0xBEEFBEEF + __LINE__, 0, (uint32_t)NULL);
#endif
			// Clear overflow condition
			AccelReadFifo(&samples[0],ACCEL_FIFO_WATERMARK);
			AccelReadEvents();
			// Early exit
			return;
		}

		// Streaming modes, data or debug info
		if(status.streamMode > 0)
		{
			uint16_t length;
			// Debug stream - Add to serial buffer 
			if(status.streamMode == 2)
			{
				static uint32_t lastTime = 0;
				if(lastTime != rtcEpochTriplicate[0])
				{
					char buffer[32];
					lastTime = rtcEpochTriplicate[0];
					length = sprintf(buffer, "%u,%s,%lu,%u,%02X\r",
						(unsigned int)AdcToMillivolt(battRaw*2),
						(const char*)TempFloat(tempRaw),
						(unsigned long)eepoch_sum, 
						(unsigned int)pedState.steps, 
						accel_regs.int1_src
					);
					// Add debug info to serial buffer ever second if space
					if(QueueFree(&serial_out_queue) >= length)
					{
						// Output to stream
						ble_serial_service_send(buffer, length);	
					}
				}
			}
			// Raw accelerometer data - add to serial buffer 
			else if((status.streamMode == 1) || (status.streamMode == 3))
			{
				// Output[] = timeStamp,Battery,Temp,Samples[WATER_MARK]
				if(QueueFree(&serial_out_queue) >= (8 + 4 + 4 + 2*(sizeof(accel_t) * ACCEL_FIFO_WATERMARK)) + 2 )
				{
					#define WRITE_SEGMENT_SIZE (5) // (5samples x 6bytes) x 2chars = 60 ascii hex bytes per pass
					uint32_t timeStamp = SYSTIME_RTC->COUNTER;
					uint16_t index = 0;
					uint8_t buffer[ (WRITE_SEGMENT_SIZE * sizeof(accel_t) * 2) ];

					// Add streaming packet data header for time, batt, temp, count, etc... 
					length = WriteBinaryToHex(buffer, (void*)&timeStamp, sizeof(uint32_t), false);			// 8 bytes
					length += WriteBinaryToHex(buffer+length, (void*)&battRaw, sizeof(uint16_t), false);	// 4 bytes
					length += WriteBinaryToHex(buffer+length, (void*)&tempRaw, sizeof(uint16_t), false);	// 4 bytes
					ble_serial_service_send(buffer, length);										// Total 16 bytes

					// Write out the encoded block in short sections to the queue
					while(index < count)
					{
						uint16_t size;
						// Calculate write length in sensor samples
						length = count - index;
						if(length > WRITE_SEGMENT_SIZE)
							length = WRITE_SEGMENT_SIZE;
						// Convert length to bytes
						size = length * sizeof(accel_t);
						// Encode into ascii hex in place. Convert data byte length to character count
						size = WriteBinaryToHex(buffer, &samples[index], size, false);
						// Add the text segment to the queue. Early exit if push fails to write all output
						if( (QueuePush(&serial_out_queue, buffer, size)) != size )
							break;
						// Increment the write offset
						index += length;
					}
					// Terminate data packet		
					ble_serial_service_send("\r\n", 2);
				}// If enough space to add to text queue
			}// Stream mode 1
		}// Stream modes
	}// INT1 source

	// Movement event pin
	if(event == ACCEL_INT2)
	{
		uint8_t click_src, int1_src;
		// Read interrupt sources to clear pending interrupts
		AccelReadEvents();
	
		click_src = accel_regs.click_src;
		int1_src = accel_regs.int1_src;

		// Check orientation
		if(int1_src & 0x40)
		{
			// Orientation change detected
			//hw_ctrl.Led2 = HW_SET_MODE(1,1,8);	// Flash 
			//hw_ctrl.Led3 = HW_SET_MODE(1,1,8);	// Flash 
		}	
		// Tap/Double-tap detection on an axis
		if(click_src & 0x7)
		{
			// Flash briefly
			if(click_src & 0x20) // Double tap
			{
				hw_ctrl.Led2 = HW_SET_MODE(1,0,1);
			}	
			else				// Single tap
			{
				//hw_ctrl.Led3 = HW_SET_MODE(1,1,16);
			}
		}
	}

	// Check pins to ensure no events are missed
	AccelDeviceEventCheck((nrf_drv_gpiote_pin_t) 0, (nrf_gpiote_polarity_t) 0);

	#ifndef GESTURE_DISABLE
	// Read orientation and run gesture detection - show goal state / enable cueing
	AccelReadEvents();
	// Gesture detection call
	GestureDetectTasks(); 
	#endif
}

void AccelDeviceEventCheck(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	ret_code_t err_code;
	static const uint8_t ACCEL_FIFO_EVENT = ACCEL_INT1;
	static const uint8_t ACCEL_MOTION_EVENT = ACCEL_INT2;
	// Ignore event data - check pin levels
	if(NRF_GPIO->IN & (1 << ACCEL_INT1))
	{
		err_code = app_sched_event_put((void*)&ACCEL_FIFO_EVENT, 1, (app_sched_event_handler_t)AccelDeviceEventHandler);
		APP_ERROR_CHECK(err_code);	
	}
	if(NRF_GPIO->IN & (1 << ACCEL_INT2))
	{
		err_code = app_sched_event_put((void*)&ACCEL_MOTION_EVENT, 1, (app_sched_event_handler_t)AccelDeviceEventHandler);
		APP_ERROR_CHECK(err_code);	
	}
}

void AccelDeviceInterruptSetup(bool enable)
{
	// Enable/disable interrupt pins on accelerometer
	ret_code_t err_code;
	if(enable == true)
	{
		// Initialize gpiote module
		if (!nrf_drv_gpiote_is_init())
		{
			err_code = nrf_drv_gpiote_init();
			APP_ERROR_CHECK(err_code);
			// Input pin setting for low to high transition with event detect
			nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(false);
			// Assign handlers to pins
			err_code = nrf_drv_gpiote_in_init(ACCEL_INT1, &in_config, AccelDeviceEventCheck);
			APP_ERROR_CHECK(err_code);
			err_code = nrf_drv_gpiote_in_init(ACCEL_INT2, &in_config, AccelDeviceEventCheck);
			APP_ERROR_CHECK(err_code);
		}
		// Enable input detection for accelerometer interrupt pins
		nrf_drv_gpiote_in_event_enable(ACCEL_INT1, true);
		nrf_drv_gpiote_in_event_enable(ACCEL_INT2, true);
	}
	else
	{
		// Disable accelerometer interrupt pins
		nrf_drv_gpiote_in_event_disable(ACCEL_INT1);
		nrf_drv_gpiote_in_event_disable(ACCEL_INT2);
		// Turn off gpiote module
		nrf_drv_gpiote_uninit();
	}
}



// EOF
