#include <iostream>

#include "ReceivePoint.h"
#include <fstream>

typedef struct _messageData
{
	//std::string messageHead;
	char* message[1024];
	//std::string messageEnd;
	char sendIPv4[64];
	wchar_t sendIPv6[64];
	int sendPort;
}messageData, * PMessageData;

void setIP(PMessageData mData)
{
	char IPv4[] = "192.168.43.79";
	wchar_t IPv6[] = L"fe80::c1ee:7b8b:e5c:ab73%23";
	strncpy_s(mData->sendIPv4, 64, IPv4, strlen(IPv4) + 1);
	wcsncpy_s(mData->sendIPv6, 64, IPv6, wcslen(IPv6) + 1);
	return;
}

int main(void)
{
	PMessageData mData = (PMessageData)malloc(sizeof(messageData));

	mData->sendPort = 9008;
	setIP(mData);
	std::fstream fs;
	fs.open("tar.cpp", std::ios::binary | std::ios::in);
	if (!fs)
	{
		std::cout << "Open Error" << std::endl;
		return -1;
	}
	int count = 0;
	while (!fs.eof())
	{
		char* temp = (char*)malloc(sizeof(char) * 1024);
		fs.read(temp, 1024);
		mData->message[count] = temp;
	}
	fs.close();

	std::cout << mData->sendIPv4 << std::endl;
	std::wcout << mData->sendIPv6 << std::endl;
	std::cout << mData->sendPort << std::endl;
	while (mData->message[count])
	{
		std::cout << mData->message[count];
		delete mData->message[count];
		mData->message[count] = nullptr;
		--count;
	}
	return 0;
}