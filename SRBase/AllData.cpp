#include "AllData.h"

using namespace NetWork;

char* NetWork::messageToChar(PMessageData mdata)
{
	// 只将IPv4地址和消息内容放进去
	LONG lengthHeap = 0l;
	int count = 0;

	while (mdata->message[count])
	{
		lengthHeap += 1024;
		++count;
	}

	char* value = (char*)malloc(sizeof(char) * lengthHeap);
	memset(value, 0, lengthHeap);

	for (int i = 0; i < count; ++i)
	{
		strncat_s(value, 1024, mdata->message[i], 1024);
		free(mdata->message[i]);
		mdata->message[i] = nullptr;
	}

	return value;
}

void NetWork::charToMessage(PVOID data, std::string& message)
{
	char* cData = reinterpret_cast<char*>(data);
	int i = 0;
	std::string messageLen;
	while (cData[i] != ':')
	{
		messageLen += cData[i];
		++i;
	}
	i += 1;
	LONG mLen = atol(messageLen.c_str());
	while (i < mLen)
	{
		message += cData[i];
		++i;
	}
}

void 
NetWork::getIP()
{
	INT iRetval;
	DWORD dwRetval;
	int i = 1;
	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;

	/*in_addr*		inAddr;
	char str[64] = { 0 };*/
	struct sockaddr_in* sockaddr_ipv4;
	//    struct sockaddr_in6 *sockaddr_ipv6;
	LPSOCKADDR sockaddr_ip;

	wchar_t ipstringbuffer[46];
	DWORD ipbufferlength = 46;


	//  存放主机名的缓冲区
	char szHost[256];
	//  取得本地主机名称
	::gethostname(szHost, 256);

	//-------------------------------
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	dwRetval = getaddrinfo(szHost, nullptr, &hints, &result);
	if (dwRetval != 0) {
		printf("getaddrinfo failed with error: %d\n", dwRetval);
		WSACleanup();
		return;
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		switch (ptr->ai_family) {
		case AF_UNSPEC:
			printf("Unspecified\n");
			break;
		case AF_INET:
			printf("AF_INET (IPv4)\n");
			/*inAddr = reinterpret_cast<in_addr*>(ptr->ai_addr);
			::inet_ntop(AF_INET, inAddr, str, 64);
			printf("IPv4 address %s\n", str);
			IPv4.assign(str);*/
			sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
			IPv4.assign(inet_ntoa(sockaddr_ipv4->sin_addr));
			printf("IPv4 address %s\n", IPv4.c_str());
			break;
		case AF_INET6:
			printf("AF_INET6 (IPv6)\n");
			sockaddr_ip = (LPSOCKADDR)ptr->ai_addr;
			ipbufferlength = 46;
			iRetval = WSAAddressToString(sockaddr_ip, (DWORD)ptr->ai_addrlen, NULL,
				ipstringbuffer, &ipbufferlength);
			if (iRetval)
				printf("WSAAddressToString failed with %u\n", WSAGetLastError());
			else
			{
				IPv6.assign(ipstringbuffer);
				std::wcout << L"IPv6 address  " << ipstringbuffer << std::endl;
			}
			break;
		case AF_NETBIOS:
			printf("AF_NETBIOS (NetBIOS)\n");
			break;
		default:
			printf("Other %ld\n", ptr->ai_family);
			break;
		}
	}
	freeaddrinfo(result);
}
