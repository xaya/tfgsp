noinst_LTLIBRARIES = libwinsane.la
libwinsane_la_SOURCES = init.c
libwinsane_la_CXXFLAGS = -I$(top_srcdir)
libwinsane_la_LIBADD = manifest.lo
libwinsane_la_DEPENDENCIES  = manifest.$(OBJEXT)

manifest.$(OBJEXT): manifest.rc utf8.xml
	libtool --mode=compile windres -o $@ $(top_builddir)/libwinsane/manifest.rc