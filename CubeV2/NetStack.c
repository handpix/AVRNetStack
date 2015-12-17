/*
* NetStack.c
*
* Created: 12/13/2015 6:36:00 PM
*  Author: mhendricks
*/
#include <avr/io.h>
#include "enc28j60.h"
#include "NetStack.h"

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
	ipPkt->Protocol = 6;
	
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
		//case 6:	// TCP
		//ProcessPacket_TCP(len, eth, ip, (PktTCP*)&ip->data[0]);
		//break;
	}
}

#pragma endregion IP

uint16_t checksumTCP(uint8_t *buf, uint16_t len, uint8_t *pseudoHeader)
{
	uint32_t sum = 0;

	uint8_t psl = 12;
	while(psl >1){
		sum += 0xFFFF & (((uint32_t)*pseudoHeader<<8)|*(pseudoHeader+1));
		pseudoHeader+=2;
		psl-=2;
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

uint16_t checksum(uint8_t *buf, uint16_t len)
{
	uint32_t sum = 0;

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


#define waitspi() while(!(SPSR&(1<<SPIF)))