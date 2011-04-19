BUILT_SOURCES = usf_wrap.cpp
SWIG_SOURCES = usf.i

pkgpyexec_PYTHON = pyusf.py #__init__.py
pkgpyexec_LTLIBRARIES = _pyusf.la
_pyusf_la_SOURCES = usf_wrap.cpp $(SWIG_SOURCES)
_pyusf_la_CPPFLAGS = $(AX_SWIG_PYTHON_CPPFLAGS) -I$(top_srcdir)/include
_pyusf_la_LDFLAGS = -module #-version-info $(USF_VERSION_MAJOR):$(USF_VERSION_MINOR):$(USF_VERSION_MICRO)
_pyusf_la_LIBADD = ../lib/libusf.la

usf_wrap.cpp : $(SWIG_SOURCES)
	$(SWIG) -Wall -v $(AX_SWIG_PYTHON_OPT) -I$(top_srcdir)/lib -I$(top_srcdir)/include -o $@ $^
