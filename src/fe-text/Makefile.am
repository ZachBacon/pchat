localedir = $(datadir)/locale
include $(top_srcdir)/common.mk
bin_PROGRAMS = pchat-text

AM_CPPFLAGS = $(COMMON_CFLAGS) -DLOCALEDIR=\"$(localedir)\"

pchat_text_LDADD = ../common/libpchatcommon.a $(COMMON_LIBS)
pchat_text_SOURCES = fe-text.c fe-text.h

