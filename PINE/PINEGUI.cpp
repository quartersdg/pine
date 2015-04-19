// PINE.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <Windows.h>
#include <tchar.h>
#include <CommCtrl.h>
#include "PINE.h"
#include "PINEGUI.h"

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
	r.bottom = (r.bottom * my) + (wr.right - cr.right);
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

void PineClear()
{
	RECT r = { 0, 0, PEBBLE_FACE_WIDTH, PEBBLE_FACE_HEIGHT };
	FillRect(faceHDC, &r, (HBRUSH)(BLACK_BRUSH));
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

void psleep(int millis)
{
	Sleep(millis);
}

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
		return RGB(0,0,0);
		break;
	case PINE_COLOR_WHITE:
		return RGB(255,255,255);
		break;
	}
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

#include "pebble_fonts.h"
typedef struct PINE_SYSTEM_FONT_T {
	const char* font_key;
	const char* system_font;
	int height;
	bool bold;
} PINE_SYSTEM_FONT_T;
static PINE_SYSTEM_FONT_T gs_system_fonts[] = {
	{ FONT_KEY_GOTHIC_14, "Franklin Gothic", 14, false },
	{ FONT_KEY_GOTHIC_14_BOLD, "Franklin Gothic", 14, true },
	{ FONT_KEY_GOTHIC_18, "Franklin Gothic", 18, false },
	{ FONT_KEY_GOTHIC_18_BOLD, "Franklin Gothic", 18, true },
	{ FONT_KEY_GOTHIC_24, "Franklin Gothic", 24, false },
	{ FONT_KEY_GOTHIC_24_BOLD, "Franklin Gothic", 24, true },
	{ FONT_KEY_GOTHIC_28, "Franklin Gothic", 28, false },
	{ FONT_KEY_GOTHIC_28_BOLD, "Franklin Gothic", 28, true },
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
	SetRect(&r, x, y, x + w, y + h);
	SetTextColor(faceHDC, PineColorToColorRef(tcolor));
	SetBkColor(faceHDC, PineColorToColorRef(bcolor));
	SelectObject(faceHDC,(HFONT)font);
	DrawText(faceHDC, text, -1, &r, DT_BOTTOM);
}

void PinePaintEnd()
{
	InvalidateRect(pineHwnd, NULL, FALSE);
	//SendMessage(pineHwnd, WM_PAINT, 0, 0);
}

extern void main();
DWORD appThreadId = 0;
static DWORD WINAPI AppThread(void* args) {
	main();
	return 0;
}

BOOL CALLBACK DialogProc(HWND hwnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		ResizeDialogToPixels(hwnd, PEBBLE_BASE_WIDTH, PEBBLE_BASE_HEIGHT);
		pineHwnd = hwnd;
		CreateThread(NULL, 0, AppThread, NULL, 0, &appThreadId);
		return TRUE;
	case WM_COMMAND:
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
	}
	return FALSE;
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
