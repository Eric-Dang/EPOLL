//-------------------------------------------------------------------------------------------------
//	Created:	2015-6-23   17:47
//	File Name:	System.h
//	Author:		Eric(沙鹰)
//	PS:			如果发现说明错误，代码风格错误，逻辑错问题，设计问题，请告诉我。谢谢！
//  Email:		frederick.dang@gmail.com
//	Purpose:	EPOLL需要包含的一些系统文件
//-------------------------------------------------------------------------------------------------

#ifndef __WIN32
// 基础类库
#include <stdio.h>
#include <string.h>
#include <assert.h>

// 网络相关
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netdb.h>


#endif // __WIN32
