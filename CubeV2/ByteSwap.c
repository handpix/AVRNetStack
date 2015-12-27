/*
 * ByteSwap.c
 *
 * Created: 12/20/2015 2:10:51 AM
 *  Author: mhendricks
 */ 

#include "ByteSwap.h"

//! Byte swap unsigned int
uint32_t swap_uint32( uint32_t val )
{
	val = ((uint32_t)(val << 8) & 0xFF00FF00 ) | ((uint32_t)(val >> 8) & 0xFF00FF );
	return (uint32_t)(val << 16) | (uint32_t)(val >> 16);
}

//! Byte swap int
int32_t swap_int32( int32_t val )
{
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF );
	return (val << 16) | ((val >> 16) & 0xFFFF);
}