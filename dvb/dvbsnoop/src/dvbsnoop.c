/*
$Id: dvbsnoop.c,v 1.24 2004/01/06 03:13:24 rasc Exp $

 DVBSNOOP

 a dvb sniffer and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de






$Log: dvbsnoop.c,v $
Revision 1.24  2004/01/06 03:13:24  rasc
TS prints PES/Section ID on payload_start

Revision 1.23  2004/01/03 16:40:11  rasc
no message

Revision 1.22  2004/01/03 15:40:45  rasc
simple frontend signal status query added "-s signal"

Revision 1.21  2004/01/02 02:18:33  rasc
more DSM-CC  INT/UNT descriptors

Revision 1.20  2004/01/01 20:09:15  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.19  2003/12/28 14:00:25  rasc
bugfix: section read from input file
some changes on packet header output

Revision 1.18  2003/12/15 20:09:48  rasc
no message

Revision 1.17  2003/12/14 23:38:46  rasc
- bandwidth reporting for a PID

Revision 1.16  2003/12/10 20:07:14  rasc
minor stuff

Revision 1.15  2003/12/07 23:36:12  rasc
pidscan on transponder
- experimental(!)

Revision 1.14  2003/12/03 20:06:33  obi
- reduced auto* to minimal required checks, obsoletes acinclude.m4
- added version number to configure.ac, removed it from version.h
  (needed for "make dist" anyway)
- removed autoheader dependency

Revision 1.13  2003/11/07 16:33:31  rasc
no message

Revision 1.12  2003/11/01 21:40:26  rasc
some broadcast/linkage descriptor stuff

Revision 1.11  2003/11/01 17:05:46  rasc
no message

Revision 1.10  2003/10/24 23:01:40  rasc
code reorg...

Revision 1.8  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.7  2003/07/06 05:28:52  obi
compatibility stuff.. now there is only one version for old and new drivers
which selects the api at configure time


Revision 1.5  2003/02/26 16:45:16  obi
- make dvbsnoop work on little endian machines again
- fixed mask in getBits for bitlen >= 32

Revision 1.4  2003/02/09 22:59:33  rasc
-- endian check (bug fix)

Revision 1.3  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.2  2001/10/02 21:52:44  rasc
- init der time_delta
- PES erweitert, PES arbeitet im read() noch nicht richtig!!
- muss tmbinc fragem, ob ich Mist baue, oder der Treiber (??)

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"

#include "misc/cmdline.h"
#include "misc/hexprint.h"
#include "misc/pkt_time.h"

#include "dvb_api/dmx_sect.h"
#include "dvb_api/dmx_pes.h"
#include "dvb_api/dmx_ts.h"
#include "dvb_api/dmx_tspidscan.h"
#include "dvb_api/dmx_tspidbandwidth.h"
#include "dvb_api/fe_signal.h"






int main(int argc, char **argv)

{

  OPTION  opt;
  int     err = 0;


  /*
    -- init options and cmdline parsing
  */


  if (! cmdline_options (argc,argv, &opt) ) return (-1);

  setVerboseLevel (opt.printdecode);
  setHexPrintMode (opt.printhex);



  indent (0);
  if (! opt.binary_out) {
     if (! opt.hide_copyright) {
        out_nl (1, "%s -- %s ", DVBSNOOP_PROG_VERSION, DVBSNOOP_URL);
     }

     if (! opt.inpPidFile) {
	if (opt.pid != DUMMY_PID) {
		out_nl (9, "   PID   : %d (0x%04x)",opt.pid,opt.pid);
	}
	if (opt.packet_mode == SECT) {		// filter are only for sections
		out_nl (9, "   Filter: %d (0x%04x)",opt.filter,opt.filter);
		out_nl (9, "   Mask  : %d (0x%04x)",opt.mask,opt.mask);
	}
	out_nl (9, "   DEMUX : %s",opt.devDemux);
	out_nl (9, "   DVR   : %s",opt.devDvr);
	out_nl (9, "   FRONTEND: %s",opt.devFE);
     }
  }



  init_receive_time ();


	  switch (opt.packet_mode) {
		case SECT:
			err = doReadSECT (&opt);
			break;

		case PES:
			err = doReadPES (&opt);
			break;

		case TS:
			err = doReadTS (&opt);
			break;

		case PIDSCAN:
			err = ts_pidscan (&opt);
			break;

		case PIDBANDWIDTH:
			err = ts_pidbandwidth (&opt);
			break;

		case SIGNALSCAN:
			err = do_SignalStrength (&opt);
			break;

		default:
			fprintf (stderr,"unknown snoop type (this should never happen..\n");
			break;

	  }

 return err;

}






