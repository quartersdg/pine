#include <math.h>
#include <pebble.h>>
#include "PINE.h"
#include "PINEGUI.h"

GPath* gpath_create(const GPathInfo *init) {
	GPath* p = (GPath*)calloc(1, sizeof(GPath));
	p->num_points = init->num_points;
	p->points = (GPoint*)calloc(init->num_points, sizeof(GPoint));
	memcpy(p->points, init->points, init->num_points * sizeof(GPoint));
	return p;
}

void gpath_destroy(GPath* gpath) {
	free(gpath->points);
	free(gpath);
}

static PPoint rotate_point(float angle, GPoint p)
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

static PPoint* rotate_and_offset_gpath(GPath* path) {
	PPoint* points = (PPoint*)malloc(path->num_points * sizeof(PPoint));

	double a = pebble_angle_to_radians(path->rotation);

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

