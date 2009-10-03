installdir = $(DATADIR)/neutrino/icons

install_DATA = \
	16_9.raw 16_9_gray.raw audio.raw blau.raw border_lr.raw border_ul.raw \
	colors.raw dd.raw dd_avail.raw dd_gray.raw error.raw features.raw \
	file.raw folder.raw games.raw gelb.raw gruen.raw help.raw help_small.raw \
	info.raw keybinding.raw language.raw lcd.raw lock.raw mainmenue.raw \
	mounted.raw mp3.raw mute.raw network.raw not_mounted.raw numericpad.raw ok.raw power.raw radar.pal \
	radar0.raw radar1.raw radar2.raw radar3.raw radar4.raw radar5.raw radar6.raw \
	radar7.raw radar8.raw radar9.raw radiomode.pal radiomode.raw recording.raw \
	rot.raw scan.pal scan.raw settings.raw shell.raw shutdown.pal shutdown.raw softupdate.raw \
	streaming.raw timer.raw upnp.raw video.raw volume.raw volumebody.raw volumeslider2.raw \
	volumeslider2alpha.raw volumeslider2blue.raw volumeslider2green.raw \
	volumeslider2red.raw vtxt.raw vtxt_gray.raw movie.raw movieplayer.raw \
	hidden.raw bookmarkmanager.raw mute_small.raw \
	0.raw 1.raw 2.raw 3.raw 4.raw 5.raw 6.raw 7.raw 8.raw 9.raw \
	up.raw right.raw down.raw left.raw fta.raw ca.raw \
	pause.raw play.raw protecting.raw epginfo.raw\
	conflict.raw record.raw record_conflict.raw zapto.raw zapto_conflict.raw \
	ok_small.raw up_small.raw down_small.raw plus.raw minus.raw \
	subt.raw subt_gray.raw

if ENABLE_RADIOTEXT
install_DATA += \
	radiotextget.raw radiotextoff.raw radiotextwait.raw
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
