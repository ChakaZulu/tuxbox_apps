/*
$Id: helper.c,v 1.14 2003/12/27 14:35:01 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de




$Log: helper.c,v $
Revision 1.14  2003/12/27 14:35:01  rasc
dvb-t descriptors
DSM-CC: SSU Linkage/DataBroadcast descriptors

Revision 1.13  2003/11/26 23:54:48  rasc
-- bugfixes on Linkage descriptor

Revision 1.12  2003/11/26 16:27:46  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.11  2003/10/24 22:17:19  rasc
code reorg...

Revision 1.10  2003/10/16 19:02:29  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.9  2003/06/24 23:51:03  rasc
bugfixes and enhancements

Revision 1.8  2003/02/26 16:45:16  obi
- make dvbsnoop work on little endian machines again
- fixed mask in getBits for bitlen >= 32

Revision 1.7  2003/02/09 23:11:07  rasc
no message

Revision 1.6  2003/02/09 23:02:47  rasc
-- endian check (bug fix)

Revision 1.5  2003/02/09 23:01:10  rasc
-- endian check (bug fix)

Revision 1.4  2003/02/09 22:59:33  rasc
-- endian check (bug fix)

Revision 1.3  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "helper.h"
#include "output.h"
#include "strings/dvb_str.h"






/*
  -- output bits
  -- get bits and print decode
  -- return: (unsigned long) value
 */

u_long outBit_Sx (int verbosity, char *text, u_char *buf, int startbit, int bitlen)

{
   u_long value;

   value =  getBits(buf,0,startbit,bitlen);

   if (bitlen <= 8) {
	 out_SB (verbosity,text,(int)value);
   } else if (bitlen <= 16) {
	 out_SW (verbosity,text,(int)value);
   } else if (bitlen <= 24) {
	 out_ST (verbosity,text,(int)value);
   } else {
	 out_SL (verbosity,text,value);
   }

   return value;
}


u_long outBit_Sx_NL (int verbosity, char *text, u_char *buf, int startbit, int bitlen)

{
  u_long value;

  value = outBit_Sx (verbosity,text,buf,startbit,bitlen);
  out_NL (verbosity);
  return value;

}




u_long outBit_S2x_NL (int verbosity, char *text, u_char *buf, int startbit, int bitlen, char *(*f)(u_long) )

{
   u_long value;

   value =  getBits(buf,0,startbit,bitlen);

   if (bitlen <= 8) {
	 out_S2B_NL (verbosity,text,(int)value, (*f)(value));
   } else if (bitlen <= 16) {
	 out_S2W_NL (verbosity,text,(int)value, (*f)(value));
   } else if (bitlen <= 24) {
	 out_S2T_NL (verbosity,text,(int)value, (*f)(value));
   } else {
	 out_S2L_NL (verbosity,text,     value, (*f)(value));
   }

   return value;
}




/*
  -----------------------------------------------------------------------------------
 */


/* 
  -- get bits out of buffer
  -- (getting more than 24 bits is not save)
  -- return: value
*/

unsigned long getBits (u_char *buf, int byte_offset, int startbit, int bitlen)

{
 u_char *b;
 unsigned long  v;
 unsigned long mask;
 unsigned long tmp_long;

 b = &buf[byte_offset + (startbit / 8)];
 startbit %= 8;

 tmp_long = (unsigned long)( ((*b)<<24) + (*(b+1)<<16) +
		 (*(b+2)<<8) + *(b+3) );

 startbit = 32 - startbit - bitlen;

 tmp_long = tmp_long >> startbit;

 // ja, das ULL muss so sein (fuer bitlen == 32 z.b.)...
 mask = (1ULL << bitlen) - 1;

 v = tmp_long & mask;

 return v;
}



/*
  -- get ISO 639  (3char) language code into string[4]
  -- terminate string with \0
  -- return ptr to buf;
 */

u_char *getISO639_3 (u_char *str, u_char *buf)

{
  int i;

  strncpy (str, buf, 3);
  *(str+3) = '\0';

  // secure print of string
  for (i=0; i<3; i++) {
     if (!isprint(*(str+i))) {
	     *(str+i) = '.';
     }
  }
  
  return str;
}




/*
  -- print_name_Short 
  -- print_name_Long
  -- ETSI EN 300 468  Annex A

  -- evaluate string and look on DVB control codes
  -- print the string

*/

void print_name (int v, u_char *b, u_int len)

{
 //int i;

 if (len <= 0) {
    out (v,"\"\"");
 } else {
    out (v,"\"");
    print_name2 (v, b,len);
    out (v,"\"");
    out (v,"  -- Charset: %s", dvbstrTextCharset_TYPE (*b));
 }

}



void print_name2 (int v, u_char *b, u_int len)

{
  int    in_emphasis = 0;
  int    i;
  u_char c;
  u_char em_ON  = 0x86;
  u_char em_OFF = 0x87;

 
  for (i=0; i<len; i++) {
    c = b[i];

    if (i == 0 && c < 0x20) continue;   // opt. charset descript.

    if (c == em_ON) {
       in_emphasis = 1;
       out (v,"<EM>");
       continue;
    }
    if (c == em_OFF) {
       in_emphasis = 0;
       out (v,"</EM>");
       continue;
    }

       if (c == 0x8A)     out (v, "<BR>");
       else if (c < 0x20) out (v, ".");
       else               out (v, "%c", c);

  } // for
  

}





/*
 -- print time   40 bit
 --   16 Bit  MJD,  24 Bit UTC
*/ 

void print_time40 (int v, u_long mjd, u_long utc)

{

 out (v, "0x%lx%06lx (=",mjd, utc);

 if (mjd > 0) {
   long   y,m,d ,k;

   // algo: ETSI EN 300 468 - ANNEX C

   y =  (long) ((mjd  - 15078.2) / 365.25);
   m =  (long) ((mjd - 14956.1 - (long)(y * 365.25) ) / 30.6001);
   d =  (long) (mjd - 14956 - (long)(y * 365.25) - (long)(m * 30.6001));
   k =  (m == 14 || m == 15) ? 1 : 0;
   y = y + k + 1900;
   m = m - 1 - k*12;

   out (v, "%02d-%02d-%02d",y,m,d);
 }

 out (v, " %02lx:%02lx:%02lx [UTC])",
	 (utc>>16) &0xFF, (utc>>8) &0xFF, (utc) &0xFF);

}



/*
   -- str2i
   -- string to integer
   --   x, 0x ist Hex
   --   ansonsten Dezimal
   return:  long int

*/

long  str2i  (char *s)

{
 long v;
  
 v = strtol (s, NULL, 0);
 return v;

}





/*
 -- print latitude coordinates   (Cell Descriptors)
 -- print longitude coordinates   (Cell Descriptors)
 -- ETSI EN 300 468
*/ 

static char *_str_cell_latitude_longitude (long ll, int angle);

char *str_cell_latitude (long latitude)

{
 // cell_latitude: This 16-bit field, coded as a two's complement number,
 // shall specify the latitude of the corner of a spherical rectangle that
 // approximately describes the coverage area of the cell indicated. It shall
 // be calculated by multiplying the value of the latitude field by
 // (90 ° /2^15 ). Southern latitudes shall be considered negative and
 // northern latitudes positive.

 return _str_cell_latitude_longitude (latitude, 90);
}


char *str_cell_longitude (long longitude)

{
 // cell_longitude: This 16-bit field, coded as a two's complement number, shall
 // specify the longitude of the corner of a spherical rectangle that approximately
 // describes the coverage area of the cell indicated. It shall be calculated by
 // multiplying the value of the longitude field by (180 ° /2^15 ). Western
 // longitudes shall be considered negative and eastern longitudes positive.

 return _str_cell_latitude_longitude (longitude, 180);
}


static char *_str_cell_latitude_longitude (long ll, int angle)

{
 long long  x;
 long       g1,g2;
 static     char s[40];	// $$$ not thread safe!

 x = (long long) ll * angle * 1000;
 x = x / (2<<15);

 g1 = x / 1000;
 g2 = x % 1000;
 if (g2 <0) g2 = 0 - g2;

 sprintf (s,"%ld.%04ld grad",g1,g2);
 return s;
}




