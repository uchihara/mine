#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mine.h"
#include "fields.h"
#include "screens.h"
#include "signals.h"
#include "timer.h"

static int debug;

static int get_max_lines(void)
{
	return get_lines() - 3;
}

static int get_max_cols(void)
{
	return get_cols() - 2;
}

static void usage(const char *prog)
{
	printf("usage: %s [-y height] [-x width] [-M ] [-b boms%%=0.20]\n", prog);
	printf("\t-y: 1 <= y <= %d\n", get_max_lines());
	printf("\t-x: 1 <= x <= %d\n", get_max_cols());
	printf("\t-M: y=%d, x=%d\n", get_max_lines(), get_max_cols());
}

static int outbounds(int y, int x, int nbombs)
{
	return y < 0 || get_max_lines() < y || x < 0 || get_max_cols() < x || y*x < nbombs;
}

static void terminate(void)
{
	static int terminated = 0;
	if (!terminated) {
		destroy_canvas();
		destroy_screen();
		terminated = 1;
	}
}

int main(int argc, char **argv)
{
	int c;
	int y = 10, x = 20;
	int nbombs = 0;
	double bomsp = 0.20;

	init_screen();
	handle_signal();
	atexit(terminate);

	while ((c = getopt(argc, argv, "y:x:Mb:D")) != -1) {
		switch (c) {
			case 'y': y = atoi(optarg); break;
			case 'x': x = atoi(optarg); break;
			case 'M': y = get_max_lines(); x = get_max_cols(); break;
			case 'b': bomsp = atof(optarg); break;
			case 'D': debug = 1; break;
			default: opterr = 1; break;
		}
	}

	nbombs = y * x * bomsp;
	if (!opterr || outbounds(y, x, nbombs)) {
		terminate();
		usage(*argv);
		return 1;
	}

	init_uptime();
	init_fields(y, x);
	init_canvas(get_canvas_h(), get_canvas_w(), get_y_max(), get_x_max());
	draw_canvas(get_y_max(), get_x_max(), debug);

	int first_open = 1;
	while (!caught_sig_exit()) {
		if (caught_sig_resize()) {
			resize_canvas();
			reset_sig_resize();
		}

		if (debug) {
			dbgprintf("%dx%d, y:%d x:%d st:%s", get_lines(), get_cols(), get_curr_y(), get_curr_x(), dump_field(get_field(get_curr_y(), get_curr_x())));
		}

		update_guide(nbombs);
		move_cursol(get_curr_y(), get_curr_x());
		refresh_field();
		update_screen();

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
			change_cursol(c);

		} else {
			if (c == K_MARK) {
				apply_mark(debug);

			} else if (c == K_OPEN) {
				if (first_open) {
					set_bombs(get_curr_y(), get_curr_x(), nbombs);
					first_open = 0;
				}

				int isbomb = open_field(get_curr_y(), get_curr_x());
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

		refresh_field();
	}

	return 1;
}
