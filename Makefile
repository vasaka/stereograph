#
# Makefile, Stereograph 0.33a
#
# If necessary please edit to suit your system.
#
# since release 0.17 libpng is required;
# since release 0.32 libjpeg is required;
# since release 0.33 libX11 is required;
#
# if this library is not intalled in /usr/lib
# please correct the LDFAGS down there.
#

#PREFIX=/usr/local
PREFIX=`grep -i "prefix =" ../Makefile | grep -i -v exec | cut -d"=" -f 2`

# Linux
CC  = gcc
CFLAGS = -Wall -O2 -Dlinux -DSTEREOGRAPH_ONLY -DX11GUI
LDFLAGS = -ljpeg -lpng -lz -lm -lX11 -ldl -rdynamic -L/usr/X11R6/lib

# HPUX
#CC = cc
#CFLAGS = -Aa -D_HPUX_SOURCE -DSYSV +O3

# DEC
#CC = cc
#CFLAGS = -std1 -float -O2

# SunOS with acc
#CC = acc
#CFLAGS = -fast

# Generic Unix
#CC = cc
#CFLAGS = -ansi -O

# Generic Unix with gcc
#CC = gcc
#CFLAGS = -ansi -Wall -O2


# no need to change anything below here
#----------------------------------------------------------

include Makefile.in

