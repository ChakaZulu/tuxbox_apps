/*
$Id: cmdline.h,v 1.17 2004/03/21 00:37:47 rasc Exp $


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

enum PACKET_MODE  {SECT, TS, PES, PIDSCAN, PIDBANDWIDTH,
			SCAN_FE_SIGNAL, SCAN_FE_INFO};
enum TIME_MODE    {NO_TIME, FULL_TIME, DELTA_TIME};

typedef struct _OPTIONS {
  int         packet_mode;
  int         packet_header_sync;	// Try to do a softsync of packet sync bytes
  int         buffer_hexdump;		// 0/1: print hexdump prior to decoding
  int         printhex;			// hexdump mode
  int         printdecode;		// decode verbose mode
  int         binary_out;		// binary output?
  char        *inpPidFile;		// read from file instead of dmux if not NULL
  char        *devDemux;		// input device DMX
  char        *devDvr;			// input device DVR
  char        *devFE;			// input device Frontend
  u_int       pid;			// decode PID
  u_int       filter;			// PID filter
  u_int       mask;			// PID mask
  int         crc;			// section CRC check?
  long        timeout_ms;		// read timeout in ms
  long        packet_count;		// read n packets
  int         time_mode;		// time print mode
  int         hide_copyright;  		// suppress message at prog start
  int         help;
} OPTION;


/*
 -- prototypes
*/

int  cmdline_options (int argc, char **argv, OPTION *opt);

#endif


