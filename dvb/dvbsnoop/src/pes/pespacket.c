/*
$Id: pespacket.c,v 1.2 2001/10/02 21:52:44 rasc Exp $

   -- PES Decode/Table section

   (c) rasc


$Log: pespacket.c,v $
Revision 1.2  2001/10/02 21:52:44  rasc
- init der time_delta
- PES erweitert, PES arbeitet im read() noch nicht richtig!!
- muss tmbinc fragem, ob ich Mist baue, oder der Treiber (??)

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS



*/




#include "dvbsnoop.h"
#include "pespacket.h"


int PES_decode2 (u_char *b, int len, int pid);




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


 out_nl (3," Packet_Start_CODE has to be 0x000001 !!!!  $$$$ ToDO \n");

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




}
