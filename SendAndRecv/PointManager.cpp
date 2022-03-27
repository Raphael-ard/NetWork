#include "PointManager.h"

NetWork::pointManager* 
NetWork::pointManager::_pointMgr = nullptr;

std::mutex
NetWork::pointManager::_mx;

NetWork::pointManager* 
NetWork::pointManager::
getInstance(void)
{
	if (_pointMgr == nullptr)
	{
		std::unique_lock lck(_mx);
		if (_pointMgr == nullptr)
		{
			_pointMgr = new pointManager();
		}
	}
	return _pointMgr;
}

void 
NetWork::pointManager::
deleteInstance(void)
{
	std::unique_lock lck(_mx);
	if (_pointMgr != nullptr)
	{
		delete _pointMgr;
		_pointMgr = nullptr;
	}
}

NetWork::pointManager::
pointManager(void)
{
}

NetWork::pointManager::
~pointManager(void)
{
}
