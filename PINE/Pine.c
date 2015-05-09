#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include "queue.h"
#include "pebble.h"
#include "PINEGUI.h"

#define DEFAULT_BOUNDS	GRect(0,0,144,168)

/*	pretty much everything gets redrawn if anything gets dirty
	so just track it in one place and redraw everything 
	TODO: pretty much every function auto dirties everything,
	update those functions*/
static bool gs_dirty = true;

bool clock_is_24h_style(void) {
	return PineIs24hStyle();
}

void app_log(uint8_t log_level, const char* src_filename, int src_line_number, const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	PineLog(log_level, src_filename, src_line_number, fmt, va);
	va_end(va);
}

int32_t sin_lookup(int32_t angle) {
	double a = (double)angle / TRIG_MAX_ANGLE;
	a = a * M_PI * 2;
	double r = sin(a);
	r = r * TRIG_MAX_ANGLE;
	return (int32_t)r;
}

int32_t cos_lookup(int32_t angle) {
	double a = (double)angle / TRIG_MAX_ANGLE;
	a = a * M_PI * 2;
	double r = cos(a);
	r = r * TRIG_MAX_ANGLE;
	return (int32_t)r;
}

PPoint rotate_point(float angle, GPoint p)
{
	PPoint r;
	float s = sin(angle);
	float c = cos(angle);

	// rotate point
	float xnew = p.x * c - p.y * s;
	float ynew = p.x * s + p.y * c;

	// translate point back:
	r.x = xnew;
	r.y = ynew;
	return r;
}
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

GPoint grect_center_point(const GRect *rect) {
	return GPoint(rect->origin.x + rect->size.w / 2, rect->origin.y + rect->size.h / 2);
}

struct GContext {
	GColor stroke_color;
	GColor fill_color;
} gs_ctx;

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

GPath* gpath_create(const GPathInfo *init) {
	GPath* p = (GPath*)calloc(1,sizeof(GPath));
	p->num_points = init->num_points;
	p->points = (GPoint*)calloc(init->num_points, sizeof(GPoint));
	memcpy(p->points, init->points, init->num_points * sizeof(GPoint));
	return p;
}

void gpath_destroy(GPath* gpath) {
	free(gpath->points);
	free(gpath);
}

static PPoint* rotate_and_offset_gpath(GPath* path) {
	PPoint* points = (PPoint*)malloc(path->num_points * sizeof(PPoint));
	double a = (double)path->rotation / TRIG_MAX_ANGLE;
	a = a * M_PI * 2;

	for (int i = 0; i < path->num_points; i++) {
		points[i] = rotate_point(a, path->points[i]);
		points[i].x += path->offset.x;
		points[i].y += path->offset.y;
	}
	return points;
}

void gpath_draw_filled(GContext* ctx, GPath *path) {
	PPoint* points = rotate_and_offset_gpath(path);

	PineDrawPolyFilled(path->num_points, points);

	free(points);
}

void gpath_draw_outline(GContext* ctx, GPath *path) {
	PPoint* points = rotate_and_offset_gpath(path);

	PineDrawPolyLine(path->num_points, points);

	free(points);
}

void gpath_rotate_to(GPath *path, int32_t angle) {
	path->rotation = angle;
}

void gpath_move_to(GPath *path, GPoint point) {
	path->offset = point;
}

GFont fonts_get_system_font(const char *font_key) {
	return (GFont)PineGetSystemFont(font_key);
}

enum {
	USER_LAYER,
	TEXT_LAYER,
	BITMAP_LAYER,
};

static int layer_number = 0;
struct BaseLayer {
	int number;
	TAILQ_ENTRY(BaseLayer) next;
	TAILQ_HEAD(, BaseLayer) children;
	struct BaseLayer* parent;
	int type;
	GRect bounds;
	bool clips;
	bool hidden;
	bool dirty;
};

struct Layer {
	struct BaseLayer base;
	LayerUpdateProc update;
};
struct TextLayer {
	struct BaseLayer base;
	const char *text;
	GColor bcolor;
	GColor tcolor;
	GFont font;
	GTextAlignment text_alignment;
};
struct BitmapLayer {
	struct BaseLayer base;
	GBitmap* bitmap;
};

static void base_layer_init(struct BaseLayer* l, GRect frame)
{
	l->number = layer_number++;
	l->bounds = frame;
	l->clips = true;
	l->hidden = false;
	l->dirty = true;
	TAILQ_INIT(&l->children);
}

static void base_layer_destroy(struct BaseLayer* l)
{
	if (l->parent) {
		struct BaseLayer* parent = l->parent;
		TAILQ_REMOVE(&parent->children, l, next);
	}
}

Layer* layer_create(GRect frame) {
	Layer* l = calloc(1,sizeof(struct Layer));
	base_layer_init(l,frame);
	l->base.type = USER_LAYER;
	return l;
}

void layer_destroy(Layer* layer) {
	base_layer_destroy(&layer->base);
	free(layer);
}

void layer_mark_dirty(Layer *layer) {
	gs_dirty = layer->base.dirty = true;
}

void layer_set_update_proc(Layer *layer, LayerUpdateProc update_proc) {
	layer->update = update_proc;
}

GRect layer_get_bounds(const Layer *layer) {
	return layer->base.bounds;
}

GRect layer_get_relative_frame(const Layer *layer, const GRect r) {
	GRect f;
	f.origin.x = r.origin.x - layer->base.bounds.origin.x;
	f.origin.y = r.origin.y - layer->base.bounds.origin.y;
	f.size.w = r.size.w - layer->base.bounds.size.w;
	f.size.h = r.size.h - layer->base.bounds.size.h;
	return f;
}

GRect layer_get_frame(const Layer *layer) {
	if (layer->base.parent)
		return layer_get_relative_frame(layer->base.parent, layer->base.bounds);
	return layer->base.bounds;
}
void layer_add_child(Layer *parent, Layer *child) {
	struct BaseLayer* base = &child->base;
	base->parent = parent;
	TAILQ_INSERT_TAIL(&parent->base.children, base, next);
	parent->base.dirty = child->base.dirty = true;

	gs_dirty = true;
}

struct Window {
	LIST_ENTRY(Window) next;
	TAILQ_HEAD(, BaseLayer) layers;
	WindowHandlers handlers;

	GColor bcolor;
};

static LIST_HEAD(gs_window_list, Window) gs_window_list;

Window* window_create(void) {
	struct Window *w = (struct Window*)calloc(1,sizeof(struct Window));
	LIST_INSERT_HEAD(&gs_window_list, w, next);
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
	return TAILQ_FIRST(&window->layers);
}

void window_stack_push(Window *window, bool animated) {
	LIST_INSERT_HEAD(&gs_window_list, window, next);
}

void window_set_background_color(Window *window, GColor background_color) {
	window->bcolor = background_color;
}

TextLayer* text_layer_create(GRect frame) {
	struct TextLayer* l = (struct TextLayer*)calloc(1, sizeof(struct TextLayer));
	base_layer_init(l,frame);
	l->base.type = TEXT_LAYER;
	return l;
}

void text_layer_destroy(TextLayer* text_layer) {
	base_layer_destroy(&text_layer->base);
	free(text_layer);
}

Layer* text_layer_get_layer(TextLayer *text_layer) {
	return (Layer*)text_layer;
}

void text_layer_set_text(TextLayer *text_layer, const char *text) {
	if (text_layer->text) free(text_layer->text);
	text_layer->text = _strdup(text);
	gs_dirty = true;
}

void text_layer_set_background_color(TextLayer *text_layer, GColor color) {
	text_layer->bcolor = color;
}

void text_layer_set_text_color(TextLayer *text_layer, GColor color) {
	text_layer->tcolor = color;
}

void text_layer_set_font(TextLayer *text_layer, GFont font) {
	text_layer->font = font;
}

void text_layer_set_text_alignment(TextLayer *text_layer, GTextAlignment text_alignment) {
	text_layer->text_alignment = text_alignment;
}

void fire_tick_handlers(struct tm* tick_time, TimeUnits u) {
	struct TickTimerServiceEntry* e;
	LIST_FOREACH(e, &TickTimerServiceList, next) {
		if (e->units & u) {
			e->handler(tick_time, u);
		}
	}
}

void pine_draw_text_layer(struct TextLayer* tl) {
	PineDrawText(tl->base.bounds.origin.x, tl->base.bounds.origin.y,
		tl->base.bounds.size.w, tl->base.bounds.size.h,
		tl->bcolor, tl->tcolor, tl->font,
		tl->text);
}

void pine_draw_bitmap_layer(struct BitmapLayer* bl) {
	PineDrawBitmap(bl->base.bounds.origin.x, bl->base.bounds.origin.y,
		bl->base.bounds.size.w, bl->base.bounds.size.h,
		bl->bitmap);
}

void pine_update_child_layers(struct BaseLayer* l);

void pine_update_layer(struct BaseLayer* l) {
	switch (l->type) {
	case TEXT_LAYER: {
		struct TextLayer* tl = (struct TextLayer*)l;
		pine_draw_text_layer(tl);
		}
		break;
	case USER_LAYER: {
		struct Layer* ul = (struct Layer*)l;
		if (ul->update) {
			ul->update(l, &gs_ctx);
		}
		}
		break;
	case BITMAP_LAYER: {
		pine_draw_bitmap_layer(l);
		}
		break;
	}
	pine_update_child_layers(l);
}

void pine_update_layers(struct BaseLayer* l) {
	pine_update_layer(l);
}

void pine_update_child_layers(struct BaseLayer* l) {
	struct BaseLayer* c;
	TAILQ_FOREACH(c, &l->children, next) {
		pine_update_layer(c);
	}
}

void pine_update_windows() {
	Window* w = LIST_FIRST(&gs_window_list);
	PineClear(w->bcolor);
	struct BaseLayer* l;
	TAILQ_FOREACH(l, &w->layers, next) {
		pine_update_layers(l);
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

		if (gs_dirty) {
			PinePaintBegin();
			pine_update_windows();
			PinePaintEnd();
			gs_dirty = false;
		}
	}
}

BitmapLayer* bitmap_layer_create(GRect frame) {
	struct BitmapLayer* l = (struct BitmapLayer*)calloc(1, sizeof(*l));
	base_layer_init(l, frame);
	l->base.type = BITMAP_LAYER;
	return l;
}

void bitmap_layer_destroy(BitmapLayer* bitmap_layer) {

}

Layer* bitmap_layer_get_layer(const BitmapLayer *bitmap_layer) {
	return (Layer*)bitmap_layer;
}

const GBitmap* bitmap_layer_get_bitmap(BitmapLayer *bitmap_layer) {
	return NULL;
}

void bitmap_layer_set_bitmap(BitmapLayer *bitmap_layer, const GBitmap *bitmap) {
	bitmap_layer->bitmap = bitmap->addr;
}

void bitmap_layer_set_alignment(BitmapLayer *bitmap_layer, GAlign alignment) {

}

void bitmap_layer_set_background_color(BitmapLayer *bitmap_layer, GColor color) {

}

void bitmap_layer_set_compositing_mode(BitmapLayer *bitmap_layer, GCompOp mode) {

}

GBitmap* gbitmap_create_with_resource(uint32_t resource_id) {
	GBitmap* b = (GBitmap*)calloc(1,sizeof(*b));
	b->addr = PineLoadBitmap(resource_id);
	PPoint size = PineGetBitmapSize(b->addr);
	b->bounds.origin.x = 0;
	b->bounds.origin.y = 0;
	b->bounds.size.w = size.x;
	b->bounds.size.h = size.y;

	return b;
}

void gbitmap_destroy(GBitmap* bitmap) {

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
