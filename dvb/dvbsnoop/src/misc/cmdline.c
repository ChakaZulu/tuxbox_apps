/*
$Id: cmdline.c,v 1.3 2003/07/06 05:28:52 obi Exp $

 -- (c) 2001 rasc


$Log: cmdline.c,v $
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
  opt->help = 0;
  opt->devDemux = DEMUX_DEVICE;
  opt->devDvr = DVR_DEVICE;
  opt->pid = 0xFFFF;		/* invaild PID */
  opt->filter = 0;
  opt->crc = 0;
  opt->packet_count = 0;
  opt->mask = 0;
  opt->packet_mode = SECT;
  opt->time_mode = FULL_TIME;



  /*
   -- Simple parse of cmdline
  */

  i = 0;
  while (++i < argc) {

     if (!strcmp (argv[i],"-demux")) opt->devDemux = argv[++i];
     else if (!strcmp (argv[i],"-dvr")) opt->devDvr = argv[++i];
     else if (!strcmp (argv[i],"-f")) opt->filter = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-m")) opt->mask = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-crc")) opt->crc = 1;
     else if (!strcmp (argv[i],"-nocrc")) opt->crc = 0;
     else if (!strcmp (argv[i],"-n")) opt->packet_count = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-b")) opt->binary_out = 1;
     else if (!strcmp (argv[i],"-ph")) opt->printhex = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-nph")) opt->printhex = 0;
     else if (!strcmp (argv[i],"-pd")) opt->printdecode = str2i(argv[++i]);
     else if (!strcmp (argv[i],"-npd")) opt->printdecode = 0;
     else if (!strcmp (argv[i],"-tf")) opt->time_mode = FULL_TIME;
     else if (!strcmp (argv[i],"-td")) opt->time_mode = DELTA_TIME;
     else if (!strcmp (argv[i],"-tn")) opt->time_mode = NO_TIME;
     else if (!strcmp (argv[i],"-s")) {
         s = argv[++i];
         if (!strcmp (s,"sec")) opt->packet_mode = SECT;
         else if (!strcmp (s,"ts")) opt->packet_mode = TS;
         else if (!strcmp (s,"pes")) opt->packet_mode = PES;
         else opt->help = 1;
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

  if (argc==1 || opt->help || opt->pid == 0xFFFF) {
    printf("dvbsnoop [opt] pid \n\n");
    printf("Version: %s  (%s %s)\n",DVBSNOOP_VERSION,__DATE__,__TIME__);
    printf("\nThis programm was done for learning and understanding\n");
    printf("DVB, the streams and the coordination between the streams...\n");
    printf("Tnx to all the guys of the linux-dbox2 project for their help\n");
    printf("Please report errors!\n");
    printf(" (c) 2001 rasc ...\n");
    printf("     Use on your own risk - no warrenty in any way given.\n");
    printf("     Miss-use is prohibited...\n");
    printf("     \n");
    printf(" Options:  \n");
    printf("   -demux device:      demux device [%s]\n",DEMUX_DEVICE);
    printf("   -dvr device:        dvr device [%s]\n",DVR_DEVICE);
    printf("   -s [sec|ts|pes]:    stream type [-s sec]\n");
    printf("   -f filter:    filtervalue for 'sec' demux [-f 0]\n");
    printf("   -m mask:      maskvalue for 'sec' demux [-m 0]\n");
    printf("   -crc:         CRC check when reading 'sec'\n");
    printf("   -nocrc:       No CRC check when reading 'sec' [-nocrc]\n");
    printf("   -n count:     receive count packets (0=no limit) [-n 0]\n");
    printf("   -b:           binary output of packets (disables other output)\n");
    printf("   -ph mode:     print hex buffer, modes: [-ph 1]\n");
    printf("                   0=none, 1=hexdump, 2=hex line 3=ascii line\n");
    printf("   -nph:         don't print hex buffer (= -ph 0)\n");
    printf("   -pd verbose:  print stream decode (verbose level 0..9) [-pd 7]\n");
    printf("   -npd:         don't print decoded stream (= -pd 0) \n");
    printf("   -t[n|d|f]:    print timestamp (no, delta, full) [-tf] \n");
    printf("     \n");
    
    return(0);   /* fail */
  } 


 return 1;
}


