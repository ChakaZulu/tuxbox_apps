/*
$Id: sectables.c,v 1.4 2003/10/17 18:16:54 rasc Exp $

 --  For more information please see: ISO 13818 (-1) and ETSI 300 468
 -- Verbose Level >= 2
  (c) rasc


$Log: sectables.c,v $
Revision 1.4  2003/10/17 18:16:54  rasc
- started more work on newer ISO 13818  descriptors
- some reorg work started

Revision 1.3  2003/07/06 05:49:25  obi
CAMT fix and indentation

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "sectables.h"
#include "pat.h"
#include "cat.h"
#include "pmt.h"
#include "tsdt.h"
#include "nit.h"
#include "sdt.h"
#include "st.h"
#include "bat.h"
#include "tdt.h"
#include "tot.h"
#include "rst.h"
#include "dit.h"
#include "sit.h"
#include "eit.h"
#include "emm_ecm.h"




void  guess_table (u_char *buf, int len, u_int pid);





/* 
 -- decode Sections buffer by identifying the table IDs
 -- siehe EN 300 468 S.14 
*/

void decodeSections_buf (u_char *buf, int len, u_int pid)

{
  int table_id;


  // nothing to output ?  
  if (getVerboseLevel() < 2) return;


  out_nl (2,"PID:  %u (0x%04x)",pid,pid);
  table_id = buf[0];

  switch (pid) {

	case  0x000:		/* PAT  Program Association Table */
		decode_PAT  (buf, len);
		break; 

	case  0x001:		/* CAT  Conditional Access Table */
		decode_CAT  (buf, len);
		break; 

	case  0x002:		/* TSDT Transport Stream Description Sect */
		decode_TSDT  (buf, len);	/* Table ID == 0x03 */
		break; 

	case  0x010:		/* NIT, ST  */
		if (table_id == 0x72)   decode_ST   (buf,len);
		else                    decode_NIT  (buf, len);
		break; 

	case  0x011:		/* SDT, BAT, ST  */
		if      (table_id == 0x72) decode_ST  (buf,len);
		else if (table_id == 0x42) decode_SDT (buf,len); 
		else if (table_id == 0x46) decode_SDT (buf,len); 
		else                       decode_BAT (buf,len);
		break; 

	case  0x012:		/* EIT, ST  */
		if (table_id == 0x72)   decode_ST  (buf,len);
		else                    decode_EIT (buf,len);
		break; 

	case  0x013:		/* RST, ST  */
		if (table_id == 0x72) decode_ST   (buf,len);
		else                  decode_RST  (buf,len); 
		break; 

	case  0x014:		/* TDT, TOT, ST  */
		if      (table_id == 0x72) decode_ST   (buf,len);
		else if (table_id == 0x70) decode_TDT  (buf,len); 
		else                       decode_TOT  (buf,len);
		break; 

	case  0x015:		/* Net Sync  */
		// $$$$
		break; 

	case  0x01C:		/* Inband Signalling  */
		// $$$$
		break; 

	case  0x01D:		/* Measurement */
		// $$$$
		break; 

	case  0x01E:		/* DIT */
		decode_DIT  (buf, len);
		break; 

	case  0x01F:		/* SIT */
		decode_SIT  (buf, len);
		break; 


	case  0x1FFC:		/* ATSC SI */
		out_nl (2,"ATSC SI Packet");
		break;

	case  0x1FFD:		/* ATSC Master Program Guide */
		out_nl (2,"ATSC Master Program Guide  Packet");
		break;

	case  0x1FFF:		/* Null packet */
		out_nl (2,"Null Packet");
		break;


        default:			// multiple PIDs possible
                guess_table (buf, len, pid);
		break;


  }

  fflush (stdout);


}




/*
  -- content of packet can not be determined via PID,
  -- so gess from first byte of packet header
*/

void  guess_table (u_char *buf, int len, u_int pid)

{

   out_nl (2,"Guess packet type from table id...");

   /* $$$$ more tables to guess
    *   Use Table to map functions calls ....   TODO
    * */

   switch (buf[0]) {	/* Table ID, guess what... */


	case  0x02:		/* Program Map Section */
		decode_PMT  (buf, len);
		break; 

	case  0x80 ... 0x8F:	/* Conditional Access Message Section */
		decode_EMM_ECM  (buf, len);
		break; 

	default:
   		out_SB_NL (2,"Unknown, reserved or not implemented - TableID: ",buf[0]);
		break;

   }


   return;
}


