/*
$Id: dsmcc_int_unt_descriptor.c,v 1.7 2003/12/29 22:14:53 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


 -- Private TAG Space  DSM-CC INT UNT
 -- DSM-CC Descriptors  ISO 13818-6  // TR 102 006




$Log: dsmcc_int_unt_descriptor.c,v $
Revision 1.7  2003/12/29 22:14:53  rasc
more dsm-cc INT UNT descriptors

Revision 1.6  2003/12/28 00:01:14  rasc
some minor changes/adds...

Revision 1.5  2003/12/27 22:02:43  rasc
dsmcc INT UNT descriptors started

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
#include "dsm_int_unt_descriptor.h"
#include "descriptor.h"
#include "dsm_descriptor.h"
#include "dvb_descriptor.h"
#include "strings/dsmcc_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"
#include "misc/helper.h"





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
     case 0x06:  descriptorDSMCC_target_smartcard (b); break;
     case 0x07:  descriptorDSMCC_target_MAC_address (b); break;
     case 0x08:  descriptorDSMCC_target_serial_number (b); break;
     case 0x09:  descriptorDSMCC_target_IP_address (b); break;
     case 0x0A:  descriptorDSMCC_target_IPv6_address (b); break;
//     {  0x0B, 0x0B,  "ssu_subgroup_association_descriptor" },
     case 0x0C:  descriptorDSMCC_IP_MAC_platform_name (b); break;
     case 0x0D:  descriptorDSMCC_IP_MAC_platform_provider_name (b); break;
     case 0x0E:  descriptorDSMCC_target_MAC_address_range (b); break;
     case 0x0F:  descriptorDSMCC_target_IP_slash (b); break;
     case 0x10:  descriptorDSMCC_target_IP_source_slash (b); break;
     case 0x11:  descriptorDSMCC_target_IPv6_slash (b); break;
     case 0x12:  descriptorDSMCC_target_IPv6_source_slash (b); break;
//     0x13 IP/MAC stream_location_descriptor
//     {  0x14, 0x14,  "ISP_access_mode_descriptor" },	/// ???? $$$ TODO
//     {  0x14, 0x3F,  "reserved" },
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













/* $$$ TODO   EN 301 192  / TR 102 006
 *
 * private DSM-CC INT UNT descriptors
 *
 * */


// EN 301 192  S. 26  $$$



















/*
  0x06 - target_smartcard 
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_target_smartcard (u_char *b)
{
 int len;

 // descriptor_tag	= b[0];
 len			= b[1];

 outBit_Sx_NL (4,"Super_CA_system_id: ",  b,16,32);  // $$$ TODO ? TS 101 197

 out_nl (4,"Private Data:");
 printhexdump_buf (4,b+6,len-4);
}





/*
  0x07 - MAC_address
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_target_MAC_address (u_char *b)
{
 int  len;
 long mac_H, mac_L;

 // descriptor_tag	= b[0];
 len			= b[1];

 mac_H 		= getBits (b, 0,  16, 24);
 mac_L 		= getBits (b, 0,  40, 24);
 out (4,"Mac_addr_mask: %06lx%06lx [= ",mac_H, mac_L);
  	displ_mac_addr (4, mac_H, mac_L);
	out_nl (4,"]");
 b += 8;
 len -= 6;

 while (len > 0) {
 	mac_H 		= getBits (b, 0,   0, 24);
 	mac_L 		= getBits (b, 0,  24, 24);
 	out (4,"Mac_addr: %06lx%06lx [= ",mac_H, mac_L);
  		displ_mac_addr (4, mac_H, mac_L);
		out_nl (4,"]");
 	b += 6;
	len -= 6;
 }

}




/*
  0x08 - target_serial_number 
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_target_serial_number (u_char *b)
{
 int len;

 // descriptor_tag	= b[0];
 len			= b[1];

 out_nl (4,"Serial Data Bytes:");
 printhexdump_buf (4,b+2,len);
}





/*
  0x09 - IP_address
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_target_IP_address (u_char *b)
{
 int    len;
 u_long ip;

 // descriptor_tag	= b[0];
 len			= b[1];

 ip = getBits (b, 0,  16, 32);
 out (4,"IPv4_addr_mask: %08lx [= ",ip);
  	displ_IPv4_addr (4, ip);
	out_nl (4,"]");
 b += 6;
 len -= 4;

 while (len > 0) {
 	ip  = getBits (b, 0,   0, 32);
 	out (4,"IPv4_addr: %08lx [= ",ip);
  		displ_IPv4_addr (4, ip);
		out_nl (4,"]");
 	b += 4;
	len -= 4;
 }

}





/*
  0x0A - IPv6_address
  ETSI EN 301 192  (ISO 13818-6)
*/

static struct IPv6ADDR *_getIPv6Addr (u_char *b, struct IPv6ADDR *x);

void descriptorDSMCC_target_IPv6_address (u_char *b)
{
 int    len;
 struct IPv6ADDR  x;

 // descriptor_tag	= b[0];
 len			= b[1];

 _getIPv6Addr (b, &x);
 out (4,"IPv6_addr_mask: %08lx%08lx%08lx%08lx [= ",
		 x.ip[0], x.ip[1], x.ip[2], x.ip[3] );
  	displ_IPv6_addr (4, &x);
	out_nl (4,"]");
 b += 18;
 len -= 16;

 while (len > 0) {
 	_getIPv6Addr (b, &x);
	out (4,"IPv6_addr_mask: %08lx%08lx%08lx%08lx [= ",
		 	x.ip[0], x.ip[1], x.ip[2], x.ip[3] );
	  	displ_IPv6_addr (4, &x);
		out_nl (4,"]");
 	b += 16;
	len -= 16;
 }

}


static struct IPv6ADDR *_getIPv6Addr (u_char *b, struct IPv6ADDR *x)
{
 	x->ip[0] = getBits (b, 0,   0, 32);
	x->ip[1] = getBits (b, 0,  32, 32);
	x->ip[2] = getBits (b, 0,  64, 32);
	x->ip[3] = getBits (b, 0,  96, 32);
	return x;
}




























/*
  0x0C - IP/MAC_platform_name_descriptor
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_IP_MAC_platform_name (u_char *b)
{
  int        len;
  u_char     ISO639_language_code[4];

  // descriptor_tag		 = b[0];
  len			       	 = b[1];


  getISO639_3 (ISO639_language_code, b+2);
  out_nl (4,"  ISO639_language_code:  %3.3s", ISO639_language_code);

  out (4,"Text: ");
 	print_name (4, b+2+3, len-3);
	out_NL (4);

}


/*
  0x0D - IP/MAC_platform_provider_name_descriptor
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_IP_MAC_platform_provider_name (u_char *b)
{
  descriptorDSMCC_IP_MAC_platform_name (b); // same encoding...
}




/*
  0x0E - target_MAC_address_range
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_target_MAC_address_range (u_char *b)
{
 int  len;
 long mac_H, mac_L;

 // descriptor_tag	= b[0];
 len			= b[1];
 b += 2;

 while (len > 0) {
 	mac_H 		= getBits (b, 0,   0, 24);
 	mac_L 		= getBits (b, 0,  24, 24);
 	out (4,"Mac_addr_low: %06lx%06lx [= ",mac_H, mac_L);
  		displ_mac_addr (4, mac_H, mac_L);
		out_nl (4,"]");

 	mac_H 		= getBits (b, 0,  48, 24);
 	mac_L 		= getBits (b, 0,  72, 24);
 	out (4,"Mac_addr_high: %06lx%06lx [= ",mac_H, mac_L);
  		displ_mac_addr (4, mac_H, mac_L);
		out_nl (4,"]");

 	b += 12;
	len -= 12;
 }

}






/*
  0x0F - IP_slash
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_target_IP_slash (u_char *b)
{
 int    len;

 // descriptor_tag	= b[0];
 len			= b[1];
 b += 2;

 while (len > 0) {
 	u_long ip;
	int    mask;

	ip   = outBit_Sx_NL (4,"IPv4_addr: ",        b, 0,32);
	mask = outBit_Sx_NL (4,"IPv4_slash_mask: ",  b,32, 8);
 	out (4,"  [= ");
		displ_IPv4_addr (4, ip);
		out_nl (4,"/%d]",mask);

 	b += 5;
	len -= 5;
 }

}






/*
  0x10 - IP_source_slash
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_target_IP_source_slash (u_char *b)
{
 int    len;

 // descriptor_tag	= b[0];
 len			= b[1];
 b += 2;

 while (len > 0) {
 	u_long ip;
	int    mask;

	ip   = outBit_Sx_NL (4,"IPv4_source_addr: ",        b, 0,32);
	mask = outBit_Sx_NL (4,"IPv4_source_slash_mask: ",  b,32, 8);
 	out (4,"  [= ");
		displ_IPv4_addr (4, ip);
		out_nl (4,"/%d]",mask);

	ip   = outBit_Sx_NL (4,"IPv4_dest_addr: ",          b,40,32);
	mask = outBit_Sx_NL (4,"IPv4_dest_slash_mask: ",    b,72, 8);
 	out (4,"  [= ");
		displ_IPv4_addr (4, ip);
		out_nl (4,"/%d]",mask);

 	b += 10;
	len -= 10;
 }

}






/*
  0x11 - IPv6_slash
  ETSI EN 301 192  (ISO 13818-6)
*/


void descriptorDSMCC_target_IPv6_slash (u_char *b)
{
 int    len;

 // descriptor_tag	= b[0];
 len			= b[1];
 b += 2;

 while (len > 0) {
 	struct IPv6ADDR  x;
	int    mask;

 	_getIPv6Addr (b, &x);
	out_nl (4,"IPv6_addr_mask: %08lx%08lx%08lx%08lx",
		 	x.ip[0], x.ip[1], x.ip[2], x.ip[3] );

	mask = outBit_Sx_NL (4,"IPv6_slash_mask: ",  b,128, 8);
 	out (4,"  [= ");
		displ_IPv6_addr (4, &x);
		out_nl (4,"/%d]",mask);

 	b += 17;
	len -= 17;
 }

}





/*
  0x12 - IPv6_source_slash
  ETSI EN 301 192  (ISO 13818-6)
*/

void descriptorDSMCC_target_IPv6_source_slash (u_char *b)
{
 int    len;

 // descriptor_tag	= b[0];
 len			= b[1];
 b += 2;

 while (len > 0) {
 	struct IPv6ADDR  x;
	int    mask;

 	_getIPv6Addr (b, &x);
	out_nl (4,"IPv6_source_addr: %08lx%08lx%08lx%08lx",
		 	x.ip[0], x.ip[1], x.ip[2], x.ip[3] );

	mask = outBit_Sx_NL (4,"IPv6_source_slash_mask: ",  b,128, 8);
 	out (4,"  [= ");
		displ_IPv6_addr (4, &x);
		out_nl (4,"/%d]",mask);

 	b += 17;
	len -= 17;


 	_getIPv6Addr (b, &x);
	out_nl (4,"IPv6_dest_addr: %08lx%08lx%08lx%08lx",
		 	x.ip[0], x.ip[1], x.ip[2], x.ip[3] );

	mask = outBit_Sx_NL (4,"IPv6_dest_slash_mask: ",  b,128, 8);
 	out (4,"  [= ");
		displ_IPv6_addr (4, &x);
		out_nl (4,"/%d]",mask);

 	b += 17;
	len -= 17;

 }

}


















