#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include "mine.h"
#include "timer.h"

#ifndef timersub
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

int watch_input(void)
{
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
		return F_ERROR;

	} else if (ret == 0) {
		return F_CONTINUE;
	}

	return 0;
}

static struct timeval uptime;
void init_uptime(void)
{
	gettimeofday(&uptime, NULL);
}

int get_uptime(void)
{
	struct timeval now;
	struct timeval res;

	gettimeofday(&now, NULL);
	timersub(&now, &uptime, &res);

	return res.tv_sec;
}
