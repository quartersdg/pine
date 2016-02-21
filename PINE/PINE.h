#pragma once
#define WIN32_LEAN_AND_MEAN
#define _X86_
#include <windef.h>
#include <wingdi.h>

#include "resource.h"
#include <stdint.h>
#include "queue.h"
#include "pebble.h"

struct BaseLayer {
	int number; //debugging help
	int type;   //polymorphism

	TAILQ_ENTRY(BaseLayer) next;
	TAILQ_HEAD(, BaseLayer) children;
	struct BaseLayer* parent;
	GRect bounds;
	bool clips;
	bool hidden;
	bool dirty;
};
void pine_update_layers(struct BaseLayer* l);

void pine_something_is_dirty(void);
double pebble_angle_to_radians(int32_t angle);

void* pine_get_global_context();
POINT* rotate_and_offset_gpath(GPath* path);

void PineSetBrushColor(GColor c);
void PineDrawLine(int x1, int y1, int x2, int y2);
void PineDrawPolyFilled(GContext* ctx, int num_points, GPoint* points);
void PineDrawPolyLine(int num_points, GPoint* points);
void PineDrawRectFilled(GColor c, int x, int y, int w, int h);
void PineDrawCircleFilled(GContext* ctx, int x, int y, int radius);
void PineLog(uint8_t log_level, const char* src_filename, int src_line_number, const char* fmt, va_list va);
void PineDrawText(int x, int y, int w, int h, GColor bcolor, GColor tcolor, void* font, const char* text);
