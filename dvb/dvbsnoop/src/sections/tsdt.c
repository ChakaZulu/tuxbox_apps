/*
$Id: tsdt.c,v 1.6 2003/11/01 17:05:47 rasc Exp $

   -- TSDT section
   -- Transport Stream Description Section
   -- ISO 13818

   (c) rasc


$Log: tsdt.c,v $
Revision 1.6  2003/11/01 17:05:47  rasc
no message

Revision 1.5  2003/10/29 20:54:57  rasc
more PES stuff, DSM descriptors, testdata



*/




#include "dvbsnoop.h"
#include "tsdt.h"
#include "descriptors/descriptor.h"
#include "strings/dvb_str.h"
#include "misc/output.h"
#include "misc/hexprint.h"



void decode_TSDT (u_char *b, int len)
{

 typedef struct  _TDST {
    u_int      table_id;
    u_int      section_syntax_indicator;		
    u_int      reserved_1;
    u_int      reserved_2;
    u_int      section_length;
    u_int      reserved_3;
    u_int      version_number;
    u_int      current_next_indicator;
    u_int      section_number;
    u_int      last_section_number;

    // N  descriptor

    u_long     crc;
 } TDST;


 TDST   t;
 int	len;


 
 t.table_id 			 = b[0];
 t.section_syntax_indicator	 = getBits (b, 0, 8, 1);
 t.reserved_1 			 = getBits (b, 0, 9, 1);
 t.reserved_2 			 = getBits (b, 0, 10, 2);
 t.section_length		 = getBits (b, 0, 12, 12);
 t.reserved_3 			 = getBits (b, 0, 24, 18);
 t.version_number 		 = getBits (b, 0, 42, 5);
 t.current_next_indicator	 = getBits (b, 0, 47, 1);
 t.section_number 		 = getBits (b, 0, 48, 8);
 t.last_section_number 		 = getBits (b, 0, 56, 8);

 b   += 8;
 len = t.section_length - 5;



 out_nl (3,"TDST-decoding....");
 out_S2B_NL (3,"Table_ID: ",t.table_id, dvbstrTableID (t.table_id));
 if (t.table_id != 0x03) {
   out_nl (3,"wrong Table ID");
   return;
 }
 
 out_SB_NL (3,"section_syntax_indicator: ",t.section_syntax_indicator);
 out_SB_NL (6,"Fixed '0': ",t.reserved_1);
 out_SB_NL (6,"reserved_2: ",t.reserved_2);
 out_SW_NL (5,"Section_length: ",t.section_length);
 out_SL_NL (6,"reserved_3: ",t.reserved_3);
 out_SB_NL (3,"Version_number: ",t.version_number);
 out_SB_NL (3,"Current_next_indicator: ",t.current_next_indicator);
 out_SB_NL (3,"Section_number: ",t.section_number);
 out_SB_NL (3,"Last_Section_number: ",t.last_section_number);



 while (len > 4) {
   int x;

   x = descriptor (b);
   b   += x;
   len -= x;
 }


 t.crc		 = getBits (b, 0, 0, 32);
 out_SL_NL (5,"CRC: ",t.crc);

}


