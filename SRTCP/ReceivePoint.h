#pragma once
#ifndef _RECEIVE_POINT_H_
#define _RECEIVE_POINT_H_

#include <mutex>
#include <winsock2.h>
#include <mswsock.h>

#include "AllData.h"
#include "ThreadPool.h"

namespace NetWork
{
	// 使用单例模式创建
	class receivePoint
	{
	public:
		static receivePoint* getInstace(void);
		static void destoryIns(void);
		void startReceiver(void);
		
		WSADATA getWSAData(void);
		std::string IPv4;

	private:
		void threadEvent(void);
		void runEvent(NetWork::PIOData tpIoData, NetWork::PCompletionKey pComKey);
		void transCode(void);
		bool sendToClip(const wchar_t* data_str);

		BOOL Recv(NetWork::PIOData tpIoData, NetWork::PCompletionKey pComKey);
		BOOL Send(NetWork::PIOData tpIoData, NetWork::PCompletionKey pComKey);

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

		LPFN_ACCEPTEX								lpfnAcceptEx;
		DWORD										dwBytes;
		GUID										guidAcceptEx;

		DWORD										dwBytes1;
		LPFN_GETACCEPTEXSOCKADDRS					lpfnGetAcceptExSockaddrs;
		GUID										guidGetAcceptExSockaddrs;
		
		HANDLE										_hIocp;
		NetWork::threadPool							_thdPool;
	};	
}

#endif // !_RECEIVE_POINT_H_
