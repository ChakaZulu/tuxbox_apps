INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/system

lib_LTLIBRARIES = libdriver.la

libdriver_la_SOURCES = \
	eavswitch.cpp rc.cpp rc.moc.cpp rcdbox.cpp rcdbox.moc.cpp rcdreambox.cpp rcdreambox.moc.cpp streamwd.cpp streamwd.moc.cpp

%.moc.cpp: %.h
	$(MOC) $< -o $@

clean-local:
	-rm *.moc.cpp
