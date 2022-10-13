#include "stdafx.h"
#include "UDPCommon.h"

#ifdef WIN32

BOOL UDPSocketRecvFrom(SOCKET s, BYTE* pBuffer, DWORD dwBufferSize, DWORD* RecvLength, struct sockaddr* addr, DWORD* addrLen, HANDLE hStopEvent)
{
    BOOL bRet = TRUE;
    int rc;
    int err = 0;
    DWORD RecvBytes = 0, dwRet, Flags;
    WSABUF DataBuf;
    WSAOVERLAPPED RecvOverlapped;
    SecureZeroMemory((PVOID)& RecvOverlapped, sizeof(WSAOVERLAPPED));
    RecvOverlapped.hEvent = WSACreateEvent();

    DataBuf.len = dwBufferSize;
    DataBuf.buf = (CHAR*)pBuffer;
    while (1)
    {
        HANDLE hEvents[2] = { RecvOverlapped.hEvent, hStopEvent };
        Flags = 0;
        rc = WSARecvFrom(s, &DataBuf, 1, &RecvBytes, &Flags, addr, (LPINT)addrLen, &RecvOverlapped, NULL);
        if ((rc == SOCKET_ERROR) && (WSA_IO_PENDING != (err = WSAGetLastError())))
        {
            DBG_ERROR("WSARecv failed with error: %d\r\n", err);
            bRet = FALSE;
            break;
        }

        dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
        if (dwRet == WAIT_OBJECT_0)
        {
            rc = WSAGetOverlappedResult(s, &RecvOverlapped, &RecvBytes, TRUE, &Flags);
            if (rc == FALSE)
            {
                DBG_ERROR("WSARecv operation failed with error: %d\r\n", WSAGetLastError());
                bRet = FALSE;
                break;
            }
            if (RecvBytes == 0)
            {
                // connection is closed
                bRet = FALSE;
                break;
            }
            break;
        }
        else
        {
            if (dwRet == WAIT_OBJECT_0 + 1)
            {
                DBG_TRACE("UDP get exit event \r\n");
            }

            WSASetEvent(RecvOverlapped.hEvent);
            rc = WSAGetOverlappedResult(s, &RecvOverlapped, &RecvBytes, TRUE, &Flags);
            bRet = FALSE;
            break;
        }
    }
    WSACloseEvent(RecvOverlapped.hEvent);

    *RecvLength = RecvBytes;
    
    return bRet;
}

BOOL UDPSocketSendTo(SOCKET s, BYTE* pBuffer, DWORD dwBufferSize, struct sockaddr* addr, DWORD addrLen, HANDLE hStopEvent)
{
    WSAOVERLAPPED SendOverlapped;
    WSABUF DataBuf;
    DWORD SendBytes = 0;
    int err = 0;
    int rc;
    HANDLE hEvents[2] = { 0 };

    DWORD dwRet;
    DWORD Flags = 0;
    BOOL bRet = FALSE;

    SecureZeroMemory((PVOID)& SendOverlapped, sizeof(WSAOVERLAPPED));
    SendOverlapped.hEvent = WSACreateEvent();

    hEvents[0] = SendOverlapped.hEvent;
    hEvents[1] = hStopEvent;

    if (SendOverlapped.hEvent == NULL)
    {
        printf("create wsaevent fail \r\n");
        return FALSE;
    }

    DataBuf.len = dwBufferSize;
    DataBuf.buf = (char*)pBuffer;

    do
    {
        rc = WSASendTo(s, &DataBuf, 1, &SendBytes, 0, addr, addrLen, &SendOverlapped, NULL);
        if (rc == 0)
        {
            bRet = TRUE;
        }

        if ((rc == SOCKET_ERROR) &&
            (WSA_IO_PENDING != (err = WSAGetLastError()))) {
            DBG_ERROR("WSASend failed with error: %d\r\n", err);
            break;
        }

        dwRet = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);

        if (dwRet == WAIT_OBJECT_0)
        {
            if (WSAGetOverlappedResult(s, &SendOverlapped, &SendBytes,
                TRUE, &Flags))
            {
                bRet = TRUE;
            }
        }
        else
        {
            if (dwRet == WAIT_OBJECT_0 + 1)
            {
                DBG_TRACE("get exit event \r\n");

            }
            WSASetEvent(SendOverlapped.hEvent);

            WSAGetOverlappedResult(s, &SendOverlapped, &SendBytes,
                TRUE, &Flags);
        }

    } while (FALSE);

    WSACloseEvent(SendOverlapped.hEvent);

    return bRet;
}

#else

#ifndef ANDROID
#include <execinfo.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <exception>
#include <poll.h>

#ifndef MAX
#define MAX(a,b) ((a > b) ? a : b)
#endif

#if defined(MACOS) || defined(IOS)
#define POLLRDHUP POLLHUP
#endif

#define ERR_MASK (POLLHUP | POLLERR | POLLNVAL | POLLRDHUP)

BOOL UDPSocketRecvFrom(SOCKET s, BYTE* pBuffer, DWORD dwBufferSize, struct sockaddr* addr, DWORD* addrLen, HANDLE hStopEvent)
{
    struct pollfd fds[2];

    int stopfd = 0;
    int n;
    int ret = 0;
    int dwReaded = 0;

    if (s == -1)
    {
        return FALSE;
    }

    memset(fds, 0, sizeof(struct pollfd) * 2);

    fds[0].fd = s;
    fds[0].events = POLLIN | POLLRDHUP;

    n = 1;

    if (hStopEvent)
    {
        stopfd = GetEventFileDescriptor(hStopEvent);

        fds[1].fd = stopfd;
        fds[1].events = POLLIN;

        n++;
    }

    ret = poll(fds, n, -1);

    if (ret < 0)
    {
        DBG_ERROR("should not be here !!! \r\n");
        return FALSE;
    }
    else if (ret == 0)
    {
        DBG_ERROR("recv poll timeout\r\n");
        return FALSE;
    }
    else
    {
        if (n > 1 && fds[1].revents != 0)
        {
            DBG_INFO("recv get stop event \r\n");
            return FALSE;
        }

        if ((fds[0].revents & ERR_MASK) != 0)
        {
            DBG_INFO("udp SocketRead other client close socket %d event %x \r\n", fds[0].fd, fds[0].revents);
            return FALSE;
        }

        if (fds[0].revents & POLLIN)
        {
            dwReaded = recvfrom(s, pBuffer, dwBufferSize, 0, addr, addrLen);

            if (dwReaded == 0)
            {
                DBG_INFO("udp SocketRead recv 0 bytes\r\n");
                return FALSE;
            } 
            else if (dwReaded == -1)
            {
                if (errno == EWOULDBLOCK)
                {
                    return TRUE;
                }

                DBG_INFO("udp SocketRead failed errno %d\r\n", errno);
                return FALSE;
            }
            else
            {
                return TRUE;
            }
        }

        DBG_ERROR("udp SocketRead not any event, should not be here\n");
        return FALSE;
    }
}

BOOL UDPSocketSendTo(SOCKET s, BYTE* pBuffer, DWORD dwBufferSize, struct sockaddr* addr, DWORD addrLen, HANDLE hStopEvent)
{
    struct pollfd fds[2];

    int stopfd = 0;
    int n;
    int ret = 0;
    int dwWrite = 0;

    if (s == -1)
    {
        return FALSE;
    }

    memset(fds, 0, sizeof(struct pollfd) * 2);

    fds[0].fd = s;
    fds[0].events = POLLOUT | POLLRDHUP;

    n = 1;

    if (hStopEvent)
    {
        stopfd = GetEventFileDescriptor(hStopEvent);

        fds[1].fd = stopfd;
        fds[1].events = POLLIN;

        n++;
    }
    
    ret = poll(fds, n, -1);

    if (ret < 0)
    {
        DBG_ERROR("udp send poll error %d \r\n", errno);
        return FALSE;
    }
    else if (ret == 0)
    {
        DBG_ERROR("udp send poll timeout\r\n");
        return FALSE;
    }
    else
    {
        if (n > 1 && fds[1].revents != 0)
        {
            DBG_INFO("udp send get stop event \r\n");
            return FALSE;
        }

        if ((fds[0].revents & ERR_MASK) != 0)
        {
            DBG_INFO("udp other client close socket %x\r\n", fds[0].revents);
            return FALSE;
        }

        if (fds[0].revents & POLLOUT)
        {
            signal(SIGPIPE, SIG_IGN);
            try
            {
                dwWrite = sendto(s, pBuffer, dwBufferSize, 0, addr, addrLen);

                if (dwWrite == 0)
                {
                    DBG_INFO("udp SocketWrite recv 0 bytes\r\n");
                    return FALSE;
                }
                else if (dwWrite == -1)
                {
                    if (errno == EWOULDBLOCK || errno == EINTR)
                    {
                        return TRUE;
                    }

                    DBG_INFO("udp SocketWrite failed errno %d\r\n", errno);
                    return FALSE;
                }
                else
                {
                    return TRUE;
                }
            }
            catch (std::exception& e)
            {
                DBG_INFO("udp SocketWrite exception\r\n");
                return FALSE;
            }
        }

        DBG_ERROR("udp SocketWrite not any event, should not be here\n");
        return FALSE;
    }
}
#endif
