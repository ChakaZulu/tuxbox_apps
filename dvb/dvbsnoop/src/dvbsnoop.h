/*
$Id: dvbsnoop.h,v 1.16 2003/12/03 20:06:33 obi Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de




$Log: dvbsnoop.h,v $
Revision 1.16  2003/12/03 20:06:33  obi
- reduced auto* to minimal required checks, obsoletes acinclude.m4
- added version number to configure.ac, removed it from version.h
  (needed for "make dist" anyway)
- removed autoheader dependency

Revision 1.15  2003/11/26 19:55:31  rasc
no message

Revision 1.14  2003/11/07 16:33:31  rasc
no message

Revision 1.13  2003/11/01 17:05:46  rasc
no message

Revision 1.12  2003/10/24 22:17:13  rasc
code reorg...

Revision 1.11  2003/10/17 19:14:40  rasc
no message

Revision 1.10  2003/10/16 19:02:29  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.9  2003/10/13 23:35:35  rasc
Bugfix, verbose < 4 segfaulted, tnx to 'mws'  for reporting.

Revision 1.8  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?

Revision 1.7  2003/07/06 05:28:52  obi
compatibility stuff.. now there is only one version for old and new drivers
which selects the api at configure time

Revision 1.6  2003/06/24 23:51:03  rasc
bugfixes and enhancements

Revision 1.5  2002/11/01 20:38:40  Jolt
Changes for the new API

Revision 1.4  2002/09/13 23:57:04  obi
define NEWSTRUCT to work with current linuxtv cvs

Revision 1.3  2002/08/27 19:00:45  obi
use devfs device names

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/

#ifndef __DVBSNOOP_H
#define __DVBSNOOP_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#if defined(HAVE_LINUX_DVB_DMX_H)
#include <linux/dvb/dmx.h>
#define DEMUX_DEVICE "/dev/dvb/adapter0/demux0"
#define DVR_DEVICE   "/dev/dvb/adapter0/dvr0"
#elif defined(HAVE_OST_DMX_H)
#include <ost/dmx.h>
#define DEMUX_DEVICE "/dev/dvb/card0/demux0"
#define DVR_DEVICE   "/dev/dvb/card0/dvr0"
#define dmx_pes_filter_params dmxPesFilterParams
#define dmx_sct_filter_params dmxSctFilterParams
#define pes_type pesType
#endif

#include "version.h"
#include "strings/dvb_str.h"
#include "misc/helper.h"
#include "misc/output.h"


#endif


