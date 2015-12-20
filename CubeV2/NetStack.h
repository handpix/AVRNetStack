/*
* NetStack.h
*
* Created: 12/14/2015 7:23:00 PM
*  Author: mhendricks
*/

#include "LinkedList.h"
#include <stdio.h>
#include <string.h>
#include <avr/pgmspace.h>

#ifndef NETSTACK_H_
#define NETSTACK_H_

// These are settings that the user can change
#define LoggingLevel	LevelDebug

#define MAXPACKET	1500

extern uint32_t RXEthernet;
extern uint32_t RXIP;
extern uint32_t RXICMP;
extern uint32_t RXARP;

extern uint32_t RXEthernetOctets;
extern uint32_t RXIPOctets;
extern uint32_t RXICMPOctets;
extern uint32_t RXARPOctets;

extern uint16_t embrionicPort;
extern uint8_t buf[];

extern node ARPCache;

extern uint8_t macaddr[];
extern uint8_t bcast[];
extern uint8_t ipaddr[];
extern uint8_t dstmac[];
extern uint8_t ipbcast[];


#define SWAP_2(x) ( (((x) & 0xff) << 8) | ((unsigned short)(x) >> 8) )

//#define  copyMac(src, dst)			dst[0] = src[0]; dst[1] = src[1];	dst[2] = src[2]; dst[3] = src[3]; dst[4] = src[4]; dst[5] = src[5];
//#define  copyIP(src, dst)			dst[0] = src[0]; dst[1] = src[1];	dst[2] = src[2]; dst[3] = src[3];
#define  copyMac(src, dst)			memcpy(dst, src, 6);
#define  copyIP(src, dst)			memcpy(dst, src, 4);

#define  IsMAC(MAC1, MAC2)			(MAC1[0] == MAC2[0] && MAC1[1] == MAC2[1] && MAC1[2] == MAC2[2] && MAC1[3] == MAC2[3] && MAC1[4] == MAC2[4] && MAC1[5] == MAC2[5])
#define  IsIP(IP1, IP2)				(IP1[0] == IP2[0] && IP1[1] == IP2[1] && IP1[2] == IP2[2] && IP1[3] == IP2[3])

#define sbi(port,bit) (port) |= (1 << (bit))
#define cbi(port, bit) (port) &= ~(1 << (bit))

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
	uint8_t Resolved:1;
	uint8_t Age:7;
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
void AddARPCache(uint8_t *hw, uint8_t *proto);

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

#pragma region ICMP

typedef struct PktICMP
{
	uint8_t Type;
	uint8_t Code;
	uint16_t Checksum;
	uint8_t data[1500];
} PktICMP;

void ProcessPacket_ICMP(uint16_t len, PktEthernet *eth, PktIP *ip, PktICMP *icmp);

#pragma endregion ICMP

typedef struct PktUDP
{
	uint16_t SrcPort;
	uint16_t DstPort;
	uint16_t Length;
	uint16_t Checksum;
	uint8_t data[1500];
} PktUDP;


#pragma region TCP

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
	ACTIVEOPEN,
	ESTABLISHED,
	CLOSEWAIT
};

typedef struct TransmissionControlBlock {
	uint8_t RemoteIp[4];
	uint16_t SrcPort;
	uint16_t DstPort;
	uint32_t RxSeq;
	uint32_t RxAck;
	uint32_t TxSeq;
	uint32_t TxAck;
	uint8_t FiniteState;
} TransmissionControlBlock;

void ProcessPacket_TCP(uint16_t len, PktEthernet *eth, PktIP *ip, PktTCP *tcp);
TransmissionControlBlock *TCPConnect(uint8_t *dstIp, uint16_t dstPort);

#pragma endregion TCP

#pragma region Utilities

uint16_t checksum(uint8_t *buf, uint16_t len, uint8_t *pseudoHeader);
void SendSyslog(uint8_t f, uint8_t level, const uint8_t *file, int16_t line, const uint8_t *str, ...);

#pragma endregion Utilities

#define		FacilityKernel				0
#define		FacilityUser				1
#define		FacilityMail				2
#define		FacilitySystemDaemonsMail	3
#define		FacilitySecurity			4
#define		FacilitySyslogd				5
#define		FacilityLinePrinter			6
#define		FacilityNetworkNews			7

//7             network news subsystem
//8             UUCP subsystem
//9             clock daemon
//10            security/authorization messages
//11            FTP daemon
//12            NTP subsystem
//13            log audit
//14            log alert
//15            clock daemon
//16            local use 0  (local0)
//17            local use 1  (local1)
//18            local use 2  (local2)
//19            local use 3  (local3)
//20            local use 4  (local4)
//21            local use 5  (local5)
//22            local use 6  (local6)
//23            local use 7  (local7)

#define		LevelNone				-1
#define		LevelEmergency			0
#define		LevelAlert				1
#define		LevelCritical			2
#define		LevelError				3
#define		LevelWarning			4
#define		LevelNotice				5
#define		LevelInformational		6
#define		LevelDebug				7

#define LogNotice(f, ...) SendSyslog(f, LevelNotice, PSTR(__FILE__), __LINE__ ,__VA_ARGS__)
#define LogWarning(f, ...) SendSyslog(f, LevelWarning, PSTR(__FILE__), __LINE__ ,__VA_ARGS__)
#define LogError(f, ...) SendSyslog(f, LevelError, PSTR(__FILE__), __LINE__ ,__VA_ARGS__)
#define LogCritical(f, ...) SendSyslog(f, LevelCritical, PSTR(__FILE__), __LINE__ ,__VA_ARGS__)

#if LoggingLevel >= LevelEmergency
#define LogEmergency(f, ...) SendSyslog(f, LevelEmergency, PSTR(__FILE__), __LINE__ ,__VA_ARGS__)
#else
#define LogEmergency(f, ...)
#endif

#if LoggingLevel >= LevelAlert
#define LogAlert(f, ...) SendSyslog(f, LevelAlert, PSTR(__FILE__), __LINE__ ,__VA_ARGS__)
#else
#define LogAlert(f, ...)
#endif

#if LoggingLevel >= LevelInformational
#define LogInfo(f, ...) SendSyslog(f, LevelInformational, PSTR(__FILE__), __LINE__ ,__VA_ARGS__)
#else
#define LogInfo(f, ...)
#endif

#if LoggingLevel >= LevelDebug
#define LogDebug(f, ...) SendSyslog(f, LevelDebug, PSTR(__FILE__), __LINE__, __VA_ARGS__)
#else
#define LogDebug(f, ...)
#endif


#endif /* NETSTACK_H_ */