#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
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

    void PinePaint();

    PINE_EVENT_T PineWaitForEvent(uint32_t* timeout);

    typedef enum {
        PineCompOpAssign,
        PineCompOpAssignInverted,
        PineCompOpOr,
        PineCompOpAnd,
        PineCompOpClear,
        PineCompOpSet,
    } PineCompOp;

	void* PineLoadBitmap(int);
    void PineSetBitmapCompMode(PineCompOp comp_mode);
	void PineDrawBitmap(int x, int y, int w, int h, void* bitmap);
	POINT PineGetBitmapSize(void*);
	void PineFreeBitmap(void* b);

    typedef struct BatteryState {
        int charge, charging, plugged;
    } BatteryState;

    extern BatteryState PineBatteryState;
#if defined(__cplusplus)
}
#endif
