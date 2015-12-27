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
#include <util/atomic.h>
#include "CPUTime.h"
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

	
	TCCR3B = (1<<CS20);
	TCNT3 = 0;
	TIMSK3 = (1<<TOIE3);
	TIFR3 = (1<<TOV3);
	enc28j60Init(&macaddr[0]);
}



// We use Timer3 for our multiprocessing, tracking CPU Usage and timed events
ISR (TIMER3_OVF_vect)
{
	netTick+=65536;
	if(netTick % 1122==0)
	{
		LogInfo(FacilityUser, PSTR("----------------------------------------------"));
		LogInfo(FacilityUser, PSTR("TX     [T: %lu / C: %lu / Avg: %lu])"), TXTicks.Ticks, TXTicks.Invokes, TXTicks.Ticks / TXTicks.Invokes);
		LogInfo(FacilityUser, PSTR("RX     [T: %lu / C: %lu / Avg: %lu])"), RXTicks.Ticks, RXTicks.Invokes, RXTicks.Ticks/ RXTicks.Invokes);
		LogInfo(FacilityUser, PSTR("Arp    [T: %lu / C: %lu / Avg: %lu])"), ArpTicks.Ticks, ArpTicks.Invokes, ArpTicks.Ticks/ ArpTicks.Invokes);
		LogInfo(FacilityUser, PSTR("ICMP   [T: %lu / C: %lu / Avg: %lu])"), ICMPTicks.Ticks, ICMPTicks.Invokes, ICMPTicks.Ticks/ ICMPTicks.Invokes);
		LogInfo(FacilityUser, PSTR("Chksum [T: %lu / C: %lu / Avg: %lu])"), ChksumTicks.Ticks, ChksumTicks.Invokes, ChksumTicks.Ticks / ChksumTicks.Invokes);
		LogInfo(FacilityUser, PSTR("Syslog [T: %lu / C: %lu / Avg: %lu])"), SyslogTicks.Ticks, SyslogTicks.Invokes, SyslogTicks.Ticks / SyslogTicks.Invokes);
		LogInfo(FacilityUser, PSTR("Total  [T: %lu])"), netTick);
	}

	//PORTA ^= 1;
	//
	//node *list = &ARPCache;
	//while(list->next != NULL)
	//{
	//LogDebug(FacilityUser, PSTR("Reaping ARP Cache"));
	//ArpCacheEntry *ace = (ArpCacheEntry *)list->next->data;
	//if(ace != NULL)
	//{
	//LogDebug(FacilityUser, PSTR("%02X%02X%02X%02X%02X%02X : %i.%i.%i.%i : %i : %is"), ace->PhysicalAddress[0],ace->PhysicalAddress[1],ace->PhysicalAddress[2],ace->PhysicalAddress[3],ace->PhysicalAddress[4],ace->PhysicalAddress[5],ace->ProtocolAddress[0],ace->ProtocolAddress[1],ace->ProtocolAddress[2],ace->ProtocolAddress[3], ace->Resolved, ace->Age);
	//
	//ace->Age+=2;
	//if(ace->Age>=120)
	//{
	//free(ace);
	//list = deleteNode(&ARPCache, list);
	//LogDebug(FacilityUser, PSTR("Deleted Old ARP Entry"));
	//if(list==NULL)
	//{
	//break;
	//}
	//}
	//}
	//list = list -> next;
	//}
	//
	//if(netTick%10==0)
	//{
	//LogInfo(FacilityUser, PSTR("Frames RX:%lu (%lu b) TX:%lu (%lu b)\n ARP RX:%lu (%lu b)\n IP RX:%lu (%lu b)\n ICMP RX:%lu (%lu b)"),
	//RXPkts,RXOctets,
	//TXPkts, TXOctets,
	//RXARP, RXARPOctets,
	//RXIP, RXIPOctets,
	//RXICMP, RXICMPOctets);
	//}
}



int main (void)
{
	// Shutdown some peripherals we don't need
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
	uint8_t dstIp[4] = {192,168,43,150};
	TransmissionControlBlock *clientSocket = TCPConnect(&dstIp[0], 80);

	char *str = "GET /time HTTP/1.1\r\nHOST: 127.0.0.1\r\nAccept: application/octet-stream\r\n\r\n";
	
	TCPSend(clientSocket, (uint8_t *)str, strlen(str));
	_delay_ms(5000);
	TCPClose(clientSocket);

	while(1){
	}

}

void led_init( void )
{
	PORTA = 0x00 ; // All outputs to 0.
	DDRA = 0xff ; // All outputs.
}
