#ifndef __MINE_H__
#define __MINE_H__

#include <curses.h>

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

#endif
