/*
$Id: datagram.c,v 1.2 2003/10/21 19:54:44 rasc Exp $

   DATAGRAM section
   DSM-CC Data Carousel  EN 301 192 

   (c) rasc


$Log: datagram.c,v $
Revision 1.2  2003/10/21 19:54:44  rasc
no message

Revision 1.1  2003/10/19 22:22:58  rasc
- some datacarousell stuff started




*/




#include "dvbsnoop.h"
#include "datagram.h"
#include "descriptor.h"




void decode_DATAGRAM_DSMCC (u_char *b, int len)
{
 /* EN 301 192 7.x */

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
    unsigned long checksum;
    unsigned long crc;
 } DATAGRAM;



 DATAGRAM   d;
 int        len1,len2;


 d.table_id 			 = b[0];
 d.section_syntax_indicator	 = getBits (b, 0, 8, 1);
 d.private_indicator		 = getBits (b, 0, 9, 1);
 d.reserved_1 			 = getBits (b, 0, 10, 2);
 d.section_length		 = getBits (b, 0, 12, 12);




 out_nl (3,"DATAGRAM-decoding....");
 out_S2B_NL (3,"Table_ID: ",d.table_id, dvbstrTableID (d.table_id));
 if (d.table_id != 0x3e) {
   out_nl (3,"wrong Table ID");
   return;
 }


 out_SB_NL (3,"section_syntax_indicator: ",d.section_syntax_indicator);
 out_SB_NL (3,"private_indicator: ",d.private_indicator);
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_SW_NL (5,"Section_length: ",d.section_length);


 out_nl (1," ..... to be finished... ");
/*
 *
 * .... $$$$$$ TODO
 * */








}


