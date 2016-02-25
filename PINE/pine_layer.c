
#include <stdint.h>
#include "queue.h"
#include "pebble.h"
#include "PINE.h"
#include "PINEGUI.h"

enum {
	USER_LAYER,
	TEXT_LAYER,
	BITMAP_LAYER,
};

static int layer_number = 0;

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

#pragma region BaseLayer
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
#pragma endregion

#pragma region PebbleLayer
Layer* layer_create(GRect frame) {
	Layer* l = calloc(1, sizeof(struct Layer));
	base_layer_init((struct BaseLayer*)l, frame);
	l->base.type = USER_LAYER;
	return l;
}

void layer_destroy(Layer* layer) {
	base_layer_destroy(&layer->base);
	free(layer);
}

void layer_mark_dirty(Layer *layer) {
    if (!layer) return;
	layer->base.dirty = true;
	pine_something_is_dirty();
}

void layer_set_update_proc(Layer *layer, LayerUpdateProc update_proc) {
	layer->update = update_proc;
}

GRect layer_get_bounds(const Layer *layer) {
	return layer->base.bounds;
}

GRect layer_get_relative_frame(Layer *layer, const GRect r) {
	GRect f;
	f.origin.x = r.origin.x - layer->base.bounds.origin.x;
	f.origin.y = r.origin.y - layer->base.bounds.origin.y;
	f.size.w = r.size.w - layer->base.bounds.size.w;
	f.size.h = r.size.h - layer->base.bounds.size.h;
	return f;
}

GRect layer_get_frame(const Layer *layer) {
	if (layer->base.parent)
		return layer_get_relative_frame((Layer*)layer->base.parent, layer->base.bounds);
	return layer->base.bounds;
}

void layer_add_child(Layer *parent, Layer *child) {
	struct BaseLayer* base = &child->base;
	base->parent = (struct BaseLayer*)parent;
	TAILQ_INSERT_TAIL(&parent->base.children, base, next);
	parent->base.dirty = child->base.dirty = true;

	pine_something_is_dirty();;
}

void layer_set_frame(Layer *layer, GRect frame) {
	struct BaseLayer *b = (struct BaseLayer*)layer;
	b->bounds = frame;
	pine_something_is_dirty();
}

void layer_set_hidden(Layer *layer, bool hidden) {
	((struct BaseLayer*)layer)->hidden = hidden;
	pine_something_is_dirty();
}

void layer_set_bounds(Layer *layer, GRect bounds) {
	((struct BaseLayer*)layer)->bounds = bounds;
	pine_something_is_dirty();
}

#pragma endregion

#pragma region TextLayer
TextLayer* text_layer_create(GRect frame) {
	struct TextLayer* l = (struct TextLayer*)calloc(1, sizeof(struct TextLayer));
	base_layer_init((struct BaseLayer*)l, frame);
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
    if (!text_layer) return;
	if (text_layer->text) free((void*)text_layer->text);
	text_layer->text = _strdup(text);
	pine_something_is_dirty();
}

void text_layer_set_background_color(TextLayer *text_layer, GColor color) {
	text_layer->bcolor = color;
	pine_something_is_dirty();
}

void text_layer_set_text_color(TextLayer *text_layer, GColor color) {
	text_layer->tcolor = color;
	pine_something_is_dirty();
}

void text_layer_set_font(TextLayer *text_layer, GFont font) {
	text_layer->font = font;
	pine_something_is_dirty();
}

void text_layer_set_text_alignment(TextLayer *text_layer, GTextAlignment text_alignment) {
	text_layer->text_alignment = text_alignment;
	pine_something_is_dirty();
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
#pragma endregion

#pragma region BitmapLayer
BitmapLayer* bitmap_layer_create(GRect frame) {
	struct BitmapLayer* l = (struct BitmapLayer*)calloc(1, sizeof(*l));
	base_layer_init((struct BaseLayer*)l, frame);
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
    if (!bitmap_layer || !bitmap) return;
	bitmap_layer->bitmap = bitmap->addr;
	pine_something_is_dirty();
}

void bitmap_layer_set_alignment(BitmapLayer *bitmap_layer, GAlign alignment) {
	pine_something_is_dirty();
}

void bitmap_layer_set_background_color(BitmapLayer *bitmap_layer, GColor color) {
	pine_something_is_dirty();
}

void bitmap_layer_set_compositing_mode(BitmapLayer *bitmap_layer, GCompOp mode) {
	pine_something_is_dirty();
}
#pragma endregion

#pragma region PineLayerUpdates
// Predefine for mututally recursive functions to work
static void pine_update_child_layers(struct BaseLayer* l);

static void pine_update_layer(struct BaseLayer* l) {
	if (l->hidden)
		return;
	switch (l->type) {
	case TEXT_LAYER: {
		struct TextLayer* tl = (struct TextLayer*)l;
		pine_draw_text_layer(tl);
	}
					 break;
	case USER_LAYER: {
		struct Layer* ul = (struct Layer*)l;
		if (ul->update) {
			ul->update(ul, pine_get_global_context());
		}
	}
					 break;
	case BITMAP_LAYER: {
		pine_draw_bitmap_layer((struct BitmapLayer*)l);
	}
					   break;
	}
	pine_update_child_layers(l);
}

void pine_update_layers(struct BaseLayer* l) {
	pine_update_layer(l);
}

static void pine_update_child_layers(struct BaseLayer* l) {
	struct BaseLayer* c;
	TAILQ_FOREACH(c, &l->children, next) {
		pine_update_layer(c);
	}
}
#pragma endregion