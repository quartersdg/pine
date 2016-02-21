#include <math.h>
#include <pebble.h>
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

static POINT rotate_point(float angle, GPoint center, GPoint p)
{
    POINT r;
	float s = sin(angle);
	float c = cos(angle);

    r.x = p.x;
    r.y = p.y;

	// rotate point
	float xnew = r.x * c - r.y * s;
	float ynew = r.x * s + r.y * c;

	// translate point back:
	r.x = xnew;
	r.y = ynew;
	return r;
}

static POINT* rotate_and_offset_gpath(GPath* path) {
    POINT* points = (POINT*)malloc(path->num_points * sizeof(POINT));

	double a = pebble_angle_to_radians(path->rotation);

	for (int i = 0; i < path->num_points; i++) {
		points[i] = rotate_point(a, path->offset, path->points[i]);
        points[i].x += path->offset.x;
        points[i].y += path->offset.y;
    }
	return points;
}

void gpath_draw_filled(GContext* ctx, GPath *path) {
    POINT* points = rotate_and_offset_gpath(path);

    PineDrawPolyFilled(ctx,path->num_points, points);
    /*
    for (int i = 0;i < path->num_points;i++) {
        PineDrawCircleFilled(ctx, points[i].x, points[i].y, 20);
    }
    */

	free(points);
}

void gpath_draw_outline(GContext* ctx, GPath *path) {
    GPoint* points = rotate_and_offset_gpath(path);

	PineDrawPolyLine(path->num_points, points);

	free(points);
}

void gpath_rotate_to(GPath *path, int32_t angle) {
	path->rotation = angle;
}

void gpath_move_to(GPath *path, GPoint point) {
	path->offset = point;
}

