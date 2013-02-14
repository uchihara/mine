#!/bin/sh

rm -rf \
"Makefile Makefile.in aclocal.m4 autom4te.cache config.h config.log \
config.status configure libtool config/Makefile config/Makefile.in \
config/gdlib-config tests/Makefile tests/Makefile.in"

if autoreconf 2>/dev/null || \
  aclocal 2>/dev/null && \
  automake --gnu --add-missing 2>/dev/null && \
  autoconf 2>/dev/null && [ -x configure ]; then
  echo "You can configure now."
else
  echo "You must install autoreconf, or autotools(aclocal, automake, autoconf)."
fi
