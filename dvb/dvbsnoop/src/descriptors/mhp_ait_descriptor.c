/*
$Id: mhp_ait_descriptor.c,v 1.3 2004/02/10 22:57:54 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 -- Private TAG Space  MHP  AIT
 -- TS 102 812  v1.2.1  10.11




$Log: mhp_ait_descriptor.c,v $
Revision 1.3  2004/02/10 22:57:54  rasc
MHP descriptor, missing DVB descriptor done

Revision 1.2  2004/02/09 21:24:58  rasc
AIT descriptors
minor redesign on output routines

Revision 1.1  2004/02/07 01:28:01  rasc
MHP Application  Information Table
some AIT descriptors






*/


#include "dvbsnoop.h"
#include "descriptor.h"
#include "mhp_ait_descriptor.h"
#include "dvb_descriptor.h"
#include "datacarousel/mhp_misc.h"
#include "strings/dsmcc_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"





/*
  determine descriptor type and print it...
  !!! MHP AIT descriptors are in a private tag space !!!

  return byte length
*/

int  descriptorMHP_AIT (u_char *b)

{
 int len;
 int id;


  id  =  (int) b[0];
  len = ((int) b[1]) + 2;

  out_NL (4);
  out_S2B_NL (4,"MHP_AIT-DescriptorTag: ",id,
		  dsmccStrMHP_AIT_DescriptorTAG (id));
  out_SB_NL  (5,"Descriptor_length: ",b[1]);

  // empty ??
  len = ((int)b[1]) + 2;
  if (b[1] == 0)
	 return len;

  // print hex buf of descriptor
  printhex_buf (9, b,len);



  switch (b[0]) {

     case 0x00:  descriptorMHP_AIT_application (b); break;
     case 0x01:  descriptorMHP_AIT_application_name (b); break;
     case 0x02:  descriptorMHP_AIT_transport_protocol (b); break;
     case 0x03:  descriptorMHP_AIT_dvb_j_application (b); break;
     case 0x04:  descriptorMHP_AIT_dvb_j_application_location (b); break;
     case 0x05:  descriptorMHP_AIT_external_application_authorisation (b); break;
     // case 0x06: reserved
     // case 0x07: reserved
//     {  0x06, 0x06,  "Routing Descriptor IPv4" },
//     {  0x07, 0x07,  "Routing Descriptor IPv6" },
//     {  0x08, 0x08,  "DVB-HTML application descriptor" },
//     {  0x09, 0x09,  "DVB-HTML application location descriptor" },
//     {  0x0A, 0x0A,  "DVB-HTML application boundary descriptor" },
//     {  0x0B, 0x0B,  "Application icons descriptor" },
//     {  0x0C, 0x0C,  "Pre-fetch descriptor" },
//     {  0x0D, 0x0D,  "DII location descriptor" },
//     {  0x0E, 0x0E,  "delegated application descriptor" },
//     {  0x0F, 0x0F,  "Plug-in descriptor" },
//     {  0x10, 0x10,  "Application storage descriptor" },

     case 0x5F:  descriptorDVB_PrivateDataSpecifier (b); break;

     default: 
	if (b[0] < 0x80) {
	    out_nl (0,"  ----> ERROR: unimplemented descriptor (MHP_AIT context), Report!");
	}
	descriptor_any (b);
	break;
  } 


  return len;   // (descriptor total length)
}






//
// Unless otherwise specfied, all fields interpreted as text strings in the AIT
// shall be encoded as UTF8 (see 7.1.5, "Monomedia format for text"on page 54).
// See also 14.5, "Text encoding of application identifiers" on page 222.
//






/*
  0x00 - application
  ETSI  TS 102 812
*/

void descriptorMHP_AIT_application (u_char *b)
{
  int        len;
  int        len2;

  // descriptor_tag	= b[0];
  len		        = b[1];


 len2 = outBit_Sx_NL (4,"application_profile_length: ",	b, 16, 8);
 b += 3;
 len -= 1;

 indent (+1);
 while (len2 > 0) {
	out_NL(4);
 	outBit_Sx_NL (4,"application_profile: ",	b,   0, 16);
 	outBit_Sx_NL (4,"version.major: ",		b,  16,  8);
 	outBit_Sx_NL (4,"version.minor: ",		b,  24,  8);
 	outBit_Sx_NL (4,"version.micro: ",		b,  32,  8);
	b += 5;
	len -= 5;
	len2 -= 5;
 }
 out_NL(4);
 indent (-1);


 outBit_Sx_NL (4,"service_bound_flag: ",	b,   0,  1);
 outBit_S2x_NL(4,"visibility: ",		b,   1,  2,
	 	(char *(*)(u_long)) dsmccStrMHP_visibility_state );
 outBit_Sx_NL (4,"reserved: ",			b,   3,  5);
 outBit_Sx_NL (4,"application_priority: ",	b,   8,  8);
 b += 2;
 len -= 2;

 while (len > 0) {
 	outBit_Sx_NL (4,"transport_protocol_label: ",	b,   0, 16);
	b++;
	len--;
 }

}






/*
  0x01 - application name
  ETSI  TS 102 812
*/

void descriptorMHP_AIT_application_name (u_char *b)
{
  int        len;
  u_char     ISO639_language_code[4];


  // descriptor_tag	= b[0];
  len		        = b[1];
  b += 2;

  indent (+1);
  while (len > 0) {
	int len2;

	getISO639_3 (ISO639_language_code, b);
	out_nl (4,"ISO639_language_code:  %3.3s", ISO639_language_code);

 	len2 = outBit_Sx_NL (4,"application_name_length: ",	b, 24, 8);
 	print_text_468A (4, "application_name: ", b+4, len2);

	b += 4 + len2;
	len -= 4 + len2;

	out_NL (4);
  }
  indent (-1);

}








/*
  0x02 - transport_protocol
  ETSI  TS 102 812
*/

void descriptorMHP_AIT_transport_protocol (u_char *b)
{
  int  len;
  int  p_id;	


  // descriptor_tag	= b[0];
  len		        = b[1];

  p_id = outBit_S2x_NL (4,"protocol_id: ",	b, 16, 16,
	 	(char *(*)(u_long)) dsmccStrMHP_protocol_id);
  outBit_Sx_NL (4,"transport_protocol_label: ",	b, 32, 8);
  b += 5;
  len -= 3;


  // -- 0x0001 = Transport via OC
  // -- 0x0002 = Transport via IP
  // -- 0x0003 = Transport via interaction channel
  if (p_id == 0x0001 || p_id == 0x0002) {

	int   remote_conn;


  	remote_conn = outBit_Sx_NL (4,"remote_connection: ",	b,  0, 1);
  	              outBit_Sx_NL (6,"reserved: ",		b,  1, 7);
	b++;
	len--;

	if (remote_conn == 0x01) {
		// $$$ TODO reminder: this code part is used several time
		outBit_S2x_NL (4,"Original_network_id: ",	b,  0, 16,
			(char *(*)(u_long)) dvbstrOriginalNetwork_ID);
		outBit_Sx_NL  (4,"transport_stream_ID: ",	b, 16, 16);
		outBit_Sx     (4,"service_ID: ",		b, 32, 16);
			out_nl (4," --> refers to PMS program_number"); 
		b += 6;
		len -= 6;
	}


	if (p_id == 0x0001) {
  		// --  Transport via OC
		outBit_Sx_NL  (4,"component_tag: ",		b,  0,  8);
		b++;
		len--;
	} else {
  		// --  Transport via IP
		outBit_Sx_NL  (4,"alignment_indicator: ",	b,  0,  1);
		outBit_Sx_NL  (6,"reserved: ",			b,  1,  7);
		b++;
		len--;

		while (len > 0) {
			int len2;

			out_NL (4);
			len2 = outBit_Sx_NL (4,"URL_length: ",	b,  0,  8);
  			out (4,"URL: ");
	 		print_std_ascii (4, "URL: ", b+1, len2);
			b += 1+len2;
			len -= 1+len2;
		}

	}


  } if (p_id == 0x0003) {

	int len2;


	out_NL (4);
	len2 = outBit_Sx_NL (4,"URL_base_length: ",	b,  0,  8);
	print_std_ascii (4, "URL_base: ", b+1, len2);
	b += 1+len2;
	len -= 1+len2;


	while (len > 0) {
		int len2;

		out_NL (4);
		len2 = outBit_Sx_NL (4,"URL_extension_length: ", b,  0,  8);
		print_std_ascii (4, "URL_extension: ", b+1, len2);
		b += 1+len2;
		len -= 1+len2;
	}

  } else {
	// -- all other
  	print_databytes(4,"selector_bytes:", b, len); 

  }


}







/*
  0x03 -  DVB-J Application
  ETSI  TS 102 812
*/

void descriptorMHP_AIT_dvb_j_application (u_char *b)
{
  int  len;


  // descriptor_tag	= b[0];
  len		        = b[1];
  b += 2;

  indent (+1);
  while  (len > 0) {
	int len2;

  	len2 = outBit_Sx_NL (4,"parameter_length: ",	b, 0, 8);
	print_std_ascii (4, "Parameter: ", b+1, len2);

	b += len2+1;
	len -= len2+1;
  }
  indent (-1);
}





/*
  0x04 -  DVB-J Application Location
  ETSI  TS 102 812
*/

void descriptorMHP_AIT_dvb_j_application_location (u_char *b)
{
  int  len;
  int  len2;


  // descriptor_tag	= b[0];
  len		        = b[1];
  b += 2;


  len2 = outBit_Sx_NL (4,"base_directory_length: ",	b, 0, 8);
  print_std_ascii (4, "base_directory: ", b+1, len2);
  b += len2+1;
  len -= len2+1;

  len2 = outBit_Sx_NL (4,"classpath_extension_length: ",	b, 0, 8);
  print_std_ascii (4, "classpath_extension: ", b+1, len2);
  b += len2+1;
  len -= len2+1;

  print_std_ascii (4, "initial_class: ", b, len);

}






/*
  0x05 -  External Application Authorisation
  ETSI  TS 102 812
*/

void descriptorMHP_AIT_external_application_authorisation (u_char *b)
{
  int  len;


  // descriptor_tag	= b[0];
  len		        = b[1];
  b += 2;

  indent(+1);
  while (len > 0) {
	  int x;

	  x = mhp_application_identifier (4, b);
	  b += x;
	  len -= x;

  	  outBit_Sx_NL (4,"application_priority: ",	b, 0, 8);
	  b++;
	  len--;

	  out_NL (4);
  }
  indent(-1);

}






































