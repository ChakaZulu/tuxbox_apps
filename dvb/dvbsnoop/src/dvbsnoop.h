/*
$Id: dvbsnoop.h,v 1.6 2003/06/24 23:51:03 rasc Exp $

 -- dvbsnoop
 -- a dvb sniffer tool
 -- mainly for me to learn the dvb streams

   (c) rasc

$Log: dvbsnoop.h,v $
Revision 1.6  2003/06/24 23:51:03  rasc
bugfixes and enhancements

Revision 1.5  2002/11/01 20:38:40  Jolt
Changes for the new API

Revision 1.4  2002/09/13 23:57:04  obi
define NEWSTRUCT to work with current linuxtv cvs

Revision 1.3  2002/08/27 19:00:45  obi
use devfs device names

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/

#define VERSION  "0.9b"

#define NEWSTRUCT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef NEWSTRUCT
#include <linux/dvb/dmx.h>
#define DEMUX_DEVICE "/dev/dvb/adapter0/demux0"
#define DVR_DEVICE   "/dev/dvb/adapter0/dvr0"
#else
#include <ost/dmx.h>
#define DEMUX_DEVICE "/dev/dvb/card0/demux0"
#define DVR_DEVICE   "/dev/dvb/card0/dvr0"
#endif

#include "helper.h"
#include "dvb_str.h"
#include "output.h"

