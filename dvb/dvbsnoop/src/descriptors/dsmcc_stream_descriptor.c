/*
$Id: dsmcc_stream_descriptor.c,v 1.2 2004/01/11 21:01:31 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)


 -- DSM-CC STREAM Descriptors  ISO 13818-6




$Log: dsmcc_stream_descriptor.c,v $
Revision 1.2  2004/01/11 21:01:31  rasc
PES stream directory, PES restructured

Revision 1.1  2004/01/03 00:30:06  rasc
DSM-CC  STREAM descriptors module (started)




*/


#include "dvbsnoop.h"
#include "descriptor.h"
#include "strings/dvb_str.h"
#include "strings/dsmcc_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"
#include "misc/helper.h"






/*
  determine descriptor type and print it...
  !!! DSMCC stream descriptors are in a private tag space !!!

  Stream descriptors may be used to provide DSM-CC information that is
  correlated with a MPEG-2 Transport Stream or Program Stream. These
  descriptors are in the format of Program and Program Element Descriptors
  as defined in ISO/IEC 13818-1. DSM-CC stream descriptors shall only
  be carried in a DSMCC_section (see Chapter 9). This creates a unique
  identifier space (from that defined by ISO/IEC 13818-1) for descriptor
  tag values


  return byte length
*/

int  descriptorDSMCC_STREAM  (u_char *b)

{
 int len;
 int id;


  id  =  (int) b[0];
  len = ((int) b[1]) + 2;

  out_NL (4);
  out_S2B_NL (4,"DSM_CC-STREAM-DescriptorTag: ",id,
		  dsmccStrDSMCC_STREAM_DescriptorTAG(id));
  out_SB_NL  (5,"Descriptor_length: ",b[1]);

  // empty ??
  len = ((int)b[1]) + 2;
  if (b[1] == 0)
	 return len;

  // print hex buf of descriptor
  printhex_buf (9, b,len);



  switch (b[0]) {

// $$$ TODO   descriptors dsmcc stream
//  0x00	ISO/IEC 13818-6 reserved.
//  0x01	NPT Reference descriptor.
//  0x02	NPT Endpoint descriptor.
//  0x03	Stream Mode descriptor.
//  0x04	Stream Event descriptor.
//  0x05 - 0xFF	ISO/IEC 13818-6 reserved.

     default: 
	if (b[0] < 0x80) {
	    out_nl (0,"  ----> ERROR: unimplemented descriptor (DSM-CC STREAM context), Report!");
	}
	descriptor_any (b);
	break;
  } 


  return len;   // (descriptor total length)
}








/*
 * ISO/IEC 13818-6
 * private DSM-CC stream descriptors
 */




/*
  0x01 - scheduling
  ISO 13818-6
*/

void descriptorDSMCC_xx (u_char *b)
{
 int        len;



  // descriptor_tag	= b[0];
  len			= b[1];

  // $$$ TODO

}












