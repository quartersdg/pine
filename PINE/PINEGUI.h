#pragma once
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

	typedef enum {
		PINE_EVENT_TICK,
		PINE_EVENT_TIMER,
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

	void PineClear();
	void PinePaintBegin();
	void PinePaintEnd();

	PINE_EVENT_T PineWaitForEvent(uint32_t* timeout);

	void psleep(int millis);
	void PineSetBrushColor(PINE_COLOR_T c);
	void PineSetFillColor(PINE_COLOR_T c);
	void PineDrawLine(int x1, int y1, int x2, int y2);
	void PineDrawPolyFilled(int num_points, PPoint*);
	void PineDrawPolyLine(int num_points, PPoint*);
	void PineDrawRectFilled(PINE_COLOR_T c, int x, int y, int w, int h);
	void* PineGetSystemFont(const char* font_key);
	void PineDrawText(int x, int y, int w, int h, PINE_COLOR_T bcolor, PINE_COLOR_T tcolor, void* font, const char* text);
	int PineIs24hStyle();

#if defined(__cplusplus)
}
#endif
