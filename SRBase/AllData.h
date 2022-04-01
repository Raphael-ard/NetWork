#pragma once
#ifndef _ALL_DATA_H_
#define _ALL_DATA_H_
#define TAR "tar.cpp"

#include <iostream>
#include <WS2tcpip.h>

namespace NetWork
{
	enum statusData
	{
		ConnectSuccess = 1,
	};
	typedef struct _messageData
	{
		char* message[1024];
		char sendIPv4[64];
		wchar_t sendIPv6[64];
		int sendPort;
	}messageData, * PMessageData;
	
	typedef struct _IOData
	{
		OVERLAPPED	overlapped;
		WSABUF		buf;
		char		dataBuffer[1024];
		int			dataLength;
		int			operatorType;
		SOCKET		csock;
	}IOData, *PIOData;
	typedef struct _CompletionKey
	{
		SOCKET sock;
		char IP[64];
	}CompletionKey, *PCompletionKey;

	char* messageToChar(PMessageData mdata);

	void charToMessage(PVOID data, std::string& message);

	void getIP(std::string& IPv4, std::wstring& IPv6);
}

#endif // !_ALL_DATA_H_
