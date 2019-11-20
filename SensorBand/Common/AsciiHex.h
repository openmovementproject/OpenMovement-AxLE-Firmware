// Karim Ladha 2014
// Utils for converting between binary and ascii hex
#ifndef _ASCII_HEX_H_
#define _ASCII_HEX_H_
// Include
#include <stdint.h>

// Simple function to write capitalised hex to a buffer from binary
// adds no spaces, adds a terminating null, returns chars written
// Endianess specified, for little endian, read starts at last ptr pos backwards
uint16_t WriteBinaryToHex(char* dest, void* source, uint16_t len, uint8_t littleEndian);

// Simple function to read an ascii string of hex chars from a buffer 
// For each hex pair, a byte is written to the out buffer
// Returns number read, earlys out on none hex char (caps not important)
uint16_t ReadHexToBinary(uint8_t* dest, const char* source, uint16_t maxLen);

#endif
