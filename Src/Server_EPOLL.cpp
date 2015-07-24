//-------------------------------------------------------------------------------------------------
//	Created:	2015-6-23   17:47
//	File Name:	Server_EPOLL.h
//	Author:		Eric(沙鹰)
//	PS:			如果发现说明错误，代码风格错误，逻辑错问题，设计问题，请告诉我。谢谢！
//  Email:		frederick.dang@gmail.com
//	Purpose:	EPOLL 示例
//-------------------------------------------------------------------------------------------------
#include "System.h"
#include "Common.h"

// 文件句柄
typedef FileHandle 	int;
// 网络连接 socket
typedef ConnectType int;

// 也可以设置为NULL,getaddrinfo处理的IP为INADDR_ANY
#define ServerIP "192.168.150.128"
#define ServerPort "6666"

#define UseIPv4

#define SOCKET_ERROR -1
#define EPOLL_MAX_EVENT_NUM 200

struct WorkTreadData
{
	ConnectType sConn;
	FikeHandle  epollHandle;
};


void* Network_EPOLL_WorkThread(LPVOID pParam)
{
	WorkTreadData* pThreadData = (WorkTreadData*) pParam;
	epoll_event events[EPOLL_MAX_EVENT_NUM];
	
	while(true)
	{
		
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

#ifndef UseIPv4	
	// 使用 getaddrinfo函数处理
	struct addrinfo addrHints, *addrRet, *addrNext;
	memset(&hints, 0, sizeof(addrHost));
	addrHints.ai_family = AF_INET;
	addrHints.ai_socktype = SOCK_STREAM;
	addrHints.ai_protocol = IPPROTO_IP;
	addrHints.ai_flags = AI_PASSIVE;
	
	int iRet = LogPrint(ServerIP, (char*)ServerPort, &addrHints, &addrRet);
	if(iRet != 0)
	{
		LogPrint("LogPrint Error [%d] [%s]", iRet, gai_strerror(iRet));
	}
	else
	{
		for(addrNext = addrRet; addrNext != NULL; addrNext = addrNext->ai_next)
		{
			sConnect = socket(addrNext->ai_family, addrNext->ai_socktype, addrNext->ai_protocol);
			if(sConnect == -1)
				continue;

			iRet = bind(sConnect, addrNext->ai_addr, addrNext->ai_addrlen);
			if(iRet == 0)
			{
				break; // we managed to bind successfully!
			}
			close(sConnect);
			sConnect = 0;
		}

		if(addrNext == NULL)
		{
			LogPrint("Could not Bind");
			assert(false);
		}
		freeaddrinfo(addrRet);
	}
#else
	// 使用IP4接口 gethostbyname
	sConnect = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if(sConnect == SOCKET_ERROR)
	
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	if(ServerIP)
	{
		struct hosten *h;
		if((h = gethostbyname(ServerIP)) == NULL)
		{
			LogPrint("gethostbyname error[%d] [%s]", h_errno, hstrerror(h_errno));
			return 0;
		}
		// 将number型的IP地址转换为a.b.c.d类型的地址
		char* aIP = inet_ntoa(*((struct in_addr *)h->h_addr));
		
		inet_aton(aIP, &(addr.sin_addr));
	}
	else
	{
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	addr.sin_port = htons(ServerPort);

	if(bind(sConnect, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		LogPrint("bind error [%d] [%s]", errno, strerror(errno));
		return 0;
	}
#endif // not UseIPv4 	
	
	if(listen(sConnect, SOMAXCONN) == SOCKET_ERROR)
	{
		LogPrint("listen error [%d], [%s]", errno, strerror(errno));
		return 0;
	}

	// 设置端口重用， 程序退出后， 重启可继续使用此端口(否则需要等待一段时间)
	int nReuseAddr = 1;
	setsockopt(sConnect, SOL_SOCKET, SO_REUSEADDR, (const char*)&nReuseAddr, sizeof(nReuseAddr));

	// 设置socket关闭时，等到待发送数据发送完成后再关闭连接
	linger lLinger;
	lLinger.l_onoff = 1;
	lLinger.l_linger = 0;
	setsockopt(sConnect, SOL_SOCKET, SO_LINGER, (const char*)&lLinger, sizeof(lLinger));

	// 不使用Nagle算法， 不会将小包拼接成大包，直接将小包发送出去
	BOOL bNoDelay = true;
	setsockopt(sConnect, IPPROTO_IP, TCP_NODELAY, (const char*)&bNoDelay, sizeof(bNoDelay));

	// 设置非阻塞
	if(!SetNonBlocking(sConnect))
		return 0;

	struct WorkTreadData* pThreadData = new WorhTreadData;
	pThreadData->sConn = sConnect;

	int iWorkThreadID, iRet;
	if((iRet = pthread_create(&iWorkThreadID,  NULL, Network_WorkThread, pThreadData)) != 0)
	{
		// 创建线程失败
		LogPrint("Create Thread Failed [%d]", iRet);
		return 0;
	}
	
	// 创建EPOLL
	FileHandle fhEPOLL = epoll_create(200);
	if(fhEPOLL == -1)
	{
		LogPrint("epoll_create Failed [%d]", errno);
		return 0;
	}

	epoll_event ee;
	ee.events = EPOLLIN;
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

