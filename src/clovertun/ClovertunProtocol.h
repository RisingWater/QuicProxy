#pragma once

#ifndef __P2P_PROTOCOL_H__
#define __P2P_PROTOCOL_H__

#ifdef WIN32
#include <WinSock2.h>
#include <windows.h>
#else
#include <winpr/wtypes.h>
#endif

#define UDP_PORT_SERVER_BASE 29000
#define UDP_PORT_CLIENT_BASE 30000

#define KEYWORD_SIZE 32
#define NAME_SIZE 128

#define UDP_DATA_MAX 1400

#pragma pack(1)

typedef struct in_addr ADDRESS;

typedef struct
{
	ADDRESS ipaddr;
	WORD    port;
} CLIENT_INFO;

typedef struct
{
	DWORD type;
	DWORD length;
	BYTE  data[UDP_DATA_MAX];
	BYTE  checksum;
} UDPBASE_PACKET;

typedef struct
{
    UDPBASE_PACKET BasePacket;
    CLIENT_INFO    PacketInfo;
} UDP_PACKET;

#pragma pack()


#endif