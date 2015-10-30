include $(top_srcdir)/common.mk
localedir = $(datadir)/locale

bin_PROGRAMS = pchat

AM_CPPFLAGS = $(GUI_CFLAGS) -DLOCALEDIR=\"$(localedir)\"

pchat_LDADD = ../common/libpchatcommon.a $(GUI_LIBS)

EXTRA_DIST = \
	ascii.h banlist.h chanlist.h chanview.h chanview-tabs.c \
	chanview-tree.c custom-list.h editlist.h fe-gtk.h fkeys.h gtkutil.h joind.h \
	maingui.h menu.h notifygui.h palette.h pixmaps.h \
	plugin-tray.h plugingui.c plugingui.h rawlog.h sexy-iso-codes.h \
	sexy-spell-entry.h textgui.h urlgrab.h userlistgui.h xtext.h \
	../../data/pchat.gresource.xml

BUILT_SOURCES = resources.c

CLEANFILES = $(BUILT_SOURCES)

if OS_W32
w32_res = pchat_rc.o
w32_res_ldflags = -Wl,pchat_rc.o
pchat_rc.o : pchat.rc pchat.exe.manifest
	$(WINDRES) -I $(srcdir) -I $(top_srcdir)/data/icons --input pchat.rc --output $@ --output-format=coff
endif

if DO_PLUGIN
plugingui_c = plugingui.c
endif

if HAVE_ISO_CODES
iso_codes_c = sexy-iso-codes.c
endif

pchat_SOURCES = ascii.c banlist.c chanlist.c chanview.c custom-list.c \
	dccgui.c editlist.c fe-gtk.c fkeys.c gtkutil.c ignoregui.c joind.c menu.c \
	maingui.c notifygui.c palette.c pixmaps.c plugin-tray.c $(plugingui_c) \
	rawlog.c resources.c servlistgui.c setup.c $(iso_codes_c) \
	sexy-spell-entry.c textgui.c urlgrab.c userlistgui.c xtext.c
pchat_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_builddir)/src/common
pchat_LDFLAGS = $(w32_res_ldflags)
pchat_DEPENDENCIES = $(w32_res)

resources.c: $(top_srcdir)/data/pchat.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(top_srcdir)/data --generate-dependencies $(top_srcdir)/data/pchat.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(top_srcdir)/data --generate-source $<

