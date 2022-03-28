#pragma once
#ifndef _RECEIVE_POINT_H_
#define _RECEIVE_POINT_H_

#include <mutex>
#include <winsock2.h>

#include "UserId.h"
#include "ThreadPool.h"

namespace NetWork
{
	// 使用单例模式创建
	class receivePoint
	{
	public:
		static receivePoint* getInstace(void);
		static void destoryIns(void);
		void startServer(void);
		
	private:
		void runEvent(void);
		void getIp(std::string& str, int iResult);
		void transCode(void);
		bool sendToClip(const wchar_t* data_str);

	private:
		receivePoint(void);
		~receivePoint(void);
		receivePoint(receivePoint const&) = delete;
		receivePoint(receivePoint&&) = delete;


	private:
		static std::mutex							_mx;
		static receivePoint*						_rcvPoint;

		WSADATA										_wsaData;
		SOCKET										_sock;
		sockaddr_in									_addr{ 0 };

		HANDLE										_hIocp;
		NetWork::threadPool							_thdPool;
	};	
}

#endif // !_RECEIVE_POINT_H_
