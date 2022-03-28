#pragma once
#ifndef _POINT_MANAGER_H_
#define _POINT_MANAGER_H_

#include <mutex>
#include <list>
#include <memory>

namespace NetWork
{
	class sendPoint;
	class pointManager
	{
	public:
		static std::shared_ptr<pointManager> getInstance(void);
		static void deleteInstance(void);

		void addSendPoint(std::shared_ptr<sendPoint> sPoint);
		void removeSendPoint(std::shared_ptr<sendPoint> sPoint);
		void clearSendPoint(void);

	private:
		pointManager();
		~pointManager();
		pointManager(pointManager const&) = delete;
		pointManager(pointManager&&) = delete;
		static void destory(pointManager* pMgr);

	private:
		std::list<std::shared_ptr<NetWork::sendPoint>>		_lstSPoint;
		static std::shared_ptr<pointManager>				_pMgr;
	};
}

#endif // !_POINT_MANAGER_H_
