/*
$Id: emm_ecm.c,v 1.6 2003/07/06 05:49:25 obi Exp $


   -- EMM / ECM Data packet

   (c) rasc


$Log: emm_ecm.c,v $
Revision 1.6  2003/07/06 05:49:25  obi
CAMT fix and indentation

Revision 1.5  2003/07/05 21:22:45  rasc
TypoFix - Tnx MWS

Revision 1.4  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.3  2001/10/16 18:05:43  rasc
no message

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS



*/




#include "dvbsnoop.h"
#include "emm_ecm.h"
#include "hexprint.h"



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
 //int n;


 
 e.table_id 			 = b[0];
 e.section_syntax_indicator	 = getBits (b, 0,  8, 1);
 e.reserved_1 			 = getBits (b, 0,  9, 1);
 e.reserved_2 			 = getBits (b, 0, 10, 2);
 e.section_length		 = getBits (b, 0, 12, 12);



 out_nl (3,"CAMT-decoding....");
 out_S2B_NL (3,"Table_ID: ",e.table_id, dvbstrTableID (e.table_id));

 out_SB_NL (3,"section_syntax_indicator: ",e.section_syntax_indicator);
 out_SB_NL (6,"reserved_1: ",e.reserved_1);
 out_SB_NL (6,"reserved_2: ",e.reserved_2);
 out_SW_NL (5,"Section_length: ",e.section_length);

 indent (+1);
 out_nl (3, "CA_message_section_data:");
 printhexdump_buf (3,b+3,e.section_length);
 indent (-1);

 //  !!! decoding the complete ECM/EMM stream may be illegal
 //      so we don't do this!
 //      secrets are secrets even if they are openly transmitted!
 //      you are not allowed to enhance this section!

}
