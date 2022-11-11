#include "stdafx.h"
#include "QUICCommLib.h"
#include "packetunit.h"

#define DATA_SIZE (64 * 1024)

#ifdef WIN32
#define CERT_PATH "C:\\code\\networkSample\\cert\\ca.crt"
#define KEY_PATH  "C:\\code\\networkSample\\cert\\ca.key"
#else
#define CERT_PATH "/home/wangxu/workdir/networkSample/cert/ca.crt"
#define KEY_PATH  "/home/wangxu/workdir/networkSample/cert/ca.key"
#endif

#define MAX_CHANNEL_COUNT 8

#pragma pack(1)

typedef struct {
    CHAR  szChannel[256];
    WORD  Prority;
} ChannelInfo;
 

typedef struct {
    DWORD        dwCount;
    ChannelInfo  stChannel[MAX_CHANNEL_COUNT];
} ChannelExchange;

#pragma pack()

extern HANDLE g_StopEvent;
extern DWORD  g_RecvBytes;
extern CRITICAL_SECTION g_RecvByetLock;

BOOL QUICServerChannelProcess(PBYTE Data, DWORD Length, IQUICChannel* quic, CBaseObject* Param)
{
#ifdef UNICODE
    DBG_INFO(_T("Channel[%S] recv %S\r\n"), quic->GetChannelName(), Data);
#else
    DBG_INFO(_T("Channel[%s] recv %s\r\n"), quic->GetChannelName(), Data);
#endif
    quic->SendPacket(Data, Length);

    return TRUE;
}

DWORD WINAPI ServerRecvProc(void* pParam)
{
    IQUICChannel* channels[MAX_CHANNEL_COUNT] = { NULL };

    IQUICServer* Server = (IQUICServer*)pParam;

    DWORD len;
    PBYTE Data = Server->RecvCtrlPacket(&len, 10000);

    if (Data == NULL)
    {
        DBG_ERROR(_T("RecvCtrlPacket1 failed\r\n"));
        return 0;
    }
    ChannelExchange* Exchange = (ChannelExchange*)Data;

    if (!Server->SendCtrlPacket((PBYTE)"ok", strlen("ok") + 1))
    {
        DBG_ERROR(_T("SendCtrlPacket1 failed\r\n"));
        return 0;
    }

    Data = Server->RecvCtrlPacket(&len, 10000);

    if (Data == NULL)
    {
        DBG_ERROR(_T("RecvCtrlPacket2 failed\r\n"));
        return 0;
    }

    if (strcmp((char*)Data, "channel ok") != 0)
    {
        DBG_ERROR(_T("RecvCtrlPacket2 failed %s\r\n"), Data);
        return 0;
    }

    for (DWORD i = 0; i < Exchange->dwCount; i++)
    {
        channels[i] = Server->WaitForChannelReady(Exchange->stChannel[i].szChannel, g_StopEvent);
        channels[i]->RegisterRecvProcess(QUICServerChannelProcess, NULL);
    }

    if (!Server->SendCtrlPacket((PBYTE)"ok", strlen("ok") + 1))
    {
        DBG_ERROR(_T("SendCtrlPacket1 failed\r\n"));
        return 0;
    }

    return 0;
}

void QUICServerTest(int port)
{
    IQUICService* QUICService = CreateIQUICService(port, "xredtest", CERT_PATH, KEY_PATH);
    
    if (!QUICService->StartListen())
    {
        DBG_ERROR(_T("StartListen Failed\r\n"));
    }

    while (TRUE)
    {
        IQUICServer* Server = QUICService->Accept(g_StopEvent);

        if (Server == NULL)
        {
            break;
        }

        CreateThread(NULL, 0, ServerRecvProc, Server, 0, NULL);
    }

    QUICService->StopListen();
    QUICService->Release();
}

BOOL QUICClientChannelProcess(PBYTE Data, DWORD Length, IQUICChannel* quic, CBaseObject* Param)
{
#ifdef UNICODE
    DBG_INFO(_T("Channel[%S] recv %S\r\n"), quic->GetChannelName(), Data);
#else
    DBG_INFO(_T("Channel[%s] recv %s\r\n"), quic->GetChannelName(), Data);
#endif
    quic->SendPacket(Data, Length);

    return TRUE;
}

void QUICClientTest(char* address, int port)
{
    IQUICClient* pClient = CreateIQUICClient(address, port, "xredtest");

    if (pClient->Connect())
    {
        ChannelExchange exchange;
        memset(&exchange, 0, sizeof(ChannelExchange));
        exchange.dwCount = 4;

        DWORD len;

        IQUICChannel* channels[MAX_CHANNEL_COUNT] = { NULL };
        for (DWORD i = 0; i < exchange.dwCount; i++)
        {
            sprintf(exchange.stChannel[i].szChannel, "channel%d", i);
            exchange.stChannel[i].Prority = (WORD)i;
        }

        if (!pClient->SendCtrlPacket((PBYTE)&exchange, sizeof(ChannelExchange)))
        {
            DBG_ERROR(_T("SendCtrlPacket1 failed\r\n"));
            return;
        }

        char* resp = (char*)pClient->RecvCtrlPacket(&len, 10000);

        if (resp == NULL)
        {
            DBG_ERROR(_T("RecvCtrlPacket1 failed\r\n"));
            return;
        }

        if (strcmp(resp, "ok") != 0)
        {
            DBG_ERROR(_T("PACKET1 failed %s\r\n"), resp);
            return;
        }

        pClient->FreeRecvedCtrlPacket((PBYTE)resp);

        for (DWORD i = 0; i < exchange.dwCount; i++)
        {
            channels[i] = pClient->CreateChannel(exchange.stChannel[i].szChannel, exchange.stChannel[i].Prority);
            channels[i]->RegisterRecvProcess(QUICClientChannelProcess, NULL);
        }

        if (!pClient->SendCtrlPacket((PBYTE)"channel ok", strlen("channel ok") + 1))
        {
            DBG_ERROR(_T("SendCtrlPacket2 failed\r\n"));
            return;
        }

        resp = (char*)pClient->RecvCtrlPacket(&len, 10000);

        if (resp == NULL)
        {
            DBG_ERROR(_T("RecvCtrlPacket2 failed\r\n"));
            return;
        }

        if (strcmp(resp, "ok") != 0)
        {
            DBG_ERROR(_T("PACKET2 failed %s\r\n"), resp);
            return;
        }

        pClient->FreeRecvedCtrlPacket((PBYTE)resp);

        for (DWORD i = 0; i < exchange.dwCount; i++)
        {
            channels[i]->SendPacket((PBYTE)"test string", strlen("test string") + 1);
        }

        WaitForSingleObject(g_StopEvent, INFINITE);

    }
    else
    {
        DBG_ERROR(_T("Connect Failed\r\n"));
    }

    pClient->Release();
}