/*
$Id: dsmcc_int_unt_descriptor.c,v 1.1 2003/10/29 20:56:18 rasc Exp $ 


  dvbsnoop
  (c) Rainer Scherg 2001-2003

  Private TAG Space  DSM-CC INT UNT
  DSM-CC Descriptors  ISO 13818-6  // TR 102 006




$Log: dsmcc_int_unt_descriptor.c,v $
Revision 1.1  2003/10/29 20:56:18  rasc
more PES stuff, DSM descriptors, testdata




*/


#include "dvbsnoop.h"
#include "descriptor.h"
#include "dsm_descriptor.h"
#include "strings/dsmcc_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"





/*
  determine descriptor type and print it...
  !!! DSMCC descriptors are in a private tag space !!!

  return byte length
*/

int  descriptorDSMCC_INT_UNT_Private  (u_char *b)

{
 int len;
 int id;


  id  =  (int) b[0];
  len = ((int) b[1]) + 2;

  out_NL (4);
  out_S2B_NL (4,"DSM_CC-INT-UNT-DescriptorTag: ",id,
		  dsmccStrDSMCC_INT_UNT_DescriptorTAG(id));
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

void descriptorDSMCC_INT_UNT_any (u_char *b)

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








/* $$$ TODO   EN 301 192  / TR 102 006
 *
 * private DSM-CC INT UNT descriptors
 *
 * */



