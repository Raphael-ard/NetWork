#pragma once
#ifndef _ALL_DATA_H_
#define _ALL_DATA_H_
#define TAR "tar.cpp"

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
}

#endif // !_ALL_DATA_H_
