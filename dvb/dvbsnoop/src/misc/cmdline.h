/*

 -- (c) 2001 rasc


$Id: cmdline.h,v 1.5 2003/11/24 23:52:17 rasc Exp $


*/




/*
 -- defs...
*/

enum PACKET_MODE  {SECT, TS, PES};
enum TIME_MODE    {NO_TIME, FULL_TIME, DELTA_TIME};

typedef struct _OPTIONS {
  int         packet_mode;
  int         packet_header_sync;	// Try to do a softsync of packet sync bytes
  int         printhex;
  int         printdecode;
  int         binary_out;
  char        *inpPidFile;		// read from file instead of dmux if not NULL
  int         help;
  char        *devDemux;
  char        *devDvr;
  u_int       pid;
  u_int       filter;
  u_int       mask;
  int         crc;
  long        packet_count;
  int         time_mode;
  int         hide_copyright;  // suppress message at prog start
} OPTION;


/*
 -- prototypes
*/

int  cmdline_options (int argc, char **argv, OPTION *opt);




