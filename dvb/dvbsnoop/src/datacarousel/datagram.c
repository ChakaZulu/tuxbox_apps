/*
$Id: datagram.c,v 1.15 2004/01/04 22:03:21 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 --  DATAGRAM section
 --  DSM-CC Data Carousel  EN 301 192 



$Log: datagram.c,v $
Revision 1.15  2004/01/04 22:03:21  rasc
time for a version leap

Revision 1.14  2004/01/01 20:09:16  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.13  2003/12/26 23:27:38  rasc
DSM-CC  UNT section

Revision 1.12  2003/11/26 23:54:46  rasc
-- bugfixes on Linkage descriptor

Revision 1.11  2003/11/26 20:31:50  rasc
no message

Revision 1.10  2003/11/26 19:55:31  rasc
no message

Revision 1.9  2003/11/24 23:52:15  rasc
-sync option, some TS and PES stuff;
dsm_addr inactive, may be wrong - due to missing ISO 13818-6

Revision 1.8  2003/11/09 22:54:16  rasc
no message

Revision 1.7  2003/11/09 22:50:32  rasc
no message

Revision 1.6  2003/11/09 22:26:11  rasc
filename change

Revision 1.5  2003/10/25 19:11:49  rasc
no message

Revision 1.4  2003/10/24 22:17:14  rasc
code reorg...

Revision 1.1  2003/10/19 22:22:58  rasc
- some datacarousell stuff started

*/




#include "dvbsnoop.h"
#include "datagram.h"
#include "llc_snap.h"
#include "strings/dvb_str.h"
#include "strings/dsmcc_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"





void decode_DSMCC_DATAGRAM (u_char *b, int len)
{
 /* EN 301 192 7.x */
// $$$ TODO this differs from ISO/IEC 13818-6:1998 AMD_1_2000_Cor_1_2002

 typedef struct  _DATAGRAM {
    u_int      table_id;
    u_int      section_syntax_indicator;		
    u_int      private_indicator;
    u_int      reserved_1;
    u_int      section_length;

    u_int      MAC_addr6;
    u_int      MAC_addr5;
    u_int      reserved_2;
    u_int      payload_scrambling_control;
    u_int      address_scrambling_control;
    u_int      LLC_SNAP_flag;
    
    u_int      current_next_indicator;
    u_int      section_number;
    u_int      last_section_number;

    u_int      MAC_addr4;
    u_int      MAC_addr3;	// MAC-Bits:  MSB first order
    u_int      MAC_addr2;
    u_int      MAC_addr1;

    // conditional
    // LLC SNAP   according ISO 8802-2

    // N ip datagram bytes
    u_int      ip_datagram_data_bytes;

    // conditional
    u_int      stuffing_bytes;

    // conditional
    unsigned long crc_checksum;
 } DATAGRAM;



 DATAGRAM   d;
 int        len1;


 d.table_id 			 = b[0];
 d.section_syntax_indicator	 = getBits (b, 0, 8, 1);
 d.private_indicator		 = getBits (b, 0, 9, 1);   // $$$ TODO error indicator
 d.reserved_1 			 = getBits (b, 0, 10, 2);
 d.section_length		 = getBits (b, 0, 12, 12);

 d.MAC_addr6			 = getBits (b, 0, 24, 8);
 d.MAC_addr5			 = getBits (b, 0, 32, 8);
 d.reserved_2			 = getBits (b, 0, 40, 2);
 d.payload_scrambling_control	 = getBits (b, 0, 42, 2);
 d.address_scrambling_control	 = getBits (b, 0, 44, 2);
 d.LLC_SNAP_flag		 = getBits (b, 0, 46, 1);

 d.current_next_indicator	 = getBits (b, 0, 47, 1);
 d.section_number 		 = getBits (b, 0, 48, 8);
 d.last_section_number 		 = getBits (b, 0, 56, 8);

 d.MAC_addr4			 = getBits (b, 0, 64, 8);
 d.MAC_addr3			 = getBits (b, 0, 72, 8);
 d.MAC_addr2			 = getBits (b, 0, 80, 8);
 d.MAC_addr1			 = getBits (b, 0, 88, 8);
    	// MAC-Bits:  MSB first ! 
	
 b += 12;
 len1 = d.section_length - 9;


 out_nl (3,"DSM-CC DATAGRAM-decoding....");
 out_S2B_NL (3,"Table_ID: ",d.table_id, dvbstrTableID (d.table_id));
 if (d.table_id != 0x3e) {
   out_nl (3,"wrong Table ID");
   return;
 }


 out_SB_NL (3,"section_syntax_indicator: ",d.section_syntax_indicator);
 out_SB_NL (3,"private_indicator: ",d.private_indicator);
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_SW_NL (5,"Section_length: ",d.section_length);

 out_SB_NL (5,"MACaddrbyte/DevicdID 6: ",d.MAC_addr6);
 out_SB_NL (5,"MACaddrbyte/DeviceID 5: ",d.MAC_addr5);
 out_SB_NL (6,"reserved_2: ",d.reserved_2);

 out_S2B_NL (3,"payload_scrambling_control: ",d.payload_scrambling_control,
		 dsmccStrPayload_scrambling_control(d.payload_scrambling_control));
 out_S2B_NL (3,"address_scrambling_control: ",d.address_scrambling_control,
		 dsmccStrAddress_scrambling_control(d.address_scrambling_control));

 out_SB_NL (3,"LLC_SNAP_flag: ",d.LLC_SNAP_flag);

 out_S2B_NL(3,"current_next_indicator: ",d.current_next_indicator, dvbstrCurrentNextIndicator(d.current_next_indicator));
 out_SB_NL (3,"Section_number: ",d.section_number);
 out_SB_NL (3,"Last_Section_number: ",d.last_section_number);

 out_SB_NL (5,"MACaddrbyte/DeviceID 4: ",d.MAC_addr4);
 out_SB_NL (5,"MACaddrbyte/DeviceID 3: ",d.MAC_addr3);
 out_SB_NL (5,"MACaddrbyte/DeviceID 2: ",d.MAC_addr2);
 out_SB    (5,"MACaddrbyte/DeviceID 1: ",d.MAC_addr1);
 out_nl    (3," => MAC-Address/DeviceID: %02x:%02x:%02x:%02x:%02x:%02x",
		 d.MAC_addr1,d.MAC_addr2,d.MAC_addr3,
		 d.MAC_addr4,d.MAC_addr5,d.MAC_addr6);


 if (d.LLC_SNAP_flag == 0x01) {
	 /*  ISO/IEC 8802-2   */
	 int k;
	 k = llc_snap (4,b);
 } else {
 	 print_databytes (4, "IP_datagram_bytes", b, len1-4);
 }
 b += (len1 - 4);


 d.crc_checksum		 = getBits (b, 0, 0, 32);
 if (d.section_syntax_indicator) {
     out_SB_NL (5,"CRC: ",d.crc_checksum);
 } else {
     out_SB_NL (5,"Checksum: ",d.crc_checksum);
 }


}


