#include "stdafx.h"
#include "corelib.h"
#include "ENetClient.h"

CENetClient::CENetClient(DWORD PeerId, CLIENT_INFO info, BOOL IsHost)
{
    m_bIsHost = IsHost;
    m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    m_hMainThread = NULL;

    m_pfnRecvFunc = NULL;
    m_pRecvParam = NULL;

    m_pENetHost = NULL;
    m_pENetPeer = NULL;

    m_dwPeerId = PeerId;
    memcpy(&m_stPeerInfo, &info, sizeof(CLIENT_INFO));
    
    InitializeCriticalSection(&m_csSendLock);
    InitializeCriticalSection(&m_csLock);
}

CENetClient::~CENetClient()
{
    if (m_hStopEvent)
	{
		CloseHandle(m_hStopEvent);
	}

	if (m_hMainThread)
	{
		CloseHandle(m_hMainThread);
	}

    EnterCriticalSection(&m_csSendLock);

    while (!m_SendList.empty())
    {
        ENetSendNode packet = m_SendList.front();
        m_SendList.pop_front();
        enet_packet_destroy(packet.Packet);
    }

    LeaveCriticalSection(&m_csSendLock);

    if (m_pENetHost)
    {
        enet_host_destroy(m_pENetHost);
        m_pENetHost = NULL;
    }

    DeleteCriticalSection(&m_csSendLock);
    DeleteCriticalSection(&m_csLock);
}

BOOL CENetClient::Init()
{
    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = m_stPeerInfo.port;

    if (!m_bIsHost)
    {
        address.port++;
    }

    address.port = htons(address.port);

    DBG_INFO(_T("create host in port %d\r\n"), ntohs(address.port));
    m_pENetHost = enet_host_create(&address, 255, 0, 0, 0);
    if (m_pENetHost == NULL)
    {
        DBG_ERROR(_T("An error occurred while trying to create an ENet server host.\r\n"));
        return FALSE;
    }

    if (!m_bIsHost)
    {
        ENetAddress hostAddress;
        ENetEvent event;

        enet_address_set_host_ip(&hostAddress, inet_ntoa(m_stPeerInfo.ipaddr));
        hostAddress.port = htons(m_stPeerInfo.port);

#ifdef UNICODE
        DBG_INFO(_T("connect to udp socket %S:%d\r\n"), inet_ntoa(m_stPeerInfo.ipaddr), ntohs(hostAddress.port));
#else
        DBG_INFO(_T("connect to udp socket %s:%d\r\n"), inet_ntoa(m_stPeerInfo.ipaddr), ntohs(hostAddress.port));
#endif

        for (int i = 0; i < 10; i++)
        {
            m_pENetPeer = enet_host_connect(m_pENetHost, &hostAddress, 16, 0);
            if (m_pENetPeer == NULL)
            {
                DBG_ERROR(_T("No available peers for initiating an ENet connection.\r\n"));
                return FALSE;
            }

            int ret = enet_host_service(m_pENetHost, &event, 1000);

            if (ret > 0 &&
                event.type == ENET_EVENT_TYPE_CONNECT)
            {
                DBG_INFO(_T("Connection to Host succeeded.\r\n"));
                break;
            }
            else
            {
                enet_peer_reset(m_pENetPeer);
                m_pENetPeer = NULL;
                DBG_ERROR(_T("Connection to Host failed.\r\n"));
            }
        }
    }

    AddRef();
	m_hMainThread = CreateThread(NULL, 0, CENetClient::MainProc, this, 0, NULL);

    return TRUE;
}

VOID CENetClient::Done()
{
    RegisterRecvProcess(NULL, NULL);
    SetEvent(m_hStopEvent);
}

VOID CENetClient::SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle)
{
    if (WaitForSingleObject(m_hStopEvent, 0) == WAIT_TIMEOUT)
    {
        ENetPacket* packet = enet_packet_create(Data, Length, ENET_PACKET_FLAG_RELIABLE);

        ENetSendNode Node;
        Node.Packet = packet;
        Node.SyncHandle = SyncHandle;

        EnterCriticalSection(&m_csSendLock);
        m_SendList.push_back(Node);
        LeaveCriticalSection(&m_csSendLock);
    }
}

VOID CENetClient::RegisterRecvProcess(_ENetRecvPacketProcess Process, CBaseObject* Param)
{
    CBaseObject* pOldParam = NULL;

    EnterCriticalSection(&m_csLock);
	m_pfnRecvFunc = Process;

    pOldParam = m_pRecvParam;
    m_pRecvParam = Param;
    if (m_pRecvParam)
    {
        m_pRecvParam->AddRef();
    }
    LeaveCriticalSection(&m_csLock);

    if (pOldParam)
    {
        pOldParam->Release();
    }
}

DWORD WINAPI CENetClient::MainProc(void* pParam)
{
    CENetClient* tcp = (CENetClient*)pParam;
    
	while (TRUE)
	{
		if (!tcp->ENetProcess(tcp->m_hStopEvent))
		{
			break;
		}
	}
    
    DBG_INFO(_T("CENetClient: Main Thread Stop\r\n"));

	tcp->Release();

	return 0;
}
    
BOOL CENetClient::ENetProcess(HANDLE StopEvent)
{
    UNREFERENCED_PARAMETER(StopEvent);
    
    if (WaitForSingleObject(StopEvent, 0) != WAIT_TIMEOUT)
    {
        return FALSE;
    }

    ENetEvent event;
    int ret = enet_host_service(m_pENetHost, &event, 20);
    
    if (ret > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                char tmp[64] = { 0 };
#ifdef UNICODE
                DBG_INFO(_T("guest connected host %S:%d\r\n"), inet_ntop(AF_INET, &event.peer->address.host, tmp, 64), ntohs(event.peer->address.port));
#else
                DBG_INFO(_T("guest connected host %s:%d\r\n"), inet_ntop(AF_INET, &event.peer->address.host, tmp, 64), ntohs(event.peer->address.port));
#endif

                event.peer->data = (void*)m_dwPeerId;
                m_pENetPeer = event.peer;
                
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE:
            {
                if ((DWORD)event.peer->data == m_dwPeerId)
                {
                    EnterCriticalSection(&m_csLock);
                    if (m_pfnRecvFunc)
                    {
                        m_pfnRecvFunc(event.packet->data, event.packet->dataLength, this, m_pRecvParam);
                    }
                    LeaveCriticalSection(&m_csLock);
                }

                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
            {
                if (event.peer->data != NULL)
                {
                    m_pENetPeer = NULL;
                    DBG_INFO(_T("disconnected.\n"));
                    return FALSE;
                }
                break;
            }
            default:
            {
                DBG_ERROR(_T("unknow event %d\r\n"), event.type);
                break;
            }
        }
    }
    else if (ret < 0)
    {
        DBG_ERROR(_T("enet_host_service failed %d\r\n"), ret);
        return FALSE;
    }
        
    if (m_pENetPeer != NULL)
    {
        EnterCriticalSection(&m_csSendLock);

        while (!m_SendList.empty())
        {
            ENetSendNode node = m_SendList.front();
            ENetPacket* packet = node.Packet;
            m_SendList.pop_front();
            enet_peer_send(m_pENetPeer, 0, packet);

            if (node.SyncHandle)
            {
                SetEvent(node.SyncHandle);
            }
        }

        LeaveCriticalSection(&m_csSendLock);
    }

    return TRUE;
}
