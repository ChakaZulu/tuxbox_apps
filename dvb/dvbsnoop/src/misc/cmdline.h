/*
$Id: cmdline.h,v 1.22 2004/11/03 21:00:59 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/



#ifndef __CMDLINE_H
#define __CMDLINE_H 


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
  long        rd_buffer_size;		// read buffer size in (0L = default)
  u_int       pid;			// decode PID
  u_int       filter;			// PID filter
  u_int       mask;			// PID mask
  int         crc;			// section CRC check?
  int         max_dmx_filter;		// max dmx filter use? (pidscan)
  long        timeout_ms;		// read timeout in ms
  long        rd_packet_count;		// read max. n packets
  long        dec_packet_count;		// decode max. n packets
  int         spider_pid;		// Section PID spider mode
  int         ts_subdecode;		// sub decode PES or SEC in TS stream
  int         time_mode;		// time print mode
  char        *privateProviderStr;	// Private Provider ID str (usedef tables, descr)
  int         hide_copyright;  		// suppress message at prog start
  int         help;
} OPTION;


/*
 -- prototypes
*/

int  cmdline_options (int argc, char **argv, OPTION *opt);

#endif


