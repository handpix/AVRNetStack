/*
* NetStack.h
*
* Created: 12/14/2015 7:23:00 PM
*  Author: mhendricks
*/

#include <string.h>
#ifndef NETSTACK_H_
#define NETSTACK_H_

extern uint8_t macaddr[] ;
extern uint8_t bcast[] ;
extern uint8_t ipaddr[] ;
extern uint8_t dstmac[] ;

#define SWAP_2(x) ( (((x) & 0xff) << 8) | ((unsigned short)(x) >> 8) )

//#define  copyMac(src, dst)			dst[0] = src[0]; dst[1] = src[1];	dst[2] = src[2]; dst[3] = src[3]; dst[4] = src[4]; dst[5] = src[5];
//#define  copyIP(src, dst)			dst[0] = src[0]; dst[1] = src[1];	dst[2] = src[2]; dst[3] = src[3];
#define  copyMac(src, dst)			memcpy(dst, src, 6);
#define  copyIP(src, dst)			memcpy(dst, src, 4);

#define  IsMAC(MAC1, MAC2)			(MAC1[0] == MAC2[0] && MAC1[1] == MAC2[1] && MAC1[2] == MAC2[2] && MAC1[3] == MAC2[3] && MAC1[4] == MAC2[4] && MAC1[5] == MAC2[5])
#define  IsIP(IP1, IP2)				(IP1[0] == IP2[0] && IP1[1] == IP2[1] && IP1[2] == IP2[2] && IP1[3] == IP2[3])

// Define our Ethernet Type fields
#define EthernetTypeARP 0x0608
#define EthernetTypeIP  0x0008

#pragma region Ethernet

typedef struct PktEthernet {
	uint8_t DstMacAddress[6];
	uint8_t SrcMacAddress[6];
	uint16_t type;
	uint8_t data[1500];
} PktEthernet;

void EthBuildFrame(PktEthernet *eth, uint8_t *dst);
void CopyEthernetSrcToDst(PktEthernet *eth);

#pragma endregion Ethernet

#pragma region ARP

typedef struct ArpCacheEntry {
	uint8_t ProtocolAddress[4];
	uint8_t PhysicalAddress[6];
} ArpCacheEntry;

typedef struct PktArp {
	uint16_t HardwareType;
	uint16_t ProtocolType;
	uint8_t HardwareSize;
	uint8_t ProtocolSize;
	uint16_t Opcode;
	uint8_t SenderMacAddress[6];
	uint8_t SenderIpAddress[4];
	uint8_t TargetMacAddress[6];
	uint8_t TargetIpAddress[4];
} PktArp;

void ProcessPacket_ARP(uint16_t len, PktEthernet *eth, PktArp *arp);

#pragma endregion ARP

#pragma region IP

typedef struct PktIP
{
	uint8_t HeaderLength:4;
	uint8_t Version:4;
	uint8_t TOS;
	uint16_t TotalLength;
	uint16_t Identification;
	uint8_t Flags:3;
	uint16_t FragmentOffset:13;
	uint8_t TTL;
	uint8_t Protocol;
	uint16_t Checksum;
	uint8_t SrcIp[4];
	uint8_t DstIp[4];
	uint8_t data[1500];
} PktIP;
PktIP *IPBuildPacket(PktEthernet *eth, uint8_t *dstIp, uint8_t protocol);

void ProcessPacket_IP(uint16_t len, PktEthernet *eth, PktIP *ip);
#pragma endregion IP


typedef struct PktICMP
{
	uint8_t Type;
	uint8_t Code;
	uint16_t Checksum;
	uint8_t data[1500];
} PktICMP;

typedef struct PktTCPPseudoHeader
{
	uint8_t SrcIp[4];
	uint8_t DstIp[4];
	uint8_t Reserved;
	uint8_t Protocol;
	uint16_t Length;
} PktTCPPseudoHeader;

typedef struct PktTCP
{
	uint16_t SrcPort;
	uint16_t DstPort;
	uint32_t Seq;
	uint32_t Ack;

	// Flags are reverse edian
	uint8_t FlagNS:1;
	uint8_t Reserved:3;
	uint8_t DataOffset:4;

	// Flags are reversed
	uint8_t FlagFIN:1;
	uint8_t FlagSYN:1;
	uint8_t FlagRST:1;
	uint8_t FlagPSH:1;
	uint8_t FlagACK:1;
	uint8_t FlagURG:1;
	uint8_t FlagECE:1;
	uint8_t FlagCWR:1;
	
	uint16_t WindowSize;
	uint16_t Checksum;
	uint16_t UrgentPointer;
	uint8_t data[1500];
} PktTCP;

enum TCPFiniteStates {
	CLOSED,
	SYNSENT,
	ESTABLISHED,
	CLOSEWAIT
};

typedef struct TransmissionControlBlock {
	uint8_t DstIp[4];
	uint16_t SrcPort;
	uint16_t DstPort;
	uint32_t Seq;
	uint32_t Ack;
	uint8_t FiniteState;
} TransmissionControlBlock;

uint16_t checksum(uint8_t *buf, uint16_t len);
uint16_t checksumTCP(uint8_t *buf, uint16_t len, uint8_t *pseudoHeader);

#endif /* NETSTACK_H_ */