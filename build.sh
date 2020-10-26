#!/bin/sh
rm configure.in
autoscan
cp configure.in.bak configure.in
aclocal
autoconf
autoheader
automake --add-missing
./configure CXXFLAGS= CFLAGS=
make
find . -name "*.o"  | xargs rm -f