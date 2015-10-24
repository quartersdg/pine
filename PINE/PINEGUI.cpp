// PINE.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <Windows.h>
#include <CommCtrl.h>
#include <Ole2.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <gdiplus.h>
#include <io.h>
#include <fcntl.h>
#include <vector>
#include <iostream>
#include <queue>
#include "PINEGUI.h"
#include "Resource.h"

#pragma comment(linker, \
  "\"/manifestdependency:type='Win32' "\
  "name='Microsoft.Windows.Common-Controls' "\
  "version='6.0.0.0' "\
  "processorArchitecture='*' "\
  "publicKeyToken='6595b64144ccf1df' "\
  "language='*'\"")
#pragma comment(lib, "ComCtl32.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

#define PEBBLE_BASE_WIDTH 236
#define PEBBLE_BASE_HEIGHT 394

#define PEBBLE_BASE_FACE_X 43
#define PEBBLE_BASE_FACE_Y 106

#define PEBBLE_FACE_WIDTH 144
#define PEBBLE_FACE_HEIGHT 168

static int watch_base = 2;


int PineColorToBrush(PINE_COLOR_T c)
{
	switch (c) {
	case PINE_COLOR_BLACK:
		return BLACK_BRUSH;
		break;
	case PINE_COLOR_WHITE:
		return WHITE_BRUSH;
		break;
	}
}

int PineColorToPen(PINE_COLOR_T c)
{
	switch (c) {
	case PINE_COLOR_BLACK:
		return BLACK_PEN;
		break;
	case PINE_COLOR_WHITE:
		return WHITE_PEN;
		break;
	}
}

COLORREF PineColorToColorRef(PINE_COLOR_T c)
{
	switch (c) {
	case PINE_COLOR_BLACK:
		return RGB(0, 0, 0);
		break;
	case PINE_COLOR_WHITE:
		return RGB(255, 255, 255);
		break;
	}
}


void ResizeDialogToPixels(HWND hwnd, DWORD w, DWORD h)
{
	RECT r = { 0, 0, w, h };
	MapDialogRect(hwnd, &r);

	RECT cr;
	GetClientRect(hwnd, &cr);
	RECT wr;
	GetWindowRect(hwnd, &wr);

	double mx = (double)w / r.right;
	double my = (double)h / r.bottom;
	r.right = (r.right * mx) + (wr.right - cr.right);
	r.bottom = (r.bottom * my) + (wr.bottom - cr.bottom);
	SetWindowPos(hwnd, NULL, 0, 0, r.right, r.bottom, SWP_NOMOVE | SWP_SHOWWINDOW);
}

HWND pineHwnd;
HDC faceHDC;
HBITMAP faceBitmap;

void PinePaintBegin()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(pineHwnd, &ps);

	faceHDC = CreateCompatibleDC(hdc);
	faceBitmap = CreateCompatibleBitmap(hdc, PEBBLE_FACE_WIDTH, PEBBLE_FACE_HEIGHT);
	SelectObject(faceHDC, faceBitmap);

	EndPaint(pineHwnd, &ps);
}

void PineClear(PINE_COLOR_T bg)
{
	RECT r = { 0, 0, PEBBLE_FACE_WIDTH, PEBBLE_FACE_HEIGHT };
	FillRect(faceHDC, &r, (HBRUSH)GetStockObject(PineColorToBrush(bg)));
}

void PinePaint()
{
	HDC hdc;
	PAINTSTRUCT ps;
	hdc = BeginPaint(pineHwnd, &ps);

	HDC chdc = CreateCompatibleDC(hdc);
	HBITMAP bitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDC_BODY_BMP));

	SelectObject(chdc, bitmap);
	BitBlt(hdc, 0, 0, PEBBLE_BASE_WIDTH, PEBBLE_BASE_HEIGHT, chdc, 0, watch_base * PEBBLE_BASE_HEIGHT, SRCCOPY);

	RECT r = { PEBBLE_BASE_FACE_X, PEBBLE_BASE_FACE_Y, PEBBLE_BASE_FACE_X + PEBBLE_FACE_WIDTH, PEBBLE_BASE_FACE_Y + PEBBLE_FACE_HEIGHT };
	//FillRect(hdc, &r, (HBRUSH)(0));

	BitBlt(hdc, PEBBLE_BASE_FACE_X, PEBBLE_BASE_FACE_Y, PEBBLE_FACE_WIDTH, PEBBLE_FACE_HEIGHT, faceHDC, 0, 0, SRCCOPY);

	EndPaint(pineHwnd, &ps);
}

extern "C" void psleep(int millis)
{
	Sleep(millis);
}

void PineLog(uint8_t log_level, const char* src_filename, int src_line_number, const char* fmt, va_list va)
{
	const char* basename;
	char msg[1024];
	int off;

	basename = strrchr(src_filename, '\\');
	if (NULL == basename)
		basename = strrchr(src_filename, '/');
	if (NULL == basename)
		basename = "";
	basename++;

	off = _snprintf(msg, 1024, "[%s:%d] ", basename, src_line_number);
	vsnprintf(msg + off, 1024 - off, fmt, va);

	std::cout << msg << std::endl;
}


void PineSetBrushColor(PINE_COLOR_T c)
{
	SelectObject(faceHDC, GetStockObject(PineColorToPen(c)));
}

void PineSetFillColor(PINE_COLOR_T c)
{
	SelectObject(faceHDC, GetStockObject(PineColorToBrush(c)));
}

void PineDrawLine(int x1, int y1, int x2, int y2)
{
	MoveToEx(faceHDC, x1, y1, NULL);
	LineTo(faceHDC, x2, y2);
}

void PineDrawPolyFilled(int num_points, PPoint* points)
{
	Polygon(faceHDC, (PPOINT)points, num_points);
}

void PineDrawPolyLine(int num_points, PPoint* points)
{
	Polyline(faceHDC, (PPOINT)points, num_points);
}

void PineDrawRectFilled(PINE_COLOR_T c, int x, int y, int w, int h)
{
	RECT r = { x, y, x + w, y + h };
	FillRect(faceHDC, &r, (HBRUSH)GetStockObject(PineColorToBrush(c)));
}

void PineDrawCircleFilled(PINE_COLOR_T c, int x, int y, int radius)
{
    Ellipse(faceHDC, x - radius, y - radius, x + radius, y + radius);
}

void PineDrawCircle(PINE_COLOR_T c, int x, int y, int radius)
{
    Arc(faceHDC, x - radius, y - radius, x + radius, y + radius, x-radius,0,x-radius,0);
}


#include "pebble_fonts.h"
typedef struct PINE_SYSTEM_FONT_T {
	const char* font_key;
	const char* system_font;
	int height;
	bool bold;
} PINE_SYSTEM_FONT_T;
static PINE_SYSTEM_FONT_T gs_system_fonts[] = {
	{ FONT_KEY_GOTHIC_14, "Impact", 14, false },
	{ FONT_KEY_GOTHIC_14_BOLD, "Impact", 14, true },
	{ FONT_KEY_GOTHIC_18, "Impact", 18, false },
	{ FONT_KEY_GOTHIC_18_BOLD, "Impact", 18, true },
	{ FONT_KEY_GOTHIC_24, "Impact", 24, false },
	{ FONT_KEY_GOTHIC_24_BOLD, "Impact", 24, true },
	{ FONT_KEY_GOTHIC_28, "Impact", 28, false },
	{ FONT_KEY_GOTHIC_28_BOLD, "Impact", 28, true },
	{ FONT_KEY_BITHAM_30_BLACK, "Franklin Gothic", 14, false },
	{ FONT_KEY_BITHAM_42_BOLD, "Franklin Gothic", 14, false },
	{ FONT_KEY_BITHAM_42_LIGHT, "Franklin Gothic", 14, false },
	{ FONT_KEY_BITHAM_42_MEDIUM_NUMBERS, "Franklin Gothic", 14, false },
	{ FONT_KEY_BITHAM_34_MEDIUM_NUMBERS, "Franklin Gothic", 14, false },
	{ FONT_KEY_BITHAM_34_LIGHT_SUBSET, "Franklin Gothic", 14, false },
	{ FONT_KEY_BITHAM_18_LIGHT_SUBSET, "Franklin Gothic", 14, false },
	{ FONT_KEY_ROBOTO_CONDENSED_21, "Franklin Gothic", 14, false },
	{ FONT_KEY_ROBOTO_BOLD_SUBSET_49, "Franklin Gothic", 14, false },
	{ FONT_KEY_DROID_SERIF_28_BOLD, "Franklin Gothic", 14, false },
	{ FONT_KEY_FONT_FALLBACK, "Franklin Gothic", 14, false },
	{ 0 },
};

void* PineGetSystemFont(const char* font_key)
{
	PINE_SYSTEM_FONT_T* f = &gs_system_fonts[0];
	for (; f->font_key; f++) {
		if (0 == strcmp(font_key, f->font_key)) break;
	}

	if (!f->font_key) return NULL;

	HFONT hf = CreateFont(-f->height, 0, 0, 0, f->bold ? FW_BOLD : FW_DONTCARE, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, f->system_font);
	
	return hf;
}

void PineDrawText(int x, int y, int w, int h, PINE_COLOR_T bcolor, PINE_COLOR_T tcolor, void* font, const char* text)
{
	RECT r;
	int oldmode;
	SetRect(&r, x, y, x + w, y + h);
	SetTextColor(faceHDC, PineColorToColorRef(tcolor));
	if (bcolor == PINE_COLOR_CLEAR) {
		oldmode = SetBkMode(faceHDC, TRANSPARENT);
	} else {
		SetBkColor(faceHDC, PineColorToColorRef(bcolor));
	}
	SelectObject(faceHDC, (HFONT)font);
	DrawText(faceHDC, text, -1, &r, DT_BOTTOM);
	if (bcolor == PINE_COLOR_CLEAR) {
		SetBkMode(faceHDC, oldmode);
	}
}

void PineDrawBitmap(int x, int y, int w, int h, void* bitmap)
{
	Gdiplus::Bitmap* b = (Gdiplus::Bitmap*)bitmap;
	HBITMAP hb;
	b->GetHBITMAP(Gdiplus::Color::White, &hb);

	HDC chdc = CreateCompatibleDC(faceHDC);

	SelectObject(chdc, hb);
	BitBlt(faceHDC, x, y, w, h, chdc, 0, 0, SRCCOPY);
}

extern "C" bool clock_is_24h_style(void) {
	wchar_t buf[128];
	GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_STIMEFORMAT, buf, sizeof(buf));
	return wcschr(buf,L'H')!=NULL;
}

void PinePaintEnd()
{
	InvalidateRect(pineHwnd, NULL, FALSE);
	//SendMessage(pineHwnd, WM_PAINT, 0, 0);
}

PineBatteryState gs_pbs = { 75, 1, 1 };
PineBatteryState PineGetBatteryState()
{
	return gs_pbs;
}

static CRITICAL_SECTION gs_cs;
static HANDLE gs_event;
static std::queue<PINE_EVENT_T> gs_pine_events;

PINE_EVENT_T PineWaitForEvent(uint32_t* timeout)
{
	DWORD result;
	ULONGLONG start, end;
	
	start = GetTickCount64();
	result = WaitForSingleObject(gs_event, *timeout);
	end = GetTickCount64();

	*timeout = end - start;

	EnterCriticalSection(&gs_cs);
	PINE_EVENT_T e = PINE_EVENT_TICK;
	if (!gs_pine_events.empty()) {
		e = gs_pine_events.front();
		gs_pine_events.pop();
	}
	LeaveCriticalSection(&gs_cs);
	return e;
}

extern void main();
DWORD appThreadId = 0;
static DWORD WINAPI AppThread(void* args) {
	main();
	return 0;
}

static int gs_bluetoothConnected = FALSE;
int PineGetBluetoothConnected()
{
	return gs_bluetoothConnected;
}

void PineFireEvent(PINE_EVENT_T e)
{
	EnterCriticalSection(&gs_cs);
	gs_pine_events.push(e);
	SetEvent(gs_event);
	LeaveCriticalSection(&gs_cs);
}
void CreateConsoleAndFileHandles()
{
	BOOL res = AllocConsole();
	int h = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
	FILE* fh = _fdopen(h, "w");
	*stdout = *fh;
	std::cout.clear();
	std::cout << "PINE" << std::endl;
}

static void SetDialogItemToInt(HWND hwndDlg, int item, int value)
{
	char buf[32];
	_snprintf(buf, sizeof(buf), "%d", value);
	SetDlgItemText(hwndDlg, item, buf);
}

static int GetDialogItemAsInt(HWND hwndDlg, int item)
{
	char buf[32];
	GetDlgItemText(hwndDlg, item, buf, sizeof(buf));
	return atoi(buf);
}

BOOL CALLBACK BatteryDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetDialogItemToInt(hwndDlg, IDC_CHARGE, gs_pbs.charge);
		CheckDlgButton(hwndDlg, IDC_CHARGING, gs_pbs.charging);
		CheckDlgButton(hwndDlg, IDC_PLUGGED, gs_pbs.plugged);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHARGE:
			gs_pbs.charge = GetDialogItemAsInt(hwndDlg, IDC_CHARGE);
			break;
		case IDC_CHARGING:
			gs_pbs.charging = !gs_pbs.charging;
			CheckDlgButton(hwndDlg, IDC_CHARGING, gs_pbs.charging);
			PineFireEvent(PINE_EVENT_BATTERY);
			break;
		case IDC_PLUGGED:
			gs_pbs.plugged = !gs_pbs.plugged;
			CheckDlgButton(hwndDlg, IDC_PLUGGED, gs_pbs.plugged);
			PineFireEvent(PINE_EVENT_BATTERY);
			break;
		case IDCANCEL:
			EndDialog(hwndDlg, wParam);
			return TRUE;
			break;
		}
		break;
	}

	return FALSE;
}

using std::vector;
using std::pair;

static vector<RECT> gs_buttonBoxes = {
	{0,108,16,188},
	{210,80,240,156},
	{210,176,240,221},
	{210,241,240,318},
};

bool IsInRect(int x, int y, RECT r)
{
	return((x >= r.left) && (x <= r.right) && (y >= r.top) && (y <= r.bottom));
}

int IsButton(int x, int y)
{
	for (int i = 0;i < 4;i++)
	{
		if (IsInRect(x, y, gs_buttonBoxes[i])) return i;
	}
	return -1;
}

BOOL CALLBACK DialogProc(HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	static POINTS ptsCursor = { 0 };
	switch (message)
	{
	case WM_INITDIALOG:
		ResizeDialogToPixels(hwnd, PEBBLE_BASE_WIDTH, PEBBLE_BASE_HEIGHT);
		InitializeCriticalSection(&gs_cs);
		gs_event = CreateEvent(NULL, FALSE, FALSE, "pineevents");
		pineHwnd = hwnd;
		CreateThread(NULL, 0, AppThread, NULL, 0, &appThreadId);
		return TRUE;
	case WM_COMMAND:
		switch (wParam)
		{
		case IDM_EXIT:
			PostQuitMessage(0);
			break;
		case IDM_BLUETOOTH:
			gs_bluetoothConnected = !gs_bluetoothConnected;
			PineFireEvent(PINE_EVENT_BLUETOOTH);
			break;
		case IDM_BATTERY:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_BATTERY_CHANGE), hwnd, BatteryDialogProc);
			break;
		}
		return TRUE;
	case WM_HSCROLL:
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return TRUE;
	case WM_PAINT:
		PinePaint();
		return TRUE;
	case WM_LBUTTONDOWN:
	{
		ptsCursor = MAKEPOINTS(lParam);
		int b = IsButton(ptsCursor.x, ptsCursor.y);
		if (b > -1) {
			char tmp[128];
			sprintf(tmp, "button %d down\n", b);
			OutputDebugStringA(tmp);
			PineFireEvent((PINE_EVENT_T)((int)PINE_EVENT_BUTTON0_DOWN + b * 2));
		}
	}
	break;
	case WM_LBUTTONUP:
	{
		ptsCursor = MAKEPOINTS(lParam);
		int b = IsButton(ptsCursor.x, ptsCursor.y);
		if (b > -1) {
			char tmp[128];
			sprintf(tmp, "button %d up\n", b);
			OutputDebugStringA(tmp);
			PineFireEvent((PINE_EVENT_T)((int)PINE_EVENT_BUTTON0_UP + b * 2));
		}
	}
	break;
	}
	return FALSE;
}

void* PineLoadResource(int r)
{
	HRSRC hr = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(r), RT_RCDATA);
	HGLOBAL hg = LoadResource(GetModuleHandle(NULL), hr);
	return LockResource(hg);
}

void* PineLoadBitmap(int r)
{
	HRSRC hr = FindResource(hInst, MAKEINTRESOURCE(r), RT_RCDATA);
	DWORD size = SizeofResource(hInst, hr);
	void* res = LockResource(LoadResource(hInst,hr));
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, size);
	void* buffer = GlobalLock(hg);
	CopyMemory(buffer, res, size);
	IStream* str;
	CreateStreamOnHGlobal(hg, FALSE, &str);

	Gdiplus::Bitmap* b = Gdiplus::Bitmap::FromStream(str);
	auto cnt = str->Release();

	GlobalUnlock(hg);
	GlobalFree(hg);
	UnlockResource(res);

	return b;
}

void PineFreeBitmap(void* b)
{
	Gdiplus::GdiplusBase* bmp = (Gdiplus::GdiplusBase*)b;
	delete bmp;
}

PPoint PineGetBitmapSize(void* b)
{
	Gdiplus::Bitmap* bitmap = (Gdiplus::Bitmap*)b;

	Gdiplus::RectF rect;
	Gdiplus::Unit u = Gdiplus::Unit::UnitPixel;
	bitmap->GetBounds(&rect, &u);

	PPoint p = { rect.Width, rect.Height };

	return p;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	hInst = hInstance;
	HWND hDialog = 0;

	InitCommonControls();

	ULONG_PTR m_gdiplusToken;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	CreateConsoleAndFileHandles();

	hDialog = CreateDialogParam(hInst,
		MAKEINTRESOURCE(IDD_MAINPAGE),
		0,
		DialogProc,
		0);

	if (!hDialog)
	{
		TCHAR buf[100];
//		_stprintf_s(buf, _T("Error x%x"), GetLastError());
		MessageBox(0, buf, _T("CreateDialog"), MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}
	ShowWindow(hDialog, nCmdShow);

	MSG  msg;
	int status;
	while ((status = GetMessage(&msg, 0, 0, 0)) != 0)
	{
		if (status == -1)
			return -1;
		if (!IsDialogMessage(hDialog, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}
