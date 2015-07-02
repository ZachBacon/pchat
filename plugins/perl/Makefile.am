
EXTRA_DIST=generate_header lib/Xchat.pm lib/HexChat/Embed.pm lib/XChat/List/Network.pm \
	lib/XChat/List/Network/Entry.pm lib/XChat/List/Network/AutoJoin.pm lib/IRC.pm

libdir = $(xchatlibdir)

lib_LTLIBRARIES = perl.la
perl_la_SOURCES = perl.c
perl_la_LDFLAGS = -avoid-version -module -no-undefined
perl_la_LIBADD = $(PERL_LDFLAGS)
BUILT_SOURCES = xchat.pm.h irc.pm.h
#CFLAGS = @CFLAGS@ -Wno-unused
AM_CPPFLAGS = $(PERL_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/../../src/common
CLEANFILES = xchat.pm.h irc.pm.h
xchat.pm.h irc.pm.h: lib/XChat.pm lib/XChat/Embed.pm \
	lib/XChat/List/Network.pm lib/XChat/List/Network/Entry.pm \
	lib/XChat/List/Network/AutoJoin.pm lib/IRC.pm
	perl generate_header
