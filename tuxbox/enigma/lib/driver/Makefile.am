INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/system

noinst_LIBRARIES = libdriver.a

libdriver_a_SOURCES = \
	eavswitch.cpp rc.cpp rc.moc.cpp streamwd.cpp streamwd.moc.cpp

%.moc.cpp: %.h
	$(MOC) $< -o $@

clean-local:
	-rm *.moc.cpp
