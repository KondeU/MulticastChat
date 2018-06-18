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
	���Ʒ��ͺͽ��չ��̴����ĺ궨��                                                                   
*/
#define MSG_FILESTART		0				
#define MSG_FILEEND			1
#define MSG_FILECONTENT		2
#define MSG_RESEND			3
#define MSG_SHA1			4
	
struct sockaddr_in locAddr, remAddr;
SOCKET SendSocket;
//struct ip_mreq mcast;

//���Ͱ��Ľṹ
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
		cout << "�����׽���ʧ�ܣ�������룺%d" << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}

	// ��䱾�ص�ַ
	locAddr.sin_family = AF_INET;
	locAddr.sin_addr.s_addr = INADDR_ANY;
	locAddr.sin_port = htons(LOCAL_PORT);

	// ���׽���
	if(bind(SendSocket, (struct sockaddr *)&locAddr, sizeof(locAddr)) == SOCKET_ERROR)
	{
		cout << "��ʧ�ܣ�������룺%d" << WSAGetLastError() << endl;
		closesocket(SendSocket);
		WSACleanup();
		return -1;
	}


	// ����鲥�ĵ�ַ���˿ں�
	remAddr.sin_family = AF_INET;
	remAddr.sin_port = htons(CAST_PORT);
	remAddr.sin_addr.s_addr = inet_addr(CAST_ADDR);

	return 1;

}

int SendFile(LPCTSTR szFile)
{
	//��ʼ���׽�����Ϣ
	if(InitSock() == -1)
	{
		cout << "��ʼ���׽���ʧ�ܣ�" << endl;
	}
	else
	{
		cout << "��ʼ���׽��ֳɹ���" << endl;
		cout << "***************************************" << endl;
		cout << "�鲥��ַ: " << CAST_ADDR <<endl;
		cout << "�˿ں�	: " << CAST_PORT << endl;
		cout << "***************************************" << endl; 
	}

	FILE *fp;
	
	//��ȡ��ǰ�������·��
	TCHAR currentPath[_MAX_PATH]={0};
	GetModuleFileName(NULL,currentPath,1024);

	GetCurrentDirectory(_MAX_PATH,currentPath);             //��ȡ����ĵ�ǰĿ¼
	strcat(currentPath, "\\");


	TCHAR tszFilename[1024] = {0};

	_tprintf(_T("\n�ļ���: "));
	fflush(stdin);
	//_getts_s(tszFilename);
	//strcat(currentPath, tszFilename);
	//lstrcpy(currentPath, szFile);
	lstrcpy(tszFilename, szFile);

	//���㷢���ļ���SHA1У���룬�����outSHA1��
	CSHA1 sha1;
	const bool bSuccess = sha1.HashFile(tszFilename);
	sha1.Final();

	TCHAR tszReport[41];
	sha1.ReportHash(tszReport, CSHA1::REPORT_HEX_SHORT);
	if(bSuccess)
	{
		_tprintf(_T("\n���������ļ����ݵĹ�ϣֵ:\n "));
		_tprintf(tszReport);
		_tprintf(_T("\n\n"));
	}
	
	//���Ȱ�md5У�����ȷ����ಥ��ַ
	int nCmd=MSG_SHA1;

	cout << "�鲥��������ʼ�����ļ�......" << endl;

	//ѭ�������ļ�
	while (true)
	{	
		//��շ��ͻ���
		memset(&buf,0,sizeof(SendBuffer));

		if(nCmd==MSG_SHA1)
		{
			//��md5У���뷢���ಥ��ַ
			buf.nID=MSG_SHA1;			
			memcpy(buf.buf, tszReport, sizeof(tszReport));
			// ��������
			if(sendto(SendSocket, (char*)&buf, nSBlen, 0, 
				(SOCKADDR *)&remAddr, sizeof(remAddr)) == SOCKET_ERROR)
			{
				cout << "sendto failed with: %d" << WSAGetLastError() << endl;
				closesocket(SendSocket);
				WSACleanup();
				return -1;
			}

			//Ȼ��ʼ��ʽ���������ļ�
			nCmd=MSG_FILESTART;
		}
		else if(nCmd==MSG_FILESTART)
		{			
			//����Ҫ��Ҫ���͵��ļ����ļ��������ಥ��ַ���Ա��ļ����ն˽���һ��ͬ�����ļ���
			buf.nID=MSG_FILESTART;
			memcpy(buf.buf,tszFilename, sizeof(tszFilename));
			// ��������
			if(sendto(SendSocket, (char*)&buf, nSBlen, 0, 
				(SOCKADDR *)&remAddr, sizeof(remAddr)) == SOCKET_ERROR)
			{
				cout << "sendto failed with: %d" << WSAGetLastError() << endl;
				closesocket(SendSocket);
				WSACleanup();
				return -1;
			}
			
			//��Ҫ���͵��ļ�			
			//fp = fopen( currentPath, "rb" );
			fp = fopen(tszFilename, "rb");
			
			//Ȼ��ʼ��ʽ�����ļ�����
			nCmd=MSG_FILECONTENT;
		}		
		else if(nCmd==MSG_FILECONTENT)
		{
			//�����ļ�����
			buf.nID=MSG_FILECONTENT;			
			int readsize=fread(buf.buf,sizeof(char),BUFSIZE,fp);//��ȡ�¶��ļ�

			//����������ļ�С�����ݻ���ĳ��ȣ���ô˵���ļ��Ѿ������ˡ�
			if(readsize<BUFSIZE)
			{
				//�ر��ļ��������������ݣ����˳�whileѭ��
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
					cout<<"�ļ�������ɣ�\n";
					break;
				}
			}
			else
			{
				//�����ļ����ġ�
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

	//�ر��׽��֣�����winsock�����˳�����
	closesocket(SendSocket);
	WSACleanup();	
	//system("pause");
	return 0;

}

