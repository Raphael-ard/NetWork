#include "SubProgress.h"
#include <memory>

using namespace NetWork;

subProgress*
NetWork::subProgress::getInstance(void)
{
	if (_sProgress == nullptr)
	{
		std::unique_lock lck(_mx);
		if (_sProgress == nullptr)
		{
			_sProgress = new subProgress;
		}
	}
	return _sProgress;
}

void
NetWork::subProgress::destory(void)
{
	std::unique_lock lck(_mx);
	if (_sProgress != nullptr)
		delete _sProgress;
	_sProgress = nullptr;
}

NetWork::subProgress::
subProgress(void)
{
}

NetWork::subProgress::
~subProgress(void)
{
}
