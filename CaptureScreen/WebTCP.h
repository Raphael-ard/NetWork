#pragma once
#ifndef _WEB_TCP_H_

namespace NetWork
{
	class webTCP
	{
	public:
		webTCP(void);
		~webTCP(void);
		////
		int start(std::string& IPv4);
		void setJpegQuality(int quality);
		void frame(struct dp_frame_t* frame);

	protected:
		static DWORD CALLBACK  acceptThread(void* _p);
		static DWORD CALLBACK  clientThread(void* _p);

	protected:
		SOCKET										_sock;
		sockaddr_in									_addr{ 0 };

		bool										_quit;
		std::vector<SOCKET>							_socks;
		int											_jpegQuality;
		CRITICAL_SECTION							cs;
	};
}

#endif // !_WEB_TCP_H_
