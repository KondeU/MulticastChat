#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
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
	
struct sockaddr_in locAddr, remAddr;
SOCKET SendSocket;
//struct ip_mreq mcast;

//发送包的结构
struct SendBuffer
{
	int		nID;
	char	buf[BUFSIZE];
};
SendBuffer buf;
int nSBlen=sizeof(buf);

int InitSock()
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

	SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(SendSocket == INVALID_SOCKET)
	{
		cout << "创建套接字失败，错误代码：%d" << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}

	// 填充本地地址
	locAddr.sin_family = AF_INET;
	locAddr.sin_addr.s_addr = INADDR_ANY;
	locAddr.sin_port = htons(LOCAL_PORT);

	// 绑定套接字
	if(bind(SendSocket, (struct sockaddr *)&locAddr, sizeof(locAddr)) == SOCKET_ERROR)
	{
		cout << "绑定失败，错误代码：%d" << WSAGetLastError() << endl;
		closesocket(SendSocket);
		WSACleanup();
		return -1;
	}


	// 填充组播的地址及端口号
	remAddr.sin_family = AF_INET;
	remAddr.sin_port = htons(CAST_PORT);
	remAddr.sin_addr.s_addr = inet_addr(CAST_ADDR);

	return 1;

}

int SendFile(LPCTSTR szFile)
{
	//初始化套接字信息
	if(InitSock() == -1)
	{
		cout << "初始化套接字失败！" << endl;
	}
	else
	{
		cout << "初始化套接字成功！" << endl;
		cout << "***************************************" << endl;
		cout << "组播地址: " << CAST_ADDR <<endl;
		cout << "端口号	: " << CAST_PORT << endl;
		cout << "***************************************" << endl; 
	}

	FILE *fp;
	
	//获取当前程序绝对路径
	TCHAR currentPath[_MAX_PATH]={0};
	GetModuleFileName(NULL,currentPath,1024);

	GetCurrentDirectory(_MAX_PATH,currentPath);             //获取程序的当前目录
	strcat(currentPath, "\\");


	TCHAR tszFilename[1024] = {0};

	_tprintf(_T("\n文件名: "));
	fflush(stdin);
	//_getts_s(tszFilename);
	//strcat(currentPath, tszFilename);
	//lstrcpy(currentPath, szFile);
	lstrcpy(tszFilename, szFile);

	//计算发送文件的SHA1校验码，存放于outSHA1中
	CSHA1 sha1;
	const bool bSuccess = sha1.HashFile(tszFilename);
	sha1.Final();

	TCHAR tszReport[41];
	sha1.ReportHash(tszReport, CSHA1::REPORT_HEX_SHORT);
	if(bSuccess)
	{
		_tprintf(_T("\n服务器端文件内容的哈希值:\n "));
		_tprintf(tszReport);
		_tprintf(_T("\n\n"));
	}
	
	//首先把md5校验码先发给多播地址
	int nCmd=MSG_SHA1;

	cout << "组播服务器开始传送文件......" << endl;

	//循环发送文件
	while (true)
	{	
		//清空发送缓存
		memset(&buf,0,sizeof(SendBuffer));

		if(nCmd==MSG_SHA1)
		{
			//把md5校验码发给多播地址
			buf.nID=MSG_SHA1;			
			memcpy(buf.buf, tszReport, sizeof(tszReport));
			// 发送数据
			if(sendto(SendSocket, (char*)&buf, nSBlen, 0, 
				(SOCKADDR *)&remAddr, sizeof(remAddr)) == SOCKET_ERROR)
			{
				cout << "sendto failed with: %d" << WSAGetLastError() << endl;
				closesocket(SendSocket);
				WSACleanup();
				return -1;
			}

			//然后开始正式启动发送文件
			nCmd=MSG_FILESTART;
		}
		else if(nCmd==MSG_FILESTART)
		{			
			//首先要把要传送的文件的文件名发给多播地址，以便文件接收端建立一个同名的文件。
			buf.nID=MSG_FILESTART;
			memcpy(buf.buf,tszFilename, sizeof(tszFilename));
			// 发送数据
			if(sendto(SendSocket, (char*)&buf, nSBlen, 0, 
				(SOCKADDR *)&remAddr, sizeof(remAddr)) == SOCKET_ERROR)
			{
				cout << "sendto failed with: %d" << WSAGetLastError() << endl;
				closesocket(SendSocket);
				WSACleanup();
				return -1;
			}
			
			//打开要发送的文件			
			//fp = fopen( currentPath, "rb" );
			fp = fopen(tszFilename, "rb");
			
			//然后开始正式发送文件内容
			nCmd=MSG_FILECONTENT;
		}		
		else if(nCmd==MSG_FILECONTENT)
		{
			//发送文件内容
			buf.nID=MSG_FILECONTENT;			
			int readsize=fread(buf.buf,sizeof(char),BUFSIZE,fp);//读取下段文件

			//如果读到的文件小于数据缓存的长度，那么说明文件已经读完了。
			if(readsize<BUFSIZE)
			{
				//关闭文件，发送最后的数据，并退出while循环
				fclose(fp);
				buf.nID=MSG_FILEEND;

				if(sendto(SendSocket, (char*)&buf, sizeof(int)+readsize, 0, 
					(SOCKADDR *)&remAddr, sizeof(remAddr)) == SOCKET_ERROR)
				{
					cout << "sendto failed with: %d" << WSAGetLastError() << endl;
					closesocket(SendSocket);
					WSACleanup();
					return -1;
				}
				else
				{
					cout<<"文件传输完成！\n";
					break;
				}
			}
			else
			{
				//发送文件正文。
				if(sendto(SendSocket, (char*)&buf, sizeof(int)+readsize, 0, 
					(SOCKADDR *)&remAddr, sizeof(remAddr)) == SOCKET_ERROR)
				{
					cout << "sendto failed with: %d" << WSAGetLastError() << endl;
					closesocket(SendSocket);
					WSACleanup();
					return -1;
				}
			}
		}
	}

	//关闭套接字，清理winsock，并退出程序
	closesocket(SendSocket);
	WSACleanup();	
	//system("pause");
	return 0;

}

