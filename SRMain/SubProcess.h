#pragma once
#ifndef _SUB_PROCESS_H_
#define _SUB_PROCESS_H_

#include <Windows.h>
#include <string>
#include <mutex>

namespace NetWork
{
	class subProcess
	{
	public:
		static subProcess* getInstance(void);
		static void destory(void);

		void startProcess(std::wstring subName);
		/*void writePipe(std::string mes);
		void readPipe(HANDLE mHandle);*/

	private:
		subProcess(void);
		~subProcess(void);
		subProcess(subProcess const&) = delete;
		subProcess(subProcess&&) = delete;

	private:
		static subProcess*		_s;
		static std::mutex		_mx;

		/*HANDLE					_hParOut;
		HANDLE					_hParIn;
		HANDLE					_hChiOut;
		HANDLE					_hChiIn;
		SECURITY_ATTRIBUTES		_sa;*/

		PROCESS_INFORMATION		process_info;
	};
}

#endif // !_SUB_PROCESS_H_
