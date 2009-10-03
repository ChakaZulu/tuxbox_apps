AC_INIT(tuxbox-neutrino,1.0.1)
AM_INIT_AUTOMAKE(tuxbox-neutrino,1.0.1) 

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY
TUXBOX_APPS_PKGCONFIG
TUXBOX_BOXTYPE

AC_PROG_CC
AC_PROG_CXX
AC_DISABLE_STATIC
AM_PROG_LIBTOOL

TUXBOX_APPS_DVB
TUXBOX_APPS_DRIVER
TUXBOX_APPS_LIB_CONFIG(FREETYPE,freetype-config)
TUXBOX_APPS_LIB_PKGCONFIG(CONFIGFILE,tuxbox-configfile)
TUXBOX_APPS_LIB_PKGCONFIG(CONNECTION,tuxbox-connection)
TUXBOX_APPS_LIB_PKGCONFIG(EVENTSERVER,tuxbox-eventserver)
TUXBOX_APPS_LIB_PKGCONFIG(LCDDISPLAY,tuxbox-lcddisplay)
TUXBOX_APPS_LIB_PKGCONFIG(NET,tuxbox-net)
TUXBOX_APPS_LIB_PKGCONFIG(PLUGINS,tuxbox-plugins)
TUXBOX_APPS_LIB_PKGCONFIG(TUXBOX,tuxbox)
TUXBOX_APPS_LIB_PKGCONFIG(UCODES,tuxbox-ucodes)
TUXBOX_APPS_LIB_PKGCONFIG(ZAPIT,tuxbox-zapit)
TUXBOX_APPS_LIB_PKGCONFIG(MPEGTOOLS,tuxbox-mpegtools)
TUXBOX_APPS_LIB_PKGCONFIG(TUXTXT,tuxbox-tuxtxt)
TUXBOX_APPS_LIB_PKGCONFIG(XMLTREE,tuxbox-xmltree)

AC_ARG_ENABLE(16-bit-mode,
         [  --enable-16-bit-mode                enable 16 bit graphics mode],
         [],
         [AC_DEFINE(FB_USE_PALETTE,1,[8 bit graphics mode])])

AC_ARG_ENABLE(keyboard-no-rc,
         [  --enable-keyboard-no-rc             enable keyboard control, disable rc control],
         [AC_DEFINE(KEYBOARD_INSTEAD_OF_REMOTE_CONTROL,1,[enable keyboard control, disable rc control])])

AC_ARG_ENABLE(restore-prev-mode,
         [  --enable-restore-prev-mode          enable return from graphics mode],
         [AC_DEFINE(RETURN_FROM_GRAPHICS_MODE,1,[enable return from graphics mode])])

AC_ARG_ENABLE(upnp,
	AS_HELP_STRING(--enable-upnp,include upnp support),
	,[enable_upnp=no])
AM_CONDITIONAL(ENABLE_UPNP,test "$enable_upnp" = "yes")

AC_ARG_ENABLE(lirc,
	AS_HELP_STRING(--enable-lirc,include lirc support),
	,[enable_lirc=no])
AM_CONDITIONAL(ENABLE_LIRC,test "$enable_lirc" = "yes")

AC_ARG_ENABLE(flac,
	AS_HELP_STRING(--enable-flac,include flac support),
	,[enable_flac=no])
AM_CONDITIONAL(ENABLE_FLAC,test "$enable_flac" = "yes")

# neutrino UK FreeSat EPG (experimental) #########################
AC_ARG_ENABLE(freesatepg,
	AS_HELP_STRING(--enable-freesatepg,enable experimental UK FreeSat Neutrino EPG),
	,[enable_freesatepg=no])
AM_CONDITIONAL(ENABLE_FREESATEPG,test "$enable_freesatepg" = "yes")

AC_ARG_ENABLE(esd,
	AS_HELP_STRING(--enable-esd,include esd sound daemon),
	,[enable_esd=no])
AM_CONDITIONAL(ENABLE_ESD,test "$enable_esd" = "yes")

AC_ARG_ENABLE(radiotext,
	AS_HELP_STRING(--enable-radiotext,include Radiotext support),
	,[enable_radiotext=no])
AM_CONDITIONAL(ENABLE_RADIOTEXT,test "$enable_radiotext" = "yes")

AC_ARG_ENABLE(audioplayer,
	AS_HELP_STRING(--disable-audioplayer,include Neutrino audioplayer/internetradio),
	,[enable_audioplayer=yes])
AM_CONDITIONAL(ENABLE_AUDIOPLAYER,test "$enable_audioplayer" = "yes")
AM_CONDITIONAL(ENABLE_INTERNETRADIO,test "$enable_audioplayer" = "yes")

AC_ARG_ENABLE(movieplayer,
	AS_HELP_STRING(--disable-movieplayer,include Neutrino movieplayer),
	,[enable_movieplayer=yes])
AM_CONDITIONAL(ENABLE_MOVIEPLAYER,test "$enable_movieplayer" = "yes")

# neutrino movieplayer2 (experimental) #########################
AC_ARG_ENABLE(movieplayer2,
	AS_HELP_STRING(--enable-movieplayer2,enable experimental neutrino movieplayer2),
	,[enable_movieplayer2=no])

AC_ARG_ENABLE(pictureviewer,
	AS_HELP_STRING(--disable-pictureviewer,include Neutrino pictureviewer),
	,[enable_pictureviewer=yes])
AM_CONDITIONAL(ENABLE_PICTUREVIEWER,test "$enable_pictureviewer" = "yes")

AC_ARG_ENABLE(use-libxml2,
   [  --enable-use-libxml2    use libxml2 instead of libxmltree],
      [AC_DEFINE(USE_LIBXML,1,[use libxml2 instead of libxmltree])
               TUXBOX_APPS_LIB_PKGCONFIG(XML,libxml-2.0)],
                       [TUXBOX_APPS_LIB_PKGCONFIG(XML,tuxbox-xmltree)])

AC_ARG_ENABLE(http-update,
	AS_HELP_STRING(--enable-http-update,activate http update),
	,[enable_http_update=yes])
AM_CONDITIONAL(DISABLE_INTERNET_UPDATE,test "$enable_http_update" = "no")

AC_ARG_ENABLE(gui-mount,
	AS_HELP_STRING(--disable-gui-mount,disable GUI mount functionality),
	,[enable_gui_mount=yes])
AM_CONDITIONAL(ENABLE_GUI_MOUNT,test "$enable_gui_mount" = "yes")

if test "$enable_upnp" = "yes"; then
TUXBOX_APPS_LIB_PKGCONFIG(UPNPCLIENT,tuxbox-upnpclient)
AC_DEFINE(ENABLE_UPNP,1,[include upnp support])
fi

if test "$enable_lirc" = "yes"; then
TUXBOX_APPS_LIB_PKGCONFIG(LIRCDCLIENT,tuxbox-lircdclient)
AC_DEFINE(ENABLE_LIRC,1,[include lirc support])
fi

if test "$enable_flac" = "yes"; then
AC_DEFINE(ENABLE_FLAC,1,[include FLAC support])
fi

if test "$enable_http_update" = "no"; then
AC_DEFINE(DISABLE_INTERNET_UPDATE,1,[deactivate http update])
fi

if test "$enable_gui_mount" = "yes"; then
AC_DEFINE(ENABLE_GUI_MOUNT,1,[activate GUI mount functionality])
fi

if test "$enable_freesatepg" = "yes"; then
AC_DEFINE(ENABLE_FREESATEPG,1,[enable UK FreeSat EPG])
fi

if test "$enable_esd" = "yes"; then
AC_DEFINE(ENABLE_ESD,1,[include esd sound daemon])
fi

if test "$enable_radiotext" = "yes"; then
AC_DEFINE(ENABLE_RADIOTEXT,1,[include Radiotext support])
fi

if test "$enable_audioplayer" = "yes"; then
AC_DEFINE(ENABLE_AUDIOPLAYER,1,[include Neutrino audioplayer/internetradio])
AC_DEFINE(ENABLE_INTERNETRADIO,1,[include Neutrino internetradio])
TUXBOX_APPS_LIB_PKGCONFIG(ID3TAG,id3tag)
TUXBOX_APPS_LIB_PKGCONFIG(MAD,mad)
TUXBOX_APPS_LIB_PKGCONFIG(VORBISIDEC,vorbisidec)
fi

if test "$enable_movieplayer" = "yes"; then
	AC_DEFINE(ENABLE_MOVIEPLAYER,1,[include Neutrino movieplayer])
	AC_DEFINE(ENABLE_MOVIEPLAYER_VLC,1,[include Neutrino movieplayer VLC support])
	if test "$enable_movieplayer2" = "yes"; then
		AC_DEFINE(ENABLE_MOVIEPLAYER2,1,[include Movieplayer2 support])
	else
		AC_DEFINE(ENABLE_MOVIEBROWSER,1,[include Moviebrowser support])
	fi
else
	enable_movieplayer2=no
fi
AM_CONDITIONAL(ENABLE_MOVIEPLAYER2,test "$enable_movieplayer2" = "yes")

if test "$enable_pictureviewer" = "yes"; then
AC_DEFINE(ENABLE_PICTUREVIEWER,1,[include Neutrino pictureviewer])
TUXBOX_APPS_LIB_PKGCONFIG(PNG,libpng)
fi

if test "$enable_audioplayer" = "yes" -o \
   "$enable_movieplayer" = "yes" -o \
   "$enable_http_update" = "yes"; then
TUXBOX_APPS_LIB_CONFIG(CURL,curl-config)
AC_DEFINE(HAVE_CURL,1,[include Curl support])
else
CURL_LIBS=""
CURL_CFLAGS=""
AC_SUBST(CURL_LIBS)
AC_SUBST(CURL_CFLAGS)
fi

CPPFLAGS="$CPPFLAGS -Wextra -Wshadow"

AC_OUTPUT([
Makefile
lib/Makefile
lib/sectionsdclient/Makefile
lib/timerdclient/Makefile
lib/irsend/Makefile
daemons/Makefile
daemons/nhttpd/Makefile
daemons/nhttpd/web/Makefile
daemons/nhttpd/web/images/Makefile
daemons/nhttpd/web/scripts/Makefile
daemons/nhttpd/tuxboxapi/Makefile
daemons/nhttpd/yhttpd_core/Makefile
daemons/nhttpd/yhttpd_mods/Makefile
daemons/sectionsd/Makefile
daemons/timerd/Makefile
src/Makefile
src/daemonc/Makefile
src/driver/pictureviewer/Makefile
src/driver/audiodec/Makefile
src/driver/Makefile
src/gui/Makefile
src/gui/bedit/Makefile
src/gui/widget/Makefile
src/system/Makefile
data/Makefile
data/fonts/Makefile
data/icons/Makefile
data/iso-codes/Makefile
data/lcd/Makefile
data/lcd/icons/Makefile
data/lcd/clock/Makefile
data/locale/Makefile
data/themes/Makefile
data/inetradio/Makefile
])

