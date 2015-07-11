#include <stdint.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include "queue.h"
#include "pebble.h"
#include "PINEGUI.h"
#include "PINE.h"

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

struct GContext {
	GColor stroke_color;
	GColor fill_color;
} gs_ctx;

void* pine_get_global_context(void) {
	return (void*)&gs_ctx;
}

void graphics_context_set_stroke_color(GContext* ctx, GColor color) {
	ctx->stroke_color = color;
	PineSetBrushColor((PINE_COLOR_T)color);
}

void graphics_context_set_fill_color(GContext* ctx, GColor color) {
	ctx->fill_color = color;
	PineSetFillColor((PINE_COLOR_T)color);
}

void graphics_draw_line(GContext* ctx, GPoint p0, GPoint p1) {
	PineDrawLine(p0.x, p0.y, p1.x, p1.y);
}

void graphics_fill_rect(GContext* ctx, GRect rect, uint16_t corner_radius, GCornerMask corner_mask) {
	PineDrawRectFilled(ctx->fill_color,rect.origin.x, rect.origin.y, rect.size.w, rect.size.h);
}

void graphics_fill_circle(GContext* ctx, GPoint p, uint16_t radius) {
	PineDrawCircleFilled(ctx->fill_color, p.x, p.y, radius);
}

GFont fonts_get_system_font(const char *font_key) {
	return (GFont)PineGetSystemFont(font_key);
}

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
};

static LIST_HEAD(gs_window_list, Window) gs_window_list;

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
	window->click_config_provider_called = false;
}

Window* window_stack_pop(bool animated) {
	if (LIST_EMPTY(&gs_window_list)) return NULL;
	Window* w = LIST_FIRST(&gs_window_list);
	if (NULL == LIST_NEXT(w,next)) return NULL;
	LIST_REMOVE(w, next);
	LIST_NEXT(w,next)->click_config_provider_called = false;
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

static void handle_battery_callback(PineBatteryState pbs) {
	BatteryChargeState s = { pbs.charge, pbs.charging, pbs.plugged };
	if (gs_battery_handler) {
		gs_battery_handler(s);
	}
}

static BluetoothConnectionHandler gs_bluetooth_handler = NULL;

static void handle_bluetooth_callback(bool c) {
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

static ClickHandler gs_clickHandlers[NUM_BUTTONS] = { 0 };
typedef struct LongClickHandler {
	ClickHandler down;
	ClickHandler up;
	uint16_t delay_ms;
} LongClickHandler;
static LongClickHandler gs_longClickHandlers[NUM_BUTTONS] = { 0 };
typedef struct RepeatingClickHandler {
	ClickHandler handler;
	uint16_t repeat_interval_ms;
} RepeatingClickHandler;
static RepeatingClickHandler gs_repeatingClickHandlers[NUM_BUTTONS] = { 0 };

void window_single_click_subscribe(ButtonId button_id, ClickHandler handler) {
	gs_clickHandlers[button_id] = handler;
}

void window_long_click_subscribe(ButtonId button_id, uint16_t delay_ms, ClickHandler down_handler, ClickHandler up_handler) {
	if (button_id == BUTTON_ID_BACK) return;
	gs_longClickHandlers[button_id].down = down_handler;
	gs_longClickHandlers[button_id].up = up_handler;
	gs_longClickHandlers[button_id].delay_ms = delay_ms;
}

void window_single_repeating_click_subscribe(ButtonId button_id, uint16_t repeat_interval_ms, ClickHandler handler) {
	if (button_id == BUTTON_ID_BACK) return;
	gs_repeatingClickHandlers[button_id].handler = handler;
	gs_repeatingClickHandlers[button_id].repeat_interval_ms = repeat_interval_ms;
}

static void init_click_handlers(void) {
	memset(gs_clickHandlers, 0, sizeof(gs_clickHandlers));
	memset(gs_longClickHandlers, 0, sizeof(gs_longClickHandlers));
	memset(gs_repeatingClickHandlers, 0, sizeof(gs_repeatingClickHandlers));
}

size_t pine_strftime(char * _Buf, size_t _SizeInBytes, const char * _Format, const struct tm * _Tm) {
	if (0 == strcmp(_Format, "%T")) {
		_Format = "%H:%M:%S";
	}
	return strftime(_Buf, _SizeInBytes, _Format, _Tm);
}

static void check_click_handlers(PINE_EVENT_T e) {
	Window* w = LIST_FIRST(&gs_window_list);
	if (!w) return;
	if (!w->click_config_provider_called) {
		init_click_handlers();
		if (w->click_config_provider) {
			w->click_config_provider(w->click_config_context);
		}
		w->click_config_provider_called = true;
	}
}


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
			 if (gs_longClickHandlers[button].down) {
				 gs_longClickHandlers[button].down(NULL, NULL);
			 }
		 }
		 if (gs_clickHandlers[button]) {
			 gs_clickHandlers[button](NULL, 0);
		 } else {
			 if (PINE_EVENT_BUTTON0_DOWN == e) {
				 window_stack_pop(false);
				 gs_dirty = true;
			 }
		 }
	 }
}

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
		if (1000 < time_to_wait) time_to_wait = 1000;
		e = PineWaitForEvent(&time_to_wait);

		check_click_handlers(e);
		update_and_run_app_timers(time_to_wait);

		time(&current_time);
		tick_time = *localtime(&current_time);

		fire_tick_handlers(&tick_time,SECOND_UNIT);
		CHECK_AND_CALL_TIME(tm_min, fire_tick_handlers(&tick_time,MINUTE_UNIT));
		CHECK_AND_CALL_TIME(tm_hour, fire_tick_handlers(&tick_time, HOUR_UNIT));
		CHECK_AND_CALL_TIME(tm_mday, fire_tick_handlers(&tick_time, DAY_UNIT));
		CHECK_AND_CALL_TIME(tm_mon, fire_tick_handlers(&tick_time, MONTH_UNIT));
		CHECK_AND_CALL_TIME(tm_year, fire_tick_handlers(&tick_time, YEAR_UNIT));

		previous_tick_time = tick_time;

		if (PINE_EVENT_BATTERY == e) handle_battery_callback(PineGetBatteryState());
		if (PINE_EVENT_BLUETOOTH == e) handle_bluetooth_callback(PineGetBluetoothConnected());
		if (PINE_EVENT_BUTTON0_DOWN <= e && PINE_EVENT_BUTTON3_UP >= e) {
			handle_click(e);
		}

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
	PPoint size = PineGetBitmapSize(b->addr);
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

#undef vsnprintf
int snprintf(char* buf, int len, char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	int l = vsnprintf(buf, len, fmt, va);
	va_end(va);
	return l;
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
	PineBatteryState pbs = PineGetBatteryState();
	BatteryChargeState s = { pbs.charge, pbs.charging, pbs.plugged };

	return s;
}
