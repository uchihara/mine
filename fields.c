#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mine.h"
#include "fields.h"
#include "screens.h"

const char *dump_field(const field *p)
{
	static char buf[128];
	*buf = '\0';
	if (p->sentinel) strcat(buf, " sentinel");
	if (p->bomb)     strcat(buf, " bomb");
	if (p->opened)   strcat(buf, " opened");
	if (p->marked)   strcat(buf, " marked");
	return buf;
}

static field *fields;
static int y_max, x_max;
int get_y_max(void)
{
	return y_max;
}

int get_x_max(void)
{
	return x_max;
}

static int canvas_h, canvas_w;
int get_canvas_h(void)
{
	return canvas_h;
}

int get_canvas_w(void)
{
	return canvas_w;
}

field *get_field(int y, int x)
{
	return &fields[(y+1)*canvas_w + x+1];
}

void setup_fields(int y, int x, int nbombs)
{
	int i;
	field *p;

	srand(time(NULL));
	y_max = y;
	x_max = x;
	canvas_h = y_max+2;
	canvas_w = x_max+2;
	
	fields = (field *)calloc(canvas_h * canvas_w, sizeof(field));

	for (i = 0; i < canvas_h * canvas_w; i++) {
		fields[i].sentinel = 1;
	}

	for (y = 0; y < y_max; y++) {
		for (x = 0; x < x_max; x++) {
			p = get_field(y, x);
			p->sentinel = 0;
			p->bomb = 0;
		}
	}

	while (nbombs > 0) {
		field *p;
		y = rand() % y_max;
		x = rand() % x_max;
		p = get_field(y, x);
		if (!p->bomb) {
			p->bomb = 1;
			nbombs--;
		}
	}

}

void set_around_bombs(int y, int x)
{
	int dy, dx;
	int around_bombs = 0;
	struct dir { int y, x; } dirs[8];
	int ndirs = 0;

	for (dy = -1; dy <= 1; dy++) {
		for (dx = -1; dx <= 1; dx++) {
			field *p = get_field(y+dy, x+dx);

			if (p->sentinel || p->opened) {
				continue;

			} else if (p->bomb) {
				around_bombs++;

			} else {
				dirs[ndirs].y = y+dy;
				dirs[ndirs].x = x+dx;
				ndirs++;
			}

		}
	}

	get_field(y, x)->opened = 1;
	if (around_bombs != 0) {
		draw_field(y, x, around_bombs + '0');
	} else {
		draw_field(y, x, M_OPEN);
		while (--ndirs >= 0) {
			int ny = dirs[ndirs].y;
			int nx = dirs[ndirs].x;
			set_around_bombs(ny, nx);
		}
	}
	refresh_field();
}

int open_field(int y, int x)
{
	field *p;

	p = get_field(y, x);
	if (p->marked) {
		return 0;
	} else if (p->bomb) {
		return 1;
	} else {
		p->opened = 1;
		set_around_bombs(y, x);
		return 0;
	}
}

void reverse_mark(int y, int x, int debug)
{
	field *p = get_field(y, x);
	if (p->marked) {
		p->marked = 0;
		draw_field(y, x, debug && p->bomb ? M_BOMB : M_CLOSE);
	} else {
		p->marked = 1;
		draw_field(y, x, M_MARK);
	}
}

void apply_mark(int debug)
{
	if (!get_field(get_curr_y(), get_curr_x())->opened) {
		reverse_mark(get_curr_y(), get_curr_x(), debug);
	}
}

static int curr_y, curr_x;
void change_cursol(int c)
{
	switch (c) {
		case K_UP:    case KEY_UP:    curr_y--; break;
		case K_DOWN:  case KEY_DOWN:  curr_y++; break;
		case K_LEFT:  case KEY_LEFT:  curr_x--; break;
		case K_RIGHT: case KEY_RIGHT: curr_x++; break;
	}
	if (curr_x < 0) curr_x = 0;
	if (x_max <= curr_x) curr_x = x_max-1;
	if (curr_y < 0) curr_y = 0;
	if (y_max <= curr_y) curr_y = y_max-1;
}

int get_curr_y(void)
{
	return curr_y;
}

int get_curr_x(void)
{
	return curr_x;
}

void draw_answer(void)
{
	int y, x;
	field *p;

	for (y = 0; y < y_max; y++) {
		for (x = 0; x < x_max; x++) {
			p = get_field(y, x);
			if (p->marked) {
				if (!p->bomb) {
					draw_field(y, x, M_WRONG);
				}
			} else if (!p->opened) {
				draw_field(y, x, p->bomb ? M_BOMB : M_OPEN);
			}
		}
	}
	refresh_field();
}

int all_clear(void)
{
	int y, x;
	field *p;

	for (y = 0; y < y_max; y++) {
		for (x = 0; x < x_max; x++) {
			p = get_field(y, x);
			if (!p->opened && !p->marked) return 0;
			if (p->bomb != p->marked) return 0;
		}
	}
	return 1;
}

int rest_bombs(int nbombs)
{
	int y, x;
	field *p;

	for (y = 0; y < y_max; y++) {
		for (x = 0; x < x_max; x++) {
			p = get_field(y, x);
			if (p->marked) nbombs--;
		}
	}
	return nbombs < 0 ? 0 : nbombs;
}
