/*
$Id: cmdline.c,v 1.12 2003/12/10 20:07:15 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de




$Log: cmdline.c,v $
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
#include "cmdline.h"








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

  opt->printhex = -1;
  opt->printdecode = -1;
  opt->binary_out = 0;
  opt->inpPidFile = (char *) NULL;
  opt->help = 0;
  opt->devDemux = DEMUX_DEVICE;
  opt->devDvr = DVR_DEVICE;
  opt->pid = 0xFFFF;		/* invaild PID */
  opt->filter = 0;
  opt->mask = 0;
  opt->crc = 0;
  opt->packet_count = 0;
  opt->packet_header_sync = 0;
  opt->packet_mode = SECT;
  opt->time_mode = FULL_TIME;
  opt->hide_copyright= 0;



  /*
   -- Simple parse of cmdline
  */

  i = 0;
  while (++i < argc) {

     if (!strcmp (argv[i],"-demux")) opt->devDemux = argv[++i];
     else if (!strcmp (argv[i],"-dvr")) opt->devDvr = argv[++i];
     else if (!strcmp (argv[i],"-if")) opt->inpPidFile = argv[++i];
     else if (!strcmp (argv[i],"-f")) opt->filter = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-m")) opt->mask = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-crc")) opt->crc = 1;
     else if (!strcmp (argv[i],"-nocrc")) opt->crc = 0;
     else if (!strcmp (argv[i],"-sync")) opt->packet_header_sync = 1;
     else if (!strcmp (argv[i],"-nosync")) opt->packet_header_sync = 0;
     else if (!strcmp (argv[i],"-n")) opt->packet_count = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-b")) opt->binary_out = 1;
     else if (!strcmp (argv[i],"-ph")) opt->printhex = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-nph")) opt->printhex = 0;
     else if (!strcmp (argv[i],"-pd")) opt->printdecode = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-npd")) opt->printdecode = 0;
     else if (!strcmp (argv[i],"-HCP")) opt->hide_copyright= 1;
     else if (!strcmp (argv[i],"-tf")) opt->time_mode = FULL_TIME;
     else if (!strcmp (argv[i],"-td")) opt->time_mode = DELTA_TIME;
     else if (!strcmp (argv[i],"-tn")) opt->time_mode = NO_TIME;
     else if (!strcmp (argv[i],"-s")) {
         s = argv[++i];
         if (!strcmp (s,"sec")) opt->packet_mode = SECT;
         else if (!strcmp (s,"ts")) opt->packet_mode = TS;
         else if (!strcmp (s,"pes")) opt->packet_mode = PES;
         else if (!strcmp (s,"pidscan")) {
		 	opt->packet_mode = PIDSCAN;
			opt->pid = 0;	// dummy to avoid usage output
	 } else opt->help = 1;
     } else if (isdigit (argv[i][0])) {
       /* PID */   
       opt->pid = str2i(argv[i]);
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

  if (argc==1 || opt->help || opt->pid > 0x1FFF) {
    usage ();
    return(0); 
  } 


 return 1;
}



void usage (void)
{

    printf("dvbsnoop  - a dvb/mpeg2 stream analyzer tool\n");
    printf("Version: %s  (%s %s)\n",DVBSNOOP_VERSION,__DATE__,__TIME__);
    printf("         %s  \n",DVBSNOOP_URL);
    printf("         %s  \n",DVBSNOOP_COPYRIGHT);
    printf("\n");
    printf("This programm was done for learning and understanding\n");
    printf("DVB/Mpeg2 streams... Tnx to all the guys of the linux-dbox2\n");
    printf("project for their help.  Please report errors!\n");
    printf("\n");
    printf("Usage\n");
    printf(" dvbsnoop [opts] pid \n\n");
    printf(" Options:  \n");
    printf("   -demux device:      demux device [%s]\n",DEMUX_DEVICE);
    printf("   -dvr device:        dvr device [%s]\n",DVR_DEVICE);
    printf("   -s [sec|ts|pes|pidscan]:  snoop type  [-s sec]\n");
    printf("                 stream type or pidscan\n");
    printf("   -f filter:    filtervalue for 'sec' demux [-f 0]\n");
    printf("   -m mask:      maskvalue for 'sec' demux [-m 0]\n");
    printf("   -crc:         CRC check when reading 'sec'\n");
    printf("   -nocrc:       No CRC check when reading 'sec' [-nocrc]\n");
    printf("   -sync:        Simple packet header sync when reading 'ts' or 'pes' [-nosnyc]\n");
    printf("   -nosync:      No header sync when reading 'ts' or 'pes' [-nosnyc]\n");
    printf("   -n count:     receive count packets (0=no limit) [-n 0]\n");
    printf("   -b:           binary output of packets (disables other output)\n");
    printf("   -if:          input file, reads from binary file instead of demux device\n");
    printf("   -ph mode:     print hex buffer, modes: [-ph 1]\n");
    printf("                   0=none, 1=hexdump, 2=hex line 3=ascii line\n");
    printf("   -nph:         don't print hex buffer (= -ph 0)\n");
    printf("   -pd verbose:  print stream decode (verbose level 0..9) [-pd 7]\n");
    printf("   -npd:         don't print decoded stream (= -pd 0) \n");
    printf("   -t[n|d|f]:    print timestamp (no, delta, full) [-tf] \n");
    printf("   -HCP:         hide copyright and program info header at program start\n");
    printf("\n");
    

 return;
}



