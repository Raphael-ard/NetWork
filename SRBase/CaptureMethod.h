#pragma once
#ifndef _CAPTURE_METHOD_H_
#define _CAPTURE_METHOD_H_

#include "AllData.h"

namespace NetWork
{
	#define GRAB_TYPE_AUTO      0
	#define GRAB_TYPE_MIRROR    1
	#define GRAB_TYPE_DIRECTX   2
	#define GRAB_TYPE_GDI       3

	#define USE_MIRROR_DIRTY_RECT   1   ///使用mirror镜像驱动自己生成的脏矩形区域

	//#define USE_DXGI_DIRTY_RECT     1   ///使用DXGI的 GetFrameDirtyRect获取更新的区域，在最新的win10有问题，不知道是它的BUG还是调用不正确。
	///计算小矩形像素
	#define MIRROR_SMALL_RECT_WIDTH    256
	#define MIRROR_SMALL_RECT_HEIGHT   256

	///小矩形块
	#define GDI_SMALL_RECT_WIDTH    128
	#define GDI_SMALL_RECT_HEIGHT   8

	typedef int(*DP_DISPLAYCHANGE)(int width, int height, int bitcount, void* param);
	typedef int(*DP_FRAME) (dp_frame_t* frame);

	void* dp_create(dp_create_t* ct);
	void  dp_destroy(void* handle);
	///设置抓屏间隔时间，单位毫秒，比如33 就等于是每秒抓 30帧， 40等于每秒25帧
	int dp_grab_interval(void* handle, int grab_msec);
	///是否暂停抓屏， is_pause ！=0 暂停，否则运行
	int dp_grap_pause(void* handle, int is_pause);
}

#endif // !_CAPTURE_METHOD_H_
