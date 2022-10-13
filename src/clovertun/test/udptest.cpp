#include "stdafx.h"
#include "UDPBase.h"
#include "enet/enet.h"
#include "packetunit.h"

#define DATA_SIZE (64 * 1024)

extern HANDLE g_StopEvent;
extern DWORD  g_RecvBytes;
extern CRITICAL_SECTION g_RecvByetLock;

extern void UDPClientTest(char* address, int port);

static BOOL UDPServerRecvPacketProcess(UDP_PACKET* Packet, CUDPBase* udp, CBaseObject* Param)
{
    EnterCriticalSection(&g_RecvByetLock);
    g_RecvBytes += Packet->BasePacket.length;
    LeaveCriticalSection(&g_RecvByetLock);

    UDP_PACKET* RespPacket = (UDP_PACKET*)malloc(sizeof(UDP_PACKET));
    memcpy(&RespPacket->PacketInfo, &Packet->PacketInfo, sizeof(CLIENT_INFO));
    RespPacket->PacketInfo.port = RespPacket->PacketInfo.port + 1;

    udp->SendPacket(RespPacket);

    return TRUE;
}

void UDPServerTest(int port)
{
    CUDPBase* pUDPServer = NULL;

    pUDPServer = new CUDPBase();
    if (pUDPServer)
    {
        pUDPServer->RegisterRecvProcess(UDPServerRecvPacketProcess, NULL);
        if (pUDPServer->Init(port))
        {
            pUDPServer->Start();
            WaitForSingleObject(g_StopEvent, INFINITE);
            pUDPServer->Stop();

            pUDPServer->RegisterRecvProcess(NULL, NULL);
        }
        pUDPServer->Release();
    }
}

void UDPClientTest(char* address, int port)
{
    CUDPBase* pUDPClient = NULL;

    pUDPClient = new CUDPBase();
    if (pUDPClient)
    {
        if (pUDPClient->Init(port + 1))
        {
            pUDPClient->Start();
            while (TRUE)
            {
                HANDLE WaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                HANDLE h[2] = { 0 };
                h[0] = WaitEvent;
                h[1] = g_StopEvent;

                int len = DATA_SIZE;
                while (len > 0)
                {
                    UDP_PACKET* Packet = (UDP_PACKET*)malloc(sizeof(UDP_PACKET));
                    memset(Packet, 0, sizeof(UDP_PACKET));
                    Packet->BasePacket.type = 0;
                    Packet->BasePacket.length = len > UDP_DATA_MAX ? UDP_DATA_MAX : len;
                    Packet->PacketInfo.port = htons(port);
                    inet_pton(AF_INET, address, &Packet->PacketInfo.ipaddr);

                    len -= UDP_DATA_MAX;

                    if (len <= 0)
                    {
                        pUDPClient->SendPacket(Packet, WaitEvent);
                    }
                    else
                    {
                        pUDPClient->SendPacket(Packet);
                    }
                }

                DWORD Ret = WaitForMultipleObjects(2, h, FALSE, INFINITE);

                CloseHandle(WaitEvent);

                if (Ret == WAIT_OBJECT_0)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }

            pUDPClient->Stop();
        }

        pUDPClient->Release();
    }
}