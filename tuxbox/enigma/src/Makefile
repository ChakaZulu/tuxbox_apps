CROSS_COMPILE=powerpc-linux-
CC=$(CROSS_COMPILE)g++
CPP=$(CROSS_COMPILE)cpp
AR=$(CROSS_COMPILE)ar
RANLIB=$(CROSS_COMPILE)ranlib
STRIP=$(CROSS_COMPILE)strip

CFLAGS=-Wall -Ifreetype/freetype2/include -g -I/home/tmbinc/cvs/driver/include -I. -Iminiqt -Ixml -DDEBUG -fno-rtti -DQWS -Wall -O3
LFLAGS=
LIBS = miniqt.a xml.a -ldl -lpthread -lfreetype -lpng 
# libfreetype.a
# -lfreetype 
# miniqt.a libfreetype.a
# -lmemusage -I./miniqt 
# -lfreetype -lqte
MOC=/dbox2/cdk/bin/moc

all: ezap

EDVB_OBJS = rc.o rc.moc.o ewidget.o ewidget.moc.o fb.o font.o ewindow.o ewindow.moc.o elistbox.o elistbox.moc.o \
            esection.o esection.moc.o si.o si.moc.o si/decode.o frontend.o frontend.moc.o dvb.o dvb.moc.o scan.o scan.moc.o \
            elabel.o elabel.moc.o eprogress.o eprogress.moc.o sselect.o sselect.moc.o edvb.o edvb.moc.o decoder.o \
            streaminfo.o streaminfo.moc.o mktime.o httpd.o httpd.moc.o qserversocket.o http_file.o http_dyn.o ezap_dyn.o \
            bselect.o bselect.moc.o multipage.o multipage.moc.o showbnversion.o showbnversion.moc.o \
            epgcache.o epgcache.moc.o nconfig.o setupaudio.o setupaudio.moc.o setupnetwork.o setupnetwork.moc.o \
            enumber.o enumber.moc.o lcd.o ebutton.o ebutton.moc.o echeckbox.o echeckbox.moc.o xmlrpc.o \
            ezap_xmlrpc.o ezap_lcd.o ezap_lcd.moc.o setup_osd.o setup_osd.moc.o elbwindow.o elbwindow.moc.o \
            emessage.o emessage.moc.o ezap_plugins.o ezap_plugins.moc.o grc.o eskin.o eskin_register.o elock.o \
            epng.o gpixmap.o gfbdc.o glcddc.o epixmap.o eavswitch.o setupvideo.o setupvideo.moc.o streamwd.o streamwd.moc.o

EZAP_OBJS = $(EDVB_OBJS) ezap.o ezap.moc.o ezap_main.o ezap_main.moc.o ezap_setup.o ezap_setup.moc.o ezap_event.o ezap_event.moc.o

TUNE_OBJS = tune.o edvb.o frontend.o si.o dvb.o si.moc.o frontend.moc.o \
            esection.o esection.moc.o si/decode.o decoder.o nconfig.o \
            epgcache.o epgcache.moc.o edvb.moc.o mktime.o

XML_OBJS = xml/hashtab.o xml/xmlparse.o xml/xmlrole.o xml/xmltok.o xml/xmltree.o

MINIQT_OBJS = \
            miniqt/qapplication.o miniqt/qapplication_qws.o miniqt/qasyncio.o \
            miniqt/qbitarray.o miniqt/qbuffer.o miniqt/qcollection.o miniqt/qconnection.o \
            miniqt/qcstring.o miniqt/qdatastream.o miniqt/qdatetime.o miniqt/qdir.o miniqt/qdir_unix.o \
            miniqt/qfile.o miniqt/qfile_unix.o miniqt/qfileinfo.o miniqt/qfileinfo_unix.o \
            miniqt/qgarray.o miniqt/qgcache.o miniqt/qgdict.o miniqt/qgeneric.h \
            miniqt/qglist.o miniqt/qglobal.o miniqt/qguardedptr.o miniqt/qgvector.o miniqt/qhostaddress.o \
            miniqt/qiodevice.o miniqt/qmap.o miniqt/qmetaobject.o \
            miniqt/qobject.o miniqt/qrect.o miniqt/qregexp.o \
            miniqt/qsignal.o miniqt/qsignalmapper.o miniqt/qsize.o miniqt/qsocket.o miniqt/qsocketdevice_unix.o \
            miniqt/qsocketnotifier.o miniqt/qstring.o miniqt/qstringlist.o \
            miniqt/qtextcodec.o miniqt/qtextstream.o miniqt/qthread_unix.o miniqt/qtimer.o miniqt/qutfcodec.o \
            miniqt/qdns.o miniqt/qdns.moc.o \
            miniqt/qapplication.moc.o miniqt/qasyncio.moc.o miniqt/qguardedptr.moc.o miniqt/qserversocket.moc.o miniqt/qsignal.moc.o miniqt/qsignalmapper.moc.o \
            miniqt/qtimer.moc.o miniqt/qsocketnotifier.moc.o miniqt/qsocket.moc.o miniqt/qsocketdevice.o

miniqt.a: $(MINIQT_OBJS)
	$(AR) r miniqt.a $(MINIQT_OBJS)


xml.a: $(XML_OBJS)
	$(AR) qc xml.a $(XML_OBJS)
	$(RANLIB) xml.a

ezap: $(EZAP_OBJS) miniqt.a xml.a
	$(CC) $(LFLAGS) $(EZAP_OBJS) -o ezap $(LIBS)
	cp ezap ezap-debug
	$(STRIP) ezap

tune: $(TUNE_OBJS) miniqt.a 
	$(CC) $(LFLAGS) $(TUNE_OBJS) -o tune $(LIBS)

clean:
	rm -f *.o si/*.o xml/*.o
	rm -f *~ si/*~ xml/*~

distclean: clean
	rm -f miniqt/*.o *~ xml.a miniqt.a *.moc.cpp

install: all
	cp ezap ../../../cdkroot/bin/


%.s:    %.S
	$(CPP) $(AFLAGS) -o $@ $<
%.o:    %.S
	$(CC) $(AFLAGS) -c -o $@ $<
%.o:    %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o:    %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

%.moc.cpp: %.h
	$(MOC) $< -o $@

snapshot: .
	./dosnap
