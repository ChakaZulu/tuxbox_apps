/*
$Id: ints.c,v 1.3 2003/10/24 22:17:14 rasc Exp $

   INT Section
   IP/MAC Notification Section
   DSM-CC Data Carousel  EN 301 192 

   (c) rasc


$Log: ints.c,v $
Revision 1.3  2003/10/24 22:17:14  rasc
code reorg...

Revision 1.2  2003/10/21 21:31:29  rasc
no message

Revision 1.1  2003/10/21 19:54:44  rasc
no message



*/




#include "dvbsnoop.h"
#include "ints.h"
#include "descriptors/descriptor.h"




void decode_INT_DSMCC (u_char *b, int len)
{
 /* EN 301 192 7.x */

 typedef struct  _INTs {
    u_int      table_id;
    u_int      section_syntax_indicator;		
    u_int      private_indicator;
    u_int      reserved_1;
    u_int      reserved_2;
    u_int      section_length;
    u_int      action_type;
    u_int      platform_id_hash;
    u_int      reserved_3;
    u_int      version_nr;
    u_int      current_next_indicator;
    u_int      section_nr;
    u_int      last_section_nr;
    u_long     platform_id;
    u_int      processing_order;

    // platform descriptor loop

    // N
    //   target descriptor loop
    //   operational descriptor loop

    u_long     crc;
 } INTs;



 INTs       d;
 int        len1,len2;


 d.table_id 			 = b[0];
 d.section_syntax_indicator	 = getBits (b, 0, 8, 1);
 d.reserved_1 			 = getBits (b, 0, 9, 1);
 d.reserved_2 			 = getBits (b, 0, 10, 2);
 d.section_length		 = getBits (b, 0, 12, 12);



 out_nl (3,"INT-decoding....");
 out_S2B_NL (3,"Table_ID: ",d.table_id, dvbstrTableID (d.table_id));
 if (d.table_id != 0x4c) {
   out_nl (3,"wrong Table ID");
   return;
 }


 out_SB_NL (3,"section_syntax_indicator: ",d.section_syntax_indicator);
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_SB_NL (6,"reserved_2: ",d.reserved_1);
 out_SW_NL (5,"Section_length: ",d.section_length);


 out_nl (1," ..TODO... to be finished... ");
/*
 *
 * .... $$$$$$ TODO
 * */








}


