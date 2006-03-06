/*
$Id: biop_message.c,v 1.1 2006/03/06 00:04:49 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2006   Rainer.Scherg@gmx.de (rasc)

 -- dsmcc  BIOP:: xxxMessage
 -- ISO/IEC 13818-6
 -- ETSI TS 101 202
 -- ETSI TS 102 812
 -- ATSC A95
 -- ATSC A47




Log: biop_modinfo.c,v $




*/




#include "dvbsnoop.h"
#include "biop.h"
#include "biop_dsm.h"
#include "biop_tag_tap.h"
#include "iop_ior.h"

#include "descriptors/descriptor.h"
#include "strings/dsmcc_str.h"
#include "misc/output.h"



static u_long  body_FileMessage (int v, u_char *b);
static u_long  body_DirectoryMessage (int v, u_char *b);
static u_long  body_StreamMessage (int v, u_char *b);
static u_long  body_StreamEventMessage (int v, u_char *b);

static u_long  do_ServiceContentList (int v, u_char *b);







/*
  -- dsmcc BIOP::Message
  --  This is the basic BIOP message handler
  --  Handling the Message header part and 
  --  branching into detailed message handling is done
  --  by analyzing "objectKind_data"
  -- return: len
 */

u_long  BIOP_Message (int v, u_char *b)
{
  u_char   *b_org = b;
  u_long   len;
  int      bo;
  int      n1,n2;
  u_long   kind;



	print_databytes (v,"magic:",			b,   4); 
	outBit_Sx_NL (v,"biop_version.major: ",		b,  32,  8);
	outBit_Sx_NL (v,"biop_version.minor: ",		b,  40,  8);

   bo = outBit_S2x_NL (v,"byte_order: ",		b,  48,  8,
			(char *(*)(u_long)) dsmccStrBIOP_EndianType );
   if (bo != 0x00) {
	   out_nl (v, "==> Error: unsupported byte endian order");
   }


	outBit_Sx_NL (v,"message_type: ",		b,  56,  8);  //$$$ TODO ??
	outBit_Sx_NL (v,"message_size: ",		b,  64, 32);
  b += 12;

  n1  = outBit_Sx_NL (v,"objectKey_length: ",		b,  0,  8);
	print_databytes (v,"objectKey_data:",		b+1, n1); 
  b += 1 + n1;



  n2  = outBit_Sx_NL (v,"objectKind_length: ",		b,  0,  8);
  b += 1;
  kind = 0x00;
  if (n2 != 4) {
	print_databytes (v,"objectKind_data:",		b, n1); 
  } else {
	// -- ISO 13818-6: length == 4 bytes, use type_id aliases
 	kind = outBit_S2x_NL (v,"objectKind_data: ",	b, 0, 32,
			(char *(*)(u_long)) dsmccStrBIOP_TypeID_Alias );
  }
  b += n2;




  // -- select message processing,
  // -- due to "kind" aka type_id aliases
  // --  --> 3 character abbrev. terminated with \0
  switch (kind) {

	  case 0x64697200:		// "dir"   (DSM::Directory)
		len = body_DirectoryMessage (v, b);
		break;

	  case 0x66696c00:		// "fil"   (DSM:File)
		len = body_FileMessage (v, b);
		break;

	  case 0x73746500:		// "ste"   (DSM:StreamEvent)
		len = body_StreamEventMessage (v, b);
		break;

	  case 0x73746700:		// "stg"   (DSM:ServiceGateway)
		len = body_DirectoryMessage (v, b);
		break;

	  case 0x73747200:		// "str"   (BIOP::Stream)
		len = body_StreamMessage (v, b);
		break;

	  default:
		// ..............................................................................
		 out_nl (v, "unsopported BIOP  (abort).");  // $$$ TODO
		 exit (1);
		  break;
  }
  b += len;


  return  (u_long) (b - b_org);
}






/*
 *  BIOP::FileMessage
 *  decode message body
 *  return: len of body
 */

static u_long  body_FileMessage (int v, u_char *b)
{
  u_char   *b_org = b;
  u_long   nx,n2,n5;


   n2 = outBit_Sx_NL   (v,"objectInfo_length: ",	b,   0,  16);
  	outBit64_Sx_NL (v,"DSM::File::ContentSize: ",	b,  16,  64);
   b +=  10;
   n2 -=  8;
   out_nl (v, "Descriptor_loop:");
   indent (+1);
   while (n2 > 0) {
	int    x;

	x   = descriptor (b,  DSMCC_CAROUSEL);
	b  += x;
	n2 -= x;
   }
   indent (-1);


   nx = do_ServiceContentList (v, b);
   b += nx;


        outBit_Sx_NL   (v,"messageBody_length: ",	b,   0,   32);


   n5 = outBit_Sx_NL   (v,"content_length: ",		b,  32,   32);
 	print_databytes(v,"content__byte:", 		b+8, n5);
   b += 8 + n5;


   return (u_long)  (b - b_org);
}






/*
 *  BIOP::DirectoryMessage
 *  decode message body
 *  return: len of body
 */

static u_long  body_DirectoryMessage (int v, u_char *b)
{
  u_char   *b_org = b;
  int      i;
  u_int    nx,n2,n5,n7,n9;


   n2 = outBit_Sx_NL   (v,"objectInfo_length: ",	b,   0,  16);
 	print_databytes(v,"objectInfo_data:", 		b+2, n2);
   b += 2 + n2;


   nx = do_ServiceContentList (v, b);
   b += nx;


        outBit_Sx_NL   (v,"messageBody_length: ",	b,   0,   32);


   n5 = outBit_Sx_NL   (v,"bindings_count: ",		b,  32,   16);
   b += 6;
   indent (+1);
   for (i=0; i < n5; i++) {
	int    n6;
	u_long kinddata;

	n6 = BIOP_Name (v, b, &kinddata);
	b += n6;


	outBit_S2x_NL  (v,"BindingType: ",		b,   0,    8,
			(char *(*)(u_long)) dsmccStrBIOP_BindingType);
	b += 1;


	n7 = IOP_IOR (v, b);
	b += n7;


	n9 = outBit_Sx_NL   (v,"objectInfo_length: ",	b,   0,  16);
	b += 2;

	if (kinddata == 0x66696c00) {		// "fil"
	  	outBit64_Sx_NL (v,"DSM::File::ContentSize: ",	b,  16,  64);
		b  += 8;
		n9 -= 2;
	}

	print_databytes  (v,"descriptor_bytes:", 	b, n9);
	b += n9;

// $$$ TODO: to be checked...!!!
// 	out_nl (v, "Descriptor_loop:");
// 	indent (+1);
//	while (n9 > 0) {
//		int    x;
//
//		x   = descriptor (b,  DSMCC_CAROUSEL);
//		b  += x;
//		n9 -= x;
//	}
// 	indent (-1);
//

   } // loop n5
   indent (-1);


   return (u_long)  (b - b_org);
}






/*
 *  BIOP::StreamMessage
 *  decode message body
 *  return: len of body
 */

static u_long  body_StreamMessage (int v, u_char *b)
{
  u_char   *b_org = b;
   u_int    nx,n2,n6;


   n2 = outBit_Sx_NL   (v,"objectInfo_length: ",	b,   0,  16);
   nx = BIOP_DSM_Stream_Info_T (v, b);
   // $$$ Warning: Standards documents may describe wrong length calculation!
   	print_databytes  (v,"ObjectInfo_byte:",	 	b+nx, n2-nx);
   b += n2;
   

   nx = do_ServiceContentList (v, b);
   b += nx;


        outBit_Sx_NL   (v,"messageBody_length: ",	b,   0,   32);
   n6 = outBit_Sx_NL   (v,"tap_count: ",		b,  32,    8);
   b += 5;
   indent (+1);
   for (; n6 > 0; n6--) {
	int  ny;

	ny =  BIOP_TAP (v, "", b);
	b += ny;
   }
   indent (-1);


   return (u_long)  (b - b_org);
}






/*
 *  BIOP::StreamEventMessage
 *  decode message body
 *  return: len of body
 */

static u_long  body_StreamEventMessage (int v, u_char *b)
{
  u_char   *b_org = b;
   u_int    nx,ny,n2,n8,n4;


   n2 = outBit_Sx_NL   (v,"objectInfo_length: ",	b,   0,  16);
   nx = BIOP_DSM_Stream_Info_T (v, b);
   ny = BIOP_DSM_Event_EventList_T (v, b+nx);
   	print_databytes  (v,"ObjectInfo_byte:",	 	b+nx, n2-nx-ny);
   b += n2;
   

   nx = do_ServiceContentList (v, b);
   b += nx;


        outBit_Sx_NL   (v,"messageBody_length: ",	b,   0,   32);
   n8 = outBit_Sx_NL   (v,"tap_count: ",		b,  32,    8);
   b += 5;
   indent (+1);
   for (; n8 > 0; n8--) {
	int  nz;

	nz =  BIOP_TAP (v, "", b);
	b += nz;
   }
   indent (-1);


   n4 = outBit_Sx_NL   (v,"eventIds_count: ",		b,   0,    8);
   b += 1;
   indent (+1);
   for (; n4 > 0; n4--) {
   	outBit_Sx_NL   (v,"eventId: ",			b,   0,   16);
	b += 2;
   }
   indent (-1);


   return (u_long)  (b - b_org);
}








/*
 * some common routines parts here...
 * return: len
 */

static u_long  do_ServiceContentList (int v, u_char *b)
{
  u_char   *b_org = b;
  int      n3;



   n3 = outBit_Sx_NL   (v,"serviceContentList_count: ",	b,   0,   8);
   b += 1;

   indent (+1);
   for (; n3 > 0; n3--) {
	int    n4;

	     outBit_Sx_NL (v,"context_id: ",		b,   0,  32);
	n4 = outBit_Sx_NL (v,"context_data_length: ",	b,  32,  8);
 	     print_databytes  (v,"context_data_byte:", 	b+5, n4);
	b += 5 + n4;

   }
   indent (-1);
	

   return (u_long) (b - b_org);
}



