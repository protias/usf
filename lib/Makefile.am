
lib_LTLIBRARIES = libusf.la

libusf_la_SOURCES = 			\
	usf_events.c 			\
	usf_header.c usf_header.h 	\
	usf_file.c 			\
	usf_utils.c 			\
	usf_priv.h 			\
	error.h				\
	usf_internal.c usf_internal.h	\
	os_compat.h

libusf_la_CPPFLAGS = -I $(top_srcdir)/include -fPIC
libusf_la_LDFLAGS = -version-info $(USF_VERSION_MAJOR):$(USF_VERSION_MINOR):$(USF_VERSION_MICRO)
