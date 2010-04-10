#ifndef __MINE_SIGNALS_H__
#define __MINE_SIGNALS_H__

int caught_sig_exit(void);
int caught_sig_resize(void);
void reset_sig_resize(void);
void handle_signal(void);

#endif
