/*
$Id: ints.c,v 1.9 2003/11/26 23:54:46 rasc Exp $

   INT Section
   IP/MAC Notification Section
   DSM-CC Data Carousel  EN 301 192 

   (c) rasc


$Log: ints.c,v $
Revision 1.9  2003/11/26 23:54:46  rasc
-- bugfixes on Linkage descriptor

Revision 1.8  2003/10/29 20:54:56  rasc
more PES stuff, DSM descriptors, testdata



*/




#include "dvbsnoop.h"
#include "ints.h"
#include "descriptors/dsm_int_unt_descriptor.h"
#include "strings/dvb_str.h"
#include "strings/dsmcc_str.h"




void decode_DSMCC_INT (u_char *b, int len)
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
    u_int      version_number;
    u_int      current_next_indicator;
    u_int      section_number;
    u_int      last_section_number;
    u_long     platform_id;
    u_int      processing_order;

    // platform descriptor loop

    // N
    //   target descriptor loop
    //   operational descriptor loop

    u_long     CRC;
 } INTs;



 INTs       d;
 int        len1,i;


 d.table_id 			 = b[0];
 d.section_syntax_indicator	 = getBits (b, 0, 8, 1);
 d.reserved_1 			 = getBits (b, 0, 9, 1);
 d.reserved_2 			 = getBits (b, 0, 10, 2);
 d.section_length		 = getBits (b, 0, 12, 12);

 d.action_type			 = getBits (b, 0, 24,  8);
 d.platform_id_hash		 = getBits (b, 0, 32,  8);
 d.reserved_3 			 = getBits (b, 0, 40,  2);

 d.version_number 		 = getBits (b, 0, 42, 5);
 d.current_next_indicator	 = getBits (b, 0, 47, 1);
 d.section_number 		 = getBits (b, 0, 48, 8);
 d.last_section_number 		 = getBits (b, 0, 56, 8);

 d.platform_id			 = getBits (b, 0, 64, 24);
 d.processing_order		 = getBits (b, 0, 88, 8);

 b += 12;
 len1 = d.section_length - 9;




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

 out_S2B_NL(4,"Action_type: ",d.action_type,dsmccStrAction_Type(d.action_type));
 out_SB_NL (4,"Platform_id_hash: ",d.platform_id_hash);

 out_SB_NL (6,"reserved_2: ",d.reserved_3);
 out_SB_NL (3,"Version_number: ",d.version_number);
 out_SB_NL (3,"Current_next_indicator: ",d.current_next_indicator);
 out_SB_NL (3,"Section_number: ",d.section_number);
 out_SB_NL (3,"Last_Section_number: ",d.last_section_number);

 out_S2L_NL (4,"Platform_id: ",d.platform_id,
		 dsmccStrPlatform_ID (d.platform_id));
 out_S2B_NL (4,"Processing_order: ",d.processing_order,
		 dsmccStrProcessing_order (d.processing_order));


 i = pto_descriptor_loop ("platform",b); 
 b   += i;
 len -= i;

 while (len > 4) {
 	i = pto_descriptor_loop ("target",b); 
	b   += i;
	len -= i;

 	i = pto_descriptor_loop ("operational",b); 
	b   += i;
	len -= i;
 }


 d.CRC 		 = getBits (b, 0, 0, 32);
 out_SL_NL (5,"CRC: ",d.CRC);

}




/*
 * -- dsmcc descriptor loops  (name, buffer)
 * --- P latform_descriptors
 * --- T arget_descriptors
 * --- O perational descriptors
 * -- return: len
 */

int pto_descriptor_loop (u_char *name, u_char *b)
{
   u_int   reserved;
   u_int   loop_length;

   int len,i;


   reserved   		 = getBits (b, 0, 0,  4);
   loop_length  	 = getBits (b, 0, 4, 12);
   b += 2;
   len = loop_length;


   out_nl (3,"%s_descriptor_loop:",name);
   indent (+1);

     out_SB_NL (6,"reserved: ",reserved);
     out (4,name);
     out_SW_NL (4,"_loop_length: ",loop_length);


     indent (+1);
     while (len > 0) {
	 i   = descriptorDSMCC_INT_UNT_Private (b);
	 b   += i;
	 len -= i;
	
     }

   indent (-2);
   return  (loop_length +2);
}





