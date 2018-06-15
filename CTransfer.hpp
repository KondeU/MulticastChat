#pragma once

#include <WinSock2.h> // WinSock2.h must be before windows.h, or will cause redefinition error
#include <WS2tcpip.h>
#pragma comment(lib,"Ws2_32.lib")

#include <Windows.h>

#include <vector>
#include <sstream>
#include <string>

using std::vector;

#ifdef UNICODE
#define TString std::wstring
#define TStringStream std::wstringstream
#else
#define TString std::string
#define TStringStream std::stringstream
#endif

class CTransfer
{
public:

	CTransfer()
	{
		m_hwnd = NULL;
		m_szCaption = TEXT("");

		WSAData wsaData;
		int iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iRet != 0)
		{
			ErrMsgBox(TEXT("CTransfer"), TEXT("WSAStartup"), &iRet, true);
		}
	}

	~CTransfer()
	{
		WSACleanup();
	}

	void SetHwnd(HWND hwnd)
	{
		m_hwnd = hwnd;
	}

	void SetCaption(LPCTSTR szCaption)
	{
		m_szCaption = szCaption;
	}

	void InitParam(LPCTSTR szMulticastIP, LPCTSTR szLocalIP, unsigned int uiPort, LPCTSTR szName,
		unsigned int uiPackageSize, unsigned int uiTTL)
	{
		m_szMulticastIP = szMulticastIP;
		m_szLocalIP = szLocalIP;
		m_uiPort = uiPort;
		m_szName = szName;

		m_uiPackageSize = uiPackageSize;
		m_uiTTL = uiTTL;
	}

	TString GetMulticastIP()
	{
		return m_szMulticastIP;
	}

	unsigned int GetMulticastPort()
	{
		return m_uiPort;
	}

	bool EnterChat()
	{
		m_hSock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0,
			WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED);

		// Using setsockopt instead of WSAIoctl to simplify operations

		int iReuseAddr = 1;
		if (SOCKET_ERROR ==
			setsockopt(m_hSock, SOL_SOCKET, SO_REUSEADDR,
			(char *)&iReuseAddr, sizeof(iReuseAddr)))
		{
			ErrMsgBox(TEXT("EnterChat"), TEXT("setsockopt - SO_REUSEADDR"));
			
			closesocket(m_hSock);
			m_hSock = NULL;

			return false;
		}
		int iTTL = (int)m_uiTTL;
		if (SOCKET_ERROR ==
			setsockopt(m_hSock, IPPROTO_IP, IP_MULTICAST_TTL,
			(char *)&iTTL, sizeof(iTTL)))
		{
			ErrMsgBox(TEXT("EnterChat"), TEXT("setsockopt - IP_MULTICAST_TTL"));

			closesocket(m_hSock);
			m_hSock = NULL;

			return false;
		}
		int iLoopBack = 1;
		if (SOCKET_ERROR ==
			setsockopt(m_hSock, IPPROTO_IP, IP_MULTICAST_LOOP,
			(char *)&iLoopBack, sizeof(iLoopBack)))
		{
			ErrMsgBox(TEXT("EnterChat"), TEXT("setsockopt - IP_MULTICAST_LOOP"));

			closesocket(m_hSock);
			m_hSock = NULL;

			return false;
		}

		ZeroMemory(&m_tSockAddrIn, sizeof(m_tSockAddrIn));
		m_tSockAddrIn.sin_family = AF_INET;
		if (m_szLocalIP == TEXT("INADDR_ANY"))
		{
			m_tSockAddrIn.sin_addr.S_un.S_addr = INADDR_ANY;
		}
		else
		{
			//#ifdef UNICODE
			//int iIP[4];
			//swscanf_s(m_szLocalIP.c_str(), TEXT("%d.%d.%d.%d"), iIP[0], iIP[1], iIP[2], iIP[3]);
			//char szIP[16];
			//sprintf_s(szIP, "%d.%d.%d.%d", iIP[0], iIP[1], iIP[2], iIP[3]);
			//m_tSockAddrIn.sin_addr.S_un.S_addr = inet_addr(szIP);
			//#else
			//m_tSockAddrInSend.sin_addr.S_un.S_addr = inet_addr(m_szLocalIP.c_str());
			//#endif
			InetPton(AF_INET, m_szLocalIP.c_str(), &m_tSockAddrIn.sin_addr.S_un.S_addr);
		}
		WSAHtons(m_hSock, (u_short)m_uiPort, &m_tSockAddrIn.sin_port);
		if (SOCKET_ERROR == bind(m_hSock, (SOCKADDR *)(&m_tSockAddrIn), sizeof(m_tSockAddrIn)))
		{
			ErrMsgBox(TEXT("EnterChat"), TEXT("bind"));

			closesocket(m_hSock);
			m_hSock = NULL;
			ZeroMemory(&m_tSockAddrIn, sizeof(m_tSockAddrIn));

			return false;
		}
		
		//ZeroMemory(&m_tMulticastIPMERQ, sizeof(m_tMulticastIPMERQ)); 
		//if (m_szLocalIP == TEXT("INADDR_ANY"))
		//{
		//	m_tMulticastIPMERQ.imr_interface.S_un.S_addr = INADDR_ANY;
		//}
		//else
		//{
		//	InetPton(AF_INET, m_szLocalIP.c_str(), &m_tMulticastIPMERQ.imr_interface.S_un.S_addr);
		//}
		//InetPton(AF_INET, m_szMulticastIP.c_str(), &m_tMulticastIPMERQ.imr_multiaddr.S_un.S_addr);
		ZeroMemory(&m_tSockAddrMulticast, sizeof(m_tSockAddrMulticast));
		m_tSockAddrMulticast.sin_family = AF_INET;
		InetPton(AF_INET, m_szMulticastIP.c_str(), &m_tSockAddrMulticast.sin_addr.S_un.S_addr);
		WSAHtons(m_hSock, (u_short)m_uiPort, &m_tSockAddrMulticast.sin_port);

		return true;
	}

	bool StartChat()
	{
		m_hSockChatMark = WSAJoinLeaf(m_hSock,
			(SOCKADDR *)(&m_tSockAddrMulticast), sizeof(m_tSockAddrMulticast),
			NULL, NULL, NULL, NULL, JL_BOTH);

		if (INVALID_SOCKET == m_hSockChatMark)
		{
			ErrMsgBox(TEXT("StartChat"), TEXT("WSAJoinLeaf"));
			
			m_hSockChatMark = NULL;
			return false;
		}

		return true;
	}

	bool StopChat()
	{
		if (SOCKET_ERROR == closesocket(m_hSockChatMark))
		{
			ErrMsgBox(TEXT("StopChat"), TEXT("closesocket"));
			return false;
		}

		m_hSockChatMark = NULL;
		return true;
	}

	void ErrMsgBox(LPCTSTR szLocation, LPCTSTR szCalling,
		int * piLastErrorCode = nullptr, bool bIsCodeUseToOutputDirectly = false)
	{
		TStringStream ss;
		ss << TEXT("发生运行错误，错误代码：");

		if (bIsCodeUseToOutputDirectly)
		{
			if (piLastErrorCode)
			{
				ss << (*piLastErrorCode);
			}
			else
			{
				ss << TEXT("Unknown");
			}
		}
		else
		{
			int iLastError = WSAGetLastError();
			ss << iLastError;
			
			if (piLastErrorCode)
			{
				*piLastErrorCode = iLastError;
			}
		}

		TString szErrShow;
		ss >> szErrShow;

		szErrShow += TEXT("\r\n错误位置：");
		szErrShow += szLocation;
		szErrShow += TEXT("\r\n调用函数：");
		szErrShow += szCalling;
		//szErrShow += TEXT("\r\n请检查网络状况，若网络状态良好，请联系管理员。");

		MessageBox(m_hwnd, szErrShow.c_str(), m_szCaption.c_str(), MB_ICONERROR | MB_OK);
	}

	vector<TString> GetValidIP(bool * pbSuccess)
	{
		vector<TString> vValidIP;

		char szHostName[256] = { 0 };
		if (gethostname(szHostName, sizeof(szHostName)) == SOCKET_ERROR)
		{
			ErrMsgBox(TEXT("GetValidIP"), TEXT("gethostname"));
			
			if (pbSuccess)
			{
				*pbSuccess = false;
			}

			return vValidIP;
		}

		addrinfo hints, * result = nullptr;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;

		if (getaddrinfo(szHostName, nullptr, &hints, &result) != 0)
		{
			ErrMsgBox(TEXT("GetValidIP"), TEXT("getaddrinfo"));

			if (pbSuccess)
			{
				*pbSuccess = false;
			}

			return vValidIP;
		}

		for (addrinfo * ptr = result; ptr != nullptr; ptr = ptr->ai_next)
		{
			sockaddr_in * sockaddr_ipv4 = (struct sockaddr_in *)ptr->ai_addr;
			TCHAR szIP[16] = { 0 };
			InetNtop(AF_INET, &sockaddr_ipv4->sin_addr, szIP, 16);

			vValidIP.push_back(TString(szIP));
		}

		freeaddrinfo(result);

		return vValidIP;
	}

private:

	HWND m_hwnd;
	TString m_szCaption;

	TString m_szMulticastIP;
	TString m_szLocalIP;
	unsigned int m_uiPort;
	TString m_szName;

	unsigned int m_uiPackageSize;
	unsigned int m_uiTTL;

	SOCKET m_hSock;
	SOCKADDR_IN m_tSockAddrIn;
	//IP_MREQ m_tMulticastIPMERQ;
	SOCKADDR_IN m_tSockAddrMulticast;

	SOCKET m_hSockChatMark;
};