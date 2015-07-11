#include <stdint.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdarg.h>
#include <pebble.h>
#include "PINE.h"

void app_log(uint8_t log_level, const char* src_filename, int src_line_number, const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	PineLog(log_level, src_filename, src_line_number, fmt, va);
	va_end(va);
}

double pebble_angle_to_radians(int32_t angle) {
	double a = (double)angle / TRIG_MAX_ANGLE;
	a = a * M_PI * 2;
	return a;
}

int32_t sin_lookup(int32_t angle) {
	double a = pebble_angle_to_radians(angle);
	double r = sin(a);
	r = r * TRIG_MAX_ANGLE;
	return (int32_t)r;
}

int32_t cos_lookup(int32_t angle) {
	double a = pebble_angle_to_radians(angle);
	double r = cos(a);
	r = r * TRIG_MAX_ANGLE;
	return (int32_t)r;
}

