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

	#define USE_MIRROR_DIRTY_RECT   1   ///ʹ��mirror���������Լ����ɵ����������

	//#define USE_DXGI_DIRTY_RECT     1   ///ʹ��DXGI�� GetFrameDirtyRect��ȡ���µ����������µ�win10�����⣬��֪��������BUG���ǵ��ò���ȷ��
	///����С��������
	#define MIRROR_SMALL_RECT_WIDTH    256
	#define MIRROR_SMALL_RECT_HEIGHT   256

	///С���ο�
	#define GDI_SMALL_RECT_WIDTH    128
	#define GDI_SMALL_RECT_HEIGHT   8

	typedef int(*DP_DISPLAYCHANGE)(int width, int height, int bitcount, void* param);
	typedef int(*DP_FRAME) (dp_frame_t* frame);

	void* dp_create(dp_create_t* ct);
	void  dp_destroy(void* handle);
	///����ץ�����ʱ�䣬��λ���룬����33 �͵�����ÿ��ץ 30֡�� 40����ÿ��25֡
	int dp_grab_interval(void* handle, int grab_msec);
	///�Ƿ���ͣץ���� is_pause ��=0 ��ͣ����������
	int dp_grap_pause(void* handle, int is_pause);
}

#endif // !_CAPTURE_METHOD_H_
