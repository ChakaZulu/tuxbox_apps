/*
$Id: emm_ecm.c,v 1.1 2001/09/30 13:05:20 rasc Exp $


   -- EMM / ECM Data packet

   (c) rasc


$Log: emm_ecm.c,v $
Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS



*/




#include "dvbsnoop.h"
#include "emm_ecm.h"



void decode_EMM_ECM (u_char *b, int len)
{
 /* */

 typedef struct  _EMM_ECM {
    u_int      table_id;
    u_int      section_syntax_indicator;		
    u_int      reserved_1;
    u_int      reserved_2;
    u_int      section_length;
    

 } EMM_ECM;




 EMM_ECM  e;
 int n;


 
 e.table_id 			 = b[0];
 e.section_syntax_indicator	 = getBits (b, 0,  8, 1);
 e.reserved_1 			 = getBits (b, 0,  9, 1);
 e.reserved_2 			 = getBits (b, 0, 10, 2);
 e.section_length		 = getBits (b, 0, 12, 12);



 out_nl (3,"EMM/ECM-decoding....");
 out_S2B_NL (3,"Table_ID: ",e.table_id, dvbstrTableID (e.table_id));

 out_SB_NL (3,"section_syntax_indicator: ",e.section_syntax_indicator);
 out_SB_NL (6,"reserved_1: ",e.reserved_1);
 out_SB_NL (6,"reserved_2: ",e.reserved_1);
 out_SW_NL (5,"Section_length: ",e.section_length);


 printhexdump_buf (3,b+3,e.section_length);

 //  !!! decoding the complete ECM/EMM stream may be illegal
 //      so we don't do this!


}
