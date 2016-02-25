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
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

extern "C" {
    int PEBBLE_BASE_WIDTH = 236;
    int PEBBLE_BASE_HEIGHT = 394;

    HINSTANCE hInst;
    HWND pineHwnd;
    HDC faceHDC;
    HBITMAP faceBitmap;
};

void ResizeDialogToPixels(HWND hwnd, DWORD w, DWORD h)
{
	RECT r = { 0, 0, (LONG)w, (LONG)h };
	MapDialogRect(hwnd, &r);

	RECT cr;
	GetClientRect(hwnd, &cr);
	RECT wr;
	GetWindowRect(hwnd, &wr);

	double mx = (double)w / r.right;
	double my = (double)h / r.bottom;
	r.right = (LONG)((r.right * mx) + (wr.right - cr.right));
	r.bottom = (LONG)((r.bottom * my) + (wr.bottom - cr.bottom));
	SetWindowPos(hwnd, NULL, 0, 0, r.right, r.bottom, SWP_NOMOVE | SWP_SHOWWINDOW);
}

static DWORD gs_compMode = SRCCOPY;
void PineSetBitmapCompMode(PineCompOp comp_mode) {
    switch (comp_mode) {
    case PineCompOpAssign:
        gs_compMode = SRCCOPY;
        break;
    case PineCompOpAssignInverted:
        gs_compMode = SRCINVERT;
        break;
    case PineCompOpOr:
        gs_compMode = SRCPAINT;
        break;
    case PineCompOpAnd:
        gs_compMode = SRCAND;
        break;
    case PineCompOpClear:
        gs_compMode = SRCERASE;
        break;
    case PineCompOpSet:
        gs_compMode = NOTSRCCOPY;
        break;
    }
}

void PineDrawBitmap(int x, int y, int w, int h, void* bitmap)
{
    if (!bitmap) return;

	Gdiplus::Bitmap* b = (Gdiplus::Bitmap*)bitmap;
	HBITMAP hb;
	b->GetHBITMAP(Gdiplus::Color::White, &hb);

	HDC chdc = CreateCompatibleDC(faceHDC);

	SelectObject(chdc, hb);
	BitBlt(faceHDC, x, y, w, h, chdc, 0, 0, gs_compMode);
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

	*timeout = (uint32_t)(end - start);

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
		SetDialogItemToInt(hwndDlg, IDC_CHARGE, PineBatteryState.charge);
		CheckDlgButton(hwndDlg, IDC_CHARGING, PineBatteryState.charging);
		CheckDlgButton(hwndDlg, IDC_PLUGGED, PineBatteryState.plugged);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHARGE:
            PineBatteryState.charge = GetDialogItemAsInt(hwndDlg, IDC_CHARGE);
			break;
		case IDC_CHARGING:
            PineBatteryState.charging = !PineBatteryState.charging;
			CheckDlgButton(hwndDlg, IDC_CHARGING, PineBatteryState.charging);
			PineFireEvent(PINE_EVENT_BATTERY);
			break;
		case IDC_PLUGGED:
            PineBatteryState.plugged = !PineBatteryState.plugged;
			CheckDlgButton(hwndDlg, IDC_PLUGGED, PineBatteryState.plugged);
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

extern "C" int watch_base;

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
			PineFireEvent(PINE_EVENT_BLUETOOTH);
			break;
        case IDM_WATCH_BLACK:
        case IDM_WATCH_RED:
        case IDM_WATCH_ORANGE:
        case IDM_WATCH_SILVER:
        case IDM_WATCH_WHITE:
            watch_base = wParam - IDM_WATCH_BLACK;
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

POINT PineGetBitmapSize(void* b)
{
	Gdiplus::Bitmap* bitmap = (Gdiplus::Bitmap*)b;

	Gdiplus::RectF rect;
	Gdiplus::Unit u = Gdiplus::Unit::UnitPixel;
	bitmap->GetBounds(&rect, &u);

    POINT p = { (LONG)rect.Width, (LONG)rect.Height };

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
