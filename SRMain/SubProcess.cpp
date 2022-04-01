#include "SubProcess.h"
#include "CaptureMethod.h"

NetWork::subProcess::
subProcess(void) :_hChiIn(nullptr), _hChiOut(nullptr),
_hParIn(nullptr), _hParOut(nullptr)
{
	_sa.bInheritHandle = true;
	_sa.lpSecurityDescriptor = nullptr;
	_sa.nLength = sizeof(SECURITY_ATTRIBUTES);

	if (!::CreatePipe(&_hChiIn, &_hChiOut, &_sa, 0))
		return;

	// B ---> A InWrite ---> InRead
	if (!::CreatePipe(&_hParIn, &_hParOut, &_sa, 0))
		return;
}

NetWork::subProcess::~subProcess(void)
{
	::CloseHandle(_hChiIn);
	::CloseHandle(_hChiOut);
	::CloseHandle(_hParIn);
	::CloseHandle(_hParOut);
}

void 
NetWork::subProcess::
startProgress(std::wstring subName)
{
	STARTUPINFO start_info = { 0 };
	ZeroMemory(&start_info, sizeof(STARTUPINFO));
	start_info.cb = sizeof(STARTUPINFO);
	start_info.dwFlags |= STARTF_USESTDHANDLES;
	start_info.hStdOutput = _hChiIn;
	start_info.hStdError = _hChiIn;
	start_info.hStdInput = _hParOut;

	PROCESS_INFORMATION process_info = { 0 };
	ZeroMemory(&process_info, sizeof(process_info));

	std::wstring path = L"F:\\CPP\\NetWork\\x64\\Debug\\" + subName;

	if (::CreateProcess(path.c_str(), nullptr, nullptr, nullptr, true,
		CREATE_NEW_CONSOLE, nullptr, nullptr, &start_info, &process_info))
	{
		::WaitForSingleObject(process_info.hProcess, INFINITE);

		unsigned long ulExitCode = 0ul;
		if (::GetExitCodeProcess(process_info.hProcess, &ulExitCode))
			if (ulExitCode == 0)
				printf("success\n");
			else
				printf("failed\n");
		::CloseHandle(process_info.hThread);
		::CloseHandle(process_info.hProcess);
	}
}

void 
NetWork::subProcess::writePipe(std::string mes)
{
	unsigned long bytes_read = 0ul;
	while (true)
	{
		if (mes.compare("[ESC]") == 0)
		{
			::WriteFile(_hChiOut, mes.c_str(), mes.size(), &bytes_read, nullptr);
			break;
		}
	}
}

void 
NetWork::subProcess::readPipe(HANDLE mHandle)
{
	unsigned long bytes_read = 0ul;
	while (true)
	{
		char _tmp[64] = { 0 };

		if (::ReadFile(_hChiIn, _tmp, 64, &bytes_read, nullptr) != 1) //client cout未完成时阻塞
		{
			// 通过管道发送退出消息
			if (strcmp(_tmp, "[ESC]") == 0)
			{
				NetWork::dp_destroy(mHandle);
				break;
			}
		}
	}
}
