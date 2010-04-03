#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <curses.h>
#include <signal.h>
#include <stdarg.h>

#define K_UP    'k'
#define K_DOWN  'j'
#define K_LEFT  'h'
#define K_RIGHT 'l'
#define K_ISMOVE(c) ((c) == K_UP || (c) == KEY_UP || (c) == K_DOWN || (c) == KEY_DOWN || (c) == K_LEFT || (c) == KEY_LEFT || (c) == K_RIGHT || (c) == KEY_RIGHT)
#define K_QUIT  'q'
#define K_MARK  'm'
#define K_OPEN  ' '

#define M_BOMB  '*'
#define M_MARK  '@'
#define M_CLOSE '.'
#define M_OPEN  ' '
#define M_WRONG 'X'

#define MSG_CLEAR  0
#define MSG_BOMBED 1

typedef struct field {
	unsigned int sentinel : 1;
	unsigned int bomb : 1;
	unsigned int opened : 1;
	unsigned int marked : 1;
	unsigned int reserve : 4;
} field;

static int debug;
#if 0
static void dprintf(const char *fmt, ...)
{
	static FILE *fp;
	va_list ap;

	if (!fp) {
		fp = fopen("log", "a");
	}

	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	fflush(fp);
	va_end(ap);
}
#endif

static const char *dump_field(const field *p)
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
static int canvas_h, canvas_w;

static field *get_field(int y, int x)
{
	return &fields[(y+1)*canvas_w + x+1];
}

static void setup_fields(int y, int x, int nbombs)
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

static int sig;
static void sig_hndl(int signo)
{
	sig = 1;
}

static void handle_signal(void)
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof sigact);
	sigact.sa_handler = sig_hndl;
/*
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGSEGV, &sigact, NULL);
	sigaction(SIGHUP, &sigact, NULL);
*/
}

static WINDOW *wcanvas;
static WINDOW *wfield;
static WINDOW *wguide;
static WINDOW *wdebug;

static void init_screen(void)
{
	initscr();
	cbreak();
	noecho();
}

static void init_canvas(void)
{
	wcanvas = subwin(stdscr, canvas_h, canvas_w, 1, 0);
	wguide = subwin(stdscr, 1, COLS, 0, 0);
	wdebug = subwin(stdscr, 1, COLS, LINES-1, 0);
	wattron(wguide, A_BOLD);
	wfield = subwin(wcanvas, y_max, x_max, 2, 1);
	keypad(wfield, TRUE);
}

static void destroy_screen(void)
{
	endwin();
}

static void destroy_canvas(void)
{
	if (wfield) delwin(wfield);
	if (wdebug) delwin(wdebug);
	if (wguide) delwin(wguide);
	if (wcanvas) delwin(wcanvas);
}

static void draw_canvas(void)
{
	int y, x;

	box(wcanvas, '|', '-');
	for (y = 0; y < y_max; y++) {
		for (x = 0; x < x_max; x++) {
			mvwaddch(wfield, y, x, M_CLOSE);
			if (debug && get_field(y, x)->bomb) {
				mvwaddch(wfield, y, x, M_BOMB);
			}
		}
	}
	wmove(wfield, 0, 0);
//	wrefresh(wcanvas);
	wnoutrefresh(wcanvas);
}

static int curr_y, curr_x;
static int get_input(void)
{
	int c;
	wmove(wfield, curr_y, curr_x);
	c = wgetch(wfield);
	return (c == K_QUIT) ? EOF : c;
}

static void reverse_mark(int y, int x)
{
	field *p = get_field(y, x);
	if (p->marked) {
		p->marked = 0;
		mvwaddch(wfield, y, x, debug && p->bomb ? M_BOMB : M_CLOSE);
	} else {
		p->marked = 1;
		mvwaddch(wfield, y, x, M_MARK);
	}
}

static void move_cursol(int c)
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

static void apply_mark(void)
{
	if (!get_field(curr_y, curr_x)->opened) {
		reverse_mark(curr_y, curr_x);
	}
}

static void usage(const char *prog)
{
	printf("usage: %s [-y height] [-x width] [-b bombs]\n", prog);
}

static int outbounds(int y, int x, int nbombs)
{
	return y < 0 || LINES < y || x < 0 || COLS < x || y*x < nbombs;
}

static void gameover(int no)
{
	static const char *msgs[] = {
		"ALL CLEAR",
		"BOMBED",
	};
	werase(wguide);
	mvwprintw(wguide, 0, 0, "%s, hit any key to quit...", msgs[no]);
	wnoutrefresh(wguide);
	refresh();
	getch();
}

static void set_around_bombs(int y, int x)
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
		mvwprintw(wfield, y, x, "%d", around_bombs);
	} else {
		mvwaddch(wfield, y, x, M_OPEN);
		while (--ndirs >= 0) {
			int ny = dirs[ndirs].y;
			int nx = dirs[ndirs].x;
			set_around_bombs(ny, nx);
		}
	}
	wnoutrefresh(wfield);
}

static int open_field(int y, int x)
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

static int all_clear(void)
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

static int rest_bombs(int nbombs)
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

static void update_guide(int nbombs)
{
	werase(wguide);
	wprintw(wguide, "move:<cursol>,h,j,k,l open:<spc> mark:m quit:q rest:%d", rest_bombs(nbombs));
	wnoutrefresh(wguide);
}

static void draw_answer(void)
{
	int y, x;
	field *p;

	for (y = 0; y < y_max; y++) {
		for (x = 0; x < x_max; x++) {
			p = get_field(y, x);
			if (p->marked) {
				if (!p->bomb) {
					mvwaddch(wfield, y, x, M_WRONG);
				}
			} else if (!p->opened) {
				mvwaddch(wfield, y, x, p->bomb ? M_BOMB : M_OPEN);
			}
		}
	}
	wnoutrefresh(wfield);
}

int main(int argc, char **argv)
{
	int c;
	int y = 10, x = 20;
	int nbombs = 0;

	init_screen();
	handle_signal();

	while ((c = getopt(argc, argv, "y:x:b:D")) != -1) {
		switch (c) {
			case 'y': y = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'b': nbombs = atoi(optarg); break;
			case 'D': debug = 1; break;
			default: opterr = 1; break;
		}
	}

	if (!nbombs) nbombs = y * x * 0.25;
	if (!opterr || outbounds(y, x, nbombs)) {
		usage(*argv);
		return 1;
	}

	setup_fields(y, x, nbombs);
	init_canvas();
	draw_canvas();

	while (!sig) {
		if (debug) {
			field *p = get_field(curr_y, curr_x);

			werase(wdebug);
			mvwprintw(wdebug, 0, 0, "%dx%d, y:%d x:%d st:%s", LINES, COLS, curr_y, curr_x, dump_field(p));
			wnoutrefresh(wdebug);
		}

		update_guide(nbombs);
		refresh();

		if ((c = get_input()) == EOF) break;

		if (K_ISMOVE(c)) {
			move_cursol(c);

		} else {
			if (c == K_MARK) {
				apply_mark();

			} else if (c == K_OPEN) {
				int isbomb = open_field(curr_y, curr_x);
				if (isbomb) {
					draw_answer();
					gameover(MSG_BOMBED);
					break;
				}
			}

			if (all_clear()) {
				gameover(MSG_CLEAR);
				break;
			}
		}

		wnoutrefresh(wfield);
	}

	destroy_canvas();
	destroy_screen();

	return 1;
}
