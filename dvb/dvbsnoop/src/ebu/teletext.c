/*
$Id: teletext.c,v 1.3 2004/02/04 22:36:27 rasc Exp $



 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)


 -- misc routines for EBU teletext




$Log: teletext.c,v $
Revision 1.3  2004/02/04 22:36:27  rasc
more EBU/teletext stuff

Revision 1.2  2004/02/03 00:11:49  rasc
no message

Revision 1.1  2004/02/02 23:37:54  rasc
added EBU module (teletext), providing basic teletext decoding



*/


#include "dvbsnoop.h"
#include "teletext.h"
#include "strings/dvb_str.h"
#include "misc/helper.h"
#include "misc/hexprint.h"
#include "misc/output.h"






// Important!
// ==========
// A Teletext packet comprises 360 bits organized as 45 bytes, numbered
// 1 to 45. In each byte, the bits are numbered 1 to 8 (LSB to MSB), and
// are normally transmitted LSB first.
// (against ETSI doc. bytes numbering: -4 diff)
//
// ==>
//  - the first 3 bytes are not part of this module
//    (anyway, the clock-run-in is not part of dvb) 
//    we start with Magazine_and_Packet_addr...
//  - before any decoding, the data packets has to be transformed
//    from LSB...MSB (bits)  to the internal CPU format.
//    Warning: Byte order will not be changed, only bit order in bytes!
//    ==>  so: LSB, MSB words habe still to be tranformed, if needed
//
// ==> 
//  - Some codes are protected by Hamming code 8/4.
//





// -- the following two tables and some routines basics
// -- are from dvbtext (c) Dave Chapman  and 
// -- originally from Ralph Metzler's vbidecode package. 
// -- other basic considerations are transfered from tuxtxt (c) LazyT


// LSb... MSb  -> MSb...LSb
static u_char invtab[256] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff, 
};


// -- Hamming 8/4 table
static u_char unham84tab[256] = {
  0x01, 0xff, 0x81, 0x01, 0xff, 0x00, 0x01, 0xff, 
  0xff, 0x02, 0x01, 0xff, 0x0a, 0xff, 0xff, 0x07, 
  0xff, 0x00, 0x01, 0xff, 0x00, 0x80, 0xff, 0x00, 
  0x06, 0xff, 0xff, 0x0b, 0xff, 0x00, 0x03, 0xff, 
  0xff, 0x0c, 0x01, 0xff, 0x04, 0xff, 0xff, 0x07, 
  0x06, 0xff, 0xff, 0x07, 0xff, 0x07, 0x07, 0x87, 
  0x06, 0xff, 0xff, 0x05, 0xff, 0x00, 0x0d, 0xff, 
  0x86, 0x06, 0x06, 0xff, 0x06, 0xff, 0xff, 0x07, 
  0xff, 0x02, 0x01, 0xff, 0x04, 0xff, 0xff, 0x09, 
  0x02, 0x82, 0xff, 0x02, 0xff, 0x02, 0x03, 0xff, 
  0x08, 0xff, 0xff, 0x05, 0xff, 0x00, 0x03, 0xff, 
  0xff, 0x02, 0x03, 0xff, 0x03, 0xff, 0x83, 0x03, 
  0x04, 0xff, 0xff, 0x05, 0x84, 0x04, 0x04, 0xff, 
  0xff, 0x02, 0x0f, 0xff, 0x04, 0xff, 0xff, 0x07, 
  0xff, 0x05, 0x05, 0x85, 0x04, 0xff, 0xff, 0x05, 
  0x06, 0xff, 0xff, 0x05, 0xff, 0x0e, 0x03, 0xff, 
  0xff, 0x0c, 0x01, 0xff, 0x0a, 0xff, 0xff, 0x09, 
  0x0a, 0xff, 0xff, 0x0b, 0x8a, 0x0a, 0x0a, 0xff, 
  0x08, 0xff, 0xff, 0x0b, 0xff, 0x00, 0x0d, 0xff, 
  0xff, 0x0b, 0x0b, 0x8b, 0x0a, 0xff, 0xff, 0x0b, 
  0x0c, 0x8c, 0xff, 0x0c, 0xff, 0x0c, 0x0d, 0xff, 
  0xff, 0x0c, 0x0f, 0xff, 0x0a, 0xff, 0xff, 0x07, 
  0xff, 0x0c, 0x0d, 0xff, 0x0d, 0xff, 0x8d, 0x0d, 
  0x06, 0xff, 0xff, 0x0b, 0xff, 0x0e, 0x0d, 0xff, 
  0x08, 0xff, 0xff, 0x09, 0xff, 0x09, 0x09, 0x89, 
  0xff, 0x02, 0x0f, 0xff, 0x0a, 0xff, 0xff, 0x09, 
  0x88, 0x08, 0x08, 0xff, 0x08, 0xff, 0xff, 0x09, 
  0x08, 0xff, 0xff, 0x0b, 0xff, 0x0e, 0x03, 0xff, 
  0xff, 0x0c, 0x0f, 0xff, 0x04, 0xff, 0xff, 0x09, 
  0x0f, 0xff, 0x8f, 0x0f, 0xff, 0x0e, 0x0f, 0xff, 
  0x08, 0xff, 0xff, 0x05, 0xff, 0x0e, 0x0d, 0xff, 
  0xff, 0x0e, 0x0f, 0xff, 0x0e, 0x8e, 0xff, 0x0e, 
};







// FROM vbidecode
// unham 2 bytes into 1, report 2 bit errors but ignore them
// -- input byte order:  LSB, MSB

u_char unham84 (u_char lsb, u_char msb)
{
  u_char c1,c2;
  
  c1=unham84tab[lsb];
  c2=unham84tab[msb];
  //  if ((c1 | c2) & 0x40)  bad ham!
  return (c2 << 4) | (c1 & 0x0f);
}



// -- simple unham24_18  (3 bytes -> 18 bit)
// -- teletext data needs to be 'normalized'

u_char unham24_18 (u_char lsb, u_char msb1, u_char msb2)
{
	// $$$ TODO
        u_char x = ' ';
	return x;
}




// -- reset parity bit 
// -- buffer needs to be "normalized" before.

void unParityTeletextData (u_char *b, int len)
{
  int i;

  for (i=0; i<len; i++) {
	*(b+i) = *(b+i) & 0x7F; 
  }
}





// -- Invert Character/Buffer (LSB/MSB)
//

u_char invertChar (u_char *b)
{
   return invtab[ *b ];
}


void invertBuffer (u_char *b, int len)
{
  int i;

  for (i=0; i<len; i++) {
	*(b+i) = invtab[ *(b+i) ];
  }
}






//
// -- decode & print magazine & packet address
// -- print page_nr, subpage_nr, if given
// -- !!! BUFFER BYTES HAVE TO BE ALREADY INVERTED!
// -- return: len of decoded bytes
//

int  print_teletext_control_decode (int v, u_char *b, int len)
{
  int  x;
  int  packet_nr = -1;
  int  mag_nr = -1;
  int  page_nr = -1;
  int  sub_page_nr = -1;


  	// -- buffer starts with magazine_and_packet_addr
	// -- len should be 42 bytes

	// PES_data_field(){
	// 	data_identifier 8 uimsbf
	//	for(i=0;i<N;i++){
	// 1:		 data_unit_id 8 uimsbf
	// 2:		 data_unit_length 8 uimsbf
	// 3:			 reserved_future_use 2 bslbf
	//			 field_parity 1 bslbf
	//			 line_offset 5 uimsbf
	// 4:			 framing_code 8 bslbf
	//		-->	 magazine_and_packet_address 16 bslbf
	//		-->	 data_block 320 bslbf
	//	 }


	
	// -- packet nr
	// -- magazine nr

	x = unham84(*b,*(b+1));
	packet_nr =  (x >> 3) & 0x1F;
	mag_nr = x & 7;
	if (! mag_nr) mag_nr = 8;

	out_SB_NL  (v,"magazine number (X): ",mag_nr);
	out_S2W_NL (v,"packet number (Y): ",packet_nr, dvbstrTELETEXT_packetnr(packet_nr) );



	// -- normal display packet...
	if (packet_nr > 0 && packet_nr <= 25) {

		unParityTeletextData (b+2, len-2);
		print_teletext_data_x0_x25 (v,"packet data (parity stripped):", b+2, len-2);

		return len;
	}


	// -- special packets?
	// -- Packets X/26, X/28 and M/29 can carry data to enhance a basic Level 1
	// -- Teletext page. The general coding scheme is shown in figure 11. Byte 6
	// -- is used as an additional address byte (designation code), coded Hamming
	// -- 8/4. This allows up to 16 versions of each packet type. The remaining
	// -- 39 bytes are Hamming 24/18 coded, grouped as 13 triplets.

	if (packet_nr > 25) {
		int  designation;

		designation = unham84tab[*(b+2)]  & 0x0F;
		out_SB_NL  (v,"designation code: ",designation);

		// $$$ TODO   hamming24_18 triplets
		print_databytes (4,"packet data (hamming 24/18):", b+3, len-3);
		return len;
	}



	// -- page header!! (Packet_nr == 0)
	// if (packet_nr != 0) return  max_len;


	// -- page nr, etc.
	// -- sub page nr


	// EN 300 706
	// The Page Address consists of a Page Number (see clause 9.3.1.1) and a
	// Page Sub-code (see clause 9.3.1.2). The Page Address may take any value
	// except: Page Number = FF and Sub-code = 3F7F. The address XFF:3F7F is
	// reserved as a null page address.

	// If a magazine has only one displayable page (and in some other
	// circumstances), it may be necessary to indicate the completion of the
	// transmission of that page by closing it with another page header packet.
	// Headers with pages addresses in the range XFF:0000 to XFF:3F7E are
	// defined for use for this purpose. These headers may be referred to as
	// "Time Filling Headers", when they are used to keep the real-time clock
	// field updated in a magazine in parallel transmission mode.


	page_nr     = (unham84tab[*(b+2)] & 0xF) | ( (unham84tab[*(b+3)]  & 0xF) << 4);
	sub_page_nr = ( unham84(*(b+4),*(b+5)) | (unham84(*(b+6),*(b+7)) << 8) ) & 0x3F7F;

	out_SW_NL (v,"page number: ",page_nr);
	out_SW_NL (v,"sub-page number: ",sub_page_nr);


	// -- special pages  (A.10)

	{
	   char *s = "";
	   int  x = (mag_nr<<8 | page_nr);

	   if (x == 0x1BE) s = "Automatic Channel Installation (ACI)";
	   if (x == 0x1F0) s = "Basic TOP Table (BTT)";
	   if (page_nr == 0xFD) s = "Magazine Inventory Page (MIP)";
	   if (page_nr == 0xFE) s = "Magazine Organization Table (MOT)";
	   if (page_nr == 0xFF) s = "Time filling and terminator";
	   if (page_nr == 0xFF && sub_page_nr == 0x3F7F) s = "Null packet";

	   if (*s) out_nl (v,"  ==> %s ",s);
	}
	

	out_nl (v,"  ==> teletext display page number: %x%02x/%x",mag_nr,page_nr,sub_page_nr);

	if (page_nr == 0xFF && sub_page_nr == 0x3F7F) return 8;

	

	// -- Control bits   S. 26 EN 300 706
	// -- don't do unham, get bits directly
	
	{
	  u_char x;
	  int    c4,c5,c6,c7,c8,c9,c10,c11;

	  c4 = *(b+5) & 0x80;			// bit 8
	  
	  x  = *(b+7);
	  c5 = x & 0x20;			// bit 6
	  c6 = x & 0x80;			// bit 7

	  x  = *(b+8);
	  c7 = x & 0x02;			// bit 2
	  c8 = x & 0x08;			// bit 4
	  c9 = x & 0x20;			// bit 6
	  c10= x & 0x80;			// bit 8

	  c11  = *(b+9) & 0x02;			// bit 2

	  if (c4|c5|c6|c7|c8|c9|c10|c11) {
		  out_nl (v,"Control bits:");
		  indent (+1);
			if (c4) out_nl(v,"C4 = Erase page");
			if (c5) out_nl(v,"C5 = Newsflash");
			if (c6) out_nl(v,"C6 = Subtitle");
			if (c7) out_nl(v,"C7 = Suppress header");
			if (c8) out_nl(v,"C8 = Update indicator");
			if (c9) out_nl(v,"C9 = Interrupted sequence");
			if (c10) out_nl(v,"C10 = Inhibit display");
			if (c11) out_nl(v,"C11 = Magazine serial");
		  indent (-1);
	  }
	  
	}


	// -- country/language code  (c12,c13,c14)

	{
	  int lang;

	  lang =  (unham84tab[*(b+9)]  >> 1) & 7;	// unhammed bits 4,6,8
	  out_S2B_NL (v,"Character subset (c12-c14): ",
			  lang, dvbstrTELETEXT_lang_code(lang) );
	}



	 // Bytes 14 to 45 in page header packets carry 32 character or display
	 // control codes, coded 7 data bits plus one bit odd parity. They are
	 // normally intended for display. Bytes 38 to 45 are usually coded to
	 // represent a real-time clock.


	// -- timestring, etc.

	unParityTeletextData (b+10, len-10);
	print_teletext_data_x0_x25 (v,"page header display string:", b+10, len-10);
	// out (v,"page header display string: ");
	// print_std_ascii (v, b+10, len-10);
	// out_NL (v);


	return len;
}





// -- display teletext data x0..x24

void print_teletext_data_x0_x25 (int v, char *s, u_char *b, int len)
{

  // $$$ TODO  -- decode display codes c < 0x20
  print_databytes (v,s, b, len);

}







// $$$ TODO  Packet 30/8
