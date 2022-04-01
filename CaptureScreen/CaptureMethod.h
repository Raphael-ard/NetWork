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

	typedef int(*DP_DISPLAYCHANGE)(int width, int height, int bitcount, void* param);
	typedef int(*DP_FRAME) (dp_frame_t* frame);

	struct dp_create_t
	{
		int       grab_type;      ///ץ�������� 0 �Զ�ѡ�� 1 mirror�� 2 DXץ���� 3 GDIץ��

		////
		DP_DISPLAYCHANGE   display_change;
		DP_FRAME           frame;
		void* param;
		///
	};


	///////////////////
	void* dp_create(dp_create_t* ct);
	void  dp_destroy(void* handle);
	///����ץ�����ʱ�䣬��λ���룬����33 �͵�����ÿ��ץ 30֡�� 40����ÿ��25֡
	int dp_grab_interval(void* handle, int grab_msec);
	///�Ƿ���ͣץ���� is_pause ��=0 ��ͣ����������
	int dp_grap_pause(void* handle, int is_pause);
}

#endif // !_CAPTURE_METHOD_H_

