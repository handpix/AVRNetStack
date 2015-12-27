/*
* ByteSwap.h
*
* Created: 12/20/2015 2:11:14 AM
*  Author: mhendricks
*/

#include <stdint-gcc.h>

#ifndef BYTESWAP_H_
#define BYTESWAP_H_


//! Byte swap unsigned int
uint32_t swap_uint32( uint32_t val );
//! Byte swap int
int32_t swap_int32( int32_t val );


#endif /* BYTESWAP_H_ */