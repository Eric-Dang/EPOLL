//-------------------------------------------------------------------------------------------------
//	Created:	2015-6-23   17:47
//	File Name:	Client_EPOLL.cpp
//	Author:		Eric(ɳӥ)
//	PS:			�������˵�����󣬴���������߼������⣬������⣬������ҡ�лл��
//  Email:		frederick.dang@gmail.com
//	Purpose:	EPOLL ʾ��
//-------------------------------------------------------------------------------------------------
#include "System.h"
#include "Common.h"
//-------------------------------------------------------------------------------------------------
// �ļ����
typedef int FileHandle;
// �������� socket
typedef int ConnectType;

typedef int BOOL;

// Ҳ��������ΪNULL,getaddrinfo�����IPΪINADDR_ANY
#define ServerIP "192.168.150.128"
#define ServerPort 6666


#define SOCKET_ERROR -1
#define EPOLL_MAX_EVENT_NUM 200

struct WorkThreadData
{
	ConnectType sConn;
	FileHandle  epollHandle;
};

//-------------------------------------------------------------------------------------------------
bool SetNonBlocking(ConnectType sConn);
//-------------------------------------------------------------------------------------------------

void* Network_EPOLL_WorkThread(void* pParam)
{
	WorkThreadData* pThreadData = (WorkThreadData*) pParam;
	ConnectType sListenConn = pThreadData->sConn;
	FileHandle	eHandle = pThreadData->epollHandle;

	epoll_event events[EPOLL_MAX_EVENT_NUM];
	
	while(true)
	{
		int iEpollEvent = epoll_wait(eHandle, events, EPOLL_MAX_EVENT_NUM, -1);
	
		if(iEpollEvent == -1)
		{
			LogPrint("epoll_wait error [%s].", strerror(errno));
			return ((void*)0);
		}

		for(int i = 0; i < iEpollEvent; i++)
		{			
			if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP))
			{
				/*An error has occured on this fd, or the socket is not ready for reading (why were we notified then?)*/
				LogPrint("epoll error Error Event!");
				close(events[i].data.fd);
				continue;
			}
			else if(events[i].events & EPOLLIN)
			{
				// Read
				char ReadBuffer[10240];
				bzero(ReadBuffer, 10240);
					
				int iRecvSize = read(events[i].data.fd, ReadBuffer, 10240);
				if(iRecvSize > 0)
				{
					LogPrint("Recv From Client [%s].", ReadBuffer);

					epoll_event ee;
					ee.data.fd = events[i].data.fd;
					ee.events = EPOLLOUT | EPOLLET;
					epoll_ctl(eHandle, EPOLL_CTL_MOD, events[i].data.fd, &ee);
				}
				else if((iRecvSize < 0) && (errno == EWOULDBLOCK || errno == EINTR))
				{
					epoll_event ee;
					ee.data.fd = events[i].data.fd;
					ee.events = 0;
					epoll_ctl(eHandle, EPOLL_CTL_DEL, events[i].data.fd, &ee);
					close(events[i].data.fd);
				}
			}
			else if(events[i].events & EPOLLOUT)
			{
				// Write
				char WriteBuffer[10240];
				bzero(WriteBuffer, 10240);
				struct timeval tt;
				gettimeofday(&tt, NULL);
				sprintf(WriteBuffer, "Server New Send %d", (tt.tv_sec*1000 + tt.tv_usec));
				int iSendLen = strlen(WriteBuffer);

				int nSend = write(events[i].data.fd, WriteBuffer, iSendLen);
				if(nSend <= 0)
				{
					if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
						break;
					else
					{
						epoll_event ee;
						ee.data.fd = events[i].data.fd;
						ee.events = 0;
						epoll_ctl(eHandle, EPOLL_CTL_DEL, events[i].data.fd, &ee);
						close(events[i].data.fd);
					}
				}
				else
				{
					epoll_event ee;
					ee.data.fd = events[i].data.fd;
					ee.events = EPOLLIN | EPOLLET;
					epoll_ctl(eHandle, EPOLL_CTL_MOD, events[i].data.fd, &ee);
				}
			}
		}			
	}

	return 0;
}

bool SetNonBlocking(ConnectType sConn)
{
	int opts;
	opts = fcntl(sConn, F_GETFL);
	if(opts < 0)
	{
		LogPrint("fcntl Get Error [%d]", sConn);
		return false;
	}

	opts = opts | O_NONBLOCK;
	if(fcntl(sConn, F_SETFL, opts) < 0)
	{
		LogPrint("fcntl Set Error [%d]", sConn);
		return false;
	}

	return true;
}

int main()
{
	ConnectType sConnect;

	// ʹ��IP4�ӿ� gethostbyname
	sConnect = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if(sConnect == SOCKET_ERROR)
		return 0;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	if(ServerIP)
	{
		struct hostent *h;
		if((h = gethostbyname(ServerIP)) == NULL)
		{
			LogPrint("gethostbyname error[%d] [%s]", h_errno, hstrerror(h_errno));
			return 0;
		}
		// ��number�͵�IP��ַת��Ϊa.b.c.d���͵ĵ�ַ
		char* aIP = inet_ntoa(*((struct in_addr *)h->h_addr));
		
		inet_aton(aIP, &(addr.sin_addr));
	}
	else
	{
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	addr.sin_port = htons(ServerPort);

	// ����socket�ر�ʱ���ȵ����������ݷ�����ɺ��ٹر�����
	linger lLinger;
	lLinger.l_onoff = 1;
	lLinger.l_linger = 0;
	setsockopt(sConnect, SOL_SOCKET, SO_LINGER, (const char*)&lLinger, sizeof(lLinger));

	// ��ʹ��Nagle�㷨�� ���ὫС��ƴ�ӳɴ����ֱ�ӽ�С�����ͳ�ȥ
	BOOL bNoDelay = true;
	setsockopt(sConnect, IPPROTO_IP, TCP_NODELAY, (const char*)&bNoDelay, sizeof(bNoDelay));

	// ���÷�����
	if(!SetNonBlocking(sConnect))
		return 0;
	
	// ����EPOLL
	FileHandle fhEPOLL = epoll_create(200);
	if(fhEPOLL == -1)
	{
		LogPrint("epoll_create Failed [%d]", errno);
		return 0;
	}

	WorkThreadData* pThreadData = new WorkThreadData;
	pThreadData->sConn = sConnect;
	pThreadData->epollHandle = fhEPOLL;
	
	pthread_t iWorkThreadID;
	int iRet;
	if((iRet = pthread_create(&iWorkThreadID,  NULL, Network_EPOLL_WorkThread, pThreadData)) != 0)
	{
		// �����߳�ʧ��
		LogPrint("Create Thread Failed [%d]", iRet);
		return 0;
	}

	if (connect(sConnect, (sockaddr*)&addr, sizeof(sockaddr_in)) == -1)
	{
		printf("connect to server failed, error = (%d)(%s).\n", errno, strerror(errno));
		fflush(stdout);
		return 0;
	}
		
	epoll_event ee;
	ee.events = EPOLLOUT | EPOLLET;
	ee.data.fd = sConnect;
	if(epoll_ctl(fhEPOLL, EPOLL_CTL_ADD, sConnect, &ee) == -1)
	{
		LogPrint("epoll_ctl error!");
		return 0;
	}

	while(true)
	{
		sleep(1000);
	}

	return 1;
}

