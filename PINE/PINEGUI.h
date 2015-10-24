#pragma once
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

	typedef enum {
		PINE_EVENT_TICK,
		PINE_EVENT_TIMER,
		PINE_EVENT_BATTERY,
		PINE_EVENT_BLUETOOTH,
		PINE_EVENT_BUTTON0_DOWN,
		PINE_EVENT_BUTTON0_UP,
		PINE_EVENT_BUTTON1_DOWN,
		PINE_EVENT_BUTTON1_UP,
		PINE_EVENT_BUTTON2_DOWN,
		PINE_EVENT_BUTTON2_UP,
		PINE_EVENT_BUTTON3_DOWN,
		PINE_EVENT_BUTTON3_UP,
	} PINE_EVENT_T;

	typedef enum {
		PINE_COLOR_CLEAR = ~0,
		PINE_COLOR_BLACK = 0,
		PINE_COLOR_WHITE = 1,
	} PINE_COLOR_T;

	typedef struct PPoint {
		long x;
		long y;
	} PPoint;

	void PineClear(PINE_COLOR_T bg);
	void PinePaintBegin();
	void PinePaintEnd();

	PINE_EVENT_T PineWaitForEvent(uint32_t* timeout);

	void PineLog(uint8_t log_level, const char* src_filename, int src_line_number, const char* fmt, va_list va);
	void PineSetBrushColor(PINE_COLOR_T c);
	void PineSetFillColor(PINE_COLOR_T c);
	void PineDrawLine(int x1, int y1, int x2, int y2);
	void PineDrawPolyFilled(int num_points, PPoint*);
	void PineDrawPolyLine(int num_points, PPoint*);
	void PineDrawRectFilled(PINE_COLOR_T c, int x, int y, int w, int h);
    void PineDrawCircleFilled(PINE_COLOR_T c, int x, int y, int radius);
    void PineDrawCircle(PINE_COLOR_T c, int x, int y, int radius);

	void* PineGetSystemFont(const char* font_key);
	void PineDrawText(int x, int y, int w, int h, PINE_COLOR_T bcolor, PINE_COLOR_T tcolor, void* font, const char* text);
	void PineDrawBitmap(int x, int y, int w, int h, void* bitmap);

	void* PineLoadResource(int);
	void* PineLoadBitmap(int);
	PPoint PineGetBitmapSize(void*);
	void PineFreeBitmap(void* b);

	typedef struct PineBatteryState {
		int charge;
		int charging;
		int plugged;
	} PineBatteryState;
	PineBatteryState PineGetBatteryState();

	int PineGetBluetoothConnected();

#if defined(__cplusplus)
}
#endif
