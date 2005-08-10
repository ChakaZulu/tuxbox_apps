/*
$Id: pes_misc.c,v 1.9 2005/08/10 21:28:18 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2005   Rainer.Scherg@gmx.de  (rasc)



 -- PS misc.
 -- PES misc.





$Log: pes_misc.c,v $
Revision 1.9  2005/08/10 21:28:18  rasc
New: Program Stream handling  (-s ps)

Revision 1.8  2004/08/12 22:57:18  rasc
 - New: MPEG Content Labeling descriptor  (H.222.0 AMD1)
 - New: PES update ITU-T H.222.0 AMD2
H.222.0 AMD3 updates started

Revision 1.7  2004/02/20 22:18:41  rasc
DII complete (hopefully)
BIOP::ModuleInfo  (damned, who is spreading infos over several standards???)
maybe someone give me a hint on the selector_byte info!!!
some minor changes...

Revision 1.6  2004/02/05 10:30:57  rasc
no message

Revision 1.5  2004/02/04 23:54:37  rasc
Bugfix:  PTS wrongly displayed!!!

Revision 1.4  2004/02/02 23:34:08  rasc
- output indent changed to avoid \r  (which sucks on logged output)
- EBU PES data started (teletext, vps, wss, ...)
- bugfix: PES synch. data stream
- some other stuff

Revision 1.3  2004/01/25 21:37:28  rasc
bugfixes, minor changes & enhancments

Revision 1.2  2004/01/22 22:26:35  rasc
pes_pack_header
section read timeout

Revision 1.1  2004/01/11 21:01:32  rasc
PES stream directory, PES restructured




*/




#include "dvbsnoop.h"
#include "pes_misc.h"
#include "strings/dvb_str.h"
#include "misc/hexprint.h"
#include "misc/helper.h"
#include "misc/output.h"




/*
 *  PTS //  DTS
 *  Len is 36 bits fixed
 */

void  print_xTS_field (int v, const char *str, u_char *b, int bit_offset) 
{
  long long   xTS_32_30;
  long long   xTS_29_15;
  long long   xTS_14_0;
  long long   ull;
  int         bo = bit_offset;
  int         v1 = v+1;


  out_nl (v,"%s:",str);
  indent (+1);

    xTS_32_30 = outBit_Sx_NL (v1,"bit[32..30]: ",	b, bo+0,  3);
                outBit_Sx_NL (v1,"marker_bit: ",	b, bo+3,  1);
    xTS_29_15 = outBit_Sx_NL (v1,"bit[29..15]: ",	b, bo+4, 15);
                outBit_Sx_NL (v1,"marker_bit: ",	b, bo+19, 1);
    xTS_14_0  = outBit_Sx_NL (v1,"bit[14..0]: ",	b, bo+20,15);
                outBit_Sx_NL (v1,"marker_bit: ",	b, bo+35, 1);

    ull = (xTS_32_30<<30) + (xTS_29_15<<15) + xTS_14_0;
    out (v," ==> %s: ", str);
    print_timebase90kHz (v, ull);
    out_NL (v);

  indent (-1);
}





/*
 *  PES/PS  Pack Header
 *  if (len == -1) then
 *        pack_header is in a PS
 *        do not print packet_start_code (already printed by caller)
 *  else
 *        pack_header is within a pes packet
 *        and check if a system_header is within len
 */


void pack_header (int v, u_char *b, int len)
{

	/* z.B. H.222 ISO 13818-1 Table 2-33 */
	/* 	ISO 11172-1 pack header */


   int 	v1 = v+1;
   int  pack_stuffing_len;


   if (len == 0) return;

   out_nl (v,"Pack_header: ");
   indent (+1);

   if (len > 0) {
	// -- within PES packet, not PS!
	outBit_Sx_NL (v1,"pack_start_code: ",	b,  0, 32);
   }

   outBit_Sx_NL (v1,"fixed '01': ",		b, 32,  2);
   print_xTS_field (v1, "system_clock_reference_base", 	b, 34) ;   // len 36b
   outBit_Sx_NL (v1,"system_clock_reference_extension: ",b, 70,  9);
   outBit_Sx_NL (v1,"marker_bit: ",		b, 79, 1);

   outBit_Sx   (v1,"program_mux_rate: ",	b, 80,22);
   	out_nl (v1,"  [=  x 50 bytes/sec]");

   outBit_Sx_NL (v1,"marker_bit: ",		b, 102, 1);
   outBit_Sx_NL (v1,"marker_bit: ",		b, 103, 1);
   outBit_Sx_NL (v1,"reserved: ",		b, 104, 5);

   pack_stuffing_len = outBit_Sx_NL (v1,"pack_stuffing_len: ",	b, 109, 3);
   print_databytes (6,"stuffing bytes 0xFF:", b+14, pack_stuffing_len);

   b += 14 + pack_stuffing_len;
   if (len >= 0) len -= 14 + pack_stuffing_len;

   if (len > 0) system_header (v1, b, len); 	// only if len > 0 (PES packet)

   indent (-1);
}




/*
 *  PS System header
 *  if (len == -1) then
 *        system_header is in a PS
 *        do not print packet_start_code & ID (already printed by caller)
 *  else
 *        pack_header is within a pes packet (pack_header)
 *        print packet_start_code
 */


void system_header (int v, u_char *b, int len)
{

	/* z.B. H.222 ISO 13818-1 Table 2-34 */
	/* 	ISO 11172-1 system header */


   if (len == 0) return;

   out_nl (v,"System_header: ");
   indent (+1);

   if (len > 0) {
	// -- within PES packet, not PS!
	outBit_Sx_NL (v,"system_header_start_code: ",	b,  0, 32);
   }



   // -- get real length from system_header data
   len = outBit_Sx_NL (v,"header_length: ",	b,  32, 16);

   outBit_Sx_NL (v,"marker_bit: ",		b,  48,  1);
   outBit_Sx_NL (v,"rate_bound: ",		b,  49,  22);
   outBit_Sx_NL (v,"marker_bit: ",		b,  71,  1);
   outBit_Sx_NL (v,"audio_bound: ",		b,  72,  6);
   outBit_Sx_NL (v,"fixed_flag: ",		b,  78,  1);
   outBit_Sx_NL (v,"CSPS_flag: ",		b,  79,  1);
   outBit_Sx_NL (v,"system_audio_lock_flag: ",	b,  80,  1);
   outBit_Sx_NL (v,"system_video_lock_flag: ",	b,  81,  1);
   outBit_Sx_NL (v,"marker_bit: ",		b,  82,  1);
   outBit_Sx_NL (v,"video_bound: ",		b,  83,  5);
   outBit_Sx_NL (v,"packet_rate_restriction_flag: ",	b,  88,  1);
   outBit_Sx_NL (v,"reserved_byte: ",		b,  89,  7);

   b += 12;
   len -= 12;


   // while (nextbits () == '1') {	
   // while  ((bit = getBits (b, 0,0,1)) == 0x01) {
   while  ((*b & 0x80) == 0x01) {
	if (len <= 0)  break;

	out_NL (v);
	outBit_S2x_NL(v,"Stream_id: ",			b,  0,  8,
		 	(char *(*)(u_long))dvbstrPESstream_ID );
   	outBit_Sx_NL (v,"fixed (0x02): ",		b,  8,  2);
   	outBit_Sx_NL (v,"P-STD_buffer_bound_scale: ",	b, 10,  1);
   	outBit_Sx_NL (v,"P-STD_buffer_size_bound: ",	b, 11, 13);

	b   += 3;
	len -= 3;
	if (len < 0)  out_nl (1, "$$$ something wrong here (length<0)");
   }


   indent (-1);
}


// $$$ TODO  Program Streams   (START/END




