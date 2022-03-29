#include "PointManager.h"
#include "SendPoint.h"

std::shared_ptr<NetWork::pointManager>
NetWork::pointManager::_pMgr;

std::shared_ptr<NetWork::pointManager>
NetWork::pointManager::
getInstance(void)
{
	if (!_pMgr)
		_pMgr.reset(new NetWork::pointManager, &NetWork::pointManager::destory);
	return _pMgr;
}

void 
NetWork::pointManager::
deleteInstance(void)
{
	if (_pMgr)
		_pMgr.reset();
}

void 
NetWork::pointManager::
runProgress(void)
{
	(*_lstSPoint.back()).startConnect();
}

void
NetWork::pointManager::
addSendPoint(std::shared_ptr<sendPoint> sPoint)
{
	if (!sPoint)
		return;
	_lstSPoint.emplace_back(sPoint);
}

void 
NetWork::pointManager::
removeSendPoint(std::shared_ptr<sendPoint> sPoint)
{
	if (!sPoint)
		return;
	if (!_lstSPoint.empty())
		return;
	for (auto it = _lstSPoint.begin(); it != _lstSPoint.end();)
	{
		if (*it == sPoint)
		{
			it = _lstSPoint.erase(it)
				;
			continue;
		}
		++it;
	}
}

void 
NetWork::pointManager::
clearSendPoint(void)
{
	_lstSPoint.clear();
}



NetWork::pointManager::
pointManager(void)
{
}

NetWork::pointManager::
~pointManager(void)
{
}

void 
NetWork::pointManager::
destory(pointManager* pMgr)
{
	if (pMgr)
		delete pMgr;
}
