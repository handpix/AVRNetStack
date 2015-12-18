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


uint8_t macaddr[] = {0x74,0xD3,0xDB,0x0A,0xD7,0x00};
uint8_t bcast[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t ipaddr[] = {192,168,43,100};
uint8_t dstmac[] = {0x00,0x24,0x9B,0x08,0x4E,0xD3};





//#define ARPCacheLength	16
//ArpCacheEntry	ArpCache[16];
//
//void InsertArpCacheEntry(uint8_t *protocol, uint8_t *network)
//{
//for(int x=0; x<ARPCacheLength; x++)
//{
//if(IsIP(ArpCache[x].ProtocolAddress, protocol))
//{
//return;
//}
//}
//// If we got here, we do not have this in our arp cache, so lets add it
//uint8_t zeroip[] = {0,0,0,0};
//for(int x=0; x<ARPCacheLength; x++)
//{
//if(IsIP(ArpCache[x].ProtocolAddress, zeroip))
//{
//copyIP(protocol, ArpCache[x].ProtocolAddress);
//copyIP(network, ArpCache[x].PhysicalAddress);
//return;
//}
//}
//// No more room in the inn...
//}


void InitNetwork()
{
	//memset(ArpCache, 0, sizeof(ArpCache));
	cbi(DDRD, 1);	// PORTD1 is our interrupt port for PCINT25
	PCMSK3 |= (1 << PCINT25); /* Enable PCINT0 */
	PCICR |= (1 << PCIE3); /* Activate interrupt on enabled PCINT7-0 */
	sei();

	enc28j60Init(&macaddr[0]);
}



int main (void)
{
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

	while(1)
	{
	}
}

void led_init( void )
{
	PORTA = 0x00 ; // All outputs to 0.
	DDRA = 0xff ; // All outputs.
}

