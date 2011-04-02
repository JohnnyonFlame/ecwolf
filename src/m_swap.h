#ifndef __M_SWAP__
#define __M_SWAP__

#include "wl_def.h"

static inline const WORD ReadLittleShort(const BYTE * const ptr)
{
	return DWORD(BYTE(*ptr)) |
		(DWORD(BYTE(*(ptr+1)))<<8);
}

static inline const DWORD ReadLittle24(const BYTE * const ptr)
{
	return DWORD(BYTE(*ptr)) |
		(DWORD(BYTE(*(ptr+1)))<<8) |
		(DWORD(BYTE(*(ptr+2)))<<16);
}

static inline const DWORD ReadLittleLong(const BYTE * const ptr)
{
	return DWORD(BYTE(*ptr)) |
		(DWORD(BYTE(*(ptr+1)))<<8) |
		(DWORD(BYTE(*(ptr+2)))<<16) |
		(DWORD(BYTE(*(ptr+3)))<<24);
}


// Now for some writing
// Syntax: char data[x] = {WRITEINT32_DIRECT(integer),WRITEINT32_DIRECT(integer)...}
#if 0
#define WRITEINT32_DIRECT(integer) (BYTE)(integer&0xFF),(BYTE)((integer>>8)&0xFF),(BYTE)((integer>>16)&0xFF),(BYTE)((integer>>24)&0xFF)
#define WRITEINT16_DIRECT(integer) (BYTE)(integer&0xFF),(BYTE)((integer>>8)&0xFF)
#define WRITEINT8_DIRECT(integer) (BYTE)(integer&0xFF)

#define WRITEINT32(pointer, integer) *pointer = (BYTE)(integer&0xFF);*(pointer+1) = (BYTE)((integer>>8)&0xFF);*(pointer+2) = (BYTE)((integer>>16)&0xFF);*(pointer+3) = (BYTE)((integer>>24)&0xFF);
#define WRITEINT24(pointer, integer) *pointer = (BYTE)(integer&0xFF);*(pointer+1) = (BYTE)((integer>>8)&0xFF);*(pointer+2) = (BYTE)((integer>>16)&0xFF);
#define WRITEINT16(pointer, integer) *pointer = (BYTE)(integer&0xFF);*(pointer+1) = (BYTE)((integer>>8)&0xFF);
#define WRITEINT8(pointer, integer) *pointer = (BYTE)(integer&0xFF);
#endif

// After the fact Byte Swapping ------------------------------------------------

static inline const WORD SwapShort(WORD x)
{
	return ((x&0xFF)<<8) | ((x>>8)&0xFF);
}

static inline const WORD SwapLong(DWORD x)
{
	return ((x&0xFF)<<24) |
		(((x>>8)&0xFF)<<16) |
		(((x>>16)&0xFF)<<8) |
		((x>>24)&0xFF);
}

// TODO: Actually change these for Little/Big endian
#define BigShort SwapShort
#define BigLong SwapLong
#define LittleShort(x) (x)
#define LittleLong(x) (x)

#endif /* __M_SWAP__ */
