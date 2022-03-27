#pragma once
#ifndef _RECEIVE_POINT_H_
#define _RECEIVE_POINT_H_

#include <mutex>
#include <winsock2.h>
#include <unordered_map>
#include <vector>

#include "UserId.h"
#include "ThreadPool.h"

namespace NetWork
{
	// 使用单例模式创建
	class receivePoint
	{
	public:
		static receivePoint* getInstance(void);
		static void deleteInstance(void);

		void startServer(void);
		
	private:
		void runEvent(void);
		void getIp(std::string& str, int iResult);
		void transCode(void);
		bool sendToClip(const wchar_t* data_str);

	private:
		receivePoint(void);
		receivePoint(receivePoint const&) = delete;
		receivePoint(receivePoint&&) = delete;
		~receivePoint(void);

	private:
		static receivePoint*						_receive_point;
		static std::mutex							_mx;

		WSADATA										_wsaData;
		SOCKET										_sock;
		sockaddr_in									_addr{ 0 };

		HANDLE										_hIocp;
		NetWork::threadPool							_thdPool;
	};	
}

#endif // !_RECEIVE_POINT_H_
