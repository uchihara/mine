#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([mine], [1.0.0], [uchihara@gmail.com])
AC_CONFIG_SRCDIR([mine.c])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcurses':
AC_CHECK_LIB([curses], [initscr], [], [AC_MSG_ERROR([curses library does not find, you must install before])])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h unistd.h signal.h stdarg.h sys/select.h])
AC_CHECK_HEADERS([curses.h ncurses/curses.h], [has_curses_h=1 break], [has_curses_h=0])

if test $has_curses_h = 0; then
	AC_MSG_ERROR([curses.h does not find, you must install before])
fi

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([gettimeofday memset strerror wresize])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
