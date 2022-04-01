#pragma once
#ifndef _SUB_PROGRESS_H_
#define _SUB_PROGRESS_H_

#include <mutex>

namespace NetWork
{
	class subProgress
	{
	public:
		static subProgress* getInstance(void);
		static void destory(void);

	private:
		subProgress(void);
		~subProgress(void);
		subProgress(subProgress const&) = delete;
		subProgress(subProgress&&) = delete;

	private:
		static subProgress* _sProgress;
		static std::mutex	_mx;
	};
}

#endif // !_SUB_PROGRESS_H_
