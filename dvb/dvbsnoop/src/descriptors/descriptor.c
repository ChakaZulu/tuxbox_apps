/*
$Id: descriptor.c,v 1.13 2003/12/29 22:14:53 rasc Exp $

  dvbsnoop
  (c) Rainer Scherg 2001-2003


  Descriptor Section
  - MPEG
  - DVB
  - DSM-CC  (todo)



$Log: descriptor.c,v $
Revision 1.13  2003/12/29 22:14:53  rasc
more dsm-cc INT UNT descriptors

Revision 1.12  2003/10/24 22:45:04  rasc
code reorg...

Revision 1.11  2003/10/24 22:17:14  rasc
code reorg...

Revision 1.10  2003/10/13 23:27:35  rasc
Bugfix, verbose < 4 segfaulted, tnx to 'mws'  for reporting.

Revision 1.9  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?

Revision 1.8  2003/06/24 23:51:03  rasc
bugfixes and enhancements

Revision 1.7  2003/05/03 02:51:08  obi
skip descriptors with length == 0

Revision 1.6  2003/03/17 16:15:11  obi
fixed infinite loop
thanks to Johannes Stezenbach

Revision 1.5  2002/09/29 13:01:35  wjoost
kleiner Fehler



*/


#include "dvbsnoop.h"
#include "descriptor.h"
#include "mpeg_descriptor.h"
#include "dvb_descriptor.h"
#include "misc/hexprint.h"
#include "misc/output.h"







/*
  determine descriptor type and print it...
  return byte length
*/

int  descriptor  (u_char *b)
	// $$$ TODO    scope sollte mit uebergeben werden (DSMCC, MPEG, DVB_SI, DSMCC_INT_UNT)

{
 int len;
 int id;


  id  =  (int)b[0];  
  len = ((int)b[1]) + 2;

  // nothing to print here? 
  // well, I guess all descriptors need a verbosity >=4...
  if (getVerboseLevel() < 4) return len;

  indent (+1);



  // Context of descriptors
  // $$$ To be improved!!!
 
  if (id < 0x40) {
	  descriptorMPEG (b);
  } else {
	  descriptorDVB (b);
  }


  indent (-1);

  return len;   // (descriptor total length)
}








/*
  Any  descriptor  (Basic Descriptor output)
  ETSI 300 468 
*/

void descriptor_any (u_char *b)

{

 typedef struct  _descANY {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // private data bytes

 } descANY;


 descANY  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 out_nl (4,"Descriptor-Data:");
 printhexdump_buf (4,b+2,d.descriptor_length);

}





