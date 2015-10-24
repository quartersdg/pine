#include "analogdigital.h"

#include "pebble.h"

static Window *window;
static Layer *s_simple_bg_layer, *s_hands_layer;

static GPath *s_minute_arrow, *s_hour_arrow;

static GPoint scale_point(const GPoint p, double xscale, double yscale) {
    GPoint n;
    n.x = (uint16_t)((double)p.x * xscale);
    n.y = (uint16_t)((double)p.y * yscale);
    return n;
}

static GPoint add_points(const GPoint a, const GPoint b) {
    GPoint c;
    c.x = a.x + b.x;
    c.y = a.y + b.y;
    return c;
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, GColorWhite);
}

typedef struct digit_piece {
    int m, h;
} digit_piece;
typedef struct digit {
    digit_piece pieces[6];
} digit;

static digit digits[] = {
    { // 0
        { { 15,30 },{ 30,45 },{ 0,30 },{ 0,30 },{ 0,15 },{ 0,45 }, },
    },
    { // 1
        { { 45,45 },{ 30,30 },{ 45,45 },{ 0,30 },{ 45,45 },{ 0,0 }, },
    },
    { // 2
        { { 15,15 },{ 45,30 },{ 15,30 },{ 0,45 },{ 0,15 },{ 45,45 }, },
    },
    { // 3
        { { 15,15 },{ 45,30 },{ 15,15 },{ 0,45 },{ 15,15 },{ 0,45 }, },
    },
    { // 4
        { { 30,30 },{ 30,30 },{ 0,15 },{ 0,45 },{ 45,45 },{ 0,0 }, },
    },
    { // 5
        { { 15,30 },{ 45,45 },{ 0,15 },{ 30,45 },{ 15,15 },{ 0,45 }, },
    },
    { // 6
        { { 15,30 },{ 45,45 },{ 0,30 },{ 30,45 },{ 15,0 },{ 0,45 }, },
    },
    { // 7
        { { 15,15 },{ 45,30 },{ 45,45 },{ 0,30 },{ 45,45 },{ 0,0 }, },
    },
    { // 8
        { { 30,15 },{ 30,45 },{ 15,30 },{ 45,30 },{ 0,15 },{ 0,45 }, },
    },
    { // 9
        { { 15,30 },{ 30,45 },{ 15,0 },{ 45,30 },{ 45,45 },{ 0,0 }, },
    },
};

static void draw_digit(GContext* ctx, GPoint topleft, GPoint step, digit d) {
    int x, y, i = 0;
    GPoint p = topleft;
    for (y = 0;y < 3;y++)
    {
        p.x = topleft.x;
        for (x = 0;x < 2;x++) {
            gpath_move_to(s_minute_arrow, p);
            //graphics_draw_circle(ctx, p, step.y / 2);
            gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * d.pieces[i].m / 60);
            gpath_draw_filled(ctx, s_minute_arrow);
            gpath_draw_outline(ctx, s_minute_arrow);
            gpath_move_to(s_hour_arrow, p);
            gpath_rotate_to(s_hour_arrow, TRIG_MAX_ANGLE * d.pieces[i].h / 60);
            gpath_draw_filled(ctx, s_hour_arrow);
            gpath_draw_outline(ctx, s_hour_arrow);
            p.x += step.x;
            i++;
        }
        p.y += step.y;
    }

}

static AppTimer* draw_timer;
static int lerp = 0;
static int spin = 0;
static int prev_hour_tens = 0;
static int prev_hour_ones = 0;
static int prev_min_tens = 0;
static int prev_min_ones = 0;
static int first = 1;

static void draw_time_timer(void* context) {
#if 0
    if (spin < 60) {
        spin += 3;
        draw_timer = app_timer_register(100, draw_time_timer, NULL);
        layer_mark_dirty(window_get_root_layer(window));
    }
    else
#endif
    {
        lerp += 10;
        if (lerp <= 100)
        {
            draw_timer = app_timer_register(100, draw_time_timer, NULL);
            layer_mark_dirty(window_get_root_layer(window));
        }
        else
        {
            lerp = 0;
            spin = 0;
        }
    }
}

static digit spin_digit(digit d, int spin) {
    int i;
    for (i = 0;i < 6;i++) {
        d.pieces[i].m = (d.pieces[i].m + spin) % 60;
        d.pieces[i].h = (d.pieces[i].h - spin) % 60;
    }
    return d;
}

static digit lerp_digits(digit a, digit b, int percent) {
    digit d;
    int i;
    for (i = 0;i < 6;i++) {
        d.pieces[i].m = (b.pieces[i].m * percent + a.pieces[i].m * (100 - percent)) / 100;
        d.pieces[i].h = (b.pieces[i].h * percent + a.pieces[i].h * (100 - percent)) / 100;
    }
    return d;
}

unsigned short get_display_hour(unsigned short hour) {

    if (clock_is_24h_style()) {
        return hour;
    }

    unsigned short display_hour = hour % 12;

    return display_hour ? display_hour : 12;

}

static void lerp_and_draw_digit(GContext* ctx, GPoint center, GPoint step, int lerp, int p, int c) {
    digit a = digits[p];
    digit b = digits[c];
    digit r = lerp_digits(a, b, lerp);

    draw_digit(ctx, center, step, r);
}

static void spin_and_draw_digit(GContext* ctx, GPoint center, GPoint step, int spin, int c) {
    digit r = spin_digit(digits[c], spin);

    draw_digit(ctx, center, step, r);
}

static void spin_lerp_and_draw_digit(GContext* ctx, GPoint pos, GPoint step, int spin, int lerp, int p, int c) {
    if (spin >= 60 || p == c) {
        lerp_and_draw_digit(ctx, pos, step, lerp, p, c);
    } else {
        spin_and_draw_digit(ctx, pos, step, spin, p);
    }
}

static void draw_time(GContext* ctx, Layer *layer, int hour, int min) {
    int x, y;
    GRect bounds = layer_get_bounds(layer);
    GPoint step;
    GPoint center = grect_center_point(&bounds);
    center.x = center.x / 4;
    center.y = center.y / 6;
    step.x = bounds.size.w / 4;
    step.y = bounds.size.h / 6;

    // minute/hour hand
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorWhite);

    GPoint pos = center;
    spin_lerp_and_draw_digit(ctx, pos, step, spin, lerp, prev_hour_tens, hour / 10);
    pos.x += step.x * 2;
    spin_lerp_and_draw_digit(ctx, pos, step, spin, lerp, prev_hour_ones, hour % 10);
    pos.y += step.y * 3;
    spin_lerp_and_draw_digit(ctx, pos, step, spin, lerp, prev_min_ones, min % 10);
    pos.x -= step.x * 2;
    spin_lerp_and_draw_digit(ctx, pos, step, spin, lerp, prev_min_tens, min / 10);
    if (lerp == 100)
        prev_hour_tens = hour / 10;
    if (lerp == 100)
        prev_hour_ones = hour % 10;
    if (lerp == 100)
        prev_min_tens = min / 10;
    if (lerp == 100)
        prev_min_ones = min % 10;
}


static void hands_update_proc(Layer *layer, GContext *ctx) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    unsigned short display_hour = get_display_hour(t->tm_hour);

    draw_time(ctx, layer, display_hour, t->tm_min);
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  //layer_mark_dirty(window_get_root_layer(window));
  draw_timer = app_timer_register(0, draw_time_timer, NULL);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_simple_bg_layer);

  layer_destroy(s_hands_layer);
}

static GPathInfo* copy_path_scaled(const GPathInfo* path, double xscale, double yscale) {
    GPathInfo* copy = (GPathInfo*)malloc(sizeof(GPathInfo));
    copy->num_points = path->num_points;
    copy->points = (GPoint*)malloc(copy->num_points * sizeof(GPoint));
    int i;
    for (i = 0;i < path->num_points;i++) {
        copy->points[i] = scale_point(path->points[i],xscale,yscale);
    }
    return copy;
}


static void init() {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  GPathInfo* scaled = copy_path_scaled(&MINUTE_HAND_POINTS, 0.2, 0.2);
  // init hand paths
  s_minute_arrow = gpath_create(scaled);
  scaled = copy_path_scaled(&HOUR_HAND_POINTS, 0.2, 0.2);
  s_hour_arrow = gpath_create(scaled);

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
