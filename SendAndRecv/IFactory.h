#pragma once
#ifndef _IFACTORY_H_
#define _IFACTORY_H_

#include <memory>

namespace NetWork
{
	class sendPoint;
	class IFactory
	{
	public:
		virtual std::shared_ptr<NetWork::sendPoint> createSendPoint(void) = 0;
	};
}

#endif // !_IFACTORY_H_
