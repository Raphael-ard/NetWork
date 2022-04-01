#include "WebTCP.h"
#include "CaptureMethod.h"
#include "jpeglib.h"

#include <string>
#include <setjmp.h>

#pragma comment(lib, "jpeg.lib")
#pragma comment(lib,"ws2_32.lib")

using namespace std;

const char* boundary = "++&&**boundary--stream-jpeg++";

typedef struct _webClient
{
	SOCKET s;
	NetWork::webTCP* ws;
}webClient, * PWebClient;

typedef struct jpeg_error_t {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};


NetWork::webTCP::
webTCP(void)
{
	::InitializeCriticalSection(&cs);
	_quit = false;
	_jpegQuality = 80;
	_sock = -1;
}

NetWork::webTCP::
~webTCP(void)
{
	if (_sock)
		::closesocket(_sock);
	::DeleteCriticalSection(&cs);
	::WSACleanup();
}

int
NetWork::webTCP::
start(void)
{
	_sock = ::socket(AF_INET, SOCK_STREAM, 0);

	if (_sock == INVALID_SOCKET)
		return -1;

	int net_buf;

	if (::InetPtonA(AF_INET, NetWork::IPv4.c_str(), &net_buf) != 1)
		return;
	_addr.sin_addr.S_un.S_addr = net_buf;
	_addr.sin_family = AF_INET;
	_addr.sin_port = ::htons(9008);

	if (::bind(_sock,
		reinterpret_cast<const sockaddr*>(&_addr),
		sizeof _addr) == SOCKET_ERROR)
		return -1;

	if (::listen(_sock, SOMAXCONN) == SOCKET_ERROR)
	{
		auto i = WSAGetLastError();
		printf("Error: %d\n", i);
		return i;
	}

	DWORD tid;
	HANDLE h = CreateThread(0, 0, acceptThread, this, 0, &tid);
	CloseHandle(h);
	return 0;
}

void 
NetWork::webTCP::
setJpegQuality(int quality)
{
	_jpegQuality = quality;
}

void 
NetWork::webTCP::
frame(dp_frame_t* frame)
{
	vector<SOCKET> socks;
	::EnterCriticalSection(&cs);
	socks = this->_socks;
	::LeaveCriticalSection(&cs);
	///
	if (socks.size() == 0)return;
	/////
	if (frame->bitcount == 8)return; // 8位色不处理

	unsigned char* line_data = NULL;
	struct jpeg_compress_struct cinfo; //
	memset(&cinfo, 0, sizeof(cinfo)); //全部初始化为0， 否则要出问题

	struct jpeg_error_t jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = jpeg_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		////
		jpeg_destroy_compress(&cinfo);
		if (line_data)free(line_data); //////
		return;
	}
	jpeg_create_compress(&cinfo);

	byte* out_ptr = NULL;
	unsigned long out_size = 0;
	//////
	jpeg_mem_dest(&cinfo, &out_ptr, (unsigned long*)&out_size);
	cinfo.image_width = frame->cx;
	cinfo.image_height = frame->cy;

	int bit = frame->bitcount;
	if (bit == 16) {
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_EXT_BGR; ///
		line_data = (byte*)malloc(cinfo.image_width * 3); /// BGR data
	}
	else {
		cinfo.input_components = bit / 8; ///
		if (bit == 32) cinfo.in_color_space = JCS_EXT_BGRA;     ///
		else if (bit == 24) cinfo.in_color_space = JCS_EXT_BGR; ///
	}
	jpeg_set_defaults(&cinfo); ///

	int quality = _jpegQuality; ////压缩质量

	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height) {
		if (bit == 16) {
			WORD* dat = (WORD*)(frame->buffer + cinfo.next_scanline * frame->line_stride);
			int k = 0;
			for (int i = 0; i < cinfo.image_width; ++i) {
				WORD bb = dat[i];
				line_data[k] = ((bb & 0x1F) << 3);          // b
				line_data[k + 1] = ((bb >> 5) & 0x1F) << 3; // g
				line_data[k + 2] = ((bb >> 10) & 0x1F) << 3; // r
															 ///
				k += 3; ///
			}
			jpeg_write_scanlines(&cinfo, &line_data, 1);
			////
		}
		else {
			byte* line = (byte*)frame->buffer + cinfo.next_scanline * frame->line_stride; ///
			jpeg_write_scanlines(&cinfo, &line, 1);
		}
	}

	jpeg_finish_compress(&cinfo);

	jpeg_destroy_compress(&cinfo);
	if (line_data)free(line_data);

	/////
	char hdr[4096];
	sprintf(hdr, "--%s\r\n"
		"Content-Type: image/jpeg\r\n"
		"Content-Length: %d\r\n\r\n",
		boundary, out_size);

	for (int i = 0; i < socks.size(); ++i) {
		int s = socks[i];
		int r = send(s, hdr, strlen(hdr), 0);

		r = send(s, (char*)out_ptr, out_size, 0);
		if (r <= 0) {
			::EnterCriticalSection(&this->cs);
			for (auto it = this->_socks.begin(); it != this->_socks.end(); it != this->_socks.end()) {
				if (*it == s) {
					this->_socks.erase(it); 
					break;
				}
			}
			::LeaveCriticalSection(&this->cs);
			closesocket(s);
		}
		//////
	}
	////
	free(out_ptr); ///

}

DWORD 
NetWork::webTCP::
acceptThread(void* _p)
{
	webTCP* ws = reinterpret_cast<webTCP*>(_p);
	while (!ws->_quit) {
		fd_set rdst;
		FD_ZERO(&rdst);
		FD_SET(ws->_sock, &rdst);

		timeval tmo;
		tmo.tv_sec = 1;
		tmo.tv_usec = 0;

		int r = select(0, &rdst, NULL, NULL, &tmo);
		if (r <= 0)
			continue;
		if (ws->_quit)	
			break;

		int s = accept(ws->_sock, NULL, NULL);
		if (s < 0)
			continue;
		/////
		int tcp_nodelay = 1;
		setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&tcp_nodelay, sizeof(int));
		////
		DWORD tid;
		PWebClient p = new webClient;
		p->s = s;
		p->ws = ws;
		CloseHandle(CreateThread(0, 0, clientThread, (void*)p, 0, &tid));
	}

	return 0;
}

DWORD 
NetWork::webTCP::
clientThread(void* _p)
{
	PWebClient p = reinterpret_cast<PWebClient>(_p);
	webTCP* ws = p->ws;
	int s = p->s;

	///
	char buffer[8192];
	bool quit = false;

	int pos = 0;
	int LEN = sizeof(buffer);
	while (pos < LEN - 1) { //只能处理 GET 命令
		int r = recv(s, buffer + pos, LEN - pos, 0);
		if (r <= 0) 
		{ 
			quit = true; 
			break; 
		}
		pos += r;
		buffer[pos] = 0;
		if (strstr(buffer, "\r\n\r\n"))
			break;
	}
	if (quit) {
		closesocket(s);
		delete p;
		return 0;
	}

	buffer[pos] = 0; 
	printf("GET Info [%s]\n", buffer);
	//////
	const char* response = "HTTP/1.0 200 OK\r\n"
		"Connection: close\r\n"
		"Server: Web-Stream\r\n"
		"Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
		"Pragma: no-cache\r\n"
		"Content-Type: multipart/x-mixed-replace;boundary=%s\r\n\r\n";
	sprintf(buffer, response, boundary);
	send(s, buffer, strlen(buffer), 0);
	///
	::EnterCriticalSection(&ws->cs);
	ws->_socks.push_back(s);
	::LeaveCriticalSection(&ws->cs);
	////
	while (!ws->_quit) {
		int r = recv(s, buffer, sizeof(buffer), 0);
		if (r <= 0)break;///
	}

	/////
	::EnterCriticalSection(&ws->cs);
	for (auto it = ws->_socks.begin(); it != ws->_socks.end(); it != ws->_socks.end()) {
		if (*it == s) {
			ws->_socks.erase(it);
			break;
		}
	}
	::LeaveCriticalSection(&ws->cs);
	closesocket(s);

	delete p;
	return 0;
}


static void jpeg_error_exit(j_common_ptr cinfo)
{
	jpeg_error_t* myerr = reinterpret_cast<jpeg_error_t*>(cinfo->err);
	////
	(*cinfo->err->output_message) (cinfo);

	longjmp(myerr->setjmp_buffer, 1);
}