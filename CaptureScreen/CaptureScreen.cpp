#include "CaptureMethod.h"

#include <stdarg.h>
#include <windows.h>
#include <time.h>
#include <ddraw.h>
#include <d3d9.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>

using namespace NetWork;

#define SAFE_RELEASE(a) if(a){ (a)->Release(); (a)=NULL; }
#define SAFE_FREE(a) if(a){ free(a); (a)=NULL; }

//int log_printf(const char* pFormat, ...);
static int active_mirror_driver(BOOL is_active, PDISPLAY_DEVICE dp);
static int map_and_unmap_buffer(const WCHAR* dev_name, BOOL is_map, draw_buffer_t** p_buf);

static void tbuf_check(__tbuf_t* arr, int size)
{
	if (!arr->buf || arr->size < size) {
		void* b1 = malloc(size);
		if (arr->buf)free(arr->buf);
		arr->buf = b1;
		arr->size = size;
	}
}

////初始化和删除抓屏
static bool __init_mirror(__xdisp_t* dp, bool is_init)
{
	if (is_init) {
		dp->mirror.buffer = NULL;
		dp->mirror.is_active = false;
		int r = active_mirror_driver(true, &dp->mirror.disp);
		if (r < 0) {
			printf("*** active Mirror Driver Error\n");
			////
		}
		else { //success
			dp->mirror.is_active = true;
		}
		///
		return dp->mirror.is_active; ///
	}

	else { // deinit
		if (dp->mirror.buffer) {
			map_and_unmap_buffer(dp->mirror.disp.DeviceName, FALSE, &dp->mirror.buffer); ////
			dp->mirror.buffer = NULL;
		}

		dp->mirror.is_active = false; ///
		int r = active_mirror_driver(0, &dp->mirror.disp); ///

		printf("unload mirror driver ret=%d\n", r);
		////

		SAFE_FREE(dp->mirror.front_framebuf);
		SAFE_FREE(dp->mirror.back_framebuf);
		return true;
	}
	return 0;
}

typedef HRESULT(WINAPI* fnD3D11CreateDevice)(
	_In_opt_ IDXGIAdapter* pAdapter,
	D3D_DRIVER_TYPE DriverType,
	HMODULE Software,
	UINT Flags,
	_In_reads_opt_(FeatureLevels) CONST D3D_FEATURE_LEVEL* pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	_Out_opt_ ID3D11Device** ppDevice,
	_Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel,
	_Out_opt_ ID3D11DeviceContext** ppImmediateContext);
static bool __init_directx(__xdisp_t* dp, bool is_init);
static bool __init_dxgi(__xdisp_t* dp)
{
	HRESULT hr;
	static HMODULE hmod = 0; if (!hmod)hmod = LoadLibrary(L"d3d11.dll");
	fnD3D11CreateDevice fn = (fnD3D11CreateDevice)GetProcAddress(hmod, "D3D11CreateDevice");
	if (!fn) {
		printf("DXGI init :Not Load [D3D11CreateDevice] function \n");
		return false;
	}
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);
	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
	D3D_FEATURE_LEVEL FeatureLevel;
	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex) {
		hr = fn(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
			D3D11_SDK_VERSION, &dp->directx.d11dev, &FeatureLevel, &dp->directx.d11ctx);
		if (SUCCEEDED(hr))break;
	}
	if (FAILED(hr)) {
		printf("DXGI Init: D3D11CreateDevice Return hr=0x%X\n", hr);
		return false;
	}
	IDXGIDevice* dxdev = 0;
	hr = dp->directx.d11dev->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxdev);
	if (FAILED(hr)) {
		__init_directx(dp, false); printf("DXGI init: IDXGIDevice Error hr=0x%X\n", hr);
		return false;
	}
	IDXGIAdapter* DxgiAdapter = 0;
	hr = dxdev->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
	SAFE_RELEASE(dxdev);
	if (FAILED(hr)) {
		__init_directx(dp, false); printf("DXGI init: IDXGIAdapter Error hr=0x%X\n", hr);
		return false;
	}
	INT nOutput = 0; /// ???
	IDXGIOutput* DxgiOutput = 0;
	hr = DxgiAdapter->EnumOutputs(nOutput, &DxgiOutput);
	SAFE_RELEASE(DxgiAdapter);
	if (FAILED(hr)) {
		__init_directx(dp, false); printf("DXGI init: IDXGIOutput Error hr=0x%X\n", hr);
		return false;
	}
	DxgiOutput->GetDesc(&dp->directx.dxgi_desc);
	IDXGIOutput1* DxgiOutput1 = 0;
	hr = DxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void**>(&DxgiOutput1));
	SAFE_RELEASE(DxgiOutput);
	if (FAILED(hr)) {
		__init_directx(dp, false); printf("DXGI init: IDXGIOutput1 Error hr=0x%X\n", hr);
		return false;
	}
	hr = DxgiOutput1->DuplicateOutput(dp->directx.d11dev, &dp->directx.dxgi_dup);
	SAFE_RELEASE(DxgiOutput1);
	if (FAILED(hr)) {
		__init_directx(dp, false); printf("DXGI init: IDXGIOutputDuplication Error hr=0x%X\n", hr);
		return false;
	}
	////
	////
	return true;
}
static bool __init_directx(__xdisp_t* dp, bool is_init)
{
	HRESULT hr;
	if (is_init) {
		DEVMODE devmode;
		memset(&devmode, 0, sizeof(DEVMODE));
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmDriverExtra = 0;
		BOOL bRet = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
		if (!bRet)return false;
		dp->directx.cx = devmode.dmPelsWidth;
		dp->directx.cy = devmode.dmPelsHeight;
		dp->directx.bitcount = devmode.dmBitsPerPel;
		if (dp->directx.bitcount != 8 && dp->directx.bitcount != 16 && dp->directx.bitcount != 32 && dp->directx.bitcount != 24)return false;
		dp->directx.line_bytes = dp->directx.cx * dp->directx.bitcount / 8;
		dp->directx.line_stride = (dp->directx.line_bytes + 3) / 4 * 4;

		///DXGI above win8
		if (__init_dxgi(dp)) {
			dp->directx.bitcount = 32; //DXGI固定为32 
			dp->directx.line_bytes = dp->directx.cx * dp->directx.bitcount / 8;
			dp->directx.line_stride = (dp->directx.line_bytes + 3) / 4 * 4;
			////
			return true;
		}
		else {//原先打算采用DX9的GetFroneufferData 方式抓屏，但是无法抓取layered windows窗口，效果也不见得多好，因此不支持DXGI的都换成GDI或者mirror抓. 
			return false;////
		}
		///
		return false;
	}
	else {

		if (dp->directx.dxgi_surf) {
			dp->directx.dxgi_surf->Unmap();
		}

		SAFE_RELEASE(dp->directx.dxgi_dup);
		SAFE_RELEASE(dp->directx.d11dev);
		SAFE_RELEASE(dp->directx.d11ctx);
		SAFE_RELEASE(dp->directx.dxgi_text2d);
		SAFE_RELEASE(dp->directx.dxgi_surf);

		if (dp->directx.buffer) {
			///
			dp->directx.buffer = 0;
		}

		SAFE_FREE(dp->directx.bak_buf);
		dp->directx.is_acquire_frame = false;

		return true; ///
	}

	return false;
}
static bool __init_gdi(__xdisp_t* dp, bool is_init)
{
	if (is_init) {
		////
		DEVMODE devmode;
		memset(&devmode, 0, sizeof(DEVMODE));
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmDriverExtra = 0;
		BOOL bRet = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
		if (!bRet)return false;
		dp->gdi.cx = devmode.dmPelsWidth;
		dp->gdi.cy = devmode.dmPelsHeight;
		dp->gdi.bitcount = devmode.dmBitsPerPel;
		if (dp->gdi.bitcount != 8 && dp->gdi.bitcount != 16 && dp->gdi.bitcount != 32 && dp->gdi.bitcount != 24)return false;
		dp->gdi.line_bytes = dp->gdi.cx * dp->gdi.bitcount / 8;
		dp->gdi.line_stride = (dp->gdi.line_bytes + 3) / 4 * 4;

		BITMAPINFOHEADER bi; memset(&bi, 0, sizeof(bi));
		bi.biSize = sizeof(bi);
		bi.biWidth = dp->gdi.cx;
		bi.biHeight = -dp->gdi.cy; //从上朝下扫描
		bi.biPlanes = 1;
		bi.biBitCount = dp->gdi.bitcount; //RGB
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		BYTE bb[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256];
		memcpy(bb, &bi, sizeof(bi));
		HDC hdc = GetDC(NULL); //屏幕DC
		if (dp->gdi.bitcount == 8) {//系统调色板
			PALETTEENTRY pal[256]; memset(pal, 0, sizeof(pal));///
			GetSystemPaletteEntries(hdc, 0, 256, pal); ///获取系统调色板
			RGBQUAD* PalColor = (RGBQUAD*)(bb + sizeof(bi));
			for (int i = 0; i < 256; ++i) {
				PalColor[i].rgbReserved = 0;
				PalColor[i].rgbRed = pal[i].peRed;
				PalColor[i].rgbGreen = pal[i].peGreen;
				PalColor[i].rgbBlue = pal[i].peBlue;
			}
		}

		dp->gdi.memdc = CreateCompatibleDC(hdc);
		dp->gdi.buffer = NULL;
		dp->gdi.hbmp = CreateDIBSection(hdc, (BITMAPINFO*)bb, DIB_RGB_COLORS, (void**)&dp->gdi.buffer, NULL, 0);
		ReleaseDC(NULL, hdc);
		if (!dp->gdi.buffer) {
			DeleteDC(dp->gdi.memdc); dp->gdi.memdc = 0;
			return false;
		}
		SelectObject(dp->gdi.memdc, dp->gdi.hbmp); ///
		dp->gdi.back_buf = (byte*)malloc(dp->gdi.line_stride * dp->gdi.cy);
		memset(dp->gdi.back_buf, 0, dp->gdi.line_stride * dp->gdi.cy);
		return true;
	}
	else {
		dp->gdi.buffer = 0;
		DeleteDC(dp->gdi.memdc); dp->gdi.memdc = NULL;
		DeleteObject(dp->gdi.hbmp); dp->gdi.hbmp = NULL;

		SAFE_FREE(dp->gdi.back_buf);
	}

	return false;
}

//////
#define MIRROR_DRIVER   L"Fanxiushu Mirror Display Driver"
#define VIRTUAL_DRIVER  L"Fanxiushu Virtual Display Driver (XPDM for WinXP Only)"

static LPSTR dispCode[7] = {
	"Change Successful",
	"Must Restart",
	"Bad Flags",
	"Bad Parameters",
	"Failed",
	"Bad Mode",
	"Not Updated" };
static LPSTR GetDispCode(INT code)
{
	switch (code) {
	case DISP_CHANGE_SUCCESSFUL: return dispCode[0];
	case DISP_CHANGE_RESTART: return dispCode[1];
	case DISP_CHANGE_BADFLAGS: return dispCode[2];
	case DISP_CHANGE_BADPARAM: return dispCode[3];
	case DISP_CHANGE_FAILED: return dispCode[4];
	case DISP_CHANGE_BADMODE: return dispCode[5];
	case DISP_CHANGE_NOTUPDATED: return dispCode[6];
	default:
		static char tmp[MAX_PATH];
		sprintf(&tmp[0], "Unknown code: %08x\n", code);
		return	&tmp[0];
	}
}

////
static BOOL find_display_device(PDISPLAY_DEVICE disp, PDEVMODE mode, BOOL is_primary, BOOL is_mirror)
{
	DISPLAY_DEVICE dispDevice;
	memset(&dispDevice, 0, sizeof(DISPLAY_DEVICE));
	dispDevice.cb = sizeof(DISPLAY_DEVICE);
	int num = 0; bool find = false;
	DEVMODE devmode;
	memset(&devmode, 0, sizeof(devmode));
	devmode.dmSize = sizeof(devmode);
	while (EnumDisplayDevices(NULL, num, &dispDevice, NULL)) {
		//	printf("devName=[%s], desc=[%s], id=[%s], key[%s],flags=0x%X\n", dispDevice.DeviceName, dispDevice.DeviceString, dispDevice.DeviceID, dispDevice.DeviceKey, dispDevice.StateFlags );

		if (is_primary) {
			if (dispDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {//主显示器
				find = true;
				break;
			}
		}
		else {
			const wchar_t* desc = MIRROR_DRIVER; if (!is_mirror)desc = VIRTUAL_DRIVER;
			if (wcscmp(dispDevice.DeviceString, desc) == 0) { /// find
				find = true;
				break;
			}
			/////
		}
		///
		++num;
	}
	if (!find) {
		return FALSE;
	}
	/////
	BOOL bRet = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	if (!bRet) {
		return FALSE;
	}

	*disp = dispDevice;
	*mode = devmode;
	printf("** found devName=[%s], desc=[%s], id=[%s], key[%s],flags=0x%X\n", dispDevice.DeviceName, dispDevice.DeviceString, dispDevice.DeviceID, dispDevice.DeviceKey, dispDevice.StateFlags);
	return TRUE;
}

///激活或者取消镜像驱动
static int active_mirror_driver(BOOL is_active, PDISPLAY_DEVICE dp)
{
	DISPLAY_DEVICE dispDevice;
	PDISPLAY_DEVICE disp;
	if (is_active || !dp) {
		////
		DEVMODE mode;
		BOOL b = find_display_device(&dispDevice, &mode, FALSE, TRUE);
		if (!b) {
			printf("not found Mirror Driver maybe not install.\n");
			return -1;
		}
		disp = &dispDevice;
	}
	else {
		disp = dp; ///
	}

	DEVMODE devmode;
	memset(&devmode, 0, sizeof(DEVMODE));
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	BOOL bRet = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	if (!bRet) {
		printf("not get current display information \n");
		return -1;
	}
	///
	DWORD attach = 0; if (is_active) attach = 1;
	HKEY hkey;
	if (RegOpenKey(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\xdisp_mirror\\Device0", &hkey) == ERROR_SUCCESS) {
		RegSetValueEx(hkey, L"Attach.ToDesktop", 0, REG_DWORD, (BYTE*)&attach, sizeof(DWORD));
		RegCloseKey(hkey);
	}
	WCHAR keystr[1024];
	wchar_t* sep = wcsstr(disp->DeviceKey, L"\\{");
	if (!sep) {
		printf("invalid EnumDisplaySettings param.\n");
		return -1;
	}

	wsprintf(keystr, L"SYSTEM\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Control\\VIDEO%s", sep);
	if (RegOpenKey(HKEY_LOCAL_MACHINE, keystr, &hkey) == ERROR_SUCCESS) {
		RegSetValueEx(hkey, L"Attach.ToDesktop", 0, REG_DWORD, (BYTE*)&attach, sizeof(DWORD)); ////
		RegCloseKey(hkey);
	}

	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	strcpy((char*)devmode.dmDeviceName, "xdisp_mirror");

	if (!is_active) {
		devmode.dmPelsWidth = 0;
		devmode.dmPelsHeight = 0;
	}
	const WCHAR* DeviceName = disp->DeviceName;

	INT code = ChangeDisplaySettingsEx(DeviceName, &devmode,
		NULL, (CDS_UPDATEREGISTRY), NULL);

	printf("Update Registry on device mode: %s\n", GetDispCode(code));

	///
	if (dp && disp != dp) {
		*dp = *disp;
	}
	///
	return 0;
}

static int map_and_unmap_buffer(const WCHAR* dev_name, BOOL is_map, draw_buffer_t** p_buf)
{
	DWORD code = ESCAPE_CODE_UNMAP_USERBUFFER;
	if (is_map) code = ESCAPE_CODE_MAP_USERBUFFER;

	HDC hdc = CreateDC(NULL, dev_name, NULL, NULL);
	if (!hdc) {
		printf("*** CreateDC err=%d\n", GetLastError());
		return -1;
	}

	////
	unsigned __int64  u_addr = 0;
	int ret;
	if (is_map) {
		ret = ExtEscape(hdc, code, 0, NULL, sizeof(u_addr), (LPSTR)&u_addr);
		if (ret <= 0) {
			printf("*** Map ExtEscape ret=%d, err=%d\n", ret, GetLastError());
			DeleteDC(hdc);
			return -1;
		}
		*p_buf = (draw_buffer_t*)(ULONG_PTR)u_addr; ////
		printf("-- map user=%p\n", *p_buf);
	}
	else {
		u_addr = (ULONG_PTR)*p_buf;
		ret = ExtEscape(hdc, code, sizeof(u_addr), (LPCSTR)&u_addr, 0, NULL);
		if (ret <= 0) {
			printf("*** UnMap ExtEscape err=%d\n", GetLastError());
			DeleteDC(hdc);
			return -1;
		}
		////
	}
	//////
	DeleteDC(hdc); ///

	return 0;
}

static void change_display(__xdisp_t* dp, int w, int h, int bits)
{
	printf("change new width=%d, heigt=%d, bitcount=%d\n", w, h, bits);
	///
	if (dp->mirror.buffer) {
		map_and_unmap_buffer(dp->mirror.disp.DeviceName, FALSE, &dp->mirror.buffer); ////
		dp->mirror.last_check_drawbuf_time = 0;
		dp->mirror.buffer = NULL;
		SAFE_FREE(dp->mirror.front_framebuf);
		SAFE_FREE(dp->mirror.back_framebuf);

	}
	////
	DEVMODE devmode;
	memset(&devmode, 0, sizeof(DEVMODE));
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;
	BOOL bRet = EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);
	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	strcpy((char*)devmode.dmDeviceName, "xdisp_mirror");
	devmode.dmPelsWidth = w;
	devmode.dmPelsHeight = h;
	devmode.dmBitsPerPel = bits;
	///
	INT code = ChangeDisplaySettingsEx(dp->mirror.disp.DeviceName, &devmode,
		NULL, (CDS_UPDATEREGISTRY), NULL);

	printf("Update Registry on device mode: %s\n", GetDispCode(code));
	////

}
LRESULT CALLBACK xDispWindowProc(HWND hwnd, UINT uMsg, WPARAM  wParam, LPARAM lParam)
{
	////
	switch (uMsg)
	{
	case WM_DISPLAYCHANGE:
	{
		__xdisp_t* dp = (__xdisp_t*)GetProp(hwnd, L"xDispData");
		if (dp) {
			////先调用回调，
			dp->display_change(LOWORD(lParam), HIWORD(lParam), wParam, dp->param); /// callback 

			if (dp->grab_type == GRAB_TYPE_MIRROR) {
				change_display(dp, LOWORD(lParam), HIWORD(lParam), wParam); ///
			}
			else if (dp->grab_type == GRAB_TYPE_DIRECTX) {
				__init_directx(dp, false);
				__init_directx(dp, true); ////
			}
			else if (dp->grab_type == GRAB_TYPE_GDI) {
				__init_gdi(dp, false);
				__init_gdi(dp, true);
			}
			/////
		}
		///
//		printf("--- WM_DISPLAYCHANGE\n");
	}
	break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
static HWND createMsgWnd(__xdisp_t* dp)
{
	HMODULE hmod = GetModuleHandle(NULL);

	const WCHAR* cls_name = L"xdisp_window_class";
	WNDCLASS wndClass = { 0,
		xDispWindowProc,
		0, 0,
		hmod, /// 
		NULL,NULL,NULL,NULL,
		cls_name
	};
	RegisterClass(&wndClass);
	HWND hwnd = CreateWindow(cls_name, L"", WS_OVERLAPPEDWINDOW, 0, 0, 1, 1, NULL, NULL, hmod, NULL);
	dp->hMessageWnd = hwnd;
	if (!hwnd) {
		printf("NULL Message Window.\n");
	}
	SetProp(hwnd, L"xDispData", dp);

	////
	return hwnd;
}

///
static void __forceinline combine_rect_to_rgn(HRGN& region, int x1, int y1, int x2, int y2)
{
	if (!region) {
		region = CreateRectRgn(x1, y1, x2, y2); ////
	}
	else {
		HRGN n_rgn = CreateRectRgn(x1, y1, x2, y2);
		if (!n_rgn) {
			printf("CreateRectRgn [%d, %d, %d, %d} err=%d\n", x1, y1, x2, y2, GetLastError());
			return;
		}
		if (CombineRgn(region, region, n_rgn, RGN_OR) == NULLREGION) { //矩形框可能有重叠，因此首先组成HRGN区域，然后再计算出合并之后的矩形框
			printf("CombineRgn [%d, %d, %d, %d} err=%d\n", x1, y1, x2, y2, GetLastError());
			DeleteObject(region);
			region = NULL;
		}
		DeleteObject(n_rgn); ////
		/////
	}
}
static int combine_rectangle(byte* primary_buffer, byte* second_buffer,
	int line_stride, LPRECT rect, LPSIZE screen, int bitcount,
	int SMALL_WIDTH, int SMALL_HEIGHT, HRGN& region)
{
	int x, y;
	int num_bit = (bitcount >> 3);
	byte* dst_buf = primary_buffer;
	byte* bak_buf = second_buffer;

	for (y = max(rect->top, 0); y < rect->bottom; y += SMALL_HEIGHT) {
		int h = min(SMALL_HEIGHT, rect->bottom - y); //printf("h=%d, [ %d]\n",h, rect->bottom - y);
		for (x = max(rect->left, 0); x < rect->right; x += SMALL_WIDTH) {
			int w = min(SMALL_WIDTH, rect->right - x);
			int x2 = x + w;
			int y2 = y + h;
			if (x < 0) { x = 0; }
			else if (x >= screen->cx) { x = screen->cx; }
			if (y < 0) { y = 0; }
			else if (y >= screen->cy) { y = screen->cy; }
			if (x2 >= screen->cx) x2 = screen->cx; if (x2 <= x)continue;
			if (y2 >= screen->cy) y2 = screen->cy; if (y2 <= y)continue;

			w = x2 - x;
			h = y2 - y;
			int base_pos = line_stride * y + x * num_bit;
			int line_bytes = w * num_bit;

			int i;
			bool is_dirty = false; //printf("*** [%d, %d, %d, %d] -> {%d, %d, %d, %d}\n", rect->left,rect->top,rect->right,rect->bottom, x,y,x2,y2);
#define    REP_CD(II) \
			if( !is_dirty){ \
				for (i = II; i < h; i += 5) { /*隔行扫描*/ \
					int pos = base_pos + line_stride*i;\
					if (memcmp(dst_buf + pos, bak_buf + pos, line_bytes) != 0) { is_dirty = true; break; }\
				}\
			}
			REP_CD(0); REP_CD(1);
			REP_CD(2); REP_CD(3); REP_CD(4);

			if (is_dirty) {
				//printf(" dirty [%d, %d, %d, %d]\n", x,y,x2,y2);
				combine_rect_to_rgn(region, x, y, x2, y2); //printf(" --dirty [%d, %d, %d, %d]\n", x, y, x2, y2);////
			}
			else {

			}
			////////
		}
	}

	return 0;
}
static int region_to_rectangle(HRGN region, byte* dst_buf, int line_stride,
	int cx, int cy, int bitcount, dp_rect_t** p_rc_array, int* p_rc_count, __tbuf_t* tarr)
{
	dp_rect_t* rc_array = NULL;
	int rc_count = 0;
	int num_bit = (bitcount >> 3);
	///
	DWORD size = GetRegionData(region, NULL, NULL);
	tbuf_check(&tarr[0], size);

	LPRGNDATA rgnData = (LPRGNDATA)tarr[0].buf;
	if (GetRegionData(region, size, rgnData)) { ///
		DWORD cnt = rgnData->rdh.nCount; //printf("new cnt=%d\n", cnt);
		DWORD sz = cnt * sizeof(dp_rect_t);
		tbuf_check(&tarr[1], sz);

		dp_rect_t* rc = (dp_rect_t*)tarr[1].buf;
		//	printf("old cnt=%d, newcnt=%d\n", old_rc_cnt, cnt );
			////
		LPRECT src_rc = (LPRECT)rgnData->Buffer;
		DWORD idx = 0; int i;
		for (i = 0; i < cnt; ++i) {
			dp_rect_t* c = &rc[idx];;

			///裁剪矩形框，保证在可视区域
			int x = src_rc[i].left;     if (x < 0) { x = 0; }
			else if (x >= cx) { x = cx; }
			int y = src_rc[i].top;      if (y < 0) { y = 0; }
			else if (y >= cy) { y = cy; }
			int x2 = src_rc[i].right;   if (x2 >= cx) x2 = cx; if (x2 <= x)continue;
			int y2 = src_rc[i].bottom;  if (y2 >= cy) y2 = cy; if (y2 <= y)continue;
			int w = x2 - x; //printf("--- {%d, %d, %d, %d}\n", x, y, x2, y2);
			int h = y2 - y;
			c->rc.left = x; c->rc.top = y; c->rc.right = x2; c->rc.bottom = y2; ///

			c->line_buffer = (char*)dst_buf + line_stride * y + x * num_bit;  ///
			c->line_bytes = w * num_bit;
			c->line_nextpos = line_stride; /////
			c->line_count = h; ///
			///
			++idx; ///
		//	if(x2 > dp->buffer->cy)printf("--** right=%d,bottom=%d, cx=%d,cy=%d\n", c->rc.right,c->rc.bottom, dp->buffer->cx, dp->buffer->cy);
		}
		rc_array = rc;
		rc_count = idx; //
		//////
	}
	*p_rc_array = rc_array;
	*p_rc_count = rc_count;
	/////
	return 0;
}
/////
static void capture_mirror(__xdisp_t* dp)
{
	///初始化
	if (!dp->mirror.buffer && abs(time(0) - dp->mirror.last_check_drawbuf_time) > 5) {
		dp->mirror.last_check_drawbuf_time = time(0);
		map_and_unmap_buffer(dp->mirror.disp.DeviceName, true, &dp->mirror.buffer);
		if (dp->mirror.buffer) {
			////
			dp->mirror.last_index = dp->mirror.buffer->changes.next_index; ///

			/////分配后备缓存
#ifndef USE_MIRROR_DIRTY_RECT
			SAFE_FREE(dp->mirror.front_framebuf);
			dp->mirror.front_framebuf = (byte*)malloc(dp->mirror.buffer->data_length);
#endif
			SAFE_FREE(dp->mirror.back_framebuf);
			dp->mirror.back_framebuf = (byte*)malloc(dp->mirror.buffer->data_length); ////
			memset(dp->mirror.back_framebuf, 0, dp->mirror.buffer->data_length);
			//////
		}
		//////
	}
	if (!dp->mirror.buffer) { //没有初始化成功
		return; ///
	}

	/////
	byte* front_framebuf = (byte*)dp->mirror.buffer->data_buffer;
	byte* back_framebuf = dp->mirror.back_framebuf;
	DWORD last = GetTickCount();
	/////
//	printf("*** netx_index = %d\n", dp->buffer->changes.next_index );
	unsigned int curr_index = dp->mirror.buffer->changes.next_index;
	unsigned int i;
	HRGN region = NULL;
	int change_rect_count = 0; ///

	dp_rect_t* rc_array = NULL;
	int rc_count = 0;

#if USE_MIRROR_DIRTY_RECT==1

	///从驱动共享内存获取变化的区域矩形框集合
	for (i = dp->mirror.last_index; i != curr_index; i++, i = i % CHANGE_QUEUE_SIZE) {

		draw_change_t* dc = &dp->mirror.buffer->changes.draw_queue[i]; //printf("*** type=%d, {%d, %d, %d, %d}\n", dc->op_type,dc->rect.left,dc->rect.top,dc->rect.right,dc->rect.bottom );
		++change_rect_count; ///
		///
		if (!region) {
			region = CreateRectRgnIndirect(&dc->rect); ////
		//	region = CreateRectRgn(0, 0, dp->buffer->cx, dp->buffer->cy);
		}
		else {
			HRGN n_rgn = CreateRectRgnIndirect(&dc->rect);
			if (CombineRgn(region, region, n_rgn, RGN_OR) == NULLREGION) { //矩形框可能有重叠，因此首先组成HRGN区域，然后再计算出合并之后的矩形框
				DeleteObject(region);
				region = NULL;
			}
			DeleteObject(n_rgn); ////
			/////
		}
		///

	}
	///
	dp->mirror.last_index = curr_index; ////
	DWORD t1 = GetTickCount();
	////
	if (region && back_framebuf) {
		////
		DWORD size = GetRegionData(region, NULL, NULL);
		tbuf_check(&dp->t_arr[0], size);

		LPRGNDATA rgnData = (LPRGNDATA)dp->t_arr[0].buf;
		HRGN tmp_rgn = NULL;
		if (GetRegionData(region, size, rgnData)) {
			LPRECT src_rc = (LPRECT)rgnData->Buffer;
			DWORD cnt = rgnData->rdh.nCount; //printf(" old cnt=%d; re create RGn\n", cnt);
			int cx = dp->mirror.buffer->cx; int cy = dp->mirror.buffer->cy;
			SIZE screenSize; screenSize.cx = cx; screenSize.cy = cy;
			for (DWORD i = 0; i < cnt; ++i) {
				LPRECT rc = &src_rc[i];
				///把边框裁剪到 {0,0,cx,cy}
				rc->left = max(0, rc->left); rc->top = max(rc->top, 0);
				rc->right = min(cx, rc->right); rc->bottom = min(rc->bottom, cy);
				///
				combine_rectangle(front_framebuf, back_framebuf,
					dp->mirror.buffer->line_stride, rc, &screenSize, dp->mirror.buffer->bitcount,
					MIRROR_SMALL_RECT_WIDTH, MIRROR_SMALL_RECT_HEIGHT, tmp_rgn);
			}

			DeleteObject(region); //printf(" --old cnt=%d; re create RGn\n", cnt);
			region = tmp_rgn;
			//////
		}
		///

	}
	DWORD t2 = GetTickCount();

#else //不采用mirror的脏矩形，自己计算
	///
	front_framebuf = dp->mirror.front_framebuf;
	for (i = dp->mirror.last_index; i != curr_index; i++, i = i % CHANGE_QUEUE_SIZE) {
		++change_rect_count;
	}
	dp->mirror.last_index = curr_index; ////
	DWORD t1 = last, t2 = last;
	if (change_rect_count > 0) { //只是判断数据变化
		///
		memcpy(front_framebuf, dp->mirror.buffer->data_buffer, dp->mirror.buffer->data_length);
		t1 = GetTickCount();
		SIZE screenSize; screenSize.cx = dp->mirror.buffer->cx; screenSize.cy = dp->mirror.buffer->cy;
		RECT screenRc = { 0,0, screenSize.cx, screenSize.cy };
		combine_rectangle(front_framebuf, back_framebuf, dp->mirror.buffer->line_stride,
			&screenRc, &screenSize, dp->mirror.buffer->bitcount, GDI_SMALL_RECT_WIDTH, GDI_SMALL_RECT_HEIGHT, region);
		t2 = GetTickCount();
	}

#endif // USE_MIRROR_DIRTY_RECT

	/////
	if (region) {
		////
		region_to_rectangle(region,
			(byte*)front_framebuf, dp->mirror.buffer->line_stride,
			dp->mirror.buffer->cx, dp->mirror.buffer->cy, dp->mirror.buffer->bitcount,
			&rc_array, &rc_count, dp->t_arr);

		DeleteObject(region); ////
		//////
	}
	if (back_framebuf && rc_count > 0) { //翻转
		///
	//	memcpy( back_framebuf, front_framebuf, dp->mirror.buffer->data_length); ////
		CopyMemory(back_framebuf, front_framebuf, dp->mirror.buffer->data_length);
	}
	DWORD t3 = GetTickCount();
	//	if (t3 - last > 15)printf("Mirror T1=%d, T2=%d, T3=%d\n", t1-last, t2-t1,t3-t2);
		///
	dp_frame_t frm;
	frm.rc_array = rc_array;
	frm.rc_count = rc_count;
	frm.cx = dp->mirror.buffer->cx;
	frm.cy = dp->mirror.buffer->cy;
	frm.line_bytes = dp->mirror.buffer->line_bytes;
	frm.line_stride = dp->mirror.buffer->line_stride;
	frm.bitcount = dp->mirror.buffer->bitcount;
	frm.buffer = (char*)front_framebuf;
	frm.length = dp->mirror.buffer->data_length;
	frm.param = dp->param;

	/////
	dp->frame(&frm); ///// callback ....
//	printf("CALLBACK OK rc_count=%d\n", rc_count);
	//////

}

///

static void capture_dxgi(__xdisp_t* dp)
{
	if (!dp->directx.dxgi_dup) {
		return;
	}
	///DXGI
	IDXGIResource* DesktopResource = 0;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	HRESULT hr;
	DWORD last;
	ID3D11Texture2D* image2d = 0;
	dp_rect_t* rc_array = NULL;
	int rc_count = 0;

	if (dp->directx.is_acquire_frame) {
		dp->directx.dxgi_dup->ReleaseFrame(); //先释放，然后再获取,保证释放和获取间隔最短
		dp->directx.is_acquire_frame = 0;
	}

	last = GetTickCount();
	hr = dp->directx.dxgi_dup->AcquireNextFrame(0, &FrameInfo, &DesktopResource);
	if (FAILED(hr)) {
#define DXGI_ERROR_ACCESS_LOST           _HRESULT_TYPEDEF_(0x887A0026L)
		if (hr == DXGI_ERROR_ACCESS_LOST) {
			__init_directx(dp, false);
			__init_directx(dp, true);
			return;
		}

		if (!dp->directx.buffer)return;
		///////
		dp_frame_t frm;
		frm.rc_array = rc_array;
		frm.rc_count = rc_count;
		frm.cx = dp->directx.cx;
		frm.cy = dp->directx.cy;
		frm.line_bytes = dp->directx.line_bytes;
		frm.line_stride = dp->directx.line_stride;
		frm.bitcount = dp->directx.bitcount;
		frm.buffer = (char*)dp->directx.buffer;
		frm.length = dp->directx.line_stride * dp->directx.cy; ///
		frm.param = dp->param;

		dp->frame(&frm); /// callback 
		return;
		/*goto L;*/
	}
	dp->directx.is_acquire_frame = 1; ////

	hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&image2d);
	SAFE_RELEASE(DesktopResource);
	if (FAILED(hr)) {
		return;
	}

	DXGI_MAPPED_RECT mappedRect;
	if (!dp->directx.dxgi_text2d) {
		D3D11_TEXTURE2D_DESC frameDescriptor;
		image2d->GetDesc(&frameDescriptor);
		frameDescriptor.Usage = D3D11_USAGE_STAGING;
		frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		frameDescriptor.BindFlags = 0;
		frameDescriptor.MiscFlags = 0;
		frameDescriptor.MipLevels = 1;
		frameDescriptor.ArraySize = 1;
		frameDescriptor.SampleDesc.Count = 1;
		hr = dp->directx.d11dev->CreateTexture2D(&frameDescriptor, NULL, &dp->directx.dxgi_text2d);
		if (FAILED(hr)) {
			SAFE_RELEASE(image2d);
			dp->directx.dxgi_dup->ReleaseFrame(); printf("CreateTexture2D err=0x%X\n", hr);
			return;
		}
		hr = dp->directx.dxgi_text2d->QueryInterface(__uuidof(IDXGISurface), (void**)&dp->directx.dxgi_surf);
		if (FAILED(hr)) {
			SAFE_RELEASE(dp->directx.dxgi_text2d);
			SAFE_RELEASE(image2d);
			dp->directx.dxgi_dup->ReleaseFrame(); printf("CreateTexture2D 2 err=0x%X\n", hr);
		}

		hr = dp->directx.dxgi_surf->Map(&mappedRect, DXGI_MAP_READ);
		if (FAILED(hr)) {
			printf("*** Map Data buffer error\n");
			SAFE_RELEASE(dp->directx.dxgi_text2d);
			SAFE_RELEASE(dp->directx.dxgi_surf);
			SAFE_RELEASE(image2d);
			dp->directx.dxgi_dup->ReleaseFrame();
			return;
		}
		////
		if (!dp->directx.buffer) dp->directx.buffer = (byte*)mappedRect.pBits; ////(byte*)malloc(dp->directx.line_stride*dp->directx.cy);//

		if (!dp->directx.bak_buf) dp->directx.bak_buf = (byte*)malloc(dp->directx.line_stride * dp->directx.cy);
		memset(dp->directx.bak_buf, 0, dp->directx.line_stride * dp->directx.cy);
		//////
	}

	//获取整个帧数据
	dp->directx.d11ctx->CopyResource(dp->directx.dxgi_text2d, image2d);
	SAFE_RELEASE(image2d);

	///
#if USE_DXGI_DIRTY_RECT==1
	if (FrameInfo.TotalMetadataBufferSize > 0) { //
		tbuf_check(&dp->t_arr[0], FrameInfo.TotalMetadataBufferSize);
		DXGI_OUTDUPL_MOVE_RECT* mvRect = (DXGI_OUTDUPL_MOVE_RECT*)dp->t_arr[0].buf;
		/////
		UINT cnt1 = 0, cnt2 = 0;
		UINT sz = FrameInfo.TotalMetadataBufferSize;
		hr = dp->directx.dxgi_dup->GetFrameMoveRects(sz, mvRect, &sz);
		if (FAILED(hr)) {
			sz = 0; cnt1 = 0;
		}
		else {
			cnt1 = sz / sizeof(DXGI_OUTDUPL_MOVE_RECT);
		}
		RECT* DirtRect = (RECT*)((byte*)mvRect + sz);
		sz = FrameInfo.TotalMetadataBufferSize - sz;
		hr = dp->directx.dxgi_dup->GetFrameDirtyRects(sz, DirtRect, &sz);
		if (FAILED(hr)) {
			sz = 0; cnt2 = 0;
		}
		else {
			cnt2 = sz / sizeof(RECT);
		}
		//这里其实没必要进行矩形框组合。。。因为DXGI生成的都是独立的矩形框
		HRGN region = NULL; int i;
		for (i = 0; i < cnt1; ++i) {
			int w = mvRect[i].DestinationRect.right - mvRect[i].DestinationRect.left;
			int h = mvRect[i].DestinationRect.bottom - mvRect[i].DestinationRect.top;
			int x = mvRect[i].DestinationRect.left; int y = mvRect[i].DestinationRect.top;
			combine_rect_to_rgn(region, x, y, x + w, y + h);
			x = mvRect[i].SourcePoint.x; y = mvRect[i].SourcePoint.y;
			combine_rect_to_rgn(region, x, y, x + w, y + h);

		}
		for (i = 0; i < cnt2; ++i) {
			LPRECT rc = &DirtRect[i]; if (rc->left > 1000)printf("[%d, %d, %d, %d]\n", rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top);
			combine_rect_to_rgn(region, DirtRect[i].left, DirtRect[i].top, DirtRect[i].right, DirtRect[i].bottom);
		}
		if (region) {
			region_to_rectangle(region, dp->directx.buffer, dp->directx.line_stride, dp->directx.cx, dp->directx.cy,
				dp->directx.bitcount, &rc_array, &rc_count, dp->t_arr);
			///
			DeleteObject(region);
		}
		if (cnt1 > 0)printf("CNT1=%d,CNT2=%d\n", cnt1, cnt2);
		////
	}
#else //直接采用扫描小块的办法获取变化的区域
	///
	DWORD t1 = GetTickCount();
	SIZE screenSize; screenSize.cx = dp->directx.cx; screenSize.cy = dp->directx.cy;
	RECT ScreenRc = { 0,0,dp->directx.cx, dp->directx.cy };
	HRGN region = NULL;
	combine_rectangle(dp->directx.buffer, dp->directx.bak_buf, dp->directx.line_stride, &ScreenRc, &screenSize, dp->directx.bitcount,
		GDI_SMALL_RECT_WIDTH, GDI_SMALL_RECT_HEIGHT, region);
	DWORD t2 = GetTickCount();
	if (region) {
		region_to_rectangle(region, dp->directx.buffer, dp->directx.line_stride, dp->directx.cx, dp->directx.cy,
			dp->directx.bitcount, &rc_array, &rc_count, dp->t_arr);

		DeleteObject(region);
	}
	///
	if (rc_count > 0) {//数据有变化
		///
		memcpy(dp->directx.bak_buf, dp->directx.buffer, dp->directx.line_stride * dp->directx.cy);
		/////
	}
	DWORD t3 = GetTickCount();
	if (t3 - last > 15) printf("DXGI Capture T1=%d, T2=%d, T3=%d\n", t1 - last, t2 - t1, t3 - t2);
#endif /// USE_DXGI_DIRTY_RECT

L:
	//////
	dp_frame_t frm;
	frm.rc_array = rc_array;
	frm.rc_count = rc_count;
	frm.cx = dp->directx.cx;
	frm.cy = dp->directx.cy;
	frm.line_bytes = dp->directx.line_bytes;
	frm.line_stride = dp->directx.line_stride;
	frm.bitcount = dp->directx.bitcount;
	frm.buffer = (char*)dp->directx.buffer;
	frm.length = dp->directx.line_stride * dp->directx.cy; ///
	frm.param = dp->param;

	dp->frame(&frm); /// callback 

//	printf("DXGI captured\n");
}

static void capture_gdi(__xdisp_t* dp)
{
	if (!dp->gdi.buffer || !dp->gdi.hbmp)return;
	///
	HDC hdc = GetDC(NULL);
	BitBlt(dp->gdi.memdc, 0, 0, dp->gdi.cx, dp->gdi.cy, hdc, 0, 0, SRCCOPY | CAPTUREBLT);
	ReleaseDC(NULL, hdc);
	///
	int line_bytes = dp->gdi.line_bytes;
	int line_stride = dp->gdi.line_stride;
	int cx = dp->gdi.cx; int cy = dp->gdi.cy;
	HRGN region = NULL;

	SIZE screenSize; screenSize.cx = dp->gdi.cx; screenSize.cy = dp->gdi.cy;
	RECT screeRc = { 0,0, dp->gdi.cx, dp->gdi.cy };
	combine_rectangle(dp->gdi.buffer, dp->gdi.back_buf, dp->gdi.line_stride, &screeRc, &screenSize, dp->gdi.bitcount,
		GDI_SMALL_RECT_WIDTH, GDI_SMALL_RECT_HEIGHT, region);


	///
	int rc_count = 0;
	dp_rect_t* rc_array = NULL;
	if (region) {
		////
		region_to_rectangle(region, dp->gdi.buffer, dp->gdi.line_stride, dp->gdi.cx, dp->gdi.cy,
			dp->gdi.bitcount, &rc_array, &rc_count, dp->t_arr);

		DeleteObject(region);
		//////
	}
	if (rc_count > 0) {
		///
		memcpy(dp->gdi.back_buf, dp->gdi.buffer, dp->gdi.line_stride * cy); //// copy to backbuffer

	}

	/////
	dp_frame_t frm;
	frm.rc_array = rc_array;
	frm.rc_count = rc_count;
	frm.cx = dp->gdi.cx;
	frm.cy = dp->gdi.cy;
	frm.line_bytes = line_bytes;
	frm.line_stride = line_stride;
	frm.bitcount = dp->gdi.bitcount;
	frm.buffer = (char*)dp->gdi.buffer;
	frm.length = line_stride * dp->gdi.cy; ///
	frm.param = dp->param;

	dp->frame(&frm); /// callback 

}


static DWORD CALLBACK __loop_msg(void* _p)
{
	__xdisp_t* dp = (__xdisp_t*)_p;  /////

	dp->id_thread = GetCurrentThreadId(); ///

	::CoInitialize(0);

	//////
	int grab_type = dp->grab_type;
	bool is_ok = false;
	if (grab_type == GRAB_TYPE_AUTO) { //自动选择
		is_ok = __init_mirror(dp, true);
		if (is_ok) grab_type = GRAB_TYPE_MIRROR;
		else {
			is_ok = __init_directx(dp, true);
			if (is_ok) grab_type = GRAB_TYPE_DIRECTX;
			else {
				is_ok = __init_gdi(dp, true);
				if (is_ok) grab_type = GRAB_TYPE_GDI;
			}
			////
		}
		dp->grab_type = grab_type; ////
	}
	else if (grab_type == GRAB_TYPE_MIRROR) {
		is_ok = __init_mirror(dp, true);
	}
	else if (grab_type == GRAB_TYPE_DIRECTX) {
		//不支持的话，直接采用 mirror or GDI抓图
		is_ok = __init_directx(dp, true);
		if (!is_ok) {
			is_ok = __init_mirror(dp, true);
			if (is_ok) {
				grab_type = GRAB_TYPE_MIRROR;
				dp->grab_type = grab_type; ////
			}
			else {
				is_ok = __init_gdi(dp, true);
				if (is_ok) {
					grab_type = GRAB_TYPE_GDI;
					dp->grab_type = grab_type; ////
				}
			}
			////
		}
	}
	else if (grab_type == GRAB_TYPE_GDI) {
		is_ok = __init_gdi(dp, true); /////
	}

	///
	if (!is_ok) {
		printf("*** can not init capture screen type=%d\n", grab_type);
		dp->hMessageWnd = NULL; ///
		SetEvent(dp->hEvt);
		::CoUninitialize();
		return 0;
	}

	////

	HWND hwnd = createMsgWnd(dp); ///
	HANDLE hEvt = dp->hEvt; ///

	if (!hwnd) {
		/////
		switch (grab_type) {
		case GRAB_TYPE_MIRROR:  __init_mirror(dp, false);  break;
		case GRAB_TYPE_DIRECTX: __init_directx(dp, false); break;
		case GRAB_TYPE_GDI:     __init_gdi(dp, false);     break;
		}
		////
		if (hwnd)DestroyWindow(hwnd);
		dp->hMessageWnd = NULL; ///
		SetEvent(hEvt); ////
		CoUninitialize();
		return 0; ///
	}
	////
	SetEvent(hEvt); ////

	///
	long sleep_msec = dp->sleep_msec;

	MSG msg;
	LARGE_INTEGER counter; QueryPerformanceFrequency(&counter);
	LARGE_INTEGER tmp_s;   QueryPerformanceCounter(&tmp_s);

	while (!dp->quit) {

		///
		BOOL bQuit = FALSE;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				bQuit = TRUE;
				break;
			}
			else {
				DispatchMessage(&msg); ///
			}
			/////
		}
		if (bQuit)break;

		/////
		sleep_msec = dp->sleep_msec;
		DWORD captureFrame_time = 0;

		if (!dp->is_pause_grab) { ///

			LARGE_INTEGER frame_begin; QueryPerformanceCounter(&frame_begin);
			////
			if (grab_type == GRAB_TYPE_MIRROR) {

				capture_mirror(dp);
				/////
			}
			else if (grab_type == GRAB_TYPE_DIRECTX) {
				////
				capture_dxgi(dp);

			}
			else if (grab_type == GRAB_TYPE_GDI) {
				///
				capture_gdi(dp); ///
			}

			/////
			LARGE_INTEGER frame_end; QueryPerformanceCounter(&frame_end);

			const int MIN_DT = 10; ///

			DWORD dt = (frame_end.QuadPart - frame_begin.QuadPart) * 1000 / counter.QuadPart;
			captureFrame_time = dt; ///
			if (dt >= dp->sleep_msec - MIN_DT) dt = dp->sleep_msec - MIN_DT; //printf("encode use %dms\n",dt);

			sleep_msec = dp->sleep_msec - dt;//重新计算下次需要等待的时间

			////////
		}

		/////沉睡sleep_msec 毫秒
		LARGE_INTEGER s, e;
		QueryPerformanceCounter(&s);
		LARGE_INTEGER counter_delay;
		counter_delay.QuadPart = counter.QuadPart * sleep_msec / 1000;
		while (!dp->quit) {
			SleepEx(1, TRUE); ///
			QueryPerformanceCounter(&e);
			if (e.QuadPart - s.QuadPart >= counter_delay.QuadPart)break;
		}
		/////
//		DWORD dt2 = (e.QuadPart - tmp_s.QuadPart) * 1000 / counter.QuadPart; tmp_s = e;  printf("$$$ TowFrame Interval [%d]ms , ProcessFrame [%d],nextSleep=[%d]\n", dt2, captureFrame_time, sleep_msec);
		///////
	}

	///
	CloseHandle(hEvt);

	/////
	switch (dp->grab_type) {
	case GRAB_TYPE_MIRROR:  __init_mirror(dp, false);  break;
	case GRAB_TYPE_DIRECTX: __init_directx(dp, false); break;
	case GRAB_TYPE_GDI:     __init_gdi(dp, false);     break;
	}

	::CoUninitialize(); ///
	///////
	return 0;
}


#if 0

int dp_frame(dp_frame_t* frame)
{
	return 0;
}

int main(int argc, char** argv)
{
	dp_create_t ct;
	ct.is_mirror = 1;
	ct.frame = dp_frame; ///

	void* dp = dp_create(&ct);

	getchar();

	dp_destroy(dp); ///

	return 0;
}

#endif

void*
NetWork::dp_create(dp_create_t* ct)
{
	__xdisp_t* dp = NULL;

	////
	dp = new __xdisp_t;
	dp->quit = false;
	dp->is_pause_grab = false;
	dp->grab_type = ct->grab_type;

	memset(&dp->mirror, 0, sizeof(dp->mirror));
	memset(&dp->directx, 0, sizeof(dp->directx)); ///
	memset(&dp->gdi, 0, sizeof(dp->gdi));
	///
	dp->sleep_msec = 33; /// 30 fps

	dp->hMessageWnd = NULL;
	dp->hEvt = CreateEvent(NULL, TRUE, FALSE, NULL);

	dp->display_change = ct->display_change; ///
	dp->frame = ct->frame;
	dp->param = ct->param;

	memset(dp->t_arr, 0, sizeof(dp->t_arr));

	DWORD tid;
	dp->h_thread = CreateThread(NULL, 10 * 1024 * 1024, __loop_msg, dp, 0, &tid);

	if (!dp->h_thread) {
		CloseHandle(dp->hEvt);
		delete dp;
		//////
		return NULL;
	}
	::SetThreadPriority(dp->h_thread, THREAD_PRIORITY_HIGHEST); //提升优先级

	::WaitForSingleObject(dp->hEvt, INFINITE);
	::ResetEvent(dp->hEvt); //
	if (!dp->hMessageWnd) {
		CloseHandle(dp->hEvt); ///
		CloseHandle(dp->h_thread); ///
		delete dp;
		////
		return NULL;
	}

	//////
	return dp;
}

void
NetWork::dp_destroy(void* handle)
{
	__xdisp_t* dp = (__xdisp_t*)handle;
	if (!dp)return;
	//////
	dp->quit = true;
	SetEvent(dp->hEvt);
	DestroyWindow(dp->hMessageWnd); ////
	::WaitForSingleObject(dp->h_thread, 8 * 1000);
	::TerminateThread(dp->h_thread, 0);
	CloseHandle(dp->h_thread);
	CloseHandle(dp->hEvt);

	////
	switch (dp->grab_type) {
	case GRAB_TYPE_MIRROR:  __init_mirror(dp, false);  break;
	case GRAB_TYPE_DIRECTX: __init_directx(dp, false); break;
	case GRAB_TYPE_GDI:     __init_gdi(dp, false);     break;
	}

	////
	for (int i = 0; i < sizeof(dp->t_arr) / sizeof(dp->t_arr[0]); ++i) {
		if (dp->t_arr[i].buf)free(dp->t_arr[i].buf);
	}

	delete dp;
}

int
NetWork::dp_grab_interval(void* handle, int grab_msec)
{
	__xdisp_t* dp = (__xdisp_t*)handle;
	if (!dp)return -1;
	//////
	if (grab_msec < 10) grab_msec = 10;
	dp->sleep_msec = grab_msec;

	return 0;
}

int
NetWork::dp_grap_pause(void* handle, int is_pause)
{
	__xdisp_t* dp = (__xdisp_t*)handle;
	if (!dp)return -1;
	//////
	if (is_pause)dp->is_pause_grab = true;
	else dp->is_pause_grab = false;

	return 0;
}
