/*
* CubeV2.c
*
* Created: 12/13/2015 2:27:33 PM
* Author : mhendricks
*/

#define F_CPU 8000000
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "LinkedList.h"
#include "NetStack.h"
#include "enc28j60.h"

#define sbi(port,bit) (port) |= (1 << (bit))
#define cbi(port, bit) (port) &= ~(1 << (bit))

void led_init( void ) ;

void InitNetwork()
{
	//memset(ArpCache, 0, sizeof(ArpCache));
	cbi(DDRD, 1);	// PORTD1 is our interrupt port for PCINT25
	PCMSK3 |= (1 << PCINT25); /* Enable PCINT0 */
	PCICR |= (1 << PCIE3); /* Activate interrupt on enabled PCINT7-0 */
	sei();

	
	TCCR3B = (1<<CS22);// | (1<<CS20);
	TCNT3 = 0;
	TIMSK3 = (1<<TOIE3);
	TIFR3 = (1<<TOV3);
	enc28j60Init(&macaddr[0]);
}

volatile uint32_t netTick = 0;

ISR (TIMER3_OVF_vect){
	netTick++;
	LogInfo(FacilityUser, PSTR("Network Timers (Tick: %lu)"), netTick);

	PORTA ^= 1;

	node *list = &ARPCache;
	while(list->next != NULL)
	{
		LogDebug(FacilityUser, PSTR("Reaping ARP Cache"));
		ArpCacheEntry *ace = (ArpCacheEntry *)list->next->data;
		if(ace != NULL)
		{
			LogDebug(FacilityUser, PSTR("%02X%02X%02X%02X%02X%02X : %i.%i.%i.%i : %i : %is"), ace->PhysicalAddress[0],ace->PhysicalAddress[1],ace->PhysicalAddress[2],ace->PhysicalAddress[3],ace->PhysicalAddress[4],ace->PhysicalAddress[5],ace->ProtocolAddress[0],ace->ProtocolAddress[1],ace->ProtocolAddress[2],ace->ProtocolAddress[3], ace->Resolved, ace->Age);

			ace->Age+=2;
			if(ace->Age>=120)
			{
				free(ace);
				list = deleteNode(&ARPCache, list);
				LogDebug(FacilityUser, PSTR("Deleted Old ARP Entry"));
				if(list==NULL)
				{
					break;
				}
			}
		}
		list = list -> next;
	}

	if(netTick%10==0)
	{
		LogInfo(FacilityUser, PSTR("Frames RX:%lu (%lu b) TX:%lu (%lu b)\n ARP RX:%lu (%lu b)\n IP RX:%lu (%lu b)\n ICMP RX:%lu (%lu b)"),
		RXPkts,RXOctets,
		TXPkts, TXOctets,
		RXARP, RXARPOctets,
		RXIP, RXIPOctets,
		RXICMP, RXICMPOctets);
	}
}

int main (void)
{
	// Shutdown some perripherials we dont need
	PRR0 = (1<<PRTWI) | (1<<PRADC);

	led_init();

	_delay_ms(100);
	InitNetwork();

	_delay_ms(1000);

	uint8_t linkup = 0;
	while(!linkup)
	{
		linkup = enc28j60linkup();
		if(!linkup)
		{
			_delay_ms(1000);
		}
	}
	//sbi(PORTA, 2);
	uint8_t dstIp[4] = {192,168,43,150};
	TransmissionControlBlock *clientSocket = TCPConnect(&dstIp[0], 80);

	TCPSend(clientSocket, "GET / HTTP/1.1\r\nHOST: 127.0.0.1\r\n\r\n", strlen("GET / HTTP/1.1\r\nHOST: 127.0.0.1\r\n\r\n"));

	while(1)
	{
	}
}

void led_init( void )
{
	PORTA = 0x00 ; // All outputs to 0.
	DDRA = 0xff ; // All outputs.
}

