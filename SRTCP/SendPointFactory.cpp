#include "SendPointFactory.h"
#include "SendPoint.h"

NetWork::sendPointFactory::
sendPointFactory(void)
{
}

NetWork::sendPointFactory::
~sendPointFactory(void)
{
}

std::shared_ptr<NetWork::sendPoint> 
NetWork::sendPointFactory::
createSendPoint(void)
{
	return std::shared_ptr<NetWork::sendPoint>(new NetWork::sendPoint);
}
