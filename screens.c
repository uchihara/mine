#include "config.h"
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include "mine.h"
#include "screens.h"
#include "fields.h"

#ifndef HAVE_WRESIZE
static int wresize(WINDOW *win, int lines, int columns)
{
	return OK;
}
#endif

#ifndef HAVE_TIMERSUB
/* from /usr/include/sys/time.h */
# define timersub(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)
#endif

static WINDOW *wdebug;
void dbgprintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	werase(wdebug);
	vwprintw(wdebug, (char *)fmt, ap); /* cast for solaris 10 */
	wnoutrefresh(wdebug);
	va_end(ap);
}

static WINDOW *wcanvas;
static WINDOW *wfield;
static WINDOW *wguide;

static int lines, cols;

void init_screen(void)
{
	initscr();
	cbreak();
	noecho();
	getmaxyx(stdscr, lines, cols);
}

void init_canvas(int canvas_h, int canvas_w, int y_max, int x_max)
{
	wcanvas = newwin(canvas_h, canvas_w, 1, 0);
	wguide = newwin(1, cols, 0, 0);
	wdebug = newwin(1, cols, lines-1, 0);
	wattron(wguide, A_BOLD);
	wfield = subwin(wcanvas, y_max, x_max, 2, 1);
	keypad(wfield, TRUE);
	wnoutrefresh(stdscr);
}

void destroy_screen(void)
{
	endwin();
}

void destroy_canvas(void)
{
	erase();
	doupdate();
	if (wfield) delwin(wfield);
	if (wdebug) delwin(wdebug);
	if (wguide) delwin(wguide);
	if (wcanvas) delwin(wcanvas);
}

void draw_field(int y, int x, int mark)
{
	mvwaddch(wfield, y, x, mark);
}

void draw_canvas(int y_max, int x_max, int debug)
{
	int y, x;

	wborder(wcanvas, 0, 0, 0, 0, 0, 0, 0, 0);
	for (y = 0; y < y_max; y++) {
		for (x = 0; x < x_max; x++) {
			draw_field(y, x, M_CLOSE);
			if (debug && get_field(y, x)->bomb) {
				draw_field(y, x, M_BOMB);
			}
		}
	}
	wmove(wfield, 0, 0);
	wnoutrefresh(wcanvas);
}

void resize_canvas(void)
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

int get_input(void)
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
		dbgprintf("pselect: %s", strerror(errno));
		return F_ERROR;

	} else if (ret == 0) {
		return F_CONTINUE;
	}

	c = wgetch(wfield);
	return (c == K_QUIT) ? F_QUIT : c;
}

void game_over(int no)
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

void update_guide(int nbombs)
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

void refresh_field(void)
{
	wnoutrefresh(wfield);
}

void update_screen(void)
{
	doupdate();
}

void move_cursol(int y, int x)
{
	wmove(wfield, y, x);
}

int get_lines(void)
{
	return lines;
}

int get_cols(void)
{
	return cols;
}
