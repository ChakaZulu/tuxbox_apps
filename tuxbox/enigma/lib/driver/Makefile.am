INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/enigma/src

lib_LTLIBRARIES = libenigma_driver.la

libenigma_driver_la_SOURCES = \
	eavswitch.cpp rc.cpp rcdbox.cpp rcdreambox.cpp streamwd.cpp

AM_CXXFLAGS = $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

