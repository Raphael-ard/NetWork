#pragma once
#ifndef _SEND_POINT_H_
#define _SEND_POINT_H_

#include <string>
#include <WinSock2.h>
#include "AllData.h"

namespace NetWork
{
	class sendPoint
	{
	public:
		sendPoint(void);
		~sendPoint(void);
		void startConnect(void);

	private:
		std::wstring getMyDirectory(void);
		void initOperateFile(void);
		void listenFile(void);
		void getIP(void);

	private:
		WSADATA					_wsaData;
		SOCKET					_sock;
		sockaddr_in				_addr{ 0 };
		std::wstring			_path;
		HANDLE					m_hDirectory;
		HANDLE					hFile;
		FILE_NOTIFY_INFORMATION* pNotification;
		char					notify[1024];
		NetWork::PMessageData			_mData;
	};
}

#endif // !_SEND_POINT_H_
