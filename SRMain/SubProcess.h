#pragma once
#ifndef _SUB_PROCESS_H_
#define _SUB_PROCESS_H_

#include <Windows.h>
#include <string>

namespace NetWork
{
	class subProcess
	{
	public:
		subProcess(void);
		~subProcess(void);

		void startProgress(std::wstring subName);
		void writePipe(std::string mes);
		void readPipe(HANDLE mHandle);

	private:
		HANDLE					_hParOut;
		HANDLE					_hParIn;
		HANDLE					_hChiOut;
		HANDLE					_hChiIn;

		SECURITY_ATTRIBUTES		_sa;
	};
}

#endif // !_SUB_PROCESS_H_
