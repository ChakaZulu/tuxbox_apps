/*


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)



*/


#ifndef __HELPER_H
#define __HELPER_H 1


u_long outBit_Sx (int verbosity, const char *text, u_char *buf, int startbit, int bitlen);
u_long outBit_Sx_NL (int verbosity, const char *text, u_char *buf, int startbit, int bitlen);
u_long outBit_S2x_NL (int verbosity, const char *text, u_char *buf, int startbit, int bitlen, char *(*f)(u_long) );


u_long getBits (u_char *buf, int byte_offset, int startbit, int bitlen);
long long getBits48 (u_char *buf, int byte_offset, int startbit, int bitlen);

u_char *getISO639_3 (u_char *str, u_char *buf);

void print_name (int verbose, u_char *b, u_int len);
void print_name2 (int verbose, u_char *b, u_int len);
void print_std_ascii (int v, u_char *b, u_int len);
void print_time40 (int verbose, u_long mjd, u_long utc);
void print_databytes (int verbose, const char *str, u_char *b, u_int len);
void print_private_data (int verbose, u_char *b, u_int len);

long str2i (char *s);

char *str_cell_latitude (long latitude);
char *str_cell_longitude (long longitude);



struct IPv6ADDR {
	u_long  ip[4];		// 32 bit: ip[0] ip[1] ip[2] ip[3]
};

void displ_mac_addr (int v, long mac_H24, long mac_L24);
void displ_IPv4_addr (int v, u_long ip);
void displ_IPv6_addr (int v, struct IPv6ADDR *ip);



#endif


