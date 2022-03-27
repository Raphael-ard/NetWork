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
		//std::string messageHead;
		std::string message;
		//std::string messageEnd;
		std::string sendIPv4;
		std::wstring sendIPv6;
		int sendPort;
	}messageData, *PMessageData;
}

#endif // !_ALL_DATA_H_
