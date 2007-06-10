AC_INIT(tuxbox-apps-misc-libs,0.0.1,,tuxbox-libs)
AM_INIT_AUTOMAKE(tuxbox-libs,0.0.1)

TUXBOX_APPS_ENDIAN
TUXBOX_APPS

AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL

AC_ARG_ENABLE(upnp,
	AS_HELP_STRING(--enable-upnp,include upnp support),
	,[enable_upnp=no])
AM_CONDITIONAL(ENABLE_UPNP,test "$enable_upnp" = "yes")

AC_OUTPUT([
Makefile
libconfigfile/Makefile
libconfigfile/tuxbox-configfile.pc
libconnection/Makefile
libconnection/tuxbox-connection.pc
libcramfs/Makefile
libcramfs/tuxbox-cramfs.pc
libeventserver/Makefile
libeventserver/tuxbox-eventserver.pc
libmd5sum/Makefile
libmd5sum/tuxbox-md5sum.pc
libmpegtools/Makefile
libmpegtools/tuxbox-mpegtools.pc
libnet/Makefile
libnet/tuxbox-net.pc
libucodes/Makefile
libucodes/tuxbox-ucodes.pc
libxmltree/Makefile
libxmltree/tuxbox-xmltree.pc
libupnpclient/Makefile
libupnpclient/tuxbox-upnpclient.pc
])
