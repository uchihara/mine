#ifndef __MINE_FIELDS_H__
#define __MINE_FIELDS_H__

typedef struct field {
	unsigned int sentinel : 1;
	unsigned int bomb : 1;
	unsigned int opened : 1;
	unsigned int marked : 1;
	unsigned int reserve : 4;
} field;

int get_y_max(void);
int get_x_max(void);
int get_canvas_h(void);
int get_canvas_w(void);
const char *dump_field(const field *p);
field *get_field(int y, int x);
void setup_fields(int y, int x, int nbombs);
void set_around_bombs(int y, int x);
int open_field(int y, int x);
void reverse_mark(int y, int x, int debug);
void apply_mark(int debug);
int all_clear(void);
void draw_answer(void);
int rest_bombs(int nbombs);
void change_cursol(int c);
int get_curr_y(void);
int get_curr_x(void);

#endif
