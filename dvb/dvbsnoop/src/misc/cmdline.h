/*
$Id: cmdline.h,v 1.14 2004/01/03 15:40:47 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/



#ifndef __CMDLINE_H
#define __CMDLINE_H 1


/*
 -- defs...
*/

enum PACKET_MODE  {SECT, TS, PES, PIDSCAN, PIDBANDWIDTH, SIGNALSCAN};
enum TIME_MODE    {NO_TIME, FULL_TIME, DELTA_TIME};

typedef struct _OPTIONS {
  int         packet_mode;
  int         packet_header_sync;	// Try to do a softsync of packet sync bytes
  int         printhex;
  int         printdecode;
  int         binary_out;
  char        *inpPidFile;		// read from file instead of dmux if not NULL
  char        *devDemux;
  char        *devDvr;
  char        *devFE;
  u_int       pid;
  u_int       filter;
  u_int       mask;
  int         crc;
  // long        timeout_ms;		// read timeout in ms
  long        packet_count;
  int         time_mode;
  int         hide_copyright;  		// suppress message at prog start
  int         help;
} OPTION;


/*
 -- prototypes
*/

int  cmdline_options (int argc, char **argv, OPTION *opt);

#endif


