/*
$Id: dsmcc_int_unt_descriptor.c,v 1.4 2003/12/27 18:17:17 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


 -- Private TAG Space  DSM-CC INT UNT
 -- DSM-CC Descriptors  ISO 13818-6  // TR 102 006




$Log: dsmcc_int_unt_descriptor.c,v $
Revision 1.4  2003/12/27 18:17:17  rasc
dsmcc PES dsmcc_program_stream_descriptorlist

Revision 1.3  2003/12/27 14:35:00  rasc
dvb-t descriptors
DSM-CC: SSU Linkage/DataBroadcast descriptors

Revision 1.2  2003/11/01 21:40:27  rasc
some broadcast/linkage descriptor stuff

Revision 1.1  2003/10/29 20:56:18  rasc
more PES stuff, DSM descriptors, testdata




*/


#include "dvbsnoop.h"
#include "descriptor.h"
#include "dsm_descriptor.h"
#include "dvb_descriptor.h"
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

  // empty ??
  len = ((int)b[1]) + 2;
  if (b[1] == 0)
	 return len;

  // print hex buf of descriptor
  printhex_buf (9, b,len);



  switch (b[0]) {

// {  0x01, 0x01,  "scheduling_descriptor" },
//     {  0x02, 0x02,  "update_descriptor" },
//     {  0x03, 0x03,  "ssu_location_descriptor" },
//     {  0x04, 0x04,  "message_descriptor" },
//     {  0x05, 0x05,  "ssu_event_name_descriptor" },
//     {  0x06, 0x06,  "target_smartcard_descriptor" },
//     {  0x07, 0x07,  "target_MAC_address_descriptor" },
//     {  0x08, 0x08,  "target_serial_number_descriptor" },
//     {  0x09, 0x09,  "target_IP_address_descriptor" },
//     {  0x0A, 0x0A,  "target_IPv6_address_descriptor" },
//     {  0x0B, 0x0B,  "ssu_subgroup_association_descriptor" },
//     {  0x0C, 0x0C,  "IP/MAC_platform_name_descriptor" },
//     {  0x0D, 0x0D,  "IP/MAC_platform_provider_name_descriptor" },
//     {  0x0E, 0x0E,  "target_MAC_address_range_descriptor" },
//     {  0x0F, 0x0F,  "target_IP_slash_descriptor" },
//     {  0x10, 0x10,  "target_IP_source_slash_descriptor" },
//     {  0x11, 0x11,  "target_IPv6_slash_descriptor" },
//     {  0x12, 0x12,  "target_IPv6_source_slash_descriptor" },
//     {  0x13, 0x13,  "ISP_access_mode_descriptor" },
//     {  0x14, 0x3F,  "reserved" },
     //     {  0x40, 0x7F,  "DVB-SI scope" },  Telphone, private_data_spec
//     {  0x80, 0xFE,  "user_private_descriptor" },



     case 0x57:  descriptorDVB_Telephone (b);  break;
     case 0x5F:  descriptorDVB_PrivateDataSpecifier (b);  break;

     default: 
	if (b[0] < 0x80) {
	    out_nl (0,"  ----> ERROR: unimplemented descriptor (DSM-CC INT/UNT context), Report!");
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


// EN 301 192  S. 26  $$$


