#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <atlstr.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <direct.h>
#include "SHA1.h"

#pragma comment(lib,"ws2_32")

using namespace std;

#define CAST_ADDR	"239.1.1.1"
#define CAST_PORT	7125
#define LOCAL_PORT	5150
#define BUFSIZE     1024

/*
	控制发送和接收过程创建的宏定义                                                                   
*/
#define MSG_FILESTART		0				
#define MSG_FILEEND			1
#define MSG_FILECONTENT		2
#define MSG_RESEND			3
#define MSG_SHA1			4

struct sockaddr_in local, remote;
SOCKET ReceiveSocket;
struct ip_mreq mcast;
int	 RecLength = sizeof(remote);

struct ReceiveBuffer
{
	int		nID;
	char	buf[BUFSIZE];
};
ReceiveBuffer buf;
int nRBLen=sizeof(buf);

DWORD StartSock()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	 
	wVersionRequested = MAKEWORD( 2, 2 );
	 
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) 
	{
		return -1;
	}
	 
	/* Confirm that the WinSock DLL supports 2.2.*/
	if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ) 
	{
		WSACleanup( );
		return -1; 
	}

	ReceiveSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(ReceiveSocket == INVALID_SOCKET)
	{
		cout << "创建套接字失败，错误代码：%d" << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}

	/* 设置回环许可 */
	bool loop = true;
	if(setsockopt(ReceiveSocket,SOL_SOCKET, SO_REUSEADDR, (char *)&loop, sizeof(loop)) == SOCKET_ERROR)
	{
		printf("ERROR : setsockopt()");
		exit(0);
	}

	// 填充本地地址
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(CAST_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	// 绑定套接字
	if(bind(ReceiveSocket, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
	{
		printf("bind failed with: %d\n", WSAGetLastError());
		closesocket(ReceiveSocket);
		WSACleanup();
		return -1;
	}

	// 建立im_req结构体指示本机的IP地址及加入的组
	mcast.imr_multiaddr.s_addr = inet_addr(CAST_ADDR);
	mcast.imr_interface.s_addr = INADDR_ANY;
	if(setsockopt(ReceiveSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
		(char *)&mcast, sizeof(mcast)) == SOCKET_ERROR)
	{
		printf("setsockopt(IP_ADD_MEMBERSHIP) failed with: %d\n", WSAGetLastError());
		closesocket(ReceiveSocket);
		WSACleanup();
		return -1;
	}

	char hostname[256];

	//获取本地主机名称 
	if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) 
	{ 
		printf("Error %d when getting local host name.", WSAGetLastError()); 
		return -1; 
    } 

	//从主机名数据库中得到对应的“主机” 
    struct hostent *phe = gethostbyname(hostname); 
    if (phe == 0) 
	{ 
		printf("Yow! Bad host lookup."); 
		return 1; 
    } 

    //得出本地机器所有IP地址 
	CString hostIP = inet_ntoa(*((struct in_addr *)phe->h_addr_list[0]));

	cout << "创建套接字成功！" << endl;
	cout << "*************************************************" << endl;
	cout << hostIP << " 加入组播地址: "<< CAST_ADDR << ":" << CAST_PORT << endl;
	cout << "*************************************************" << endl;

	return 1;
}

DWORD Dropmember()
{
	// 离开组
	if(setsockopt(ReceiveSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
		(char *)&mcast, sizeof(mcast)) == SOCKET_ERROR)
	{
		printf("setsockopt(IP_DROP_MEMBERSHIP) failed with: %d\n", WSAGetLastError());
	}
	closesocket(ReceiveSocket);
	WSACleanup();
	return 1;
}

int RecvFile(LPTSTR szFile)
{
	if(StartSock() == -1)
	{
		cout << "Create socket failed!" << endl;
	}

	//清理接收缓存
	memset(&buf, 0, nRBLen);

	FILE *fp=NULL;
	int nRes;

	//存放接收到的SHA1校验码
	CString recvSHA1="";

	///获取当前程序绝对路径
	TCHAR currentPath[_MAX_PATH]={0};
	GetModuleFileName(NULL,currentPath,1024);

	GetCurrentDirectory(_MAX_PATH,currentPath);             //获取程序的当前目录
	strcat(currentPath, "\\");

	cout << "开始从组播服务器接收文件......" << endl;

	//循环接收数据
	while(true)
	{
		//首先清空接收缓存
		memset(&buf, 0, nRBLen);

		//接收数据
		nRes=recvfrom(ReceiveSocket, (char*)&buf, nRBLen, 0, (struct sockaddr *)&remote, &RecLength);
		if(nRes>0)
		{
			//如果是正文，那么就直接写入文件。
			if(buf.nID==MSG_FILECONTENT)
			{
				fwrite(buf.buf, sizeof(char),BUFSIZE, fp);
			}
			//如果是开始传送消息，那么传过来的是文件名
			else if(buf.nID == MSG_FILESTART)
			{				
				strcat(currentPath, PathFindFileName(buf.buf));
				lstrcpy(szFile, currentPath);
				
				//如果原文件存在，则将原来文件删除。并新建一个空文件
				fp=fopen(currentPath, "wb");
				if(fp==NULL)
				{
					cout<< "打开文件失败!\n错误代码：" << GetLastError() << endl;
					return 1;
				}
				fclose(fp);
				
				//打开那个空文件
				fp=fopen(currentPath, "ab");
			}

			//如果是SHA1校验码，那么就存起来，用于最后做对比
			else if(buf.nID == MSG_SHA1)
			{
				recvSHA1+=buf.buf;
			}

			//如果文件传完了。
			else if(buf.nID == MSG_FILEEND)
			{
				//写入最后的数据，并关闭文件
				fwrite(buf.buf, sizeof(char),nRes-sizeof(int), fp);
				fclose(fp);

				//计算本地建立的文件的SHA1校验码
				CSHA1 sha1;
				const bool bSuccess = sha1.HashFile(currentPath);
				sha1.Final();

				TCHAR tszReport[41];
				sha1.ReportHash(tszReport, CSHA1::REPORT_HEX_SHORT);
				if(bSuccess)
				{
					_tprintf(_T("\n客户端文件内容的哈希值:\n "));
					_tprintf(tszReport);
					_tprintf(_T("\n\n"));
				}

				//与接收到的SHA1校验码做对比
				if(!strcmp(tszReport, recvSHA1))
				{
					cout<<"SHA1校验通过!\n文件传输成功."<<endl;
					Dropmember();
					break;
				}
				else
				{
					cout<<"SHA1校验失败！！文件需要重传."<<endl;
					
					//给服务器发送请求重传消息
					memset(&buf, 0, nRBLen);
					buf.nID = MSG_RESEND;
					sendto(ReceiveSocket, (char*)&buf, nRBLen, 0, (struct sockaddr *)&remote, RecLength);
				}				
			}
		}
		else if(nRes<0)
		{
			cout<< "接收数据出错!" << endl;
			break;
		}
		else 
		{
			//connection close gracefully.
		}
	}

	//关闭套接字，清理winsock，并退出程序
	closesocket(ReceiveSocket);
	WSACleanup();
	//system("pause");
	return 0;
}
