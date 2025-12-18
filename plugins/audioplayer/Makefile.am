EXTRA_DIST = README.md

libdir = $(xchatlibdir)

lib_LTLIBRARIES = audioplayer.la

audioplayer_la_SOURCES = \
	plugin.c \
	audioplayer.c \
	audioplayer.h \
	playlist_parser.c \
	playlist_parser.h

audioplayer_la_LDFLAGS = -avoid-version -module -no-undefined

audioplayer_la_LIBADD = \
	$(AVCODEC_LIBS) \
	$(AVFORMAT_LIBS) \
	$(AVUTIL_LIBS) \
	$(SWRESAMPLE_LIBS) \
	$(FAUDIO_LIBS) \
	-lpthread

AM_CPPFLAGS = \
	$(COMMON_CFLAGS) \
	$(AVCODEC_CFLAGS) \
	$(AVFORMAT_CFLAGS) \
	$(AVUTIL_CFLAGS) \
	$(SWRESAMPLE_CFLAGS) \
	$(FAUDIO_CFLAGS) \
	-I$(srcdir)/../../src/common \
	-DPACKAGE_VERSION=\"$(PACKAGE_VERSION)\"

AM_CFLAGS = -Wall -Wextra
