#include "stdafx.h"
#include "TCPCommon.h"

#ifdef WIN32

BOOL TCPSocketRead(SOCKET s, BYTE* pBuffer, DWORD dwBufferSize, DWORD* pdwReaded, HANDLE hStopEvent)
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
        rc = WSARecv(s, &DataBuf, 1, &RecvBytes, &Flags, &RecvOverlapped, NULL);
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
                DBG_TRACE("TCP get exit event \r\n");
            }

            WSASetEvent(RecvOverlapped.hEvent);
            rc = WSAGetOverlappedResult(s, &RecvOverlapped, &RecvBytes, TRUE, &Flags);
            bRet = FALSE;
            break;
        }
    }
    WSACloseEvent(RecvOverlapped.hEvent);

    *pdwReaded = RecvBytes;

    return bRet;
}

BOOL TCPSocketWrite(SOCKET s, BYTE* pBuffer, DWORD dwBufferSize, DWORD* pdwWritten, HANDLE hStopEvent)
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
        rc = WSASend(s, &DataBuf, 1,
            &SendBytes, 0, &SendOverlapped, NULL);
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
    *pdwWritten = SendBytes;

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

BOOL TCPSocketRead(SOCKET s, BYTE* pBuffer, DWORD dwBufferSize, DWORD* pdwReaded, HANDLE hStopEvent)
{
    struct pollfd fds[2];

    int stopfd = 0;
    int n;
    int ret = 0;
    int dwReaded = 0;
    *pdwReaded = 0;

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
        DBG_ERROR("tcp should not be here !!! \r\n");
        return FALSE;
    }
    else if (ret == 0)
    {
        DBG_ERROR("tcp recv poll timeout\r\n");
        return FALSE;
    }
    else
    {
        if (n > 1 && fds[1].revents != 0)
        {
            DBG_INFO("tcp recv get stop event \r\n");
            return FALSE;
        }

        if ((fds[0].revents & ERR_MASK) != 0)
        {
            DBG_INFO("tcp SocketRead other client close socket %d event %x \r\n", fds[0].fd, fds[0].revents);
            return FALSE;
        }

        if (fds[0].revents & POLLIN)
        {
            dwReaded = recv(s, pBuffer, dwBufferSize, 0);
            *pdwReaded = dwReaded;
            if (dwReaded == 0)
            {
                DBG_INFO("tcp SocketRead recv 0 bytes\r\n");
                return FALSE;
            }
            else if (dwReaded == -1)
            {
                if (errno == EWOULDBLOCK)
                {
                    *pdwReaded = 0;
                    return TRUE;
                }

                DBG_INFO("tcp SocketRead failed errno %d\r\n", errno);
                *pdwReaded = 0;
                return FALSE;
            }
            else
            {
                return TRUE;
            }
        }

        DBG_ERROR("tcp SocketRead not any event, should not be here\n");
        return FALSE;
    }
}

BOOL TCPSocketWrite(SOCKET s, BYTE* pBuffer, DWORD dwBufferSize, DWORD* pdwWritten, HANDLE hStopEvent)
{
    struct pollfd fds[2];

    int stopfd = 0;
    int n;
    int ret = 0;
    int dwWrite = 0;
    *pdwWritten = 0;

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
        DBG_ERROR("tcp send poll error %d \r\n", errno);
        return FALSE;
    }
    else if (ret == 0)
    {
        DBG_ERROR("tcp send poll timeout\r\n");
        return FALSE;
    }
    else
    {
        if (n > 1 && fds[1].revents != 0)
        {
            DBG_INFO("tcp send get stop event \r\n");
            return FALSE;
        }

        if ((fds[0].revents & ERR_MASK) != 0)
        {
            DBG_INFO("tcp other client close socket %x\r\n", fds[0].revents);
            return FALSE;
        }

        if (fds[0].revents & POLLOUT)
        {
            signal(SIGPIPE, SIG_IGN);
            try
            {
                dwWrite = send(s, pBuffer, dwBufferSize, 0);
                *pdwWritten = dwWrite;

                if (dwWrite == 0)
                {
                    DBG_INFO("tcp SocketWrite recv 0 bytes\r\n");
                    return FALSE;
                }
                else if (dwWrite == -1)
                {
                    if (errno == EWOULDBLOCK || errno == EINTR)
                    {
                        *pdwWritten = 0;
                        return TRUE;
                    }

                    DBG_INFO("tcp SocketWrite failed errno %d\r\n", errno);
                    return FALSE;
                }
                else
                {
                    return TRUE;
                }
            }
            catch (std::exception& e)
            {
                DBG_INFO("tcp SocketWrite exception\r\n");
                return FALSE;
            }
        }

        DBG_ERROR("tcp SocketWrite not any event, should not be here\n");
        return FALSE;
    }
}
#endif
