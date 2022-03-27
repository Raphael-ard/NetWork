#pragma once
#ifndef _POINT_MANAGER_H_
#define _POINT_MANAGER_H_

#include <mutex>

namespace NetWork
{
	class pointManager
	{
	public:
		static pointManager* getInstance(void);
		static void deleteInstance(void);
	private:
		pointManager();
		~pointManager();
		pointManager(pointManager const&) = delete;
		pointManager(pointManager&&) = delete;

	private:
		static pointManager*		_pointMgr;
		static std::mutex			_mx;

	};
}

#endif // !_POINT_MANAGER_H_
