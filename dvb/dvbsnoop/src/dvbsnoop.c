/*
$Id: dvbsnoop.c,v 1.9 2003/10/24 22:17:13 rasc Exp $



 DVBSNOOP

 a dvb sniffer tool
 mainly for me to learn about dvb streams, mpeg, mhp, and dsm-cc

 it's forbidden to use this program for hacking encryption
 information (like ECM/EMM streams to get extract keys, etc.).

 I also don't garantee, that data output is correct in any way.

 (c) 2001-2003   Rainer.Scherg@gmx.de


 -- Sorry for the bad coding, it's really quick and dirty. ;-)
 -- But this programm was for learning and testing only.
 -- The code could be heavily optimized...
 -- For more information please see: ISO 13818-1 and ETSI 300 468
 -- READ THE LICENCE FILE!


$Log: dvbsnoop.c,v $
Revision 1.9  2003/10/24 22:17:13  rasc
code reorg...

Revision 1.8  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.7  2003/07/06 05:28:52  obi
compatibility stuff.. now there is only one version for old and new drivers
which selects the api at configure time

Revision 1.6  2003/07/06 02:22:46  rasc
no message

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
#include "dvb_api/dmx_sect.h"
#include "dvb_api/dmx_pes.h"
#include "dvb_api/dmx_ts.h"
#include "misc/cmdline.h"
#include "misc/hexprint.h"
#include "misc/pkt_time.h"







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
     out_nl (1, "DvbSnoop   Vers. %s   -- (c) Rainer Scherg",DVBSNOOP_VERSION);
     out_nl (9, "   PID   : %d (0x%04x)",opt.pid,opt.pid);
     out_nl (9, "   Filter: %d (0x%04x)",opt.filter,opt.filter);
     out_nl (9, "   Mask  : %d (0x%04x)",opt.mask,opt.mask);
     out_nl (9, "   DEMUX : %s",opt.devDemux);
     out_nl (9, "   DVR   : %s",opt.devDvr);
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

		default:
			fprintf (stderr,"unknown Stream Type selected...\n");
			break;

	  }

 return err;

}






