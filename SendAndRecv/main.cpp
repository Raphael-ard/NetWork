#include <iostream>
#include <memory>

#include "PointManager.h"
#include "IFactory.h"
#include "SendPointFactory.h"
#include "ReceivePoint.h"

int main(void)
{
	int option = -1;
	std::cout << "Send: 1 Or Receive: Other";
	std::cin >> option;
	if (option == 1)
	{// ����ģʽ���췢�Ͷ�
		std::shared_ptr<NetWork::IFactory> factory = std::shared_ptr<NetWork::IFactory>(new NetWork::sendPointFactory);
		if (!factory)
		{
			std::cout << "Create Send Facotry Failed" << std::endl;
			return -1;
		}
		std::shared_ptr<NetWork::sendPoint> sPoint = factory->createSendPoint();
		NetWork::pointManager::getInstance()->addSendPoint(std::shared_ptr<NetWork::sendPoint>(sPoint));

	}
	else
	{// ����ģʽ������ն�
		NetWork::receivePoint* recvPoint = NetWork::receivePoint::getInstace();
		recvPoint->startServer();
	}
	return 0;
}