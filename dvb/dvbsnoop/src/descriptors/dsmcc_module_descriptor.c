/*
$Id: dsmcc_module_descriptor.c,v 1.10 2003/12/27 18:17:17 rasc Exp $ 


  dvbsnoop
  (c) Rainer Scherg 2001-2003

  Private TAG Space  DSM-CC
  DSM-CC Descriptors  ISO 13818-6  // TR 102 006




$Log: dsmcc_module_descriptor.c,v $
Revision 1.10  2003/12/27 18:17:17  rasc
dsmcc PES dsmcc_program_stream_descriptorlist

Revision 1.9  2003/11/01 21:40:27  rasc
some broadcast/linkage descriptor stuff

Revision 1.8  2003/10/29 20:54:56  rasc
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

int  descriptorDSMCC  (u_char *b)

{
 int len;
 int id;


  id  =  (int) b[0];
  len = ((int) b[1]) + 2;

  out_NL (4);
  out_S2B_NL (4,"DSM_CC-DescriptorTag: ",id,
		  dsmccStrDSMCC_DescriptorTAG (id));
  out_SB_NL  (5,"Descriptor_length: ",b[1]);

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








/* $$$ TODO   EN 301 192  / TR 102 006
 *
 * private DSM-CC descriptors
 *
 * */



