/*
$Id: pes_misc.c,v 1.1 2004/01/11 21:01:32 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)



 -- PES misc.



$Log: pes_misc.c,v $
Revision 1.1  2004/01/11 21:01:32  rasc
PES stream directory, PES restructured




*/




#include "dvbsnoop.h"
#include "pes_misc.h"
#include "misc/hexprint.h"
#include "misc/output.h"




/*
 *  PTS //  DTS
 *  Len is 36 bits fixed
 */

void  print_xTS_field (int v, const char *str, u_char *b, int bit_offset) 
{
  u_long   xTS_32_30;
  u_long   xTS_29_15;
  u_long   xTS_14_0;
  u_long   ul;
  int      bo = bit_offset;
  int      v1 = v+1;


  out_nl (v,"%s:",str);
  indent (+1);

    xTS_32_30 = outBit_Sx_NL (v1,"bit[32..30]: ",	b+bo,  0, 3);
                outBit_Sx_NL (v1,"marker_bit: ",	b+bo,  3, 1);
    xTS_29_15 = outBit_Sx_NL (v1,"bit[29..15]: ",	b+bo,  4,15);
                outBit_Sx_NL (v1,"marker_bit: ",	b+bo, 19, 1);
    xTS_14_0  = outBit_Sx_NL (v1,"bit[14..0]: ",	b+bo, 20,15);
                outBit_Sx_NL (v1,"marker_bit: ",	b+bo, 35, 1);

    ul = (xTS_32_30<<30) + (xTS_29_15<<15) + xTS_14_0;
    out_nl (v," ==> %s: %s%lu (0x%08lx) [cycles of the 90 kHz system clock]",
		str, ul,ul);

  indent (-1);
}






void pack_header (u_char *b, int len)
{

	/*  ... $$$ TODO   */
	/* z.B. H.222 ISO 13818-1 Table 2-33 */
	/* 	ISO 11172-1 pack header */

   printhexdump_buf (4, b, len);

}


// $$$ TODO  system_header()
// $$$ TODO  Program Streams   (START/END




