#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
int doit(int, char **)
{
	char host_name[255];
	//��ȡ������������
	if (gethostname(host_name, sizeof(host_name)) == SOCKET_ERROR) {
		printf("Error %d when getting local host name.\n", WSAGetLastError());
		return 1;
	}
	printf("Host name is: %s\n", host_name);
	//�����������ݿ��еõ���Ӧ�ġ�������
	struct hostent *phe = gethostbyname(host_name);
	if (phe == 0) {
		printf("Yow! Bad host lookup.");
		return 1;
	}
	//ѭ���ó����ػ�������IP��ַ
	for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
		struct in_addr addr;
		memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		printf("Address %d : %s\n", i + 1, inet_ntoa(addr));
	}
	return 0;
}
int main1(int argc, char *argv[])
{
	WSAData wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		return 255;
	}
	int retval = doit(argc, argv);
	WSACleanup();

	system("pause");

	return retval;
}