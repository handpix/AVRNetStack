/*
* NetStack.c
*
* Created: 12/13/2015 6:36:00 PM
*  Author: mhendricks
*/
#include <stdarg.h>
#include <avr/io.h>
#include <avr/interrupt.h>#include <avr/pgmspace.h>
#include <stdlib.h>
#include "enc28j60.h"
#include "LinkedList.h"
#include "NetStack.h"

uint8_t ipbcast[] = {255,255,255,255};
uint8_t macaddr[] = {0x74,0xD3,0xDB,0x0A,0xD7,0x00};
uint8_t bcast[]   = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t ipaddr[]  = {192,168,43,100};
uint8_t dstmac[]  = {0x00,0x24,0x9B,0x08,0x4E,0xD3};

uint32_t RXEthernet = 0;
uint32_t RXIP = 0;
uint32_t RXICMP = 0;
uint32_t RXARP = 0;

uint32_t RXEthernetOctets = 0;
uint32_t RXIPOctets = 0;
uint32_t RXICMPOctets = 0;
uint32_t RXARPOctets = 0;

uint16_t embrionicPort = 1025;
uint8_t buf[MAXPACKET];
node TCB;							// Transmission Control Blocks
node ARPCache;						// Our ARP Cache

#pragma region Ethernet
void EthBuildFrame(PktEthernet *eth, uint8_t *dst)
{
	copyMac(macaddr, eth->SrcMacAddress);
	copyMac(dst, eth->DstMacAddress);
}
void CopyEthernetSrcToDst(PktEthernet *eth)
{
	copyMac(eth->SrcMacAddress, eth->DstMacAddress);	// Reply to the source of our packet
	copyMac(macaddr, eth->SrcMacAddress);				// our source address
}

#pragma endregion Ethernet

#pragma region ARP
void ProcessPacket_ARP(uint16_t len, PktEthernet *eth, PktArp *arp)
{
	RXARP++;
	RXARPOctets+=len;
	// Ethernet -> IP, we do not handle other protocols
	if(arp->HardwareType != 0x0100 || arp->ProtocolType!=0x0008)
	{
		return;
	}

	// This is a request, process it and see if it is for us
	if(arp->Opcode == 0x0100)
	{
		// We need to process only if they are asking for our IP
		if(IsIP(arp->TargetIpAddress, ipaddr))
		{
			AddARPCache(&arp->SenderMacAddress, &arp->SenderIpAddress);
			CopyEthernetSrcToDst(eth);
			arp->ProtocolType = 0x0008;
			arp->Opcode=0x0200;

			copyIP(arp->SenderIpAddress, arp->TargetIpAddress);
			copyMac(arp->SenderMacAddress, arp->TargetMacAddress);

			copyIP(ipaddr, arp->SenderIpAddress);
			copyMac(macaddr, arp->SenderMacAddress);

			enc28j60PacketSend(len, (uint8_t *)eth);

			return;
		}
	}
}

int SearchARPByMAC(ArpCacheEntry *search, uint8_t *key)
{
	if(IsMAC(search->PhysicalAddress, key))
	{
		return 1;
	}
	return 0;
}

void AddARPCache(uint8_t *hw, uint8_t *proto)
{
	node *n = find(&ARPCache, hw, SearchARPByMAC);
	if(n==NULL)
	{
		ArpCacheEntry *ace = malloc(sizeof(ArpCacheEntry));
		if(ace==NULL)
		{
			LogCritical(FacilityUser, "malloc failed !arp entry");
			return;
		}
		copyMac(hw, ace->PhysicalAddress);
		copyIP(proto, ace->ProtocolAddress);
		ace->Resolved=1;
		ace->Age=0;
		insert(&ARPCache, ace);
	}
	else{
		ArpCacheEntry *ace = n->data;
		ace->Resolved=1;
		ace->Age=0;
	}
}
#pragma endregion ARP

#pragma region IP

PktIP *IPBuildPacket(PktEthernet *eth, uint8_t *dstIp, uint8_t protocol)
{
	PktIP *ipPkt = (PktIP*)&eth->data[0];
	
	copyIP(ipaddr, ipPkt->SrcIp);
	copyIP(dstIp, ipPkt->DstIp);
	ipPkt->Protocol = protocol;
	ipPkt->FragmentOffset=0;
	ipPkt->Identification=0;		// TODO: This should be randomized
	ipPkt->TTL = 128;
	ipPkt->TOS = 0;
	ipPkt->Version = 4;
	ipPkt->HeaderLength = 5;
	ipPkt->Protocol = protocol;
	
	return ipPkt;
}

void ProcessPacket_IP(uint16_t len, PktEthernet *eth, PktIP *ip)
{
	RXIP++;
	RXIPOctets+=len;
	// We need to determine if this is for us
	if(!IsIP(ipaddr, ip->DstIp))
	{
		return;	// Nope, Outta here
	}

	switch(ip->Protocol)
	{
		case 1:	// ICMP
		ProcessPacket_ICMP(len, eth, ip, (PktICMP*)&ip->data[0]);
		break;
		case 6:	// TCP
		ProcessPacket_TCP(len, eth, ip, (PktTCP*)&ip->data[0]);
		break;
	}
}

#pragma endregion IP

#pragma region ICMP

void ProcessPacket_ICMP(uint16_t len, PktEthernet *eth, PktIP *ip, PktICMP *icmp)
{
	RXICMP++;
	RXICMPOctets+=len;
	if(icmp->Type==8)	// Echo Request
	{
		CopyEthernetSrcToDst(eth);

		copyIP(ip->SrcIp, ip->DstIp);
		copyIP(ipaddr, ip->SrcIp);
		ip->Checksum = 0; // Checksum is computed with a 0 checksum
		ip->Checksum = checksum((uint8_t*)ip, 20, NULL);

		// We have an echo request. We change src -> dst in ICMP, IP and ETH frames, and reply
		icmp->Type = 0;		// Echo reply
		icmp->Code = 0;		// Echo reply
		icmp->Checksum = 0;	// Checksum is computed with a 0 checksum
		icmp->Checksum = checksum((uint8_t*)icmp, SWAP_2(ip->TotalLength) - 20, NULL);
		
		enc28j60PacketSend(len, (uint8_t *)eth);
	}
}

#pragma endregion ICMP

#pragma region TCP

void SendSyslog(uint8_t f, uint8_t level, const uint8_t *file, int16_t line, const uint8_t *fmt, ...)
{
	uint8_t pri = f * 8 + level;
	uint8_t logPkt[1024];
	memset(&logPkt, 0, 1024);

	PktEthernet *logFrame = (PktEthernet*)&logPkt[0];

	uint8_t syslogaddr[]  = {192,168,43,150};

	EthBuildFrame(logFrame, dstmac);
	logFrame->type = EthernetTypeIP;
	PktIP *ip = IPBuildPacket(logFrame, &syslogaddr, 17);
	
	PktUDP *udpPkt = (PktUDP*)&ip->data[0];
	udpPkt->SrcPort = embrionicPort++;
	udpPkt->DstPort = SWAP_2(514);

	uint8_t *ptr = &udpPkt->data[0];
	
	ptr+= sprintf(udpPkt->data, "<%i>0 %i.%i.%i.%i %S:%i ", pri, ipaddr[0],ipaddr[1],ipaddr[2],ipaddr[3], file, line);	// TODO: This costs 2K for sprintf, look at using strcat or simular
	va_list ap;
	va_start(ap, fmt);
	ptr+= vsprintf_P(ptr, fmt, ap);	// TODO: This costs 2K for sprintf, look at using strcat or simular
	va_end(ap);

	uint16_t len = ptr-(&udpPkt->data[0]);
	
	udpPkt->Length = SWAP_2(len+8);
	udpPkt->Checksum=0;
	ip->TotalLength = SWAP_2(len+8+20);
	ip->Checksum = checksum((uint8_t*)ip, 20, NULL);

	enc28j60PacketSend(len+8+20+16, logPkt);
}

int SearchTCBByReceivedTCPPacket(TransmissionControlBlock *search, PktIP *key)
{
	if(IsIP(key->SrcIp, search->RemoteIp))
	{
		PktTCP *tcp = (PktTCP*)&key->data[0];
		
		if(SWAP_2(search->SrcPort) == tcp->DstPort && SWAP_2(search->DstPort)==tcp->SrcPort)
		{
			return 1;
		}
	}
	return 0;
}

void ProcessPacket_TCP(uint16_t len, PktEthernet *eth, PktIP *ip, PktTCP *tcp)
{
	node *tcbNode = find(&TCB, ip, SearchTCBByReceivedTCPPacket);
	if(tcbNode==NULL)
	{
		// We have no connection in the connection table, return now
		sbi(PORTA, 0);
		//while(1){};
		return;
	}


	TransmissionControlBlock *sockOpt = tcbNode->data;
	if(sockOpt==NULL)
	{
		// We have no connection in the connection table, return now
		sbi(PORTA, 1);
		return;
	}

	switch(sockOpt->FiniteState)
	{
		// We have sent a SYN to open a connection, process this packet against this.
		case CLOSED:
		sbi(PORTA, 0);

		break;
		case ACTIVEOPEN:
		
		sbi(PORTA, 0);
		break;
	}
}

// Open a TCP connection (as a client), this sets up our socket options, and
TransmissionControlBlock *TCPConnect(uint8_t *dstIp, uint16_t dstPort)
{
	uint8_t synPkt[60];
	memset(&synPkt, 0, 60);
	TransmissionControlBlock *sockOpt = malloc(sizeof(TransmissionControlBlock));
	sockOpt->SrcPort = embrionicPort++;
	sockOpt->DstPort = dstPort;
	sockOpt->Seq = 0;	// TODO: Need to add randomness
	sockOpt->Ack = 0;	// TODO: Need to add randomness
	copyIP(dstIp, sockOpt->RemoteIp);
	sockOpt->FiniteState = CLOSED;

	PktEthernet *synPktFrame = (PktEthernet*)&synPkt[0];
	
	EthBuildFrame(synPktFrame, dstmac);
	synPktFrame->type = EthernetTypeIP;
	PktIP *ip = IPBuildPacket(synPktFrame, dstIp, 6);
	ip->TotalLength = SWAP_2(46);
	ip->Checksum = checksum((uint8_t*)ip, 20, NULL);
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

	tcp->Checksum = checksum((uint8_t*)tcp, 20, (uint8_t *)&tcpps);
	
	enc28j60PacketSend(54, (uint8_t *)synPktFrame);
	insert(&TCB, (void *)sockOpt);
	return sockOpt;
}

#pragma endregion TCP

#pragma region Utilities

uint16_t checksum(uint8_t *buf, uint16_t len, uint8_t *pseudoHeader)
{
	uint32_t sum = 0;

	if(pseudoHeader!=NULL)
	{
		uint8_t psl = 12;
		while(psl >1){
			sum += 0xFFFF & ((*pseudoHeader<<8)|*(pseudoHeader+1));
			pseudoHeader+=2;
			psl-=2;
		}
	}

	// build the sum of 16bit words
	while(len >1){
		sum += 0xFFFF & (((uint32_t)*buf<<8)|*(buf+1));
		buf+=2;
		len-=2;
	}

	// if there is a byte left then add it (padded with zero)
	if (len){
		sum += ((uint32_t)(0xFF & *buf))<<8;
	}

	// now calculate the sum over the bytes in the sum until the result is only 16bit long
	while (sum>>16){
		sum = (sum & 0xFFFF)+(sum >> 16);
	}

	// build 1's complement:
	return SWAP_2(( (uint16_t) sum ^ 0xFFFF));
}

#pragma endregion Utilities


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