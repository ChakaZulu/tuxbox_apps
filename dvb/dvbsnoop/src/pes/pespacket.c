/*
$Id: pespacket.c,v 1.21 2004/01/22 22:26:35 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)



 -- PES Decode/Table section



$Log: pespacket.c,v $
Revision 1.21  2004/01/22 22:26:35  rasc
pes_pack_header
section read timeout

Revision 1.20  2004/01/11 22:49:41  rasc
PES restructured

Revision 1.19  2004/01/11 21:01:32  rasc
PES stream directory, PES restructured

Revision 1.18  2004/01/02 16:40:38  rasc
DSM-CC  INT/UNT descriptors complete
minor changes and fixes

Revision 1.17  2004/01/02 00:00:41  rasc
error output for buffer overflow

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
#include "pes_std.h"
#include "pes_data.h"
#include "pes_dsmcc.h"
#include "pes_psm.h"
#include "pes_psdir.h"
#include "pes_misc.h"
#include "strings/dvb_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"





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


 out_nl     (3,"Packet_start_code_prefix: %06lx",p.packet_start_code_prefix);
 out_S2B_NL (3,"Stream_id: ",p.stream_id,dvbstrPESstream_ID(p.stream_id));
 out_SW_NL  (3,"PES_packet_length: ",p.PES_packet_length);



 b   += 6;
 len -= 6;
 len2  = p.PES_packet_length;


 n = 0;
 switch (p.stream_id) {

	// -- special ProgramStream (PS) - IDs
	// $$$ TODO   (out of this control struct due to length handling, etc??)
//	case 0xB9:		// MPEG_program_end
//	case 0xBA:		// MPEG_pack_header_start
//	case 0xBB:		// MPEG_system_header_start



	case 0xBC:		// program_stream_map
		PES_decodePSM (b, len2);
		break;

	case 0xBF:		// private_stream_2  (EN301192-1.3.1 S.10)
	case 0xF0:		// ECM
	case 0xF1:		// EMM
	case 0xF8:		// ITE-T Rec. H.222.1 type E
		print_databytes (3,"PES_packet_data_bytes:", b, len2);
		n = len2;
		break;

	case 0xBD:		// Data Stream, privat_stream_1 (EN301192-1.3.1 S.11)
    		out_nl (3,"PES_data_packet:");
		indent (+1);
		PES_decodeDATA (b, len2);
		indent (-1);
		break;

	case 0xBE:		// padding stream!
		print_databytes (3,"Padding_bytes:", b, len2);
		n = len2;
		break;

	case 0xF2:		// DSMCC stream
		PES_decodeDSMCC (b, len2);
		break;

	case 0xFF:		// program_stream_directory
		PES_decodePSDIR (b, len2);
		break;

	default:
 		if ((p.PES_packet_length==0) && ((p.stream_id & 0xF0)==0xE0)) {

			 out_nl (3," ==> video elementary stream... \n");

 		} else {

    			out_nl (3,"Default PES decoding:");
			indent (+1);
			PES_decode_std (b, len2);
			indent (-1);

		}
		break;

 }


}


 // Annotation:
 // ISO 13818-1, 2.4.3.6:
 // PES_packet_length:  A 16-bit field specifying the number of bytes in the
 // PES packet following the last byte of the field. A value of 0 indicates that
 // the PES packet length is neither specified nor bounded and is allowed only
 // in PES packets whose payload consists of bytes from a video elementary
 // stream contained in Transport Stream packets.








// Annotations: 
//
// $$$ TODO  0x000001B9    ISO 13818-1/H.222.0 2.5.3.1  End Progam Stream ???




