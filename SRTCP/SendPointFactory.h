#pragma once
#ifndef _SEND_POINT_FACTORY_H_
#define _SEND_POINT_FACTORY_H_

#include "IFactory.h"

namespace NetWork
{
	class sendPointFactory : public NetWork::IFactory
	{
	public:
		sendPointFactory(void);
		~sendPointFactory(void);
		virtual std::shared_ptr<NetWork::sendPoint> createSendPoint(void) override;
	};
}

#endif // !_SEND_POINT_FACTORY_H_
