#include "SubProcess.h"
#include "CaptureMethod.h"

std::mutex
NetWork::subProcess::_mx;

NetWork::subProcess*
NetWork::subProcess::_s = nullptr;


NetWork::subProcess::
subProcess(void)
{
	process_info = { 0 };
	ZeroMemory(&process_info, sizeof(process_info));
}

NetWork::subProcess::~subProcess(void)
{
	::CloseHandle(process_info.hThread);
	::CloseHandle(process_info.hProcess);
}

NetWork::subProcess*
NetWork::subProcess::getInstance(void)
{
	if (_s == nullptr)
	{
		std::unique_lock lck(_mx);
		if (_s == nullptr)
		{
			_s = new subProcess();
		}
	}
	return _s;
}

void
NetWork::subProcess::destory(void)
{
	std::unique_lock lck(_mx);
	if (_s != nullptr)
		delete _s;
	_s = nullptr;
}

void
NetWork::subProcess::
startProcess(std::wstring subName)
{
	STARTUPINFO si = { sizeof(STARTUPINFO) };

	WCHAR nameCMD[] = L"SRMain.exe -1";

	if (::CreateProcess(nullptr, nameCMD, nullptr, nullptr, true,
		CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &process_info))
	{
		printf("Create Success.\n");
		// 等待子进程退出
		::WaitForSingleObject(process_info.hProcess, INFINITE);

		unsigned long ulExitCode = 0ul;
		if (::GetExitCodeProcess(process_info.hProcess, &ulExitCode))
		{
			if (ulExitCode == 0)
				printf("success\n");
			else
				printf("failed\n");
			::CloseHandle(process_info.hThread);
			::CloseHandle(process_info.hProcess);
		}
		
		return;
	}
	printf("Create Failed.\n");
}
