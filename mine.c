#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mine.h"
#include "fields.h"
#include "screens.h"
#include "signals.h"

static int debug;

static void usage(const char *prog)
{
	printf("usage: %s [-y height] [-x width] [-b bombs]\n", prog);
}

static int outbounds(int y, int x, int nbombs)
{
	return y < 0 || get_lines() < y || x < 0 || get_cols() < x || y*x < nbombs;
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
	init_canvas(get_canvas_h(), get_canvas_w(), get_y_max(), get_x_max());
	draw_canvas(get_y_max(), get_x_max(), debug);

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

	destroy_canvas();
	destroy_screen();

	return 1;
}
