/*
$Id: pespacket.c,v 1.4 2001/12/06 15:33:18 rasc Exp $

   -- PES Decode/Table section

   (c) rasc


$Log: pespacket.c,v $
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
 int          len2;
 int          n;


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
   out_SB_NL  (3,"Fixed: ", getBits (b, 0,  0, 4) );
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
   out_SW_NL  (3,"marker_bit: ",     getBits (b, 0, 47, 1) );
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



 out_nl (3,".... rest still unimplemented.... ");
 // $$$ hier muss auch noch einiges gemacht werden (aber zur Zeit
 // $$$ so gut wie keine Zeit


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


