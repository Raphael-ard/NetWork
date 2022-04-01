#include <conio.h>

#include "PointManager.h"
#include "IFactory.h"
#include "SendPointFactory.h"
#include "ReceivePoint.h"
#include "WebTCP.h"
#include "SubProcess.h"
#include "CaptureMethod.h"

int screen_size_change(int width, int height, int bitcount, void* param)
{
	printf("**** Screen Size Change.\n");
	return 0;
}

int frame_callback(NetWork::dp_frame_t* frame)
{
	NetWork::webTCP* web = reinterpret_cast<NetWork::webTCP*>(frame->param);
	////
	if (frame->rc_array && frame->rc_count > 0) {//屏幕有变化
		web->frame(frame);
	}
	return 0;
}

int main(int argc, char* argv[])
{
	printf("%s\n", *argv);
	printf("%d\n", argc);
	printf("%s\n", argv[argc - 1]);
	
	if (argc >= 2)
	{
		printf("Web Server\n");
		WSAData wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		NetWork::webTCP* web = new NetWork::webTCP();
		std::string Iv4;
		std::wstring Iv6;
		NetWork::getIP(Iv4, Iv6);

		web->start(Iv4);

		NetWork::dp_create_t ct;
		ct.grab_type = 0;
		ct.display_change = screen_size_change;
		ct.frame = frame_callback;
		ct.param = web;

		HANDLE mHandle = NetWork::dp_create(&ct);
		NetWork::dp_grab_interval(mHandle, 40); // 每秒25帧
		printf("\n\n[ESC] to exit\n\n");
		while (_getch() != 27)
			Sleep(1000);

		NetWork::dp_destroy(mHandle);

		WSACleanup();
	}
	else
	{
		int option = -1;
		std::cout << "Send: 1 Or Receive: Other:  ";
		std::cin >> option;

		if (option == 1)
		{// 工厂模式建造发送端
			std::shared_ptr<NetWork::IFactory> factory = std::shared_ptr<NetWork::IFactory>(new NetWork::sendPointFactory);
			if (!factory)
			{
				std::cout << "Create Send Facotry Failed" << std::endl;
				return -1;
			}
			std::cout << "This is Send" << std::endl;
			std::shared_ptr<NetWork::sendPoint> sPoint = factory->createSendPoint();
			NetWork::pointManager::getInstance()->addSendPoint(std::shared_ptr<NetWork::sendPoint>(sPoint));
			NetWork::pointManager::getInstance()->runProgress();
		}
		else
		{// 单例模式建造接收端
			std::cout << "This is Receiver" << std::endl;

			NetWork::receivePoint* recvPoint = NetWork::receivePoint::getInstace();
			recvPoint->startReceiver();
			// 开启新进程，父进程接收消息，子进程投屏
			std::cout << "输入T开始投屏: " << std::endl;

			while (true)
			{
				auto inputStr = _getch();
				if (inputStr == 116)
				{
					// 启动新进程，投屏
					NetWork::subProcess* sProgress = NetWork::subProcess::getInstance();
					sProgress->startProcess(L"SRMain.exe");
				}
				Sleep(1000);
			}
		}
	}

	return 0;
}