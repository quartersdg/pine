#pragma once

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