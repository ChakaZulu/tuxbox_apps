/*
$Id: tdt.c,v 1.3 2002/08/17 20:36:12 obi Exp $

   -- TDT section
   -- Time Date Table
   -- ETSI EN 300 468     5.2.5

   (c) rasc


$Log: tdt.c,v $
Revision 1.3  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS



*/




#include "dvbsnoop.h"
#include "tdt.h"




void decode_TDT (u_char *b, int len)
{

 typedef struct  _TDT {
    u_int      table_id;
    u_int      section_syntax_indicator;		
    u_int      reserved_1;
    u_int      reserved_2;
    u_int      section_length;
    u_long     UTC_time_MJD;
    u_long     UTC_time_UTC;


 } TDT;



 TDT        t;
 //int        len1;


 
 t.table_id 			 = b[0];
 t.section_syntax_indicator	 = getBits (b, 0, 8, 1);
 t.reserved_1 			 = getBits (b, 0, 9, 1);
 t.reserved_2 			 = getBits (b, 0, 10, 2);
 t.section_length		 = getBits (b, 0, 12, 12);
 t.UTC_time_MJD			 = getBits (b, 0, 24, 16);
 t.UTC_time_UTC			 = getBits (b, 0, 40, 24);


 out_nl (3,"TDT-decoding....");
 out_S2B_NL (3,"Table_ID: ",t.table_id, dvbstrTableID (t.table_id));
 if (t.table_id != 0x70) {
   out_nl (3,"wrong Table ID");
   return;
 }

 out_SB_NL (3,"section_syntax_indicator: ",t.section_syntax_indicator);
 out_SB_NL (6,"reserved_1: ",t.reserved_1);
 out_SB_NL (6,"reserved_2: ",t.reserved_2);
 out_SW_NL (5,"Section_length: ",t.section_length);

 out (3,"UTC_time: ");
 print_time40 (3,t.UTC_time_MJD,t.UTC_time_UTC);
 out_NL (3);

}




