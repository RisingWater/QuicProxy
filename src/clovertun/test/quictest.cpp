#include "stdafx.h"
#include "QUICServer.h"
#include "QUICClient.h"
#include "enet/enet.h"
#include "packetunit.h"

#define DATA_SIZE (64 * 1024)

extern HANDLE g_StopEvent;
extern DWORD  g_RecvBytes;
extern CRITICAL_SECTION g_RecvByetLock;

static BOOL QUICServerRecvPacketProcess(PBYTE Data, DWORD Length, IQUICCommunication* quic, CBaseObject* Param)
{
    EnterCriticalSection(&g_RecvByetLock);
    g_RecvBytes += Length;
    LeaveCriticalSection(&g_RecvByetLock);

    return TRUE;
}

static VOID QUICDisconnectedProcess(IQUICCommunication* quic, CBaseObject* Param)
{
    if (g_StopEvent)
    {
        SetEvent(g_StopEvent);
    }
}

static void QUICClientConnectedProcess(CQUICServer* s, CBaseObject* pParam)
{
    if (s)
    {
        s->RegisterRecvProcess(QUICServerRecvPacketProcess, NULL);
        s->RegisterEndProcess(QUICDisconnectedProcess, NULL);
    }

    return;
}

void QUICServerTest(int port)
{
    CQUICService* QUICService = NULL;

    QUICService = new CQUICService(port, "xredtest", "C:\\code\\networktest\\cert\\ca.crt", "C:\\code\\networktest\\cert\\ca.key");
    if (QUICService)
    {
        QUICService->RegisterConnectedProcess(QUICClientConnectedProcess, NULL);
        if (QUICService->Init())
        {
            WaitForSingleObject(g_StopEvent, INFINITE);

            QUICService->RegisterConnectedProcess(NULL, NULL);
            QUICService->Done();
        }
        QUICService->Release();
    }
}

void QUICClientTest(char* address, int port)
{
    CQUICClient* QUICClient = NULL;

    QUICClient = new CQUICClient(address, port, "xredtest");
    if (QUICClient)
    {
        QUICClient->RegisterEndProcess(QUICDisconnectedProcess, NULL);
        if (QUICClient->Init())
        {
            while (TRUE)
            {
                HANDLE WaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                HANDLE h[2];
                h[0] = WaitEvent;
                h[1] = g_StopEvent;
 
                BYTE* Packet = (BYTE*)malloc(DATA_SIZE);

                QUICClient->SendPacket(Packet, DATA_SIZE, WaitEvent);

                DWORD Ret = WaitForMultipleObjects(2, h, FALSE, INFINITE);

                CloseHandle(WaitEvent);

                free(Packet);

                if (Ret != WAIT_OBJECT_0)
                {
                    break;
                }
                else
                {
                    continue;
                }

            }
            
            QUICClient->Done();
        }

        QUICClient->Release();
    }
}