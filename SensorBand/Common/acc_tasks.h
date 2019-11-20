#ifndef _ACC_TASKS_H_
#define _ACC_TASKS_H_
// Headers
#include <stdbool.h>
#include "EpochCalc.h"
#include "Config.h"

// Definitions
#ifndef EPOCH_LENGTH_DEFAULT
#define EPOCH_LENGTH_DEFAULT		(60ul * 1)		// 1 minute epoch interval
#endif
#ifndef EPOCH_NVM_SIZE_TOTAL
#define EPOCH_NVM_SIZE_TOTAL		(64ul * 1024)	// Size of program flash used for data 
#endif

#define EPOCH_NVM_BLOCK_SIZE		(512ul)													// Size of an epoch data block
#define EPOCH_NVM_BLOCK_COUNT		(EPOCH_NVM_SIZE_TOTAL / EPOCH_NVM_BLOCK_SIZE)			// Number of summary blocks in the NVM
#define EPOCH_NVM_BLOCK_DATA_LEN	(EPOCH_NVM_BLOCK_SIZE - (sizeof(EpochBlockInfo_t)) - 22 - 2)
#define EPOCH_BLOCK_DATA_COUNT		(EPOCH_NVM_BLOCK_DATA_LEN / (sizeof(Epoch_sample_t)) )	// Number of epoch samples per block
#define EPOCH_BLOCK_NUMBER_LAST		(0xFFFE)												// Valid blocks numbered within this range, wraps arround
#define EPOCH_BLOCK_INDEX_INVALID	(0xFFFF)												// A value for invalid array indexes

// NVM block data formats
#define BLOCK_FORMAT_EPOCH_DATA		0		// Default/general data format
#define BLOCK_FORMAT_EPOCH_DATAv2		1	// As above but added epoch period

// Types

// Each data point saved
typedef union Epoch_sample_tag
{
	uint16_t w[4];
	uint8_t b[8];
	struct {
		int8_t batt;
		int8_t temp;
		int8_t accel;
		int8_t steps;
		int8_t epoch[4];
	} part;
} Epoch_sample_t;

// Information tag in each block
typedef struct EpochBlockInfo_tag {
	uint16_t block_number;
	uint16_t data_length;
	uint32_t time_stamp;	
} EpochBlockInfo_t;

// Epoch data block type
typedef struct Epoch_block_tag
{
	// Tag at start of each block - 8 bytes
	EpochBlockInfo_t info;	
	// Block data format
	uint16_t blockFormat;
	// Added the epoch period
	uint16_t blockEpochPeriod;
	// Extended data area, implementation specific
	uint8_t meta_data[18]; 
	// 480 bytes of sequential epoch entries (1 hour/ 60 mins)
	Epoch_sample_t epoch_data[EPOCH_BLOCK_DATA_COUNT];
	// Checksum, ECC or CRC etc.
	uint16_t check;
} Epoch_block_t;

// Variables
// Epoch activity logger
extern pstorage_handle_t	epoch_pstorage_handle;	// Persistent storage handle for blocks requested by the module
extern Epoch_block_t		activeEpochBlock;		// 512 byte buffer currently active (includes block, size and time)
extern uint16_t				activeIndex;			// Position of active block in NVM
extern const uint16_t		epockBlockCount;


// Functions
// Find the logging start position and initialize NVM
bool AccelEpochLoggerInit(void);
// Begin logging epoch data to the active block
bool AccelEpochLoggerStart(void);
// Stop the logger process and close active block
bool AccelEpochLoggerStop(void);
// Check if current epoch can be written, RAM and NVM
void AccelEpochWriteTasks(void);
// Apply the offset to the current window
void AccelCalcEpochWindow(void);
// Epoch block read routine
bool AccelEpochBlockRead(uint8_t* destination, uint16_t offset, uint16_t length, uint16_t index);
// Queue all the NVM epoch data to be erased
bool AccelEpochBlockClearAll(void);
// Add epoch data to the active block
void AccelPstorageAddEpoch(Epoch_sample_t* data);

#endif
//EOF



