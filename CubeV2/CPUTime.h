/*
* CPUTime.h
*
* Created: 12/20/2015 11:44:02 PM
*  Author: mhendricks
*/
#include <avr/io.h>
#include <stdint.h>
#include <util/atomic.h>
#ifndef CPUTIME_H_
#define CPUTIME_H_

extern volatile uint64_t netTick;

uint64_t GetTicks();

typedef struct {
	uint32_t Ticks;
	uint32_t Invokes;
} TickStats;


#endif /* CPUTIME_H_ */