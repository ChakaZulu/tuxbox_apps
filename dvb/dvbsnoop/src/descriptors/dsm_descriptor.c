/*
$Id: dsm_descriptor.c,v 1.6 2003/10/26 21:36:18 rasc Exp $ 


  dvbsnoop
  (c) Rainer Scherg 2001-2003

  DSM-CC Descriptors  ISO 13818-6




$Log: dsm_descriptor.c,v $
Revision 1.6  2003/10/26 21:36:18  rasc
private DSM-CC descriptor Tags started,
INT-Section completed..

Revision 1.5  2003/10/26 19:06:27  rasc
no message

Revision 1.4  2003/10/24 23:01:41  rasc
code reorg...

Revision 1.3  2003/10/24 22:45:04  rasc
code reorg...

Revision 1.2  2003/10/24 22:17:17  rasc
code reorg...

Revision 1.1  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?



*/


#include "dvbsnoop.h"
#include "dsm_descriptor.h"
#include "misc/hexprint.h"
#include "misc/output.h"





/*
  determine descriptor type and print it...
  !!! DSMCC descriptors are in a provate tag space !!!

  return byte length
*/

int  descriptorDSMCCPrivate  (u_char *b)

{
 int len;
 int id;


  id  =  (int) b[0];
  len = ((int) b[1]) + 2;

  out_NL (4);
  out_S2B_NL (4,"DSM_CC-DescriptorTag: ",id,
		  dsmccStrDSMCCPrivateDescriptorTAG(id));
  out_SB_NL  (5,"Descriptor_length: ",b[1]);
  /* $$$$ TODO */
out_nl (1," ... TODO... $$$ not yet done");
return len;

  // empty ??
  len = ((int)b[1]) + 2;
  if (b[1] == 0)
	 return len;

  // print hex buf of descriptor
  printhex_buf (9, b,len);



  switch (b[0]) {


     default: 
	if (b[0] < 0x80) {
	    out_nl (0,"  ----> ERROR: unimplemented descriptor (DSM-CC context), Report!");
	}
	descriptorDSMCC_any (b);
	break;
  } 


  return len;   // (descriptor total length)
}








/*
  Any  descriptor  (Basic Descriptor output)
  ETSI  TR 102 206  (ISO 13818-6)
*/

void descriptorDSMCC_any (u_char *b)

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




