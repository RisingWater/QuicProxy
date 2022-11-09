#include "stdafx.h"
#include "TCPServer.h"
#include "TCPClient.h"
#include "packetunit.h"

#define DATA_SIZE (64 * 1024)

extern HANDLE g_StopEvent;
extern DWORD  g_RecvBytes;
extern CRITICAL_SECTION g_RecvByetLock;

static BOOL TCPServerRecvPacketProcess(BASE_PACKET_T* Packet, CTCPBase* tcp, CBaseObject* Param)
{
    EnterCriticalSection(&g_RecvByetLock);
    g_RecvBytes += Packet->Length;
    LeaveCriticalSection(&g_RecvByetLock);

    return TRUE;
}

static VOID TCPServerEndProcess(CTCPBase* tcp, CBaseObject* Param)
{
    if (g_StopEvent)
    {
        SetEvent(g_StopEvent);
    }

    tcp->Release();
}

static VOID TCPClientEndProcess(CTCPBase* tcp, CBaseObject* Param)
{
    if (g_StopEvent)
    {
        SetEvent(g_StopEvent);
    }
}

static BOOL TCPSocketAcceptProcess(SOCKET s, CBaseObject* pParam)
{
    CTCPService* pService = dynamic_cast<CTCPService*>(pParam);

    if (pService)
    {
        CTCPServer* pServer = new CTCPServer(s, pService->GetPort());

        pServer->RegisterRecvProcess(TCPServerRecvPacketProcess, NULL);
        pServer->RegisterEndProcess(TCPServerEndProcess, NULL);

        if (pServer->Init())
        {
            DBG_INFO(_T("Client connected\r\n"));
        }
    }

    return TRUE;
}

void TCPServerTest(int port)
{
    CTCPService* TCPService = NULL;

    TCPService = new CTCPService(port);
    if (TCPService)
    {
        TCPService->RegisterSocketAcceptProcess(TCPSocketAcceptProcess, TCPService);
        if (TCPService->Init())
        {
            WaitForSingleObject(g_StopEvent, INFINITE);

            TCPService->RegisterSocketAcceptProcess(NULL, NULL);
            TCPService->Done();
        }
        TCPService->Release();
    }
}

void TCPClientTest(char* address, int port)
{
    CTCPClient* TCPClient = NULL;

    TCPClient = new CTCPClient(address, port);
    if (TCPClient)
    {
        TCPClient->RegisterEndProcess(TCPClientEndProcess, NULL);
        if (TCPClient->Init())
        {
            while (TRUE)
            {
                if (TCPClient->GetSendBuffSize() > (100 * 1024 * 1024))
                {
                    continue;
                }
                else
                {
                    BASE_PACKET_T* Packet = (BASE_PACKET_T*)malloc(DATA_SIZE);
                    memset(Packet, 0, DATA_SIZE);
                    Packet->Type = 0;
                    Packet->Length = DATA_SIZE;

                    TCPClient->SendPacket(Packet);
                }
            }
            
            TCPClient->Done();
        }

        TCPClient->Release();
    }
}