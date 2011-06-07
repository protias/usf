ACLOCAL_AMFLAGS = -I m4

if HAVE_PIN
OPT_PIN=pin
endif

if HAVE_SWIG
OPT_SWIG=swig
endif

SUBDIRS=include lib tools $(OPT_PIN) $(OPT_SWIG) test

EXTRA_DIST = autogen.sh

