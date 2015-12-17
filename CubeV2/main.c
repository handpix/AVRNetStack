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
void led_encode_timeslices( uint8_t a[] );

// Transmission Control Blocks
node TCB;

uint16_t embrionicPort = 1024;

#define MAXPACKET	1500

uint8_t buf[MAXPACKET];
uint8_t macaddr[] = {0x74,0xD3,0xDB,0x0A,0xD7,0x00};
uint8_t bcast[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t ipaddr[] = {192,168,43,100};
uint8_t dstmac[] = {0x00,0x24,0x9B,0x08,0x4E,0xD3};


int tcbSearch(void *data, void *key)
{
	TransmissionControlBlock *sockOpt = (TransmissionControlBlock *)data;
	TransmissionControlBlock *search = (TransmissionControlBlock *)key;

	// Find a TCB matching the DST IP, DST Port and SRC Port
	if(sockOpt->DstPort==search->DstPort && IsIP(sockOpt->DstIp, search->DstIp) && sockOpt->SrcPort == search->SrcPort)
	{
		return 1;
	}
	return 0;
}

// Open a TCP connection (as a client), this sets up our socket options, and
void TCPOpen(TransmissionControlBlock *sockOpt, uint8_t *dstIp, uint16_t dstPort)
{
	sockOpt->SrcPort = embrionicPort++;	// TODO: Need to add randomness
	sockOpt->DstPort = dstPort;
	sockOpt->Seq = 0;	// TODO: Need to add randomness
	sockOpt->Ack = 0;	// TODO: Need to add randomness
	sockOpt->FiniteState = CLOSED;

	uint8_t synPkt[100];
	PktEthernet *synPktFrame = (PktEthernet*)&synPkt[0];
	
	EthBuildFrame(synPktFrame, dstmac);
	synPktFrame->type = EthernetTypeIP;
	PktIP *ip = IPBuildPacket(synPktFrame, dstIp, 6);
	ip->TotalLength = SWAP_2(46);
	ip->Checksum = checksum((uint8_t*)ip, 20);
	PktTCP *tcp = (PktTCP*)&ip->data[0];
	
	tcp->SrcPort = SWAP_2(sockOpt->SrcPort);
	tcp->DstPort = SWAP_2(sockOpt->DstPort);
	tcp->Seq = sockOpt->Seq;
	tcp->Ack = sockOpt->Ack;
	tcp->DataOffset = 5;
	tcp->FlagSYN = 1;
	tcp->WindowSize = SWAP_2(1300);
	tcp->UrgentPointer = 0;
	
	PktTCPPseudoHeader tcpps;
	copyIP(ip->DstIp, tcpps.DstIp);
	copyIP(ip->SrcIp, tcpps.SrcIp);
	tcpps.Reserved=0;
	tcpps.Protocol = 6;
	tcpps.Length=SWAP_2(20+6);

	tcp->Checksum = checksumTCP((uint8_t*)tcp, 20, (uint8_t *)&tcpps);
	
	enc28j60PacketSend(54, (uint8_t *)synPktFrame);
	uint16_t pt = 80;
	//insert(&TCB, (void *)sockOpt);
	
	//node *t = find(&TCB, &pt, &tcbSearch);
	//if(t==NULL)
	//{
	//sbi(PORTA,2);
	//}
	//else{
	//sbi(PORTA,0);
	//delete(&TCB, &pt, &tcbSearch);
	//}
}


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
}



void ProcessPacket_ICMP(uint16_t len, PktEthernet *eth, PktIP *ip, PktICMP *icmp)
{
	if(icmp->Type==8)	// Echo Request
	{
		CopyEthernetSrcToDst(eth);

		copyIP(ip->SrcIp, ip->DstIp);
		copyIP(ipaddr, ip->SrcIp);
		ip->Checksum = 0; // Checksum is computed with a 0 checksum
		ip->Checksum = checksum((uint8_t*)ip, 20);

		// We have an echo request. We change src -> dst in ICMP, IP and ETH frames, and reply
		icmp->Type = 0;		// Echo reply
		icmp->Code = 0;		// Echo reply
		icmp->Checksum = 0;	// Checksum is computed with a 0 checksum
		icmp->Checksum = checksum((uint8_t*)icmp, SWAP_2(ip->TotalLength) - 20);
		
		enc28j60PacketSend(len, (uint8_t *)eth);
	}
}


int main (void)
{
	led_init();

	_delay_ms(100);
	InitNetwork();
	enc28j60Init(&macaddr[0]);

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
	TransmissionControlBlock clientSocket;
	uint8_t dstIp[4] = {192,168,43,150};
	TCPOpen(&clientSocket, &dstIp[0], 80);

	while(1)
	{
	}
}




void led_init( void )
{
	PORTA = 0x00 ; // All outputs to 0.
	DDRA = 0xff ; // All outputs.
}



ISR (PCINT3_vect){
	// Trigger on falling edge only
	if((PIND & 2) == 0)
	{
		uint16_t len =0;
		do
		{	// Process all pending packets
			len = enc28j60PacketReceive(MAXPACKET,&buf[0]);
			if(len>0)
			{
				PORTA |= (1<<5);
				PktEthernet *ethFrame = (PktEthernet*)&buf[0];
				// Packet should match our network address, given the filters in the enc28j60 this *SHOULD* not be necessary
				//if(IsMAC(ethFrame->DstMacAddress, bcast) || IsMAC(ethFrame->DstMacAddress, macaddr))
				switch(ethFrame->type)
				{
					case EthernetTypeARP:
					ProcessPacket_ARP(len, ethFrame, (PktArp*)&ethFrame->data[0]);
					break;
					case EthernetTypeIP:
					ProcessPacket_IP(len, ethFrame, (PktIP*)&ethFrame->data[0]);
					break;
				}
				PORTA &= ~(1<<5);
			}
		} while(len>0);
	}
}
