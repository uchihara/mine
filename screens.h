#ifndef __MINE_SCREENS_H__
#define __MINE_SCREENS_H__

void dbgprintf(const char *fmt, ...);
void init_screen(void);
void init_canvas(int canvas_h, int canvas_w, int y_max, int x_max);
void destroy_screen(void);
void destroy_canvas(void);
void draw_canvas(int y_max, int x_max, int debug);
void resize_canvas(void);
int get_input(void);
void reverse_mark(int y, int x, int debug);
void game_over(int no);
void update_guide(int nbombs);
void draw_field(int y, int x, int mark);
void refresh_field(void);
void update_screen(void);
void move_cursol(int y, int x);
int get_lines(void);
int get_cols(void);

#endif
