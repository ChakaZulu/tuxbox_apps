/*
$Id: dsmcc.c,v 1.5 2004/01/02 22:25:34 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 --  DSM-CC Section
 --  DSM-CC Data Carousel
 --   ETSI TR 101 202
 --   ISO/IEC 13818-6



$Log: dsmcc.c,v $
Revision 1.5  2004/01/02 22:25:34  rasc
DSM-CC  MODULEs descriptors complete

Revision 1.4  2004/01/02 16:40:33  rasc
DSM-CC  INT/UNT descriptors complete
minor changes and fixes

Revision 1.3  2004/01/01 20:09:16  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.2  2003/12/27 14:35:00  rasc
dvb-t descriptors
DSM-CC: SSU Linkage/DataBroadcast descriptors

Revision 1.1  2003/12/27 00:21:16  rasc
dsmcc section tables




*/



#include "dvbsnoop.h"
#include "dsmcc.h"
#include "llc_snap.h"

#include "descriptors/descriptor.h"
#include "strings/dsmcc_str.h"
#include "strings/dvb_str.h"

#include "misc/output.h"
#include "misc/hexprint.h"




static void DSMCC_descriptor_list (u_char *b, int len);



void decode_DSMCC_section (u_char *b, int len)
{
 /* TR 101 202 */

 int        len1;
 u_int      table_id;
 u_int      section_length;
 u_int      section_syntax_indicator;
 u_int      private_indicator;



 out_nl (3,"DSM-CC-decoding....");
 table_id = outBit_S2x_NL (3,"Table_ID: ",	b, 0, 8,
				(char *(*)(u_long))dvbstrTableID );
 if (table_id < 0x3a || table_id > 0x3e) {
   out_nl (3,"wrong Table ID");
   return;
 }


 section_syntax_indicator =
	 outBit_Sx_NL (4,"Section_syntax_indicator: ",	b, 8, 1);
 private_indicator = 
	 outBit_Sx_NL (4,"private_indicator: ",		b, 9, 1);
 outBit_Sx_NL (6,"reserved_1: ",			b,10, 2);
 section_length =
	 outBit_Sx_NL (5,"dsmcc_section_length: ",	b,12,12);


 outBit_Sx_NL (3,"table_id_extension: ",		b,24,16);
 outBit_Sx_NL (6,"reserved_3: ",		b,40, 2);

 outBit_Sx_NL (3,"Version_number: ",		b,42, 5);
 outBit_S2x_NL(3,"Current_next_indicator: ",	b,47, 1,
			(char *(*)(u_long))dvbstrCurrentNextIndicator );
 outBit_Sx_NL (3,"Section_number: ",		b,48, 8);
 outBit_Sx_NL (3,"Last_section_number: ",	b,56, 8);


 b += 8;
 len1 = section_length - 5 - 4;	    // -4 == CRC/Checksum


 if (table_id == 0x3A) {

	llc_snap (3,b);		 /*  ISO/IEC 8802-2   */

 } else if (table_id == 0x3B) {

	// dsmcc_userNetworkMessage()
	// $$$ TODO
	out_nl (1,"$$$ TODO...");
	indent (+1);
	printhexdump_buf (4, b, len1);
	indent (-1);

 } else if (table_id == 0x3C) {

	// dsmcc_downloadDataMessage()
	// $$$ TODO
	out_nl (1,"$$$ TODO...");
	indent (+1);
	printhexdump_buf (4, b, len1);
	indent (-1);

 } else if (table_id == 0x3D) {

	DSMCC_descriptor_list (b,len1);

 } else if (table_id == 0x3E) {

	 // $$$ Remark: DVB defines 0x3E as datagram!!   $$$ TODO ??
	 print_private_data (4, b, len1);

 }

 b += len1;


 if (section_syntax_indicator) {
 	outBit_Sx_NL (5,"CRC: ",	b,0,32);
 } else {
 	outBit_Sx_NL (5,"Checksum: ",	b,0,32);
 }

}





/*
NOTE 1: The DownloadServerInitiate message, the DownloadInfoIndication message, and the
DownloadCancel message are in the userNetworkMessage class.
NOTE 2: The DownloadDataBlock message is within the downloadMessage class.
*/



static void DSMCC_descriptor_list (u_char *b, int len)
{
   int x;

   while (len > 0) {
	  x = descriptor (b, DSMCC_STREAM);
	  b += x;
	  len -= x;
   }
}






