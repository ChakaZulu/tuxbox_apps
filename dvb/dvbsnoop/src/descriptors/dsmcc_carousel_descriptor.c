/*
$Id: dsmcc_carousel_descriptor.c,v 1.15 2004/01/11 21:01:31 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 -- Private TAG Space  DSM-CC
 -- DSM-CC Descriptors  ISO 13818-6  // TR 102 006




$Log: dsmcc_carousel_descriptor.c,v $
Revision 1.15  2004/01/11 21:01:31  rasc
PES stream directory, PES restructured

Revision 1.14  2004/01/02 22:59:59  rasc
DSM-CC  modules renaming...

Revision 1.13  2004/01/02 22:25:35  rasc
DSM-CC  MODULEs descriptors complete

Revision 1.12  2004/01/01 20:31:22  rasc
PES program stream map, minor descriptor cleanup

Revision 1.11  2004/01/01 20:09:19  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.10  2003/12/27 18:17:17  rasc
dsmcc PES dsmcc_program_stream_descriptorlist

Revision 1.9  2003/11/01 21:40:27  rasc
some broadcast/linkage descriptor stuff

Revision 1.8  2003/10/29 20:54:56  rasc
more PES stuff, DSM descriptors, testdata



*/


#include "dvbsnoop.h"
#include "descriptor.h"
#include "dsmcc_module_descriptor.h"
#include "dsmcc_int_unt_descriptor.h"
#include "strings/dsmcc_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"





/*
  determine descriptor type and print it...
  !!! DSMCC_CAROUSEL descriptors are in a private tag space !!!

  The userInfo field of BIOP::ModuleInfo shall be structured as a
  loop of descriptors which enables the use of Module descriptors
  as defined in DVB Data Carousels.


  return byte length
*/

int  descriptorDSMCC_CAROUSEL (u_char *b)

{
 int len;
 int id;


  id  =  (int) b[0];
  len = ((int) b[1]) + 2;

  out_NL (4);
  out_S2B_NL (4,"DSM-CC_CAROUSEL-DescriptorTag: ",id,
		  dsmccStrDSMCC_CAROUSEL_DescriptorTAG (id));
  out_SB_NL  (5,"Descriptor_length: ",b[1]);

  // empty ??
  len = ((int)b[1]) + 2;
  if (b[1] == 0)
	 return len;

  // print hex buf of descriptor
  printhex_buf (9, b,len);



  switch (b[0]) {

     case 0x01:  descriptorDSMCC_type (b); break;
     case 0x02:  descriptorDSMCC_name (b); break;
     case 0x03:  descriptorDSMCC_info (b); break;
     case 0x04:  descriptorDSMCC_module_link (b); break;
     case 0x05:  descriptorDSMCC_crc32 (b); break;
     case 0x06:  descriptorDSMCC_location (b); break;
     case 0x07:  descriptorDSMCC_est_download_time (b); break;
     case 0x08:  descriptorDSMCC_group_link (b); break;
     case 0x09:  descriptorDSMCC_compressed_module (b); break;
     case 0x0A:  descriptorDSMCC_subgroup_association (b); break;

     default: 
	if (b[0] < 0x80) {
	    out_nl (0,"  ----> ERROR: unimplemented descriptor (DSM-CC_CAROUSEL context), Report!");
	}
	descriptor_any (b);
	break;
  } 


  return len;   // (descriptor total length)
}








/* 
 * EN 301 192  / TR 102 006
 * private DSM-CC CAROUSEL descriptors
 *
 * 0x00 - 0x0A currently allocated by DVB
 * 0x0B - 0x6F reserved for future use by DVB
 * 0x70 - 0x7F reserved for DVB MHP
 * 0x80 - 0xFF private descriptors 
 *
 * e.g.:
 * The userInfo field of BIOP::ModuleInfo shall be structured as a
 * loop of descriptors which enables the use of Module descriptors
 * as defined in DVB Data Carousels.
 */





/*
  0x01 - type
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_type (u_char *b)
{
  int        len;

  // descriptor_tag	= b[0];
  len		        = b[1];

  out (4,"Text: ");
 	print_std_ascii (4, b+2, len);
	out_NL (4);
}




/*
  0x02 - name
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_name (u_char *b)
{
  int        len;

  // descriptor_tag	= b[0];
  len		        = b[1];

  out (4,"Text: ");
 	print_name (4, b+2, len);
	out_NL (4);
}



/*
  0x03 - info
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_info (u_char *b)
{
  int        len;
  u_char     ISO639_language_code[4];


  // descriptor_tag	= b[0];
  len		        = b[1];

  getISO639_3 (ISO639_language_code, b+2);
  out_nl (4,"  ISO639_language_code:  %3.3s", ISO639_language_code);

  out (4,"Text: ");
 	print_name (4, b+5, len-3);
	out_NL (4);
}


/*
  0x04 - module_link
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_module_link (u_char *b)
{

  // descriptor_tag	= b[0];
  // len	        = b[1];
 
  outBit_S2x_NL(4,"position: ",  b,16, 8,
			(char *(*)(u_long)) dsmccStr_GroupModuleLinkPosition);
  outBit_Sx_NL (4,"module_id: ", b,24,16);
}




/*
  0x05 - crc
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_crc32 (u_char *b)
{

  // descriptor_tag	= b[0];
  // len	        = b[1];
 
  outBit_Sx_NL (4,"CRC: ", b,16,32);
}



/*
  0x06 - location
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_location (u_char *b)
{

  // descriptor_tag	= b[0];
  // len	        = b[1];
 
  outBit_Sx (4,"location: ", b,16, 8);
  out_nl (4,"  [=  --> refers to component_tag in stream_identifier_descriptor]"); 
}


/*
  0x07 - est_download_time
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_est_download_time (u_char *b)
{

  // descriptor_tag	= b[0];
  // len	        = b[1];
 
  outBit_Sx (4,"est_download_time: ", b,16, 32);
  out_nl (4,"  [= seconds]");
}




/*
  0x08 - group link
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_group_link (u_char *b)
{

  // descriptor_tag	= b[0];
  // len	        = b[1];
 
  outBit_S2x_NL(4,"position: ",  b,16, 8,
			(char *(*)(u_long)) dsmccStr_GroupModuleLinkPosition);
  outBit_Sx_NL (4,"group_id: ", b,24,32);
}



/*
  0x09 -  compressed_module
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_compressed_module (u_char *b)
{

  // descriptor_tag	= b[0];
  // len	        = b[1];

  outBit_S2x_NL(4,"compression_method: ",  b,16, 8,
			(char *(*)(u_long)) dsmccStr_compression_method);
  outBit_Sx    (4,"original_size: ", b,24,32);
     out_nl (4,"  [= bytes]");
}




/*
  0x0A - subgroup_association
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_subgroup_association (u_char *b)
{
 descriptorDSMCC_ssu_subgroup_association (b);
}
































