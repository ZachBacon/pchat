EXTRA_DIST = 

libdir = $(xchatlibdir)

lib_LTLIBRARIES = python.la
python_la_SOURCES = python.c
python_la_LDFLAGS = -avoid-version -module -no-undefined
python_la_LIBADD = $(PY_LIBS)
AM_CPPFLAGS = $(PY_CFLAGS) $(COMMON_CFLAGS) -I$(srcdir)/../../src/common

