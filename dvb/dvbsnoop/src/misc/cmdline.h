/*

 -- (c) 2001 rasc


$Id: cmdline.h,v 1.4 2003/11/01 17:05:46 rasc Exp $


*/




/*
 -- defs...
*/

enum PACKET_MODE  {SECT, TS, PES};
enum TIME_MODE    {NO_TIME, FULL_TIME, DELTA_TIME};

typedef struct _OPTIONS {
  int         packet_mode;
  int         printhex;
  int         printdecode;
  int         binary_out;
  char        *inpPidFile;	// read from file instead of dmux if not NULL
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




