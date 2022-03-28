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
	memset(mData->sendIPv4, 0, 64);
	memset(mData->sendIPv6, 0, 64);
	memset(mData->message, 0, 1024);

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
		memset(temp, 0, 1024);
		fs.read(temp, 1024);
		mData->message[count] = temp;
		++count;
	}
	fs.close();

	std::cout << mData->sendIPv4 << std::endl;
	std::wcout << mData->sendIPv6 << std::endl;
	std::cout << mData->sendPort << std::endl;
	std::fstream fs1;
	fs1.open("tar1.cpp", std::fstream::in | std::fstream::out | std::fstream::app
			| std::fstream::binary);
	if (!fs1)
	{
		std::cout << "Open Error" << std::endl;
		return -1;
	}
	int i = 0;

	while (i < count)
	{
		auto x = mData->message[i];
		std::cout << mData->message[i];
		if (fs1.is_open())
		{
			fs1.write(mData->message[i], 1024);
		}
		delete mData->message[i];
		mData->message[i] = nullptr;
		++i;
	}
	fs1.close();
	return 0;
}