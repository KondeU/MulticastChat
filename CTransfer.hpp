#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
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

#define WM_SOCKRECV (WM_APP + 0x200)

uint32_t GetCRC32(CONST BYTE * pbyBuffer, unsigned int uiSize)
{
	static uint32_t CRC32Table[256];
	static bool bGenerated = false;
	if (!bGenerated)
	{
		// Polynomial representations
		// Normal : 0x04C11DB7
		// Reversed : 0xEDB88320
		// Reversed reciprocal : 0x82608EDB

		// Width: 32
		// Poly : 04C11DB7
		// RefIn : True
		// RefOut : True
		// XorOut : FFFFFFFF

		uint32_t CRC;
		for (int i = 0; i < 256; i++)
		{
			CRC = i;
			for (int j = 0; j < 8; j++)
			{
				if (CRC & 1)
				{
					CRC = (CRC >> 1) ^ 0x04C11DB7;
				}
				else
				{
					CRC >>= 1;
				}
			}
			CRC32Table[i] = CRC;
		}

		bGenerated = true;
	}

	uint32_t CRC;

	CRC = 0xFFFFFFFF; // Initial value

	for (unsigned int ui = 0; ui < uiSize; ui++)
	{
		CRC = CRC32Table[(CRC ^ pbyBuffer[ui]) & 0xFF] ^ (CRC >> 8);
	}
		
	return CRC ^ 0xFFFFFFFF;// Final XOR value
}

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

	TString GetMulticastIP() const
	{
		return m_szMulticastIP;
	}

	unsigned int GetMulticastPort() const
	{
		return m_uiPort;
	}

	TString GetMulticastPortString() const
	{
		TStringStream ss;
		ss << m_uiPort;

		TString sz;
		ss >> sz;

		return sz;
	}

	TString GetName() const
	{
		return m_szName;
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

	bool BeginChat()
	{
		m_hSockChatMark = WSAJoinLeaf(m_hSock,
			(SOCKADDR *)(&m_tSockAddrMulticast), sizeof(m_tSockAddrMulticast),
			NULL, NULL, NULL, NULL, JL_BOTH);

		if (INVALID_SOCKET == m_hSockChatMark)
		{
			ErrMsgBox(TEXT("BeginChat"), TEXT("WSAJoinLeaf"));
			
			m_hSockChatMark = NULL;
			return false;
		}

		if (SOCKET_ERROR == WSAAsyncSelect(m_hSockChatMark, m_hwnd, WM_SOCKRECV, FD_READ))
		{
			ErrMsgBox(TEXT("BeginChat"), TEXT("WSAAsyncSelect"));

			return false;
		}

		return true;
	}

	bool EndChat()
	{
		if (m_hSock == m_hSockChatMark)
		{
			m_hSockChatMark = NULL;
		}

		if (SOCKET_ERROR == closesocket(m_hSock))
		{
			ErrMsgBox(TEXT("EndChat"), TEXT("closesocket"));
			return false;
		}

		m_hSock = NULL;
		return true;
	}

	void ErrMsgBox(LPCTSTR szLocation, LPCTSTR szCalling,
		int * piLastErrorCode = nullptr, bool bIsCodeUseToOutputDirectly = false) const
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

	vector<TString> GetValidIP(bool * pbSuccess) const
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

	struct TPackage
	{
		BYTE byFrmBeg[2];

		BYTE byStyle;
		BYTE byMode;

		DWORD dwDataSize;
		DWORD dwDataCRC32; // Option

		DWORD dwMsgNumber;
		DWORD dwMsgTotal;

		BYTE byData[4096]; // Option size, max is 4096Bytes(4KB)

		BYTE byFrmEnd[2];

		#define TRN_PKG_STYLE_MESSAGE  0
		#define TRN_PKG_STYLE_FILE     1
		#define TRN_PKG_MODE_ENABLECRC 0x01

		TPackage()
		{
			this->byFrmBeg[0] = 'M';
			this->byFrmBeg[1] = 'C';

			this->byStyle = TRN_PKG_STYLE_MESSAGE;
			this->byMode = TRN_PKG_MODE_ENABLECRC;

			this->dwDataSize = 0;
			this->dwDataCRC32 = 0;

			this->dwMsgNumber = 0;
			this->dwMsgTotal = 1;

			this->byFrmEnd[0] = 'C';
			this->byFrmEnd[1] = '.';
		}
	} m_tPackage;

	bool Send(TPackage * ptPackage = nullptr)
	{
		TPackage * ptPkg = &m_tPackage;
		if (ptPackage)
		{
			ptPkg = ptPackage;
		}

		DWORD dwPackageSize =
			sizeof(TPackage::byFrmBeg)    + sizeof(TPackage::byFrmEnd)   +
			sizeof(TPackage::byStyle)     + sizeof(TPackage::byMode)     +
			sizeof(TPackage::dwMsgNumber) + sizeof(TPackage::dwMsgTotal) +
			sizeof(TPackage::dwDataSize)  + ptPkg->dwDataSize;
		if (ptPkg->byMode & TRN_PKG_MODE_ENABLECRC)
		{
			dwPackageSize += sizeof(TPackage::dwDataCRC32);
		}

		BYTE * pbyPackage = new BYTE[dwPackageSize];
		ZeroMemory(pbyPackage, sizeof(BYTE) * dwPackageSize);

		BYTE * pbyPkgIdx = pbyPackage;
		memcpy(pbyPkgIdx, ptPkg->byFrmBeg, sizeof(ptPkg->byFrmBeg));
		pbyPkgIdx += sizeof(ptPkg->byFrmBeg);
		memcpy(pbyPkgIdx, &ptPkg->byStyle, sizeof(ptPkg->byStyle));
		pbyPkgIdx += sizeof(ptPkg->byStyle);
		memcpy(pbyPkgIdx, &ptPkg->byMode, sizeof(ptPkg->byMode));
		pbyPkgIdx += sizeof(ptPkg->byMode);
		memcpy(pbyPkgIdx, &ptPkg->dwDataSize, sizeof(ptPkg->dwDataSize));
		pbyPkgIdx += sizeof(ptPkg->dwDataSize);
		if (ptPkg->byMode & TRN_PKG_MODE_ENABLECRC)
		{
			ptPkg->dwDataCRC32 = (DWORD)GetCRC32(ptPkg->byData, ptPkg->dwDataSize);
			memcpy(pbyPkgIdx, &ptPkg->dwDataCRC32, sizeof(ptPkg->dwDataCRC32));
			pbyPkgIdx += sizeof(ptPkg->dwDataCRC32);
		}
		memcpy(pbyPkgIdx, &ptPkg->dwMsgNumber, sizeof(ptPkg->dwMsgNumber));
		pbyPkgIdx += sizeof(ptPkg->dwMsgNumber);
		memcpy(pbyPkgIdx, &ptPkg->dwMsgTotal, sizeof(ptPkg->dwMsgTotal));
		pbyPkgIdx += sizeof(ptPkg->dwMsgTotal);
		memcpy(pbyPkgIdx, &ptPkg->byData, ptPkg->dwDataSize);
		pbyPkgIdx += ptPkg->dwDataSize;
		memcpy(pbyPkgIdx, &ptPkg->byFrmEnd, sizeof(ptPkg->byFrmEnd));
		pbyPkgIdx += sizeof(ptPkg->byFrmEnd);
		
		if (dwPackageSize == (pbyPkgIdx - pbyPackage) * sizeof(BYTE))
		{
			int iSended = 0;
			if (SOCKET_ERROR == (iSended = sendto(m_hSockChatMark,
				(char *)pbyPackage, (int)dwPackageSize, 0,
				(SOCKADDR *)(&m_tSockAddrMulticast), sizeof(m_tSockAddrMulticast))))
			{
				ErrMsgBox(TEXT("Send"), TEXT("sendto"));
				delete [] pbyPackage;
				return false;
			}

			if (iSended != dwPackageSize)
			{
				delete [] pbyPackage;
				return false;
			}
		}
		else
		{
			delete[] pbyPackage;
			return false;
		}
		
		delete[] pbyPackage;
		return true;
	}

	bool Recv(SOCKADDR_IN * ptSockAddrFrom = nullptr, TPackage * ptPackage = nullptr)
	{
		TPackage * ptPkg = &m_tPackage;
		if (ptPackage)
		{
			ptPkg = ptPackage;
		}

		BYTE * pbyPackage = new BYTE[sizeof(TPackage)];
		ZeroMemory(pbyPackage, sizeof(TPackage));
		int iSockAddrFromLen = sizeof(SOCKADDR_IN);
		int iRead = recvfrom(m_hSockChatMark,
			(char *)pbyPackage, sizeof(TPackage), 0,
			(SOCKADDR *)ptSockAddrFrom, &iSockAddrFromLen);
		if (iRead == SOCKET_ERROR)
		{
			ErrMsgBox(TEXT("Recv"), TEXT("recvfrom"));
			delete [] pbyPackage;
			return false;
		}

		if ((pbyPackage[0] != ptPkg->byFrmBeg[0]) ||
			(pbyPackage[1] != ptPkg->byFrmBeg[1]) ||
			(pbyPackage[iRead - 2] != ptPkg->byFrmEnd[0]) ||
			(pbyPackage[iRead - 1] != ptPkg->byFrmEnd[1]))
		{
			delete [] pbyPackage;
			return false;
		}

		BYTE * pbyPkgIdx = pbyPackage;
		pbyPkgIdx += sizeof(ptPkg->byFrmBeg);
		memcpy(&ptPkg->byStyle, pbyPkgIdx, sizeof(ptPkg->byStyle));
		pbyPkgIdx += sizeof(ptPkg->byStyle);
		memcpy(&ptPkg->byMode, pbyPkgIdx, sizeof(ptPkg->byMode));
		pbyPkgIdx += sizeof(ptPkg->byMode);
		memcpy(&ptPkg->dwDataSize, pbyPkgIdx, sizeof(ptPkg->dwDataSize));
		pbyPkgIdx += sizeof(ptPkg->dwDataSize);
		if (ptPkg->byMode & TRN_PKG_MODE_ENABLECRC)
		{
			memcpy(&ptPkg->dwDataCRC32, pbyPkgIdx, sizeof(ptPkg->dwDataCRC32));
			pbyPkgIdx += sizeof(ptPkg->dwDataCRC32);
		}
		memcpy(&ptPkg->dwMsgNumber, pbyPkgIdx, sizeof(ptPkg->dwMsgNumber));
		pbyPkgIdx += sizeof(ptPkg->dwMsgNumber);
		memcpy(&ptPkg->dwMsgTotal, pbyPkgIdx, sizeof(ptPkg->dwMsgTotal));
		pbyPkgIdx += sizeof(ptPkg->dwMsgTotal);
		memcpy(&ptPkg->byData, pbyPkgIdx, ptPkg->dwDataSize);
		pbyPkgIdx += ptPkg->dwDataSize;
		pbyPkgIdx += sizeof(ptPkg->byFrmEnd);

		if (iRead == (pbyPkgIdx - pbyPackage) * sizeof(BYTE))
		{
			if (ptPkg->byMode & TRN_PKG_MODE_ENABLECRC)
			{
				DWORD CRC32 = (DWORD)GetCRC32(ptPkg->byData, ptPkg->dwDataSize);
				if (CRC32 != ptPkg->dwDataCRC32)
				{
					delete [] pbyPackage;
					return false;
				}
			}
		}
		else
		{
			delete [] pbyPackage;
			return false;
		}

		delete [] pbyPackage;
		return true;
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

	SOCKET m_hSock, m_hSockChatMark; // m_hSockChatMark is the same as m_hSock
	SOCKADDR_IN m_tSockAddrIn;
	//IP_MREQ m_tMulticastIPMERQ;
	SOCKADDR_IN m_tSockAddrMulticast;
};