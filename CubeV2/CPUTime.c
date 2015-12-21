/*
* CPUTime.c
*
* Created: 12/20/2015 12:59:51 AM
*  Author: mhendricks
*/

#include "CPUTime.h"
#include "avr/interrupt.h"
#include "util/atomic.h"
volatile uint32_t netTick = 0;

uint32_t GetTicks()
{
	return ((netTick<<16) & 0xffff0000) + TCNT3;
}