/*

 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de

*/



u_long outBit_Sx (int verbosity, char *text, u_char *buf, int startbit, int bitlen);
u_long outBit_Sx_NL (int verbosity, char *text, u_char *buf, int startbit, int bitlen);
u_long outBit_S2x_NL (int verbosity, char *text, u_char *buf, int startbit, int bitlen, char *(*f)(u_int) );


u_long getBits (u_char *buf, int byte_offset, int startbit, int bitlen);
u_char *getISO639_3 (u_char *str, u_char *buf);

void print_name (int verbose, u_char *b, u_int len);
void print_name2 (int verbose, u_char *b, u_int len);
void print_time40 (int verbose, u_long mjd, u_long utc);

long str2i (char *s);


