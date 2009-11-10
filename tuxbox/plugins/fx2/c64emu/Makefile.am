AM_CPPFLAGS = -I$(srcdir)/../lib -I$(top_srcdir)/include

#noinst_LTLIBRARIES = c64emu.la

bin_PROGRAMS = c64emu

c64emu_SOURCES = main.cpp Display.cpp Prefs.cpp SID.cpp REU.cpp IEC.cpp 1541fs.cpp \
		 1541d64.cpp 1541t64.cpp 1541job.cpp SAM.cpp CmdPipe.cpp \
		 C64.cpp CPUC64.cpp VIC.cpp CIA.cpp CPU1541.cpp vga.c

#c64emu_la_SOURCES = main.cpp Display.cpp Prefs.cpp SID.cpp REU.cpp IEC.cpp 1541fs.cpp \
#                    1541d64.cpp 1541t64.cpp 1541job.cpp SAM.cpp CmdPipe.cpp

c64emu_LDADD = ../lib/libfx2.la

c64emu_LDFLAGS = -rpath $(PLUGINDIR) -R$(TARGET_PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/c64emu
#	$(LIBTOOL) --mode=install install c64emu.la $(DESTDIR)$(PLUGINDIR)

#uninstall-local:
#	-rm $(DESTDIR)$(PLUGINDIR)/c64emu.so
#	-rmdir $(DESTDIR)$(PLUGINDIR)
