/*
$Id: rnt.c,v 1.1 2004/07/25 21:13:37 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 --  Resolution Notification Table (RNT)
 --  Resolution provider Notification Section
 --  TS 102 323



$Log: rnt.c,v $
Revision 1.1  2004/07/25 21:13:37  rasc
do not forget to commit new files
- RNT resolution_authority/provider_notification_section (TS 102 323)




*/




#include "dvbsnoop.h"
#include "rnt.h"

#include "descriptors/descriptor.h"
#include "strings/dvb_str.h"
#include "misc/output.h"





void decode_RNT (u_char *b, int len)
{
 /* TS 102 323  */

 u_int      len1,len2;
 u_int      table_id;
 u_int      section_length;



  out_nl (3,"RNT-decoding....");
  table_id = outBit_S2x_NL (3,"Table_ID: ",	b, 0, 8,
				(char *(*)(u_long))dvbstrTableID );
  if (table_id != 0x79) {
    out_nl (3,"wrong Table ID");
    return;
  }


  outBit_Sx_NL (3,"Section_syntax_indicator: ",	b, 8,  1);
  outBit_Sx_NL (6,"reserved_1: ",		b, 9,  1);
  outBit_Sx_NL (6,"reserved_2: ",		b,10,  2);
  section_length = outBit_Sx_NL (5,"section_length: ",	b,12,12);

  outBit_Sx_NL (3,"context_id: ",		b,24, 16);
  outBit_Sx_NL (6,"reserved_3: ",		b,40,  2);
  outBit_Sx_NL (3,"version_number: ",		b,42,  5);

  outBit_S2x_NL(3,"Current_next_indicator: ",	b,47,  1,
			(char *(*)(u_long))dvbstrCurrentNextIndicator );
  outBit_Sx_NL (3,"Section_number: ",		b,48,  8);
  outBit_Sx_NL (3,"Last_section_number: ",	b,56,  8);


  outBit_Sx_NL (3,"context_id_type: ",		b,64,  8);  //$$$ TODO   Table 2



  len2 = outBit_Sx_NL (3,"common_descriptor_length: ",	b,72, 12);
  outBit_Sx_NL (6,"reserved_3: ",			b,84,  4);

  b += 11;
  len1 = section_length - 8;


  // common descriptor loop 

  out_nl (3,"Common_descriptor_loop:");
  indent (+1);
  while (len2 > 0) {
	 int i;
	 i   = descriptor (b, TVA_RNT);
	 b    += i;
	 len1 -= i;
	 len2 -= i;
  }
  indent (-1);







// $$$ TODO ...
out_nl (1," .... lots here missing...");




  outBit_Sx_NL (5,"CRC: ",	b,0,32);
}








/*

resolution_authority_notification_section() {		
	table_id	8	uimsbf
	section_syntax_indicator	1	bslbf
	reserved	1	bslbf
	reserved	2	bslbf
	section_length	12	uimsbf

	context_id	16	uimsbf
	reserved	2	bslbf
	version_number	5	uimsbf
	current_next_indicator	1	bslbf
	section_number	8	uimsbf
	last_section_number	8	uimsbf
	context_id_type	8	uimsbf
	common_descriptors_length	12	uimsbf
	reserved	4	bslbf
	for (i=0; i<N1; i++) {		
		descriptor()		
	}		
....
	for (i<0; i<N2; i++) {		
		resolution_provider_info_length	12	uimsbf
		reserved	4	bslbf
		resolution_provider_name_length	8	uimsbf
		for (j<0; j<resolution_provider_name_length; j++) {		
			resolution_provider_name_byte	8	uimsbf
		}		
		resolution_provider_descriptors_length	12	uimsbf
		reserved	4	bslbf
		for (j=0; j<N3; j++) {		
			descriptor() 		
		}		
		for (j=0; j<N4; j++) {		
			CRID_authority_name_length	8	uimsbf
			for (k<0; k<CRID_authority_name_length; k++) {		
				CRID_authority_name_byte	8	uimsbf
			}		
			CRID_authority_descriptors_length	12	uimsbf
			reserved	4	bslbf
			for (k=0; k<N5; k++) {		
				CRID_authority_descriptor() 		
			}		
		}		
	}		
	CRC_32	32	rpchof
}		


*/
