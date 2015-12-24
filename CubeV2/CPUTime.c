/*
* CPUTime.c
*
* Created: 12/20/2015 12:59:51 AM
*  Author: mhendricks
*/

#include "CPUTime.h"
#include "avr/interrupt.h"
#include "util/atomic.h"
volatile uint64_t netTick = 0;

uint64_t GetTicks()
{
	return ((netTick<<16) & 0xffffffffffff0000) + TCNT3;
}