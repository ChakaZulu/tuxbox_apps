AC_INIT(tuxbox-radiobox,1.0.1)
AM_INIT_AUTOMAKE(tuxbox-radiobox,1.0.1) 

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX
AC_DISABLE_STATIC
AM_PROG_LIBTOOL


TUXBOX_APPS_DVB
TUXBOX_APPS_DRIVER
TUXBOX_APPS_LIB_CONFIG(CURL,curl-config)
TUXBOX_APPS_LIB_CONFIG(FREETYPE,freetype-config)
TUXBOX_APPS_LIB_PKGCONFIG(ID3TAG,id3tag)
TUXBOX_APPS_LIB_PKGCONFIG(MAD,mad)
TUXBOX_APPS_LIB_PKGCONFIG(VORBISIDEC,vorbisidec)
TUXBOX_APPS_LIB_PKGCONFIG(PNG,libpng)
TUXBOX_APPS_LIB_PKGCONFIG(CONFIGFILE,tuxbox-configfile)
TUXBOX_APPS_LIB_PKGCONFIG(CONNECTION,tuxbox-connection)
TUXBOX_APPS_LIB_PKGCONFIG(CRAMFS,tuxbox-cramfs)
TUXBOX_APPS_LIB_PKGCONFIG(EVENTSERVER,tuxbox-eventserver)
TUXBOX_APPS_LIB_PKGCONFIG(LCDDISPLAY,tuxbox-lcddisplay)
TUXBOX_APPS_LIB_PKGCONFIG(LIRCDCLIENT,tuxbox-lircdclient)
TUXBOX_APPS_LIB_PKGCONFIG(NET,tuxbox-net)
TUXBOX_APPS_LIB_PKGCONFIG(TUXBOX,tuxbox)
TUXBOX_APPS_LIB_PKGCONFIG(MPEGTOOLS,tuxbox-mpegtools)

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

AC_ARG_ENABLE(use-libxml2,
   [  --enable-use-libxml2    use libxml2 instead of libxmltree],
      [AC_DEFINE(USE_LIBXML,1,[use libxml2 instead of libxmltree])
               TUXBOX_APPS_LIB_PKGCONFIG(XML,libxml-2.0)],
                       [TUXBOX_APPS_LIB_PKGCONFIG(XML,tuxbox-xmltree)])

AC_OUTPUT([
Makefile
src/Makefile
])

