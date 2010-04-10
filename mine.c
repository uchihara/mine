#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <curses.h>
#include <signal.h>
#include <stdarg.h>
#include <errno.h>

#define K_UP    'k'
#define K_DOWN  'j'
#define K_LEFT  'h'
#define K_RIGHT 'l'
#define K_ISMOVE(c) ((c) == K_UP || (c) == KEY_UP || (c) == K_DOWN || (c) == KEY_DOWN || (c) == K_LEFT || (c) == KEY_LEFT || (c) == K_RIGHT || (c) == KEY_RIGHT)
#define K_QUIT  'q'
#define K_MARK  'm'
#define K_OPEN  ' '

#define T_TIMEOUT 100 * 1000 * 1000;

#define F_QUIT 0
#define F_ERROR -1
#define F_CONTINUE -2

#define M_BOMB  '*'
#define M_MARK  '@'
#define M_CLOSE '.'
#define M_OPEN  ' '
#define M_WRONG 'X'

#define MSG_CLEAR  0
#define MSG_BOMBED 1
#define MSG_ERROR 2

typedef struct field {
	unsigned int sentinel : 1;
	unsigned int bomb : 1;
	unsigned int opened : 1;
	unsigned int marked : 1;
	unsigned int reserve : 4;
} field;

static int debug;
static WINDOW *wdebug;
static void dprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	werase(wdebug);
	vwprintw(wdebug, fmt, ap);
	wnoutrefresh(wdebug);
	va_end(ap);
}

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

static sig_atomic_t sig_exit;
static void sig_exit_handler(int signo)
{
	sig_exit = 1;
}

static sig_atomic_t sig_resize;
static void sig_resize_handler(int signo)
{
	sig_resize = 1;
}

static void handle_signal(void)
{
	struct sigaction sigexit, sigresize;

	memset(&sigexit, 0, sizeof sigexit);
	sigemptyset(&sigexit.sa_mask);
	sigexit.sa_handler = sig_exit_handler;
	sigaction(SIGTERM, &sigexit, NULL);
	sigaction(SIGINT, &sigexit, NULL);
	sigaction(SIGSEGV, &sigexit, NULL);
	sigaction(SIGHUP, &sigexit, NULL);

	memset(&sigresize, 0, sizeof sigresize);
	sigemptyset(&sigresize.sa_mask);
	sigresize.sa_handler = sig_resize_handler;
	sigaction(SIGWINCH, &sigresize, NULL);
}

static WINDOW *wcanvas;
static WINDOW *wfield;
static WINDOW *wguide;

static int lines, cols;
static void init_screen(void)
{
	initscr();
	cbreak();
	noecho();
	getmaxyx(stdscr, lines, cols);
}

static void init_canvas(void)
{
	wcanvas = newwin(canvas_h, canvas_w, 1, 0);
	wguide = newwin(1, cols, 0, 0);
	wdebug = newwin(1, cols, lines-1, 0);
	wattron(wguide, A_BOLD);
	wfield = subwin(wcanvas, y_max, x_max, 2, 1);
	keypad(wfield, TRUE);
	wnoutrefresh(stdscr);
}

static void destroy_screen(void)
{
	endwin();
}

static void destroy_canvas(void)
{
	erase();
	doupdate();
	if (wfield) delwin(wfield);
	if (wdebug) delwin(wdebug);
	if (wguide) delwin(wguide);
	if (wcanvas) delwin(wcanvas);
}

static void draw_canvas(void)
{
	int y, x;

	wborder(wcanvas, 0, 0, 0, 0, 0, 0, 0, 0);
	for (y = 0; y < y_max; y++) {
		for (x = 0; x < x_max; x++) {
			mvwaddch(wfield, y, x, M_CLOSE);
			if (debug && get_field(y, x)->bomb) {
				mvwaddch(wfield, y, x, M_BOMB);
			}
		}
	}
	wmove(wfield, 0, 0);
	wnoutrefresh(wcanvas);
}

static void resize_canvas(void)
{
	endwin();
	refresh();
	getmaxyx(stdscr, lines, cols);

	werase(stdscr);
	werase(wguide);
	werase(wdebug);

	wnoutrefresh(stdscr);
	wnoutrefresh(wguide);
	wnoutrefresh(wdebug);

	doupdate();

	wresize(wguide, 1, cols);

	wresize(wdebug, 1, cols);
	mvwin(wdebug, lines-1, 0);

	touchwin(wcanvas);
	wnoutrefresh(wcanvas);
	wnoutrefresh(wfield);

	doupdate();
}

static int get_input(void)
{
	int c;
	fd_set rfds;
	struct timespec tout;
	int ret;

	tout.tv_sec = 0;
	tout.tv_nsec = T_TIMEOUT;

	FD_ZERO(&rfds);
	FD_SET(0, &rfds);

	errno = 0;
	ret = pselect(1, &rfds, NULL, NULL, &tout, NULL);
	if (ret < 0) {
		if (errno == EINTR) {
			return F_CONTINUE;
		}
		dprintf("pselect: %s", strerror(errno));
		return F_ERROR;

	} else if (ret == 0) {
		return F_CONTINUE;
	}

	c = wgetch(wfield);
	return (c == K_QUIT) ? F_QUIT : c;
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

static int curr_y, curr_x;
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
	return y < 0 || lines < y || x < 0 || cols < x || y*x < nbombs;
}

static void game_over(int no)
{
	static const char *msgs[] = {
		"CLEARED",
		"BOMBED",
		"ERROR",
	};
	werase(wguide);
	mvwprintw(wguide, 0, 0, "%s, hit any key to quit...", msgs[no]);
	wnoutrefresh(wguide);
	doupdate();
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
	static struct timeval uptime;
	static int set_uptime;
	if (!set_uptime) {
		gettimeofday(&uptime, NULL);
		set_uptime = 1;
	}
	struct timeval now;
	struct timeval res;

	gettimeofday(&now, NULL);
	timersub(&now, &uptime, &res);

	werase(wguide);
	wprintw(wguide, "move:<cursol>,h,j,k,l open:<spc> mark:m quit:q rest:%d up:%ld", rest_bombs(nbombs), res.tv_sec, res.tv_usec);
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

	while (!sig_exit) {
		if (sig_resize) {
			resize_canvas();
			sig_resize = 0;
		}

		if (debug) {
			dprintf("%dx%d, y:%d x:%d st:%s, sigexit:%d", lines, cols, curr_y, curr_x, dump_field(get_field(curr_y, curr_x)), sig_exit);
		}

		update_guide(nbombs);
		wmove(wfield, curr_y, curr_x);
		wnoutrefresh(wfield);
		doupdate();

		c = get_input();
		if (c == F_QUIT) {
			break;
		} else if (c == F_ERROR) {
			game_over(MSG_ERROR);
			break;
		} else if (c == F_CONTINUE) {
			continue;
		}

		if (K_ISMOVE(c)) {
			move_cursol(c);

		} else {
			if (c == K_MARK) {
				apply_mark();

			} else if (c == K_OPEN) {
				int isbomb = open_field(curr_y, curr_x);
				if (isbomb) {
					draw_answer();
					game_over(MSG_BOMBED);
					break;
				}
			}

			if (all_clear()) {
				game_over(MSG_CLEAR);
				break;
			}
		}

		wnoutrefresh(wfield);
	}

	destroy_canvas();
	destroy_screen();

	return 1;
}
