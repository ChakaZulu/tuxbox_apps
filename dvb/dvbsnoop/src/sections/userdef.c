/*
$Id: userdef.c,v 1.1 2003/10/19 13:59:00 rasc Exp $


   -- User defined table // Private

   (c) rasc


$Log: userdef.c,v $
Revision 1.1  2003/10/19 13:59:00  rasc
-more table decoding



*/




#include "dvbsnoop.h"
#include "table_userdef.h"
#include "hexprint.h"



void decode_PRIVATE (u_char *b, int len)
{
 /* */

 typedef struct  _TPRIVATE {
    u_int      table_id;
    u_int      section_syntax_indicator;		
    u_int      reserved_1;
    u_int      reserved_2;
    u_int      section_length;
    

 } TPRIVATE;



 TPRIVATE p;
 //int n;


 
 p.table_id 			 = b[0];
 p.section_syntax_indicator	 = getBits (b, 0,  8, 1);
 p.reserved_1 			 = getBits (b, 0,  9, 1);
 p.reserved_2 			 = getBits (b, 0, 10, 2);
 p.section_length		 = getBits (b, 0, 12, 12);



 out_nl (3,"User_Defined-decoding....");
 out_S2B_NL (3,"Table_ID: ",p.table_id, dvbstrTableID (p.table_id));

 out_SB_NL (3,"section_syntax_indicator: ",p.section_syntax_indicator);
 out_SB_NL (6,"reserved_1: ",p.reserved_1);
 out_SB_NL (6,"reserved_2: ",p.reserved_2);
 out_SW_NL (5,"Section_length: ",p.section_length);

 indent (+1);
 out_nl (3, "Private data:");
 printhexdump_buf (3,b+3,p.section_length);
 indent (-1);

}
