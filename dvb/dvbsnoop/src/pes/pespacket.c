/*
$Id: pespacket.c,v 1.16 2004/01/01 20:09:29 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)



 -- PES Decode/Table section



$Log: pespacket.c,v $
Revision 1.16  2004/01/01 20:09:29  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.15  2003/12/27 14:35:01  rasc
dvb-t descriptors
DSM-CC: SSU Linkage/DataBroadcast descriptors

Revision 1.14  2003/12/17 23:15:04  rasc
PES DSM-CC  ack and control commands  according ITU H.222.0 Annex B

Revision 1.13  2003/11/26 19:55:33  rasc
no message

Revision 1.12  2003/11/26 16:27:47  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.11  2003/11/24 23:52:17  rasc
-sync option, some TS and PES stuff;
dsm_addr inactive, may be wrong - due to missing ISO 13818-6

Revision 1.10  2003/11/09 20:48:35  rasc
pes data packet (DSM-CC)

Revision 1.9  2003/11/01 17:05:47  rasc
no message

Revision 1.8  2003/10/29 22:39:18  rasc
pes packet complete now...

Revision 1.7  2003/10/29 20:54:56  rasc
more PES stuff, DSM descriptors, testdata

Revision 1.6  2003/10/24 22:17:20  rasc
code reorg...

Revision 1.5  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.4  2001/12/06 15:33:18  rasc
some small work on pespacket.c

Revision 1.3  2001/12/01 12:46:48  rasc
pespacket weitergestrickt, leider z.Zt. zuwenig Zeit um es richtig fertig zu machen.

Revision 1.2  2001/10/02 21:52:44  rasc
- init der time_delta
- PES erweitert, PES arbeitet im read() noch nicht richtig!!

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS



*/




#include "dvbsnoop.h"
#include "pespacket.h"
#include "pes_dsmcc.h"
#include "pes_psm.h"
#include "misc/hexprint.h"


void PES_decode2 (u_char *b, int len, int pid);
void print_xTS_field (u_char *b, int bit_offset);
void pack_header (u_char *b, int len);
void PES_data_packet (u_char *b, int len);





void decodePES_buf (u_char *b, u_int len, int pid)
{
 /* IS13818-1  2.4.3.6  */

 typedef struct  _PES_Packet {
	u_long	  packet_start_code_prefix;		// 24 bit
	u_int     stream_id;
	u_int     PES_packet_length;

	// N ... data

 } PES_Packet;




 PES_Packet   p;
 int          len2;
 int          n;


 p.packet_start_code_prefix		 = getBits (b, 0,  0, 24);
 p.stream_id				 = getBits (b, 0, 24,  8);
 p.PES_packet_length			 = getBits (b, 0, 32, 16);


 if (p.packet_start_code_prefix != 0x000001) {
    out_nl (3," !!! Packet_Start_CODE [%06lx] is wrong (= no PES)!!!\n",
		p.packet_start_code_prefix);
    // $$$    return;
 }
 if (len < 6) {
    out_nl (3," !!! Opps, read pes stream length too short (= no PES)!!!\n");
    return;
 }


 out_nl     (3,"Packet_start_code_prefix: %06lx",p.packet_start_code_prefix);
 out_S2B_NL (3,"Stream_id: ",p.stream_id,dvbstrPESstream_ID(p.stream_id));
 out_SW_NL  (3,"PES_packet_length: ",p.PES_packet_length);

 b   += 6;
 len -= 6;
 len2  = p.PES_packet_length;


 n = 0;
 switch (p.stream_id) {

	case 0xBC:		// program_stream_map
		PES_decodePSM (b, len2);
		break;

	case 0xBF:		// private_stream_2  (EN301192-1.3.1 S.10)
	case 0xF0:		// ECM
	case 0xF1:		// EMM
	case 0xF8:		// ITE-T Rec. H.222.1 type E
	case 0xFF:		// program_stream_directory
    		out_nl (4,"PES_packet_data_bytes:");
		printhexdump_buf (4, b, len2);
		n = len2;
		break;

	case 0xBD:		// Data Stream, privat_stream_1 (EN301192-1.3.1 S.11)
    		out_nl (3,"PES_data_packet:");
		indent (+1);
		PES_data_packet (b, len2);
		indent (-1);
		break;

	case 0xBE:		// padding stream!
    		out_nl (4,"Padding_bytes:");
		printhexdump_buf (4, b, len2);
		n = len2;
		break;

	case 0xF2:		// DSMCC stream
		PES_decodeDSMCC (b, len2);
		break;


	default:
    		out_nl (4,"Default PES decoding:");
		indent (+1);
		PES_decode2 (b, len2, pid);
		indent (-1);
		break;

 }


}



void  PES_decode2 (u_char *b, int len, int pid)

{
 /* IS13818-1  2.4.3.6  */

 typedef struct  _PES2_Packet {
        u_int     reserved1;
        u_int     PES_scrambling_control;
        u_int     PES_priority;
        u_int     data_alignment_indicator;
        u_int     copyright;
        u_int     original_or_copy;
        u_int     PTS_DTS_flags;
        u_int     ESCR_flag;
        u_int     ES_rate_flag;
        u_int     DSM_trick_mode_flag;
        u_int     additional_copy_info_flag;
        u_int     PES_CRC_flag;
        u_int     PES_extension_flag;
        u_int     PES_header_data_length;

	// N ... data

 } PES2_Packet;




 PES2_Packet  p;
 u_char       *b_start = b;


 p.reserved1				= getBits (b, 0,  0, 2);
 p.PES_scrambling_control		= getBits (b, 0,  2, 2);
 p.PES_priority				= getBits (b, 0,  4, 1);
 p.data_alignment_indicator		= getBits (b, 0,  5, 1);
 p.copyright				= getBits (b, 0,  6, 1);
 p.original_or_copy			= getBits (b, 0,  7, 1);
 p.PTS_DTS_flags			= getBits (b, 0,  8, 2);
 p.ESCR_flag				= getBits (b, 0, 10, 1);
 p.ES_rate_flag				= getBits (b, 0, 11, 1);
 p.DSM_trick_mode_flag			= getBits (b, 0, 12, 1);
 p.additional_copy_info_flag		= getBits (b, 0, 13, 1);
 p.PES_CRC_flag				= getBits (b, 0, 14, 1);
 p.PES_extension_flag			= getBits (b, 0, 15, 1);
 p.PES_header_data_length		= getBits (b, 0, 16, 8);



 out_SB_NL  (6,"reserved1: ",p.reserved1);
 out_S2B_NL (3,"PES_scrambling_control: ",p.PES_scrambling_control,
	dvbstrPESscrambling_ctrl_TYPE(p.PES_scrambling_control));
 out_SB_NL  (3,"PES_priority: ",p.PES_priority);
 out_SB_NL  (3,"data_alignment_indicator: ",p.data_alignment_indicator);
 out_SB_NL  (3,"copyright: ",p.copyright);
 out_SB_NL  (3,"original_or_copy: ",p.original_or_copy);
 out_SB_NL  (3,"PTS_DTS_flags: ",p.PTS_DTS_flags);
 out_SB_NL  (3,"ES_rate_flag: ",p.ES_rate_flag);
 out_SB_NL  (3,"additional_copy_info_flag: ",p.additional_copy_info_flag);
 out_SB_NL  (3,"PES_CRC_flag: ",p.PES_CRC_flag);
 out_SB_NL  (3,"PES_extension_flag: ",p.PES_extension_flag);
 out_SB_NL  (3,"PES_header_data_length: ",p.PES_header_data_length);

 b += 3;


 if (p.PTS_DTS_flags & 0x02) {    // 10
   out_nl (3,"PTS: ");
   indent (+1);
   outBit_Sx_NL (3,"Fixed: ",b,0,4);
   print_xTS_field (b, 4) ;
   indent (-1);
   b += 5;
 }

 if (p.PTS_DTS_flags == 0x03) {    // 10 + 01
   // PTS from "if" before...
   out_nl (3,"DTS: ");
   indent (+1);
   outBit_Sx_NL (3,"Fixed: ",b,0,4);
   print_xTS_field (b, 4) ;
   indent (-1);
   b += 5;
 }

 if (p.ESCR_flag == 0x01) {
   out_nl (3,"ESCR_flag: ");
   indent (+1);
   out_nl (3,"ESCR_base: ");
   outBit_Sx_NL (6,"reserved: ",      b,0,2);
   print_xTS_field (b, 2) ;
   outBit_Sx_NL (3,"ESCR_extension: ",b,38,9);
   outBit_Sx_NL (3,"marker_bit: ",    b,47,1);
   indent (-1);
   b += 6;
 }



 if (p.ES_rate_flag == 0x01) {
   out_nl (3,"ES_rate_flag: ");
   indent (+1);
   outBit_Sx_NL (3,"marker_bit: ",b, 0, 1);
   outBit_Sx_NL (3,"ES_rate: ",   b, 1,22);
   outBit_Sx_NL (3,"marker_bit: ",b,23, 1);
   indent (-1);
   b += 3;
 }

 if (p.DSM_trick_mode_flag == 0x01) {
   u_int  trick_mode_control;


   out_nl (3,"Trick_mode_control: ");
   indent (+1);

   trick_mode_control = outBit_S2x_NL  (3,"trick_mode_control: ", b,0,3,
		   (char *(*)(u_long)) dvbstrPESTrickModeControl);

   if ( (trick_mode_control == 0x0) ||			/* fast forward */
        (trick_mode_control == 0x3) ) {			/* fast reverse */

	   outBit_Sx_NL (3,"field_id: ",b, 3, 2);	/* $$$ TABLE ?? */
	   outBit_Sx_NL (3,"intra_slice_refresh: ",b, 5, 1);
	   outBit_Sx_NL (3,"frequency_truncation: ",b, 6, 2);

   } else if ( (trick_mode_control == 0x1) ||		/* slow motion  */
               (trick_mode_control == 0x4) ) {		/* slow reverse */

	   outBit_Sx_NL (3,"rep_control: ",b, 3, 5);

   } else if (trick_mode_control == 0x2) {		/* freeze frame */

	   outBit_Sx_NL (3,"field_id: ",b, 3, 2);	/* $$$ TABLE ?? */
	   outBit_Sx_NL (6,"reserved: ",b, 5, 3);

   } else {						/* reserved     */

	   outBit_Sx_NL (6,"reserved: ",b, 3, 8);

   }

   indent (-1);
   b += 1;

 }  /* p.DSM_trick_mode_flag  */


 if (p.additional_copy_info_flag == 0x01) {

   out_nl (3,"additional_copy_info: ");
   indent (+1);
   outBit_Sx_NL (3,"marker_bit: ",b, 0, 1);
   outBit_Sx_NL (3,"additional_copy_info: ",b, 1, 7);
   b += 1;
   indent (-1);

 }


 if (p.PES_CRC_flag == 0x01) {

   out_nl (3,"PES_CRC: ");
   indent (+1);
   outBit_Sx_NL (3,"previous_PES_packet_CRC: ",b, 0, 16);
   b += 2;
   indent (-1);

 }


 if (p.PES_extension_flag == 0x01) {

   u_int  PES_private_data_flag;
   u_int  pack_header_field_flag;
   u_int  program_packet_sequence_counter_flag;
   u_int  P_STD_buffer_flag;
   u_int  reserved;
   u_int  PES_extension_flag2;


   out_nl (3,"PES_extension: ");
   indent (+1);

   PES_private_data_flag                  = getBits (b, 0,  0,  1);
   pack_header_field_flag                 = getBits (b, 0,  1,  1);
   program_packet_sequence_counter_flag   = getBits (b, 0,  2,  1);
   P_STD_buffer_flag                      = getBits (b, 0,  3,  1);
   reserved                               = getBits (b, 0,  4,  3);
   PES_extension_flag2                    = getBits (b, 0,  7,  1);
   b += 1;

   out_SB_NL  (3,"PES_private_data_flag: ", PES_private_data_flag);
   out_SB_NL  (3,"pack_header_field_flag: ",  pack_header_field_flag);
   out_SB_NL  (3,"program_packet_sequence_counter_flag: ", program_packet_sequence_counter_flag);
   out_SB_NL  (3,"P-STD_buffer_flag: ", P_STD_buffer_flag);
   out_SB_NL  (6,"reserved: ", reserved);
   out_SB_NL  (3,"PES_extension_flag2: ", PES_extension_flag2);


   if (PES_private_data_flag == 0x01) {

   	out_nl (3,"PES_private_data: ");
	indent (+1);
	printhexdump_buf (4, b, 16);
   	b += 16;
   	indent (-1);

   }


   if (pack_header_field_flag == 0x01) {		/* ISO 11172-1 pack header */
	
	int pack_field_length;

   	out_nl (3,"pack_header_field: ");
	indent (+1);
	pack_field_length             = getBits (b, 0,  0,  8);
   	out_SB_NL  (3,"pack_field_length: ", pack_field_length);
   	out_nl (3,"Pack_header: ");
	indent (+1);
	pack_header (b+1, pack_field_length);
   	b += pack_field_length +1;
   	indent (-2);

   }



   if (program_packet_sequence_counter_flag == 0x01) {

   	out_nl (3,"program_packet_sequence_counter: ");
	indent (+1);
	out_SB_NL  (3,"Marker_bit: ", getBits (b, 0,  0,  1) );
	out_SB_NL  (3,"program_packet_sequence_counter: ", getBits (b, 0,  1,  7) );
	out_SB_NL  (3,"Marker_bit: ", getBits (b, 0,  8,  1) );
	out_SB_NL  (3,"MPEG1_MPEG2_identifier: ", getBits (b, 0,  9,  1) );
	out_SB_NL  (3,"original_stuff_length: ", getBits (b, 0, 10,  6) );
	b += 2;
   	indent (-1);

   }


   if (P_STD_buffer_flag == 0x01) {

   	out_nl (3,"P-STD_buffer: ");
	indent (+1);
	out_SB_NL  (6,"Fix 01: ",             getBits (b, 0,  0,  2) );
	out_SB_NL  (3,"P-STD_buffer_scale: ", getBits (b, 0,  2,  1) );
	out_SB_NL  (3,"P-STD_buffer_size: ",  getBits (b, 0, 3,  13) );
	b += 2;
   	indent (-1);

   }


   if (PES_extension_flag2 == 0x01) {

	u_int  PES_extension_field_length;

   	out_nl (3,"PES_extension_2: ");
	indent (+1);
	out_SB_NL  (3,"Marker_bit: ", getBits (b, 0,  0,  1) );
	PES_extension_field_length =  getBits (b, 0,  1,  7);
   	out_nl (3,"reserved: ");
	indent (+1);
	printhexdump_buf (6, b+1, PES_extension_field_length);
	b += PES_extension_field_length + 1;
   	indent (-2);

   }

   indent (-1);

 } /* p.PES_extension_flag */



 /* 
  * -- stuffing bytes
  * -- PES packet_data_bytes
  */

 out_nl (3,"PES_packet_data_bytes / stuffing bytes: ");
   indent (+1);
   printhexdump_buf (4, b, len - (b - b_start) );
   indent (-1);



}




void print_xTS_field (u_char *b, int bit_offset) 

{
 typedef struct  _xTS_Field {
        u_int     xTS_32_30;
        u_int     marker_bit_1;
        u_int     xTS_29_15;
        u_int     marker_bit_2;
        u_int     xTS_14_0;
        u_int     marker_bit_3;
 } xTS_Field;

 xTS_Field  f;
 int        bo = bit_offset;


 f.xTS_32_30				= getBits (b, 0, bo+ 0, 3);
 f.marker_bit_1				= getBits (b, 0, bo+ 3, 1);
 f.xTS_29_15				= getBits (b, 0, bo+ 4,15);
 f.marker_bit_2				= getBits (b, 0, bo+19, 1);
 f.xTS_14_0				= getBits (b, 0, bo+20,15);
 f.marker_bit_3				= getBits (b, 0, bo+35, 1);

 out_SB_NL  (4,"Bit 32-30: ",f.xTS_32_30);
 out_SB_NL  (4,"Marker_bit_1: ",f.marker_bit_1);
 out_SB_NL  (4,"Bit 29-15: ",f.xTS_29_15);
 out_SW_NL  (4,"Marker_bit_2: ",f.marker_bit_2);
 out_SB_NL  (4,"Bit 14-0: ",f.xTS_14_0);
 out_SW_NL  (4,"Marker_bit_3: ",f.marker_bit_3);
 out_SL_NL  (3," == > xTS = ",
		 (u_long) (f.xTS_32_30<<30) + (f.xTS_29_15<<15) + f.xTS_14_0);

}



void pack_header (u_char *b, int len)

{

	/*  ... $$$ TODO   */
	/* z.B. H.222 ISO 13818-1 Table 2-33 */
	/* 	ISO 11172-1 pack header */

   printhexdump_buf (4, b, len);

}



/*
   -- Data Packet Synchronous and synchronized data streaming
   -- EN 301 192  v1.3.1  S. 11

*/
void PES_data_packet (u_char *b, int len)

{

 typedef struct  _PES_DATA {
        u_int     data_identifier;
        u_int     sub_stream_id;
        u_int     PTS_extension_flag;
        u_int     output_data_rate_flag;
        u_int     reserved;
        u_int     PES_data_packet_header_length;

	// N ... optional data

 } PES_DATA;

 PES_DATA   p;
 int        len2;


   p.data_identifier			= getBits (b, 0,  0,  8);
   p.sub_stream_id			= getBits (b, 0,  8,  8);
   p.PTS_extension_flag			= getBits (b, 0, 16,  1);
   p.output_data_rate_flag		= getBits (b, 0, 17,  1);
   p.reserved                           = getBits (b, 0, 18,  2);
   p.PES_data_packet_header_length	= getBits (b, 0, 20,  4);
   b   += 3;
   len -= 3;
   len2 = p.PES_data_packet_header_length;


   out_S2B_NL  (3,"data_identifier: ", p.data_identifier,
		   dvbstrPESDataIdentifier (p.data_identifier) );
   out_SB_NL  (3,"sub_stream_id: ", p.sub_stream_id);
   out_SB_NL  (3,"PTS_extension_flag: ", p.PTS_extension_flag);
   out_SB_NL  (3,"output_data_rate_flag: ", p.output_data_rate_flag);
   out_SB_NL  (6,"reserved_1: ", p.reserved);
   out_SB_NL  (3,"PES_data_packet_header_length: ", p.PES_data_packet_header_length);

   if (p.PTS_extension_flag == 0x01) {
   	out_nl (3,"PTS_extension:");
	indent (+1);
	out_SB_NL  (6,"reserved: ", getBits (b, 0,  0,  7) );
	out_SW_NL  (3,"PTS_extension: ", getBits (b, 0,  7, 9) );
	/* $$$ TODO  PCR extension output in clear text, see ISO 13818-1*/
	b   += 2;
	len -= 2;
	len2 -= 2;
	indent (-1);
   }

   if (p.output_data_rate_flag == 0x01) {
   	out_nl (3,"output_data_rate:");
	indent (+1);
	out_SB_NL  (6,"reserved: ", getBits (b, 0,  0,  4) );
	out_SL_NL  (3,"output_data_rate: ", getBits (b, 0,  4, 28) );
	b   += 4;
	len -= 4;
	len2 -= 4;
	indent (-1);
   }


   out_nl (3,"PES_data_private_byte: ");
   indent (+1);
   printhexdump_buf (3, b, len2);
   b   += len2;
   len -= len2;
   indent (-1);


   out_nl (3,"PES_data_byte: ");
   indent (+1);
   printhexdump_buf (3, b, len);
   b   += len;
   len -= len;
   indent (-1);

}





// Annotations: 
//
// $$$ TODO  0x000001B9    ISO 13818-1/H.222.0 2.5.3.1  End Progam Stream ???




