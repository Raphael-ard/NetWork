#pragma once
#ifndef _ALL_DATA_H_
#define _ALL_DATA_H_
#define TAR "tar.cpp"

#include <iostream>
#include <WS2tcpip.h>

namespace NetWork
{
	enum statusData
	{
		ConnectSuccess = 1,
	};
	typedef struct _messageData
	{
		char* message[1024];
		char sendIPv4[64];
		wchar_t sendIPv6[64];
		int sendPort;
	}messageData, * PMessageData;
	
	typedef struct _IOData
	{
		OVERLAPPED	overlapped;
		WSABUF		buf;
		char		dataBuffer[1024];
		int			dataLength;
		int			operatorType;
		SOCKET		csock;
	}IOData, *PIOData;
	typedef struct _CompletionKey
	{
		SOCKET sock;
		char IP[64];
	}CompletionKey, *PCompletionKey;

	static char* messageToChar(PMessageData mdata)
	{
		// ֻ��IPv4��ַ����Ϣ���ݷŽ�ȥ
		LONG lengthHeap = 0l;
		int count = 0;

		while (mdata->message[count])
		{
			lengthHeap += 1024;
			++count;
		}
		
		char* value = (char*)malloc(sizeof(char) * lengthHeap);
		memset(value, 0, lengthHeap);
		
		for (int i = 0; i < count; ++i)
		{
			strncat_s(value, 1024, mdata->message[i], 1024);
			free(mdata->message[i]);
			mdata->message[i] = nullptr;
		}
		
		return value;
	}

	static void charToMessage(PVOID data, std::string& message)
	{
		char* cData = reinterpret_cast<char*>(data);
		int i = 0;
		std::string messageLen;
		while (cData[i] != ':')
		{
			messageLen += cData[i];
			++i;
		}
		i += 1;
		LONG mLen = atol(messageLen.c_str());
		while (i < mLen)
		{
			message += cData[i];
			++i;
		}
	}

	static void getIP(std::string& IPv4, std::wstring& IPv6)
	{
		INT iRetval;
		DWORD dwRetval;
		int i = 1;
		struct addrinfo* result = NULL;
		struct addrinfo* ptr = NULL;
		struct addrinfo hints;

		/*in_addr*		inAddr;
		char str[64] = { 0 };*/
		struct sockaddr_in* sockaddr_ipv4;
		//    struct sockaddr_in6 *sockaddr_ipv6;
		LPSOCKADDR sockaddr_ip;

		wchar_t ipstringbuffer[46];
		DWORD ipbufferlength = 46;


		//  ����������Ļ�����
		char szHost[256];
		//  ȡ�ñ�����������
		::gethostname(szHost, 256);

		//-------------------------------
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		dwRetval = getaddrinfo(szHost, nullptr, &hints, &result);
		if (dwRetval != 0) {
			printf("getaddrinfo failed with error: %d\n", dwRetval);
			WSACleanup();
			return;
		}

		for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
		{
			switch (ptr->ai_family) {
			case AF_UNSPEC:
				printf("Unspecified\n");
				break;
			case AF_INET:
				printf("AF_INET (IPv4)\n");
				/*inAddr = reinterpret_cast<in_addr*>(ptr->ai_addr);
				::inet_ntop(AF_INET, inAddr, str, 64);
				printf("IPv4 address %s\n", str);
				IPv4.assign(str);*/
				sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
				IPv4.assign(inet_ntoa(sockaddr_ipv4->sin_addr));
				printf("IPv4 address %s\n", IPv4.c_str());
				break;
			case AF_INET6:
				printf("AF_INET6 (IPv6)\n");
				sockaddr_ip = (LPSOCKADDR)ptr->ai_addr;
				ipbufferlength = 46;
				iRetval = WSAAddressToString(sockaddr_ip, (DWORD)ptr->ai_addrlen, NULL,
					ipstringbuffer, &ipbufferlength);
				if (iRetval)
					printf("WSAAddressToString failed with %u\n", WSAGetLastError());
				else
				{
					IPv6.assign(ipstringbuffer);
					std::wcout << L"IPv6 address  " << ipstringbuffer << std::endl;
				}
				break;
			case AF_NETBIOS:
				printf("AF_NETBIOS (NetBIOS)\n");
				break;
			default:
				printf("Other %ld\n", ptr->ai_family);
				break;
			}
		}
		freeaddrinfo(result);
	}
	
	// Capture Screen
	struct dp_rect_t
	{
		RECT   rc;             ///�����仯�ľ��ο�
		/////
		char* line_buffer;    ///���ο�������ʼ��ַ
		int    line_bytes;     ///ÿ�У����ο�width��Ӧ�������ݳ���
		int    line_nextpos;   ///��0��ʼ����N�е����ݵ�ַ: line_buffer + N*line_nextpos ��
		int    line_count;     ///���ھ��ο�߶� height
	};

	struct dp_frame_t
	{
		int        cx;          ///��Ļ���
		int        cy;          ///��Ļ�߶�
		int        line_bytes;  ///ÿ��ɨ���е�ʵ�����ݳ���
		int        line_stride; ///ÿ��ɨ���е�4�ֽڶ�������ݳ���
		int        bitcount;    ///8.16.24.32 λ���, 8λ��256��ɫ�壻 16λ��555��ʽ��ͼ��

		int        length;      ///��Ļ���ݳ��� line_stride*cy
		char* buffer;      ///��Ļ����
		/////
		int        rc_count;    ///�仯�������
		dp_rect_t* rc_array;    ///�仯����

		///
		void* param;   ///
	};

	struct dp_create_t
	{
		int       grab_type;      ///ץ�������� 0 �Զ�ѡ�� 1 mirror�� 2 DXץ���� 3 GDIץ��

		////
		DP_DISPLAYCHANGE   display_change;
		DP_FRAME           frame;
		void* param;
		///
	};


	//��������ץ��
	struct __mirror_cap_t
	{
		bool             is_active;
		DISPLAY_DEVICE   disp;
		////

		LONG             last_index;
		time_t           last_check_drawbuf_time;

		/////
		draw_buffer_t* buffer;
		byte* front_framebuf;
		byte* back_framebuf;

	};

	// DXץ��
	struct __d3d_cap_t
	{
		int                 cx;
		int                 cy;
		int                 bitcount;
		int                 line_bytes;
		int                 line_stride;

		////DXGI
		int                       is_acquire_frame;
		ID3D11Device* d11dev;
		ID3D11DeviceContext* d11ctx;
		IDXGIOutputDuplication* dxgi_dup;
		DXGI_OUTPUT_DESC          dxgi_desc;
		ID3D11Texture2D* dxgi_text2d;
		IDXGISurface* dxgi_surf;

		byte* buffer; ///
		byte* bak_buf;
	};

	///GDI ץ��
	struct __gdi_cap_t
	{
		int        cx;
		int        cy;
		int        line_bytes;
		int        line_stride;
		int        bitcount; ////
		HDC        memdc;
		HBITMAP    hbmp;
		///
		byte* buffer; ///
		byte* back_buf; ///
	};

	struct __tbuf_t
	{
		int    size;
		void* buf;
	};
	struct __xdisp_t
	{
		bool             quit;

		bool             is_pause_grab; //�Ƿ���ͣץ��
		int              grab_type; //// ץ�������� 0 �Զ�ѡ�� 1 mirror�� 2 DXץ���� 3 GDIץ��

		__mirror_cap_t   mirror;

		__d3d_cap_t      directx;

		__gdi_cap_t      gdi;

		/////
		LONG             sleep_msec;  ///

		////
		HWND             hMessageWnd;
		HANDLE           h_thread;
		DWORD            id_thread;
		HANDLE           hEvt;

		///
		DP_DISPLAYCHANGE display_change; ///
		DP_FRAME         frame;
		void* param;

		///
		__tbuf_t         t_arr[2];
		/////
	};



	#define CHANGE_QUEUE_SIZE          50000

	/// op_type
	#define  OP_TEXTOUT        1
	#define  OP_BITBLT         2
	#define  OP_COPYBITS       3
	#define  OP_STROKEPATH     4
	#define  OP_LINETO         5
	#define  OP_FILLPATH       6
	#define  OP_STROKEANDFILLPATH  7
	#define  OP_STRETCHBLT     8
	#define  OP_ALPHABLEND     9
	#define  OP_TRANSPARENTBLT 10
	#define  OP_GRADIENTFILL   11
	#define  OP_PLGBLT         12
	#define  OP_STRETCHBLTROP  13
	#define  OP_RENDERHINT     14

	#define ESCAPE_CODE_MAP_USERBUFFER     0x0DAACC00
	#define ESCAPE_CODE_UNMAP_USERBUFFER   0x0DAACC10

	struct draw_change_t
	{
		unsigned int    op_type; /// ����GDI��������,��ʵûɶ��
		///
		RECT            rect;    /// �����仯������
	};

	struct draw_queue_t
	{
		unsigned int       next_index;                 /// ��һ����Ҫ���Ƶ�λ��, �� 0�� ( CHANGE_QUEUE_SIZE - 1 ),ѭ������ָʾ��
		draw_change_t      draw_queue[CHANGE_QUEUE_SIZE];      /// 
	};

	struct draw_buffer_t
	{
		char                magic[16];   ///ûʲô����
		////
		unsigned int        cx;          ///��Ļ����
		unsigned int        cy;          ///��Ļ�߶�
		unsigned int        line_bytes;  ///ÿ��ɨ���е�ʵ�����ݳ���
		unsigned int        line_stride; ///ÿ��ɨ���е�4�ֽڶ�������ݳ���
		unsigned int        bitcount;    ///8,16,24,32 
		//////
		draw_queue_t        changes;     ///�����仯�ľ��ο򼯺�

		////
		unsigned int        data_length; //data_bufferָ�����ĻRGB���ݳ��ȣ����� line_stride*cy
		char                data_buffer[1]; //�����Ļ��������
	};
}

#endif // !_ALL_DATA_H_
