#
# Makefile for building tifficc
# Originally written by Bob Friesenhahn, June 2003
# bugs introduced by Marti Maria on October 2004

# Don't require all the GNU mandated files
AUTOMAKE_OPTIONS = 1.7 foreign

AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include

if HasTIFF
bin_PROGRAMS = tifficc
else
bin_PROGRAMS =
endif

tifficc_LDADD = $(top_builddir)/src/liblcms.la @TIFFICC_DEPLIBS@
tifficc_LDFLAGS = @LDFLAGS@
tifficc_SOURCES = tifficc.c ../samples/xgetopt.c ../samples/vprf.c
man_MANS = tifficc.1

EXTRA_DIST = $(man_MANS)