#ifndef __MYCURSES_H__
#define __MYCURSES_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_CURSES_H
#include <curses.h>
#else
#ifdef HAVE_NCURSES_CURSES_H
#include <ncurses/curses.h>
#endif
#endif

#endif
