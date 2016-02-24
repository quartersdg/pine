#define WIN32_LEAN_AND_MEAN
#define _X86_
#include <windef.h>
#include <wingdi.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include "queue.h"
#include "pebble.h"
#include "PINEGUI.h"
#include "PINE.h"



struct GContext {
    GColor stroke_color;
    GColor fill_color;
    GCompOp comp_mode;
} gs_ctx;

typedef struct PebbleColorToObject {
    uint32_t    RGB;
    HBRUSH      Brush;
    HBRUSH      Pen;
    uint8_t     Pebble_argb;
} PebbleColorToObject;
PebbleColorToObject PebbleColorObjects[] = {
    { 0x000000,0,0,GColorBlackARGB8 },
    { 0x000055,0,0,GColorOxfordBlueARGB8 },
    { 0x0000AA,0,0,GColorDukeBlueARGB8 },
    { 0x0000FF,0,0,GColorBlueARGB8 },
    { 0x005500,0,0,GColorDarkGreenARGB8 },
    { 0x005555,0,0,GColorMidnightGreenARGB8 },
    { 0x0055AA,0,0,GColorCobaltBlueARGB8 },
    { 0x0055FF,0,0,GColorBlueMoonARGB8 },
    { 0x00AA00,0,0,GColorIslamicGreenARGB8 },
    { 0x00AA55,0,0,GColorJaegerGreenARGB8 },
    { 0x00AAAA,0,0,GColorTiffanyBlueARGB8 },
    { 0x00AAFF,0,0,GColorVividCeruleanARGB8 },
    { 0x00FF00,0,0,GColorGreenARGB8 },
    { 0x00FF55,0,0,GColorMalachiteARGB8 },
    { 0x00FFAA,0,0,GColorMediumSpringGreenARGB8 },
    { 0x00FFFF,0,0,GColorCyanARGB8 },
    { 0x550000,0,0,GColorBulgarianRoseARGB8 },
    { 0x550055,0,0,GColorImperialPurpleARGB8 },
    { 0x5500AA,0,0,GColorIndigoARGB8 },
    { 0x5500FF,0,0,GColorElectricUltramarineARGB8 },
    { 0x555500,0,0,GColorArmyGreenARGB8 },
    { 0x555555,0,0,GColorDarkGrayARGB8 },
    { 0x5555AA,0,0,GColorLibertyARGB8 },
    { 0x5555FF,0,0,GColorVeryLightBlueARGB8 },
    { 0x55AA00,0,0,GColorKellyGreenARGB8 },
    { 0x55AA55,0,0,GColorMayGreenARGB8 },
    { 0x55AAAA,0,0,GColorCadetBlueARGB8 },
    { 0x55AAFF,0,0,GColorPictonBlueARGB8 },
    { 0x55FF00,0,0,GColorBrightGreenARGB8 },
    { 0x55FF55,0,0,GColorScreaminGreenARGB8 },
    { 0x55FFAA,0,0,GColorMediumAquamarineARGB8 },
    { 0x55FFFF,0,0,GColorElectricBlueARGB8 },
    { 0xAA0000,0,0,GColorDarkCandyAppleRedARGB8 },
    { 0xAA0055,0,0,GColorJazzberryJamARGB8 },
    { 0xAA00AA,0,0,GColorPurpleARGB8 },
    { 0xAA00FF,0,0,GColorVividVioletARGB8 },
    { 0xAA5500,0,0,GColorWindsorTanARGB8 },
    { 0xAA5555,0,0,GColorRoseValeARGB8 },
    { 0xAA55AA,0,0,GColorPurpureusARGB8 },
    { 0xAA55FF,0,0,GColorLavenderIndigoARGB8 },
    { 0xAAAA00,0,0,GColorLimerickARGB8 },
    { 0xAAAA55,0,0,GColorBrassARGB8 },
    { 0xAAAAAA,0,0,GColorLightGrayARGB8 },
    { 0xAAAAFF,0,0,GColorBabyBlueEyesARGB8 },
    { 0xAAFF00,0,0,GColorSpringBudARGB8 },
    { 0xAAFF55,0,0,GColorInchwormARGB8 },
    { 0xAAFFAA,0,0,GColorMintGreenARGB8 },
    { 0xAAFFFF,0,0,GColorCelesteARGB8 },
    { 0xFF0000,0,0,GColorRedARGB8 },
    { 0xFF0055,0,0,GColorFollyARGB8 },
    { 0xFF00AA,0,0,GColorFashionMagentaARGB8 },
    { 0xFF00FF,0,0,GColorMagentaARGB8 },
    { 0xFF5500,0,0,GColorOrangeARGB8 },
    { 0xFF5555,0,0,GColorSunsetOrangeARGB8 },
    { 0xFF55AA,0,0,GColorBrilliantRoseARGB8 },
    { 0xFF55FF,0,0,GColorShockingPinkARGB8 },
    { 0xFFAA00,0,0,GColorChromeYellowARGB8 },
    { 0xFFAA55,0,0,GColorRajahARGB8 },
    { 0xFFAAAA,0,0,GColorMelonARGB8 },
    { 0xFFAAFF,0,0,GColorRichBrilliantLavenderARGB8 },
    { 0xFFFF00,0,0,GColorYellowARGB8 },
    { 0xFFFF55,0,0,GColorIcterineARGB8 },
    { 0xFFFFAA,0,0,GColorPastelYellowARGB8 },
    { 0xFFFFFF,0,0,GColorWhiteARGB8 },
};

PebbleColorToObject* FindPebbleObject(GColor c) {
    for (int i = 0;i < ARRAY_LENGTH(PebbleColorObjects);i++) {
        if (c.argb == PebbleColorObjects[i].Pebble_argb) {
            return &PebbleColorObjects[i];
        }
    }
    return NULL;
}

HBRUSH PebbleColorToBrush(GColor c)
{
    PebbleColorToObject* PO = FindPebbleObject(c);
    if (!PO) return 0;
    if (!PO->Brush) {
        PO->Brush = CreateSolidBrush(PO->RGB);
    }
    return PO->Brush;
}

HPEN PebbleColorToPen(GColor c)
{
    PebbleColorToObject* PO = FindPebbleObject(c);
    if (!PO) return 0;
    if (!PO->Brush) {
        PO->Pen = CreatePen(PS_SOLID,0, PO->RGB);
    }
    return PO->Pen;
}

COLORREF PebbleColorToColorRef(GColor c)
{
    PebbleColorToObject* PO = FindPebbleObject(c);
    if (!PO) return 0;

    return PO->RGB;
}

extern int PEBBLE_BASE_WIDTH;
extern int PEBBLE_BASE_HEIGHT;

#define PEBBLE_FACE_WIDTH 144
#define PEBBLE_FACE_HEIGHT 168
#define PEBBLE_BASE_FACE_X 43
#define PEBBLE_BASE_FACE_Y 106

extern HINSTANCE hInst;
extern HWND pineHwnd;
extern HDC faceHDC;
extern HBITMAP faceBitmap;

int watch_base = 0;


void PinePaintBegin()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(pineHwnd, &ps);

    faceHDC = CreateCompatibleDC(hdc);
    faceBitmap = CreateCompatibleBitmap(hdc, PEBBLE_FACE_WIDTH, PEBBLE_FACE_HEIGHT);
    SelectObject(faceHDC, faceBitmap);

    EndPaint(pineHwnd, &ps);
}

void PineClear(GColor bg)
{
    RECT r = { 0, 0, PEBBLE_FACE_WIDTH, PEBBLE_FACE_HEIGHT };
    FillRect(faceHDC, &r, PebbleColorToBrush(bg));
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
    DeleteObject(bitmap);

    RECT r = { PEBBLE_BASE_FACE_X, PEBBLE_BASE_FACE_Y, PEBBLE_BASE_FACE_X + PEBBLE_FACE_WIDTH, PEBBLE_BASE_FACE_Y + PEBBLE_FACE_HEIGHT };
    //FillRect(hdc, &r, (HBRUSH)(0));

    BitBlt(hdc, PEBBLE_BASE_FACE_X, PEBBLE_BASE_FACE_Y, PEBBLE_FACE_WIDTH, PEBBLE_FACE_HEIGHT, faceHDC, 0, 0, SRCCOPY);
    DeleteObject(faceBitmap);
    DeleteObject(faceHDC);
    DeleteObject(chdc);

    EndPaint(pineHwnd, &ps);
}

void psleep(int millis)
{
    Sleep(millis);
}

#undef vsnprintf
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

    printf("%s\n",msg);
}


void PineSetBrushColor(GColor c)
{
    SelectObject(faceHDC, PebbleColorToPen(c));
}

void PineDrawLine(int x1, int y1, int x2, int y2)
{
    MoveToEx(faceHDC, x1, y1, NULL);
    LineTo(faceHDC, x2, y2);
}

void PineDrawPolyFilled(GContext* ctx, int num_points, PPOINT* points)
{
    Polygon(faceHDC, points, num_points);
}

void PineDrawPolyLine(int num_points, PPOINT* points)
{
    Polyline(faceHDC, points, num_points);
}

void PineDrawRectFilled(GColor c, int x, int y, int w, int h)
{
    RECT r = { x, y, x + w, y + h };
    FillRect(faceHDC, &r, PebbleColorToBrush(c));
}

void PineDrawCircleFilled(GContext* ctx, int x, int y, int radius)
{
    Ellipse(faceHDC, x - radius, y - radius, x + radius, y + radius);
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

void PineDrawText(int x, int y, int w, int h, GColor bcolor, GColor tcolor, void* font, const char* text)
{
    RECT r;
    int oldmode;
    SetRect(&r, x, y, x + w, y + h);
    SetTextColor(faceHDC, PebbleColorToColorRef(tcolor));
    if (bcolor.argb == GColorClearARGB8) {
        oldmode = SetBkMode(faceHDC, TRANSPARENT);
    }
    else {
        SetBkColor(faceHDC, PebbleColorToColorRef(bcolor));
    }
    SelectObject(faceHDC, (HFONT)font);
    DrawText(faceHDC, text, -1, &r, DT_BOTTOM);
    if (bcolor.argb == GColorClearARGB8) {
        SetBkMode(faceHDC, oldmode);
    }
}

BatteryState PineBatteryState = { 75, 1, 1 };
BatteryChargeState PineGetBatteryState()
{
    BatteryChargeState bs = { PineBatteryState.charge, PineBatteryState.charging, PineBatteryState.plugged };
    return bs;
}


bool clock_is_24h_style(void) {
    wchar_t buf[128];
    GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_STIMEFORMAT, buf, sizeof(buf));
    return wcschr(buf, L'H') != NULL;
}

void PinePaintEnd()
{
    InvalidateRect(pineHwnd, NULL, FALSE);
    //SendMessage(pineHwnd, WM_PAINT, 0, 0);
}


#define DEFAULT_BOUNDS	GRect(0,0,144,168)

/*	pretty much everything gets redrawn if anything gets dirty
	so just track it in one place and redraw everything 
	TODO: pretty much every function auto dirties everything,
	update those functions*/
static bool gs_dirty = true;

void pine_something_is_dirty(void) {
	gs_dirty = true;
}

#pragma region TickTimer
struct TickTimerServiceEntry {
	LIST_ENTRY(TickTimerServiceEntry) next;
	TimeUnits units;
	TickHandler handler;
};

static LIST_HEAD(TickTimerServiceList, TickTimerServiceEntry) TickTimerServiceList = { NULL };

void tick_timer_service_subscribe(TimeUnits tick_units, TickHandler handler) {
	struct TickTimerServiceEntry* e = (struct TickTimerServiceEntry*)calloc(1, sizeof(*e));
	e->units = tick_units;
	e->handler = handler;
	LIST_INSERT_HEAD(&TickTimerServiceList, e, next);
}

void tick_timer_service_unsubscribe(void) {

}

void fire_tick_handlers(struct tm* tick_time, TimeUnits u) {
	struct TickTimerServiceEntry* e;
	LIST_FOREACH(e, &TickTimerServiceList, next) {
		if (e->units & u) {
			e->handler(tick_time, u);
		}
	}
}
#pragma endregion

GPoint grect_center_point(const GRect *rect) {
	return GPoint(rect->origin.x + rect->size.w / 2, rect->origin.y + rect->size.h / 2);
}


void* pine_get_global_context(void) {
	return (void*)&gs_ctx;
}

void graphics_context_set_stroke_color(GContext* ctx, GColor color) {
	ctx->stroke_color = color;
	PineSetBrushColor(color);
}

void graphics_context_set_fill_color(GContext* ctx, GColor color) {
	ctx->fill_color = color;
    SelectObject(faceHDC, PebbleColorToBrush(color));
}

void graphics_draw_pixel(GContext* ctx, GPoint point) {
    SetPixel(faceHDC, point.x, point.y, PebbleColorToColorRef(ctx->stroke_color));
}

void graphics_draw_line(GContext* ctx, GPoint p0, GPoint p1) {
	PineDrawLine(p0.x, p0.y, p1.x, p1.y);
}

void graphics_fill_rect(GContext* ctx, GRect rect, uint16_t corner_radius, GCornerMask corner_mask) {
	PineDrawRectFilled(ctx->fill_color,rect.origin.x, rect.origin.y, rect.size.w, rect.size.h);
}

void graphics_fill_circle(GContext* ctx, GPoint p, uint16_t radius) {
	PineDrawCircleFilled(ctx, p.x, p.y, radius);
}

void graphics_context_set_compositing_mode(GContext* ctx, GCompOp mode) {
    ctx->comp_mode = mode;
}

GFont fonts_get_system_font(const char *font_key) {
	return (GFont)PineGetSystemFont(font_key);
}


typedef struct ButtonState {
    char        down;
    uint64_t    time;
    void*       context;
} ButtonState;
static ButtonState gs_buttonStates[NUM_BUTTONS] = { 0 };

typedef struct LongClickHandler {
    char         subscribed;
    char         fired;
    ClickHandler down;
    ClickHandler up;
    uint16_t delay_ms;
} LongClickHandler;
typedef struct RepeatingClickHandler {
    char            fired;
    ClickHandler    handler;
    uint16_t        repeat_interval_ms;
    uint64_t        last_fired;
} RepeatingClickHandler;
static int num_windows = 0;
struct Window {
	LIST_ENTRY(Window) next;
	TAILQ_HEAD(, BaseLayer) layers;
	int number;
	WindowHandlers handlers;

	GColor bcolor;

	ClickConfigProvider click_config_provider;
	void* click_config_context;
	bool click_config_provider_called;

    ClickHandler clickHandlers[NUM_BUTTONS];
    LongClickHandler longClickHandlers[NUM_BUTTONS];
    RepeatingClickHandler repeatingClickHandlers[NUM_BUTTONS];
};

static LIST_HEAD(gs_window_list, Window) gs_window_list;

static void init_click_handlers(Window* w) {
    memset(w->clickHandlers, 0, sizeof(w->clickHandlers));
    memset(w->longClickHandlers, 0, sizeof(w->longClickHandlers));
    memset(w->repeatingClickHandlers, 0, sizeof(w->repeatingClickHandlers));
}


Window* window_create(void) {
	struct Window *w = (struct Window*)calloc(1,sizeof(struct Window));
	w->number = ++num_windows;
//	LIST_INSERT_HEAD(&gs_window_list, w, next);
	TAILQ_INIT(&w->layers);
	struct BaseLayer* base = (struct BaseLayer*)layer_create(DEFAULT_BOUNDS);
	TAILQ_INSERT_HEAD(&w->layers, base, next);
	return w;
}

void window_destroy(Window* window) {
	free(window);
}

void window_set_window_handlers(Window *window, WindowHandlers handlers) {
	window->handlers = handlers;
}

struct Layer* window_get_root_layer(const Window *window) {
	return (struct Layer*)TAILQ_FIRST(&window->layers);
}

void window_stack_push(Window *window, bool animated) {
	LIST_INSERT_HEAD(&gs_window_list, window, next);
	//window->click_config_provider_called = false;
}

Window* window_stack_pop(bool animated) {
	if (LIST_EMPTY(&gs_window_list)) return NULL;
	Window* w = LIST_FIRST(&gs_window_list);
	if (NULL == LIST_NEXT(w,next)) return NULL;
	LIST_REMOVE(w, next);
	//LIST_NEXT(w,next)->click_config_provider_called = false;
	return w;
}

void window_set_background_color(Window *window, GColor background_color) {
	window->bcolor = background_color;
}

static void pine_update_windows() {
	Window* w = LIST_FIRST(&gs_window_list);
	{
		struct BaseLayer* l;
		TAILQ_FOREACH(l, &w->layers, next) {
			pine_update_layers(l);
		}
	}
}

struct app_timer_t {
	LIST_ENTRY(app_timer_t) next;
	AppTimerCallback callback;
	void* callback_data;
	uint32_t timeout_ms;
};

static LIST_HEAD(, app_timer_t) gs_app_timers = { 0 };

AppTimer* app_timer_register(uint32_t timeout_ms, AppTimerCallback callback, void* callback_data) {
	struct app_timer_t* t = (struct app_timer_t*)malloc(sizeof(*t));
	t->callback = callback;
	t->callback_data = callback_data;
	t->timeout_ms = timeout_ms;

	LIST_INSERT_HEAD(&gs_app_timers, t, next);

	return (AppTimer*)t;
}

static uint32_t find_lowest_app_timer() {
	struct app_timer_t* t;
	uint32_t min = (uint32_t)-1;
	LIST_FOREACH(t, &gs_app_timers, next) {
		if (t->timeout_ms < min) min = t->timeout_ms;
	}
	return min;
}

static void update_and_run_app_timers(uint32_t delta) {
	struct app_timer_t* t;
	LIST_FOREACH(t, &gs_app_timers, next) {
		if (t->timeout_ms <= delta) {
			t->callback(t->callback_data);
			LIST_REMOVE(t, next);
		} else {
			t->timeout_ms -= delta;
		}
	}
}
#define CHECK_AND_CALL_TIME(u,fn) if (tick_time.u != previous_tick_time.u) fn;

static BatteryStateHandler gs_battery_handler = NULL;

static void handle_battery_callback(BatteryChargeState bs) {
	if (gs_battery_handler) {
		gs_battery_handler(bs);
	}
}

static int gs_bluetoothConnected = FALSE;
int PineGetBluetoothConnected()
{
    return gs_bluetoothConnected;
}


static BluetoothConnectionHandler gs_bluetooth_handler = NULL;

static void handle_bluetooth_callback(bool c) {
    gs_bluetoothConnected = !gs_bluetoothConnected;
	if (gs_bluetooth_handler) {
		gs_bluetooth_handler(c);
	}
}


void window_set_click_config_provider(Window *window, ClickConfigProvider click_config_provider) {
	window->click_config_provider = click_config_provider;
	window->click_config_provider_called = false;
	window->click_config_context = window;
}

void window_set_fullscreen(Window *window, bool enabled) {
	/* TODO: Figure out if I want to bother with this */
}


void window_single_click_subscribe(ButtonId button_id, ClickHandler handler) {
    Window* w = LIST_FIRST(&gs_window_list);
    if (!w) return;

    w->clickHandlers[button_id] = handler;
    w->repeatingClickHandlers[button_id].handler = NULL;
}

void window_single_repeating_click_subscribe(ButtonId button_id, uint16_t repeat_interval_ms, ClickHandler handler) {
    Window* w = LIST_FIRST(&gs_window_list);
    if (!w) return;

    if (button_id == BUTTON_ID_BACK) return;
    w->clickHandlers[button_id] = NULL;
    if (repeat_interval_ms < 30) {
        window_single_click_subscribe(button_id, handler);
    } else {
        gs_buttonStates[button_id].time = GetTickCount64();
        w->repeatingClickHandlers[button_id].fired = false;
        w->repeatingClickHandlers[button_id].last_fired = GetTickCount64();
        w->repeatingClickHandlers[button_id].handler = handler;
        w->repeatingClickHandlers[button_id].repeat_interval_ms = repeat_interval_ms;
    }
}

void window_long_click_subscribe(ButtonId button_id, uint16_t delay_ms, ClickHandler down_handler, ClickHandler up_handler) {
    Window* w = LIST_FIRST(&gs_window_list);
    if (!w) return;

    if (button_id == BUTTON_ID_BACK) return;
    gs_buttonStates[button_id].time = GetTickCount64();
    w->longClickHandlers[button_id].subscribed = true;
    w->longClickHandlers[button_id].fired = false;
    w->longClickHandlers[button_id].down = down_handler;
    w->longClickHandlers[button_id].up = up_handler;
    w->longClickHandlers[button_id].delay_ms = delay_ms==0? 500:delay_ms;
}

size_t pine_strftime(char * _Buf, size_t _SizeInBytes, const char * _Format, const struct tm * _Tm) {
	if (0 == strcmp(_Format, "%T")) {
		_Format = "%H:%M:%S";
	}
	return strftime(_Buf, _SizeInBytes, _Format, _Tm);
}

static bool check_long_click(Window* w, int Button) {
    bool Handled = false;
    uint64_t CurrentTime = GetTickCount64();
    if (w->longClickHandlers[Button].subscribed) {
        if ((CurrentTime - gs_buttonStates[Button].time) >
            w->longClickHandlers[Button].delay_ms) {
            if (!w->longClickHandlers[Button].fired) {
                w->longClickHandlers[Button].fired = true;
                if (w->longClickHandlers[Button].down) {
                    w->longClickHandlers[Button].down(NULL, gs_buttonStates[Button].context);
                }
            }
        }
        Handled = true;
    }
    return Handled;
}

static void check_click_handlers(PINE_EVENT_T e) {
	Window* w = LIST_FIRST(&gs_window_list);
	if (!w) return;
	if (!w->click_config_provider_called) {
		init_click_handlers(w);
		if (w->click_config_provider) {
			w->click_config_provider(w->click_config_context);
		}
		w->click_config_provider_called = true;
	}

    bool Changed[NUM_BUTTONS] = { 0 };
    uint64_t CurrentTime = GetTickCount64();

    if (e >= PINE_EVENT_BUTTON0_DOWN && e <= PINE_EVENT_BUTTON3_UP) {
        int Button = (e - PINE_EVENT_BUTTON0_DOWN) / 2;
        int Down = !((e - PINE_EVENT_BUTTON0_DOWN) % 2);

        if (gs_buttonStates[Button].down != Down) {
            gs_buttonStates[Button].time = CurrentTime;
            gs_buttonStates[Button].down = Down;
            Changed[Button] = true;
        }
    }

    bool ShouldWindowPop = false;
    bool BackWasDowned = false;

    int Button;
    for (Button = 0;Button<NUM_BUTTONS;Button++) {
        int Down = gs_buttonStates[Button].down;
        if (Down) {
            if (0 == Button) {
                ShouldWindowPop = true;
            }
            if (!check_long_click(w,Button)) {
                if (w->repeatingClickHandlers[Button].handler) {
                    if ((CurrentTime - w->repeatingClickHandlers[Button].last_fired) >
                        w->repeatingClickHandlers[Button].repeat_interval_ms) {
                        w->repeatingClickHandlers[Button].handler(NULL, gs_buttonStates[Button].context);
                        w->repeatingClickHandlers[Button].last_fired = CurrentTime;
                    }
                } else if (w->clickHandlers[Button]) {
                    w->clickHandlers[Button](NULL, gs_buttonStates[Button].context);
                    if (0 == Button) {
                        ShouldWindowPop = false;
                    }
                }
            }
        } else {
            if (w->longClickHandlers[Button].subscribed &&
                w->longClickHandlers[Button].fired) {
                if (w->longClickHandlers[Button].up) {
                    w->longClickHandlers[Button].up(NULL, gs_buttonStates[Button].context);
                }
                w->longClickHandlers[Button].fired = false;
            } else if (w->longClickHandlers[Button].subscribed &&
                !w->longClickHandlers[Button].fired &&
                w->clickHandlers[Button] &&
                Changed[Button]) {
                w->clickHandlers[Button](NULL, gs_buttonStates[Button].context);
                if (0 == Button) {
                    ShouldWindowPop = false;
                }
            }
        }
    }

    if (ShouldWindowPop) {
        window_stack_pop(false);
    }
}

void window_set_click_context(ButtonId button_id, void *context) {
    gs_buttonStates[button_id].context = context;
}

#if 0
#define MAGIC_PUSH_TIME 500

 static void handle_click(PINE_EVENT_T e) {
	 static uint64_t press_start;
	 uint64_t press_end;
	 char down;
	 char button;
	 down = ((e - PINE_EVENT_BUTTON0_DOWN) % 2) == 0;
	 button = ((e - PINE_EVENT_BUTTON0_DOWN) / 2);

	 if (down) {
		 press_start = clock();
	 } else {
		 press_end = clock();
		 if (press_end - press_start > MAGIC_PUSH_TIME)
		 {
			 if (w->longClickHandlers[button].down) {
                 w->longClickHandlers[button].down(NULL, NULL);
			 }
		 }
		 if (w->clickHandlers[button]) {
             w->_clickHandlers[button](NULL, 0);
		 } else {
			 if (PINE_EVENT_BUTTON0_DOWN == e) {
				 window_stack_pop(false);
				 gs_dirty = true;
			 }
		 }
	 }
}
#endif

void app_event_loop(void) {
	time_t current_time;
	struct tm previous_tick_time;
	struct tm tick_time;
	uint32_t time_to_wait;
	PINE_EVENT_T e;

	time(&current_time);
	memset(&previous_tick_time, 0, sizeof(previous_tick_time));
	//previous_tick_time = *localtime(&current_time);

	Window* w = LIST_FIRST(&gs_window_list);

	if (w->handlers.load) w->handlers.load(w);

	for (;;) {
		time_to_wait = find_lowest_app_timer();
		if (100 < time_to_wait) time_to_wait = 100;
		e = PineWaitForEvent(&time_to_wait);

        time(&current_time);
        tick_time = *localtime(&current_time);

		update_and_run_app_timers(time_to_wait);

		fire_tick_handlers(&tick_time,SECOND_UNIT);
		CHECK_AND_CALL_TIME(tm_min, fire_tick_handlers(&tick_time,MINUTE_UNIT));
		CHECK_AND_CALL_TIME(tm_hour, fire_tick_handlers(&tick_time, HOUR_UNIT));
		CHECK_AND_CALL_TIME(tm_mday, fire_tick_handlers(&tick_time, DAY_UNIT));
		CHECK_AND_CALL_TIME(tm_mon, fire_tick_handlers(&tick_time, MONTH_UNIT));
		CHECK_AND_CALL_TIME(tm_year, fire_tick_handlers(&tick_time, YEAR_UNIT));

		check_click_handlers(e);

		previous_tick_time = tick_time;

		if (PINE_EVENT_BATTERY == e) handle_battery_callback(PineGetBatteryState());
		if (PINE_EVENT_BLUETOOTH == e) handle_bluetooth_callback(PineGetBluetoothConnected());
#if 0
		if (PINE_EVENT_BUTTON0_DOWN <= e && PINE_EVENT_BUTTON3_UP >= e) {
			handle_click(e);
		}
#endif

		if (gs_dirty) {
			PinePaintBegin();
			pine_update_windows();
			PinePaintEnd();
			gs_dirty = false;
		}
	}
}

GBitmap* gbitmap_create_with_resource(uint32_t resource_id) {
	GBitmap* b = (GBitmap*)calloc(1,sizeof(*b));
	b->addr = PineLoadBitmap(resource_id);
	POINT size = PineGetBitmapSize(b->addr);

	b->bounds.origin.x = 0;
	b->bounds.origin.y = 0;
	b->bounds.size.w = (int16_t)size.x;
	b->bounds.size.h = (int16_t)size.y;

	return b;
}

void gbitmap_destroy(GBitmap* bitmap) {
	PineFreeBitmap(bitmap->addr);
	free(bitmap);
}

void layer_remove_from_parent(Layer *c) {
	struct BaseLayer* child = (struct BaseLayer*)c;
	struct BaseLayer* parent = child->parent;
	if (!parent) return;

	TAILQ_REMOVE(&parent->children, child, next);
}

bool bluetooth_connection_service_peek(void) {
	return PineGetBluetoothConnected();
}

void bluetooth_connection_service_subscribe(BluetoothConnectionHandler handler) {
	gs_bluetooth_handler = handler;
}

void bluetooth_connection_service_unsubscribe(void) {
	gs_bluetooth_handler = NULL;
}

void battery_state_service_subscribe(BatteryStateHandler handler) {
	gs_battery_handler = handler;
}

void battery_state_service_unsubscribe(void) {
	gs_battery_handler = NULL;
}

BatteryChargeState battery_state_service_peek(void) {
    BatteryChargeState bs = PineGetBatteryState();

	return bs;
}
