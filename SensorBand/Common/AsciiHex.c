// Karim Ladha 2014
// Utils for converting between binary and ascii hex
#ifndef _ASCII_HEX_H_
#define _ASCII_HEX_H_
// Include
#include <stdint.h>

uint16_t WriteBinaryToHex(char* dest, void* source, uint16_t len, uint8_t littleEndian)
{
	uint16_t ret = (len*2);
	uint8_t* ptr = source;

	if(littleEndian) ptr += len-1; // Start at MSB

	char temp;
	for(;len>0;len--)
	{
		temp = '0' + (*ptr >> 4);
		if(temp>'9')temp += ('A' - '9' - 1);  
		*dest++ = temp;
		temp = '0' + (*ptr & 0xf);
		if(temp>'9')temp += ('A' - '9' - 1); 
		*dest++ = temp;

		if(littleEndian)ptr--;
		else ptr++;
	}
	*dest = '\0';
	return ret;
}

uint16_t ReadHexToBinary(uint8_t* dest, const char* source, uint16_t maxLen)
{
	uint16_t read = 0;

	char hex1, hex2;
	for(;maxLen>0;maxLen-=2)
	{
		// First char
		if		(*source >= '0' && *source <= '9') hex1 = *source - '0';
		else if	(*source >= 'a' && *source <= 'f') hex1 = *source - 'a' + 0x0A;
		else if	(*source >= 'A' && *source <= 'F') hex1 = *source - 'A' + 0x0A;
		else break;

		source++;

		// Second char
		if		(*source >= '0' && *source <= '9') hex2 = *source - '0';
		else if	(*source >= 'a' && *source <= 'f') hex2 = *source - 'a' + 0x0A;
		else if	(*source >= 'A' && *source <= 'F') hex2 = *source - 'A' + 0x0A;
		else break;

		source++;

		// Regardless of endianess, pairs are assembled LSB on right
		*dest = (uint8_t)hex2 | (hex1<<4);	// hex1 is the msb

		// Increment count and dest
		read++;
		dest++;
	}

	return read;
}

#endif
