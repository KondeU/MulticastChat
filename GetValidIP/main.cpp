////// Example 1

//#include <WinSock2.h>
//#include <IPHlpApi.h>
//#pragma comment(lib, "IpHlpApi.lib")
//
//#include <iostream>
//using namespace std;
//
//// 函数声明
//void output(PIP_ADAPTER_INFO pIpAdapterInfo);
//
//// 程序入口
//int main(int argc, char * argv[])
//{
//	// PIP_ADAPTER_INFO结构体指针存储本机网卡信息
//	PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
//
//	// 得到结构体大小,用于GetAdaptersInfo参数
//	unsigned long stSize = sizeof(IP_ADAPTER_INFO);
//
//	// 调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量;其中stSize参数既是一个输入量也是一个输出量
//	int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
//
//	if (ERROR_BUFFER_OVERFLOW == nRel)
//	{
//		// 如果函数返回的是ERROR_BUFFER_OVERFLOW
//		// 则说明GetAdaptersInfo参数传递的内存空间不够,同时其传出stSize,表示需要的空间大小
//		// 这也是说明为什么stSize既是一个输入量也是一个输出量
//		// 释放原来的内存空间
//		delete pIpAdapterInfo;
//
//		//重新申请内存空间用来存储所有网卡信息
//		pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
//
//		//再次调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量
//		nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);
//	}
//
//	if (ERROR_SUCCESS == nRel)
//	{
//		// 输出网卡信息
//		output(pIpAdapterInfo);
//	}
//
//	// 释放内存空间
//	if (pIpAdapterInfo)
//	{
//		delete pIpAdapterInfo;
//	}
//
//	system("pause");
//
//	return 0;
//}
//
//// 函数作用,输出网卡信息
//void output(PIP_ADAPTER_INFO pIpAdapterInfo)
//{
//	// 可能有多网卡,因此通过循环去判断
//	while (pIpAdapterInfo)
//	{
//		cout << "网卡名称：" << pIpAdapterInfo->AdapterName << endl;
//		cout << "网卡描述：" << pIpAdapterInfo->Description << endl;
//		cout << "网卡MAC地址：" << pIpAdapterInfo->Address;
//		for (UINT i = 0; i < pIpAdapterInfo->AddressLength; i++)
//		{
//			if (i == pIpAdapterInfo->AddressLength - 1)
//			{
//				printf("%02x\n", pIpAdapterInfo->Address[i]);
//			}
//			else
//			{
//				printf("%02x-", pIpAdapterInfo->Address[i]);
//			}
//		}
//
//		cout << "网卡IP地址如下：" << endl;
//		// 可能网卡有多IP,因此通过循环去判断
//		IP_ADDR_STRING *pIpAddrString = &(pIpAdapterInfo->IpAddressList);
//		do
//		{
//			cout << pIpAddrString->IpAddress.String << endl;
//			pIpAddrString = pIpAddrString->Next;
//		}
//		while (pIpAddrString);
//		pIpAdapterInfo = pIpAdapterInfo->Next;
//		cout << "*****************************************************" << endl;
//	}
//	return;
//}

////// Example 2

//#include <winsock2.h>
//#include <iphlpapi.h>
//#include <stdio.h>
//#include <stdlib.h>
//#pragma comment(lib, "IPHLPAPI.lib")
//
//#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
//#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
//
///* Note: could also use malloc() and free() */
//
//int __cdecl main()
//{
//
//	/* Declare and initialize variables */
//
//	// It is possible for an adapter to have multiple
//	// IPv4 addresses, gateways, and secondary WINS servers
//	// assigned to the adapter. 
//	//
//	// Note that this sample code only prints out the 
//	// first entry for the IP address/mask, and gateway, and
//	// the primary and secondary WINS server for each adapter. 
//
//	PIP_ADAPTER_INFO pAdapterInfo;
//	PIP_ADAPTER_INFO pAdapter = NULL;
//	DWORD dwRetVal = 0;
//	UINT i;
//
//	/* variables used to print DHCP time info */
//	struct tm newtime;
//	char buffer[32];
//	errno_t error;
//
//	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
//	pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(sizeof(IP_ADAPTER_INFO));
//	if (pAdapterInfo == NULL) {
//		printf("Error allocating memory needed to call GetAdaptersinfo\n");
//		return 1;
//	}
//	// Make an initial call to GetAdaptersInfo to get
//	// the necessary size into the ulOutBufLen variable
//	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
//		FREE(pAdapterInfo);
//		pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(ulOutBufLen);
//		if (pAdapterInfo == NULL) {
//			printf("Error allocating memory needed to call GetAdaptersinfo\n");
//			return 1;
//		}
//	}
//
//	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
//		pAdapter = pAdapterInfo;
//		while (pAdapter) {
//			printf("\tComboIndex: \t%d\n", pAdapter->ComboIndex);
//			printf("\tAdapter Name: \t%s\n", pAdapter->AdapterName);
//			printf("\tAdapter Desc: \t%s\n", pAdapter->Description);
//			printf("\tAdapter Addr: \t");
//			for (i = 0; i < pAdapter->AddressLength; i++) {
//				if (i == (pAdapter->AddressLength - 1))
//					printf("%.2X\n", (int)pAdapter->Address[i]);
//				else
//					printf("%.2X-", (int)pAdapter->Address[i]);
//			}
//			printf("\tIndex: \t%d\n", pAdapter->Index);
//			printf("\tType: \t");
//			switch (pAdapter->Type) {
//			case MIB_IF_TYPE_OTHER:
//				printf("Other\n");
//				break;
//			case MIB_IF_TYPE_ETHERNET:
//				printf("Ethernet\n");
//				break;
//			case MIB_IF_TYPE_TOKENRING:
//				printf("Token Ring\n");
//				break;
//			case MIB_IF_TYPE_FDDI:
//				printf("FDDI\n");
//				break;
//			case MIB_IF_TYPE_PPP:
//				printf("PPP\n");
//				break;
//			case MIB_IF_TYPE_LOOPBACK:
//				printf("Lookback\n");
//				break;
//			case MIB_IF_TYPE_SLIP:
//				printf("Slip\n");
//				break;
//			default:
//				printf("Unknown type %ld\n", pAdapter->Type);
//				break;
//			}
//
//			printf("\tIP Address: \t%s\n",
//				pAdapter->IpAddressList.IpAddress.String);
//			printf("\tIP Mask: \t%s\n", pAdapter->IpAddressList.IpMask.String);
//
//			printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String);
//			printf("\t***\n");
//
//			if (pAdapter->DhcpEnabled) {
//				printf("\tDHCP Enabled: Yes\n");
//				printf("\t  DHCP Server: \t%s\n",
//					pAdapter->DhcpServer.IpAddress.String);
//
//				printf("\t  Lease Obtained: ");
//				/* Display local time */
//				error = _localtime32_s(&newtime, (__time32_t*)&pAdapter->LeaseObtained);
//				if (error)
//					printf("Invalid Argument to _localtime32_s\n");
//				else {
//					// Convert to an ASCII representation 
//					error = asctime_s(buffer, 32, &newtime);
//					if (error)
//						printf("Invalid Argument to asctime_s\n");
//					else
//						/* asctime_s returns the string terminated by \n\0 */
//						printf("%s", buffer);
//				}
//
//				printf("\t  Lease Expires:  ");
//				error = _localtime32_s(&newtime, (__time32_t*)&pAdapter->LeaseExpires);
//				if (error)
//					printf("Invalid Argument to _localtime32_s\n");
//				else {
//					// Convert to an ASCII representation 
//					error = asctime_s(buffer, 32, &newtime);
//					if (error)
//						printf("Invalid Argument to asctime_s\n");
//					else
//						/* asctime_s returns the string terminated by \n\0 */
//						printf("%s", buffer);
//				}
//			}
//			else
//				printf("\tDHCP Enabled: No\n");
//
//			if (pAdapter->HaveWins) {
//				printf("\tHave Wins: Yes\n");
//				printf("\t  Primary Wins Server:    %s\n",
//					pAdapter->PrimaryWinsServer.IpAddress.String);
//				printf("\t  Secondary Wins Server:  %s\n",
//					pAdapter->SecondaryWinsServer.IpAddress.String);
//			}
//			else
//				printf("\tHave Wins: No\n");
//			pAdapter = pAdapter->Next;
//			printf("\n");
//		}
//	}
//	else {
//		printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
//
//	}
//	if (pAdapterInfo)
//		FREE(pAdapterInfo);
//
//	system("pause");
//
//	return 0;
//}

////// Example 3

//#define _WINSOCK_DEPRECATED_NO_WARNINGS
////#include <stdio.h>
////#include <winsock2.h>
////#pragma comment(lib,"ws2_32.lib")
////int doit(int, char **)
////{
////	char host_name[255];
////	//获取本地主机名称
////	if (gethostname(host_name, sizeof(host_name)) == SOCKET_ERROR) {
////		printf("Error %d when getting local host name.\n", WSAGetLastError());
////		return 1;
////	}
////	printf("Host name is: %s\n", host_name);
////	//从主机名数据库中得到对应的“主机”
////	struct hostent *phe = gethostbyname(host_name);
////	if (phe == 0) {
////		printf("Yow! Bad host lookup.");
////		return 1;
////	}
////	//循环得出本地机器所有IP地址
////	for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
////		struct in_addr addr;
////		memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
////		printf("Address %d : %s\n", i + 1, inet_ntoa(addr));
////		
////	}
////	return 0;
////}
////int main(int argc, char *argv[])
////{
////	WSAData wsaData;
////	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
////		return 255;
////	}
////	int retval = doit(argc, argv);
////	WSACleanup();
////
////	system("pause");
////
////	return retval;
////}



#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "../CTransfer.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

// link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

int __cdecl main(int argc, char **argv)
{

	//-----------------------------------------
	// Declare and initialize variables
	WSADATA wsaData;
	int iResult;
	INT iRetval;

	DWORD dwRetval;

	int i = 1;

	struct addrinfo *result = NULL;
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;

	struct sockaddr_in  *sockaddr_ipv4;
	//    struct sockaddr_in6 *sockaddr_ipv6;
	LPSOCKADDR sockaddr_ip;

	TCHAR ipstringbuffer[46];
	DWORD ipbufferlength = 46;

	// Validate the parameters
	if (argc != 3) {
		printf("usage: %s <hostname> <servicename>\n", argv[0]);
		printf("getaddrinfo provides protocol-independent translation\n");
		printf("   from an ANSI host name to an IP address\n");
		printf("%s example usage\n", argv[0]);
		printf("   %s www.contoso.com 0\n", argv[0]);
		return 1;
	}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	//--------------------------------
	// Setup the hints address info structure
	// which is passed to the getaddrinfo() function
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;//AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;//SOCK_STREAM;
	hints.ai_protocol = IPPROTO_UDP;

	printf("Calling getaddrinfo with following parameters:\n");
	printf("\tnodename = %s\n", argv[1]);
	printf("\tservname (or port) = %s\n\n", argv[2]);

	//--------------------------------
	// Call getaddrinfo(). If the call succeeds,
	// the result variable will hold a linked list
	// of addrinfo structures containing response
	// information
	dwRetval = getaddrinfo(argv[1], nullptr, &hints, &result);
	if (dwRetval != 0) {
		printf("getaddrinfo failed with error: %d\n", dwRetval);
		WSACleanup();
		return 1;
	}

	printf("getaddrinfo returned success\n");

	// Retrieve each address and print out the hex bytes
	for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {

		printf("getaddrinfo response %d\n", i++);
		printf("\tFlags: 0x%x\n", ptr->ai_flags);
		printf("\tFamily: ");
		switch (ptr->ai_family) {
		case AF_UNSPEC:
			printf("Unspecified\n");
			break;
		case AF_INET:
			printf("AF_INET (IPv4)\n");
			sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
			printf("\tIPv4 address %s\n",
				inet_ntoa(sockaddr_ipv4->sin_addr));
			break;
		case AF_INET6:
			printf("AF_INET6 (IPv6)\n");
			// the InetNtop function is available on Windows Vista and later
			// sockaddr_ipv6 = (struct sockaddr_in6 *) ptr->ai_addr;
			// printf("\tIPv6 address %s\n",
			//    InetNtop(AF_INET6, &sockaddr_ipv6->sin6_addr, ipstringbuffer, 46) );

			// We use WSAAddressToString since it is supported on Windows XP and later
			sockaddr_ip = (LPSOCKADDR)ptr->ai_addr;
			// The buffer length is changed by each call to WSAAddresstoString
			// So we need to set it for each iteration through the loop for safety
			ipbufferlength = 46;
			iRetval = WSAAddressToString(sockaddr_ip, (DWORD)ptr->ai_addrlen, NULL,
				ipstringbuffer, &ipbufferlength);
			if (iRetval)
				printf("WSAAddressToString failed with %u\n", WSAGetLastError());
			else
				printf("\tIPv6 address %s\n", ipstringbuffer);
			break;
		case AF_NETBIOS:
			printf("AF_NETBIOS (NetBIOS)\n");
			break;
		default:
			printf("Other %ld\n", ptr->ai_family);
			break;
		}
		printf("\tSocket type: ");
		switch (ptr->ai_socktype) {
		case 0:
			printf("Unspecified\n");
			break;
		case SOCK_STREAM:
			printf("SOCK_STREAM (stream)\n");
			break;
		case SOCK_DGRAM:
			printf("SOCK_DGRAM (datagram) \n");
			break;
		case SOCK_RAW:
			printf("SOCK_RAW (raw) \n");
			break;
		case SOCK_RDM:
			printf("SOCK_RDM (reliable message datagram)\n");
			break;
		case SOCK_SEQPACKET:
			printf("SOCK_SEQPACKET (pseudo-stream packet)\n");
			break;
		default:
			printf("Other %ld\n", ptr->ai_socktype);
			break;
		}
		printf("\tProtocol: ");
		switch (ptr->ai_protocol) {
		case 0:
			printf("Unspecified\n");
			break;
		case IPPROTO_TCP:
			printf("IPPROTO_TCP (TCP)\n");
			break;
		case IPPROTO_UDP:
			printf("IPPROTO_UDP (UDP) \n");
			break;
		default:
			printf("Other %ld\n", ptr->ai_protocol);
			break;
		}
		printf("\tLength of this sockaddr: %d\n", ptr->ai_addrlen);
		printf("\tCanonical name: %s\n", ptr->ai_canonname);
	}

	freeaddrinfo(result);
	WSACleanup();

	system("pause");

	CTransfer c;
	c.GetValidIP(nullptr);

	system("pause");

	return 0;
}