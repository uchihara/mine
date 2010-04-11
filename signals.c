#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <signal.h>

static sig_atomic_t sig_exit;
static void sig_exit_handler(int signo)
{
	sig_exit = 1;
}

int caught_sig_exit(void)
{
	return sig_exit ? 1 : 0;
}

static sig_atomic_t sig_resize;
static void sig_resize_handler(int signo)
{
	sig_resize = 1;
}

int caught_sig_resize(void)
{
	return sig_resize ? 1 : 0;
}

void reset_sig_resize(void)
{
	sig_resize = 0;
}

void handle_signal(void)
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
