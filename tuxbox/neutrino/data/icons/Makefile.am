installdir = $(DATADIR)/neutrino/icons

install_DATA = \
	0.raw 1.raw 2.raw 3.raw 4.raw 5.raw 6.raw 7.raw 8.raw 9.raw \
	16_9_gray.raw 16_9.raw \
	audio.raw \
	blau.raw bookmarkmanager.raw border_lr.raw border_ul.raw \
	ca.raw colors.raw conflict.raw \
	dd_avail.raw dd_gray.raw dd.raw down.raw down_small.raw \
	epginfo.raw error.raw \
	features.raw file.raw folder.raw fta.raw \
	games.raw gelb.raw gruen.raw \
	help.raw help_small.raw	hidden.raw \
	info.raw \
	keybinding.raw \
	language.raw lcd.raw left.raw lock.raw \
	mainmenue.raw minus.raw	mounted.raw movie.raw mp3.raw mute.raw mute_small.raw \
	network.raw not_mounted.raw numericpad.raw \
	ok.raw ok_small.raw \
	pause.raw play.raw plus.raw power.raw protecting.raw \
	radar0.raw radar1.raw radar2.raw radar3.raw radar4.raw radar5.raw radar6.raw radar7.raw	radar8.raw radar9.raw radar.pal \
	radiomode.pal radiomode.raw record_conflict.raw recording.raw record.raw right.raw rot.raw \
	scan.pal scan.raw settings.raw shell.raw shutdown.pal shutdown.raw softupdate.raw streaming.raw \
	subt_gray.raw subt.raw \
	timer.raw \
	up.raw up_small.raw \
	video.raw volumebody.raw volume.raw volumeslider2alpha.raw volumeslider2blue.raw volumeslider2green.raw volumeslider2.raw volumeslider2red.raw \
	vtxt_gray.raw vtxt.raw \
	zapto_conflict.raw zapto.raw

if ENABLE_RADIOTEXT
install_DATA += \
	radiotextget.raw radiotextoff.raw radiotextwait.raw
endif

if ENABLE_UPNP
install_DATA += \
	upnp.raw
endif

if ENABLE_MOVIEPLAYER
install_DATA += \
	movieplayer.raw
endif

if BOXTYPE_TRIPLE
install-data-local:
	$(INSTALL) -m 755 -d $(installdir)
	$(INSTALL) -m 644 exit.raw $(installdir)/home.raw
	$(INSTALL) -m 644 menu.raw $(installdir)/dbox.raw
	$(INSTALL) -m 644 menu_small.raw $(installdir)/dbox_small.raw
else
install_DATA += home.raw dbox.raw dbox_small.raw
endif

