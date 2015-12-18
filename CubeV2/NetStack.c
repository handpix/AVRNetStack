/*
* NetStack.c
*
* Created: 12/13/2015 6:36:00 PM
*  Author: mhendricks
*/
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "enc28j60.h"
#include "LinkedList.h"
#include "NetStack.h"

uint16_t embrionicPort = 1025;
uint8_t buf[MAXPACKET];

// Transmission Control Blocks
node TCB;

node ARPCache;

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

void SendSyslog(char *str)
{
	uint8_t logPkt[1200];
	memset(&logPkt, 0, 1200);

	PktEthernet *logFrame = (PktEthernet*)&logPkt[0];
	
	uint8_t ipbcast={255,255,255,255};
	
	EthBuildFrame(logFrame, dstmac);
	logFrame->type = EthernetTypeIP;
	PktIP *ip = IPBuildPacket(logFrame, &ipbcast, 17);
	
	PktUDP *udpPkt = (PktUDP*)&ip->data[0];
	udpPkt->SrcPort = embrionicPort++;
	udpPkt->DstPort = SWAP_2(514);
	
	strcpy(udpPkt->data, str);

	udpPkt->Length = SWAP_2(strlen(str)+8);
	udpPkt->Checksum=0;
	ip->TotalLength = SWAP_2(1192);
	ip->Checksum = checksum((uint8_t*)ip, 20, NULL);

	enc28j60PacketSend(1200, logPkt);
}

int SearchTCBByReceivedTCPPacket(TransmissionControlBlock *search, PktIP *key)
{
	if(IsIP(key->SrcIp, search->RemoteIp))
	{
		//SendSyslog("SearchTCBByReceivedTCPPacket -> Ip Match");
		PktTCP *tcp = (PktTCP*)&key->data[0];
		uint8_t log[1200];
		//sprintf(&log, "SearchTCBByReceivedTCPPacket -> SWAP_2(0x%04X) == 0x%04X && SWAP_2(0x%04X)==0x%04X", search->SrcPort, tcp->DstPort, search->DstPort, tcp->SrcPort);
		//SendSyslog(&log);
		//sprintf(&log, "SearchTCBByReceivedTCPPacket -> 0x%04X == 0x%04X && 0x%04X==0x%04X", SWAP_2(search->SrcPort), tcp->DstPort, SWAP_2(search->DstPort), tcp->SrcPort);
		
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
		while(1){};
		return;
	}

	TransmissionControlBlock *sockOpt = tcbNode->data;
	if(sockOpt==NULL)
	{
		// We have no connection in the connection table, return now
		sbi(PORTA, 1);
		while(1){};
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

	while(1){};
}

// Open a TCP connection (as a client), this sets up our socket options, and
TransmissionControlBlock *TCPConnect(uint8_t *dstIp, uint16_t dstPort)
{
	uint8_t synPkt[60];
	memset(&synPkt, 0, 60);
	TransmissionControlBlock *sockOpt = malloc(sizeof(TransmissionControlBlock));
	sockOpt->SrcPort = embrionicPort++;	// TODO: Need to add randomness
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
			sum += 0xFFFF & (((uint32_t)*pseudoHeader<<8)|*(pseudoHeader+1));
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

#define waitspi() while(!(SPSR&(1<<SPIF)))

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