/*
$Id: cmdline.c,v 1.26 2004/02/16 22:45:37 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)






$Log: cmdline.c,v $
Revision 1.26  2004/02/16 22:45:37  rasc
small bugfix: crc is 32 bit

Revision 1.25  2004/02/15 22:22:28  rasc
cmd option: -hexdumpbuffer -nohexdumpbuffer

Revision 1.24  2004/01/29 22:34:49  rasc
-sync: default now

Revision 1.23  2004/01/22 22:26:35  rasc
pes_pack_header
section read timeout

Revision 1.22  2004/01/06 14:06:09  rasc
no message

Revision 1.21  2004/01/03 15:40:47  rasc
simple frontend signal status query added "-s signal"

Revision 1.20  2004/01/01 20:09:26  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.19  2003/12/28 22:53:41  rasc
some minor changes/cleanup

Revision 1.18  2003/12/28 14:00:26  rasc
bugfix: section read from input file
some changes on packet header output

Revision 1.17  2003/12/28 00:01:14  rasc
some minor changes/adds...

Revision 1.16  2003/12/17 23:57:29  rasc
add. hexdump mode, different layout for some purpose
option:  -ph 4

Revision 1.15  2003/12/15 22:29:27  rasc
pidscan improved, problems with max filters on demux

Revision 1.14  2003/12/15 20:09:49  rasc
no message

Revision 1.13  2003/12/14 23:38:46  rasc
- bandwidth reporting for a PID

Revision 1.12  2003/12/10 20:07:15  rasc
minor stuff

Revision 1.11  2003/12/09 21:02:31  rasc
transponder pid-scan improved (should be sufficient now)

Revision 1.10  2003/12/07 23:36:13  rasc
pidscan on transponder
- experimental(!)

Revision 1.9  2003/12/03 20:06:35  obi
- reduced auto* to minimal required checks, obsoletes acinclude.m4
- added version number to configure.ac, removed it from version.h
  (needed for "make dist" anyway)
- removed autoheader dependency

Revision 1.8  2003/11/26 16:27:46  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.7  2003/11/24 23:52:17  rasc
-sync option, some TS and PES stuff;
dsm_addr inactive, may be wrong - due to missing ISO 13818-6

Revision 1.6  2003/11/01 17:05:46  rasc
no message

Revision 1.5  2003/10/16 20:45:47  rasc
no message

Revision 1.4  2003/10/16 19:02:27  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.3  2003/07/06 05:28:52  obi
compatibility stuff.. now there is only one version for old and new drivers
which selects the api at configure time

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "dvb_api/dvb_api.h"
#include "cmdline.h"



static void title (void);
static void usage (void);





/*
  -- set default options and decode cmdline
  -- return 0=abort, 1= ok (&opt set)

*/

int  cmdline_options (int argc, char **argv, OPTION *opt)

{
  char  *s;
  int   i;

  /*
    -- init options
  */

  opt->buffer_hexdump = 1;
  opt->printhex = -1;
  opt->printdecode = -1;
  opt->binary_out = 0;
  opt->inpPidFile = (char *) NULL;
  opt->devDemux = DEMUX_DEVICE;
  opt->devDvr = DVR_DEVICE;
  opt->devFE = FRONTEND_DEVICE;
  opt->pid = INVALID_PID;
  opt->filter = 0;
  opt->mask = 0;
  opt->timeout_ms = 0;		// no timeout (0) or default timeout in ms (SECTIONS)
  opt->crc = 0;
  opt->packet_count = 0;
  opt->packet_header_sync = 1;
  opt->packet_mode = SECT;
  opt->time_mode = FULL_TIME;
  opt->hide_copyright= 0;
  opt->help = 0;



  /*
   -- Simple parse of cmdline
  */

  i = 0;
  while (++i < argc) {

     if (!strcmp (argv[i],"-demux")) opt->devDemux = argv[++i];
     else if (!strcmp (argv[i],"-dvr")) opt->devDvr = argv[++i];
     else if (!strcmp (argv[i],"-frontend")) opt->devFE = argv[++i];
     else if (!strcmp (argv[i],"-f")) opt->filter = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-m")) opt->mask = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-crc")) opt->crc = 1;
     else if (!strcmp (argv[i],"-nocrc")) opt->crc = 0;
     else if (!strcmp (argv[i],"-sync")) opt->packet_header_sync = 1;
     else if (!strcmp (argv[i],"-nosync")) opt->packet_header_sync = 0;
     else if (!strcmp (argv[i],"-n")) opt->packet_count = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-b")) opt->binary_out = 1;
     else if (!strcmp (argv[i],"-ph")) opt->printhex = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-pd")) opt->printdecode = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-npd")) opt->printdecode = 0;
     else if (!strcmp (argv[i],"-HCP")) opt->hide_copyright= 1;
     else if (!strcmp (argv[i],"-TIMEOUT")) opt->timeout_ms = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-tf")) opt->time_mode = FULL_TIME;
     else if (!strcmp (argv[i],"-td")) opt->time_mode = DELTA_TIME;
     else if (!strcmp (argv[i],"-tn")) opt->time_mode = NO_TIME;
     else if (!strcmp (argv[i],"-hexdumpbuffer")) opt->buffer_hexdump = 1;
     else if (!strcmp (argv[i],"-nohexdumpbuffer")) opt->buffer_hexdump = 0;
     else if (!strcmp (argv[i],"-help")) opt->help = 1;
     else if (!strcmp (argv[i],"-nph")) {	// old option  use -ph and -nhdb/-hdb
	opt->buffer_hexdump = 0;
	opt->printhex = 0;
     } else if (!strcmp (argv[i],"-if")) {
	 opt->inpPidFile = argv[++i];		// input filename
	 opt->pid = DUMMY_PID;			// dummy to avoid usage output
     } else if (!strcmp (argv[i],"-s")) {
         s = argv[++i];
         if (!strcmp (s,"sec")) opt->packet_mode = SECT;
         else if (!strcmp (s,"ts")) opt->packet_mode = TS;
         else if (!strcmp (s,"pes")) opt->packet_mode = PES;
         else if (!strcmp (s,"bandwidth")) opt->packet_mode = PIDBANDWIDTH;
         else if (!strcmp (s,"pidscan")) {
		 	opt->packet_mode = PIDSCAN;
			opt->pid = DUMMY_PID;	// dummy to avoid usage output
	 } else if (!strcmp (s,"signal")) {
		 	opt->packet_mode = SIGNALSCAN;
			opt->pid = DUMMY_PID;	// dummy to avoid usage output
	 } else opt->help = 1;
     } else if (isdigit (argv[i][0])) {
         opt->pid = str2i(argv[i]); 	// PID
	 if (opt->pid > MAX_PID) opt->help = 1;
     } else {
         opt->help = 1;
         break;
     }

  } // while


  /*
   -- standard if no print decode or printhex given
  */

  if (opt->printhex    < 0) opt->printhex = 1;
  if (opt->printdecode < 0) opt->printdecode = 7;


  /*
   -- help ?  (and return abort)
  */

  if (opt->help) {
    usage ();
    return(0); 
  } 

  if ((argc==1) || ((opt->pid > MAX_PID) && (opt->pid != DUMMY_PID)) ) {
    title ();
    printf("For help type 'dvbsnoop -help' ...\n");
    return(0); 
  } 


 return 1;
}






static void title (void)
{
    printf("dvbsnoop  - a dvb/mpeg2 stream analyzer tool\n");
    printf("Version: %s/api-%d  (%s %s)\n",
		    DVBSNOOP_VERSION,DVB_API_VERSION,__DATE__,__TIME__);
    printf("         %s  \n",DVBSNOOP_URL);
    printf("         %s  \n",DVBSNOOP_COPYRIGHT);
    printf("\n");
}



static void usage (void)
{
    title ();

    printf("Usage:   dvbsnoop [opts] pid \n");
    printf("Options:  \n");
    printf("   -demux device: demux device [%s]\n",DEMUX_DEVICE);
    printf("   -dvr device:   dvr device [%s]\n",DVR_DEVICE);
    printf("   -frontend device: frontend   device [%s]\n",FRONTEND_DEVICE);
    printf("   -s [type]:    snoop type  [-s sec]\n");
    printf("                   type: stream type (sec, pes or ts),\n");
    printf("                   or special scan type:\n");
    printf("                         pidscan = transponder pid scan,\n");
    printf("                         bandwidth = data rate statistics for pid\n");
    printf("                         signal = signal rate statistics \n");
    printf("                 stream type or pidscan\n");
    printf("   -TIMEOUT ms:  section read timeout in ms [-TIMEOUT 0]\n");
    printf("   -f filter:    filtervalue for 'sec' demux [-f 0]\n");
    printf("   -f maxdmx:    max demux filters to use in pidscan mode\n");
    printf("   -m mask:      maskvalue for 'sec' demux [-m 0]\n");
    printf("   -crc:         CRC check when reading 'sec' [-nocrc]\n");
    printf("   -nocrc:       No CRC check when reading 'sec' [-nocrc]\n");
    printf("   -sync:        Simple packet header sync when reading 'ts' or 'pes' [-snyc]\n");
    printf("   -nosync:      No header sync when reading 'ts' or 'pes' [-snyc]\n");
    printf("   -n count:     receive count packets (0=no limit) [-n 0]\n");
    printf("   -b:           binary output of packets (disables other output)\n");
    printf("   -if:          input file, reads from binary file instead of demux device\n");
    printf("   -ph mode:     data hex dump mode, modes: [-ph 1]\n");
    printf("                   0=none, 1=hexdump, 2=hex line 3=ascii line 4=hexdump2\n");
    printf("   -nph:         don't print hex dump of buffer [= -nohexdumpbuffer -ph 0]\n");
    printf("   -hexdumpbuffer:    print hex dump of read buffer [-hexdumpbuffer]\n");
    printf("   -nohexdumpbuffer: don't print hex dump of read buffer [-hexdumpbuffer]\n");
    printf("   -pd verbose:  print stream decode (verbose level 0..9) [-pd 7]\n");
    printf("   -npd:         don't print decoded stream (= -pd 0) \n");
    printf("   -t[n|d|f]:    print timestamp (no, delta, full) [-tf] \n");
    printf("   -HCP:         hide copyright and program info header at program start\n");
    printf("   -help:        this usage info...\n");
    printf("\n");
    

 return;
}




// $$$ TODO  commandline needs a redesign 
