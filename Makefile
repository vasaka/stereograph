#
# Makefile, Stereograph 0.31a
#
# If necessary please edit to suit your system.
#
# since release 0.17 libpng is required;
# if this library is not intalled in /usr/lib
# please correct the LDFAGS down there.
#


# Linux
CC  = gcc
CFLAGS = -Wall -O2 -Dlinux
#CFLAGS = -Wall -O2 -Dlinux -DBIG_ENDIAN
LDFLAGS = -lm /usr/lib/libpng.so /usr/lib/libz.so

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

