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
	���Ʒ��ͺͽ��չ��̴����ĺ궨��                                                                   
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
		cout << "�����׽���ʧ�ܣ�������룺%d" << WSAGetLastError() << endl;
		WSACleanup();
		return -1;
	}

	/* ���ûػ���� */
	bool loop = true;
	if(setsockopt(ReceiveSocket,SOL_SOCKET, SO_REUSEADDR, (char *)&loop, sizeof(loop)) == SOCKET_ERROR)
	{
		printf("ERROR : setsockopt()");
		exit(0);
	}

	// ��䱾�ص�ַ
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(CAST_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	// ���׽���
	if(bind(ReceiveSocket, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
	{
		printf("bind failed with: %d\n", WSAGetLastError());
		closesocket(ReceiveSocket);
		WSACleanup();
		return -1;
	}

	// ����im_req�ṹ��ָʾ������IP��ַ���������
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

	//��ȡ������������ 
	if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) 
	{ 
		printf("Error %d when getting local host name.", WSAGetLastError()); 
		return -1; 
    } 

	//�����������ݿ��еõ���Ӧ�ġ������� 
    struct hostent *phe = gethostbyname(hostname); 
    if (phe == 0) 
	{ 
		printf("Yow! Bad host lookup."); 
		return 1; 
    } 

    //�ó����ػ�������IP��ַ 
	CString hostIP = inet_ntoa(*((struct in_addr *)phe->h_addr_list[0]));

	cout << "�����׽��ֳɹ���" << endl;
	cout << "*************************************************" << endl;
	cout << hostIP << " �����鲥��ַ: "<< CAST_ADDR << ":" << CAST_PORT << endl;
	cout << "*************************************************" << endl;

	return 1;
}

DWORD Dropmember()
{
	// �뿪��
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

	//������ջ���
	memset(&buf, 0, nRBLen);

	FILE *fp=NULL;
	int nRes;

	//��Ž��յ���SHA1У����
	CString recvSHA1="";

	///��ȡ��ǰ�������·��
	TCHAR currentPath[_MAX_PATH]={0};
	GetModuleFileName(NULL,currentPath,1024);

	GetCurrentDirectory(_MAX_PATH,currentPath);             //��ȡ����ĵ�ǰĿ¼
	strcat(currentPath, "\\");

	cout << "��ʼ���鲥�����������ļ�......" << endl;

	//ѭ����������
	while(true)
	{
		//������ս��ջ���
		memset(&buf, 0, nRBLen);

		//��������
		nRes=recvfrom(ReceiveSocket, (char*)&buf, nRBLen, 0, (struct sockaddr *)&remote, &RecLength);
		if(nRes>0)
		{
			//��������ģ���ô��ֱ��д���ļ���
			if(buf.nID==MSG_FILECONTENT)
			{
				fwrite(buf.buf, sizeof(char),BUFSIZE, fp);
			}
			//����ǿ�ʼ������Ϣ����ô�����������ļ���
			else if(buf.nID == MSG_FILESTART)
			{				
				strcat(currentPath, PathFindFileName(buf.buf));
				lstrcpy(szFile, currentPath);
				
				//���ԭ�ļ����ڣ���ԭ���ļ�ɾ�������½�һ�����ļ�
				fp=fopen(currentPath, "wb");
				if(fp==NULL)
				{
					cout<< "���ļ�ʧ��!\n������룺" << GetLastError() << endl;
					return 1;
				}
				fclose(fp);
				
				//���Ǹ����ļ�
				fp=fopen(currentPath, "ab");
			}

			//�����SHA1У���룬��ô�ʹ�����������������Ա�
			else if(buf.nID == MSG_SHA1)
			{
				recvSHA1+=buf.buf;
			}

			//����ļ������ˡ�
			else if(buf.nID == MSG_FILEEND)
			{
				//д���������ݣ����ر��ļ�
				fwrite(buf.buf, sizeof(char),nRes-sizeof(int), fp);
				fclose(fp);

				//���㱾�ؽ������ļ���SHA1У����
				CSHA1 sha1;
				const bool bSuccess = sha1.HashFile(currentPath);
				sha1.Final();

				TCHAR tszReport[41];
				sha1.ReportHash(tszReport, CSHA1::REPORT_HEX_SHORT);
				if(bSuccess)
				{
					_tprintf(_T("\n�ͻ����ļ����ݵĹ�ϣֵ:\n "));
					_tprintf(tszReport);
					_tprintf(_T("\n\n"));
				}

				//����յ���SHA1У�������Ա�
				if(!strcmp(tszReport, recvSHA1))
				{
					cout<<"SHA1У��ͨ��!\n�ļ�����ɹ�."<<endl;
					Dropmember();
					break;
				}
				else
				{
					cout<<"SHA1У��ʧ�ܣ����ļ���Ҫ�ش�."<<endl;
					
					//�����������������ش���Ϣ
					memset(&buf, 0, nRBLen);
					buf.nID = MSG_RESEND;
					sendto(ReceiveSocket, (char*)&buf, nRBLen, 0, (struct sockaddr *)&remote, RecLength);
				}				
			}
		}
		else if(nRes<0)
		{
			cout<< "�������ݳ���!" << endl;
			break;
		}
		else 
		{
			//connection close gracefully.
		}
	}

	//�ر��׽��֣�����winsock�����˳�����
	closesocket(ReceiveSocket);
	WSACleanup();
	//system("pause");
	return 0;
}
