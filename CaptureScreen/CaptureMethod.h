#pragma once
#ifndef _CAPTURE_METHOD_H_
#define _CAPTURE_METHOD_H_

namespace NetWork
{
#define GRAB_TYPE_AUTO      0
#define GRAB_TYPE_MIRROR    1
#define GRAB_TYPE_DIRECTX   2
#define GRAB_TYPE_GDI       3

	struct dp_rect_t
	{
		RECT   rc;             ///发生变化的矩形框
		/////
		char* line_buffer;    ///矩形框数据起始地址
		int    line_bytes;     ///每行（矩形框width对应）的数据长度
		int    line_nextpos;   ///从0开始，第N行的数据地址: line_buffer + N*line_nextpos 。
		int    line_count;     ///等于矩形框高度 height
	};

	struct dp_frame_t
	{
		int        cx;          ///屏幕宽度
		int        cy;          ///屏幕高度
		int        line_bytes;  ///每个扫描行的实际数据长度
		int        line_stride; ///每个扫描行的4字节对齐的数据长度
		int        bitcount;    ///8.16.24.32 位深度, 8位是256调色板； 16位是555格式的图像

		int        length;      ///屏幕数据长度 line_stride*cy
		char* buffer;      ///屏幕数据
		/////
		int        rc_count;    ///变化区域个数
		dp_rect_t* rc_array;    ///变化区域

		///
		void* param;   ///
	};

	typedef int(*DP_DISPLAYCHANGE)(int width, int height, int bitcount, void* param);
	typedef int(*DP_FRAME) (dp_frame_t* frame);

	struct dp_create_t
	{
		int       grab_type;      ///抓屏方法， 0 自动选择， 1 mirror， 2 DX抓屏， 3 GDI抓屏

		////
		DP_DISPLAYCHANGE   display_change;
		DP_FRAME           frame;
		void* param;
		///
	};


	///////////////////
	void* dp_create(dp_create_t* ct);
	void  dp_destroy(void* handle);
	///设置抓屏间隔时间，单位毫秒，比如33 就等于是每秒抓 30帧， 40等于每秒25帧
	int dp_grab_interval(void* handle, int grab_msec);
	///是否暂停抓屏， is_pause ！=0 暂停，否则运行
	int dp_grap_pause(void* handle, int is_pause);
}

#endif // !_CAPTURE_METHOD_H_

