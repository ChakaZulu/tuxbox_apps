/*
$Id: st.c,v 1.1 2001/09/30 13:05:20 rasc Exp $

   -- ST section (stuffing)

   (c) rasc


$Log: st.c,v $
Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS



*/




#include "dvbsnoop.h"
#include "st.h"


/*
 -- ST section (stuffing)
 -- ETSI EN 300 468   5.2.8
*/

void decode_ST (u_char *b, int len)
{

 typedef struct  _ST {
    u_int      table_id;
    u_int      section_syntax_indicator;		
    u_int      reserved_1;
    u_int      reserved_2;
    u_int      section_length;

    // N  databytes

 } ST;


 ST s;


 
 s.table_id 			 = b[0];
 s.section_syntax_indicator	 = getBits (b, 0, 8, 1);
 s.reserved_1 			 = getBits (b, 0, 9, 1);
 s.reserved_2 			 = getBits (b, 0, 10, 2);
 s.section_length		 = getBits (b, 0, 12, 12);


 out_nl (3,"ST-decoding....");
 out_S2B_NL (3,"Table_ID: ",s.table_id, dvbstrTableID (s.table_id));
 if (s.table_id != 0x72) {
   out_nl (3,"wrong Table ID");
   return;
 }

 out_SB_NL (3,"section_syntax_indicator: ",s.section_syntax_indicator);
 out_SB_NL (6,"reserved_1: ",s.reserved_1);
 out_SB_NL (6,"reserved_2: ",s.reserved_2);
 out_SW_NL (5,"Section_length: ",s.section_length);

 b += 3;
 out_nl    (3,"Section-Data:");
 printhex_buf (3, b, s.section_length);

}
