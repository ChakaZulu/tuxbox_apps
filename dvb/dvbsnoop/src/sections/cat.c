/*
$Id: cat.c,v 1.2 2001/10/06 18:19:18 Toerli Exp $

 -- CAT Section
 -- rasc
   (c) rasc

$Log: cat.c,v $
Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "cat.h"


void decode_CAT (u_char *b, int len)
{
 /* IS13818-1  S. 63 */
 /* see also: ETS 468, ETR 289 */

 typedef struct  _CAT {
    u_int      table_id;
    u_int      section_syntax_indicator;		
    u_int      reserved_1;
    u_int      section_length;
    u_int      reserved_2;
    u_int      version_number;
    u_int      current_next_indicator;
    u_int      section_number;
    u_int      last_section_number;

    // private section

    unsigned long CRC;
 } CAT;

 CAT  c;
 int  n;

 
 c.table_id 			 = b[0];
 c.section_syntax_indicator	 = getBits (b, 0, 8, 1);
 c.reserved_1 			 = getBits (b, 0, 10, 2);
 c.section_length		 = getBits (b, 0, 12, 12);
 c.reserved_2 			 = getBits (b, 0, 24, 18);
 c.version_number 		 = getBits (b, 0, 42, 5);
 c.current_next_indicator	 = getBits (b, 0, 47, 1);
 c.section_number 		 = getBits (b, 0, 48, 8);
 c.last_section_number 		 = getBits (b, 0, 56, 8);



 out_nl (3,"CAT-decoding....");
 out_S2B_NL (3,"Table_ID: ",c.table_id, dvbstrTableID (c.table_id));
 if (c.table_id != 0x01) {
    out_nl (3,"wrong Table ID");
    return;
 }

 out_SB_NL (3,"section_syntax_indicator: ",c.section_syntax_indicator);
 out_SB_NL (6,"(fixed): ",0);
 out_SB_NL (6,"reserved_1: ",c.reserved_1);
 out_SW_NL (5,"Section_length: ",c.section_length);
 out_SB_NL (6,"reserved_2: ",c.reserved_2);
 out_SB_NL (3,"Version_number: ",c.version_number);
 out_SB_NL (3,"Current_next_indicator: ",c.current_next_indicator);
 out_SB_NL (3,"Section_number: ",c.section_number);
 out_SB_NL (3,"Last_Section_number: ",c.last_section_number);

 // buffer + header, len = len - header - CRC
 // Descriptor ISO 13818 - 2.6.1


 // - header - CRC
 n = c.section_length - 5 - 4;
 b += 8;


 indent (1);
 while (n >= 4) {
   int i;

   i =  descriptor (b);
   n -= i;
   b += i;
   
 }
 indent (-1);


 c.CRC 		 = getBits (b, 0, 0, 32);
 out_SL_NL (5,"CRC: ",c.CRC);

}




