/*
$Id: pespacket.c,v 1.8 2003/10/29 22:39:18 rasc Exp $

   -- PES Decode/Table section

   (c) rasc


$Log: pespacket.c,v $
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
#include "misc/hexprint.h"


int PES_decode2 (u_char *b, int len, int pid);
void print_xTS_field (u_char *b, int bit_offset);




void decodePES_buf (u_char *b, int len, int pid)
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
    out_nl (3," Packet_Start_CODE is wrong (= no PES)!  Following (wrong!?) decoded:\n");
 }

 out_nl     (3,"Packet_start_code_prefix: %06lx",p.packet_start_code_prefix);
 out_S2B_NL (3,"Stream_id: ",p.stream_id,dvbstrPESstream_ID(p.stream_id));
 out_SW_NL  (3,"PES_packet_length: ",p.PES_packet_length);

 b   += 6;
 len -= 6;
 len2  = p.PES_packet_length;


 switch (p.stream_id) {

	case 0xBC:		// program_stream_map
	case 0xBF:		// private_stream_2
	case 0xF0:		// ECM
	case 0xF1:		// EMM
	case 0xF2:		// DSMCC stream
	case 0xF8:		// ITE-T Rec. H.222.1 type E
	case 0xFF:		// program_stream_directory
    		out_nl (4,"PES_packet_data_bytes:");
		printhexdump_buf (4, b, len2);
		break;


	case 0xBE:		// padding stream!
    		out_nl (4,"Padding_bytes:");
		printhexdump_buf (4, b, len2);
		break;

	default:
		n = PES_decode2 (b, len2, pid);

		b    += n;
		len2 -= n;
		len  -= n;
		break;

 }


}



int  PES_decode2 (u_char *b, int len, int pid)

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
   out_SB_NL  (3,"Fixed: ", getBits (b, 0,  0, 4) );
   print_xTS_field (b, 4) ;
   indent (-1);
   b += 5;
 }

 if (p.PTS_DTS_flags == 0x03) {    // 10 + 01
   // PTS from "if" before...
   out_nl (3,"DTS: ");
   indent (+1);
   out_SB_NL  (6,"Fixed: ", getBits (b, 0,  0, 4) );
   print_xTS_field (b, 4) ;
   indent (-1);
   b += 5;
 }

 if (p.ESCR_flag == 0x01) {
   out_nl (3,"ESCR_flag: ");
   indent (+1);
   out_nl (3,"ESCR_base: ");
   out_SB_NL  (3,"Reserved: ", getBits (b, 0,  0, 2) );
   print_xTS_field (b, 2) ;
   out_SW_NL  (3,"ESCR_extension: ", getBits (b, 0, 38, 9) );
   out_SB_NL  (3,"marker_bit: ",     getBits (b, 0, 47, 1) );
   indent (-1);
   b += 6;
 }



 if (p.ES_rate_flag == 0x01) {
   out_nl (3,"ES_rate_flag: ");
   indent (+1);
   out_SB_NL  (3,"Marker_bit: ", getBits (b, 0,  0,  1) );
   out_SL_NL  (3,"ES_rate: ",    getBits (b, 0,  1, 22) );
   out_SB_NL  (3,"Marker_bit: ", getBits (b, 0, 23,  1) );
   indent (-1);
   b += 3;
 }

 if (p.DSM_trick_mode_flag == 0x01) {
   u_int  trick_mode_control;
   u_int  field_id;
   u_int  intra_slice_refresh;
   u_int  frequency_truncation;
   u_int  rep_control;
   u_int  reserved;

   out_nl (3,"Trick_mode_control: ");
   indent (+1);

   trick_mode_control			= getBits (b, 0,  0,  3);
   out_S2B_NL  (3,"trick_mode_control: ", trick_mode_control,
		   dvbstrPESTrickModeControl (trick_mode_control) );

   if ( (trick_mode_control == 0x0) ||			/* fast forward */
        (trick_mode_control == 0x3) ) {			/* fast reverse */

	   field_id 			= getBits (b, 0,  3,  2);
	   intra_slice_refresh		= getBits (b, 0,  5,  1);
	   frequency_truncation		= getBits (b, 0,  6,  2);

   	   out_SB_NL  (3,"field_id: ", field_id);	/* $$$ Table */
   	   out_SB_NL  (3,"intra_slice_refresh: ", intra_slice_refresh);
   	   out_SB_NL  (3,"frequency_truncation: ", frequency_truncation);

   } else if ( (trick_mode_control == 0x1) ||		/* slow motion  */
               (trick_mode_control == 0x4) ) {		/* slow reverse */

	   rep_control 			= getBits (b, 0,  3,  5);

   	   out_SB_NL  (3,"rep_control: ", rep_control);

   } else if (trick_mode_control == 0x2) {		/* freeze frame */

	   field_id 			= getBits (b, 0,  3,  2);
	   reserved 			= getBits (b, 0,  3,  5);

   	   out_SB_NL  (3,"field_id: ", field_id);	/* $$$ Table */
   	   out_SB_NL  (6,"reserved: ", reserved);

   } else {						/* reserved     */

	   reserved 			= getBits (b, 0,  3,  8);
   	   out_SB_NL  (6,"reserved: ", reserved);

   }

   indent (-1);
   b += 1;

 }  /* p.DSM_trick_mode_flag  */


 if (p.additional_copy_info_flag == 0x01) {

   out_nl (3,"additional_copy_info: ");
   indent (+1);
   out_SB_NL  (3,"marker_bit: ", getBits (b, 0,  0,  1) );
   out_SB_NL  (3,"additional_copy_info: ", getBits (b, 0,  1,  7) );
   b += 1;
   indent (-1);

 }


 if (p.PES_CRC_flag == 0x01) {

   out_nl (3,"PES_CRC: ");
   indent (+1);
   out_SW_NL  (3,"previous_PES_packet_CRC: ", getBits (b, 0,  0, 16) );
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
	printhexdump_buf (4, b+1, pack_field_length);
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



 return 0;
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

}


