/*
$Id: dsmcc_misc.c,v 1.6 2004/02/14 01:24:44 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)




$Log: dsmcc_misc.c,v $
Revision 1.6  2004/02/14 01:24:44  rasc
DSM-CC started  (DSI/DII, DDB)

Revision 1.5  2004/01/02 22:25:34  rasc
DSM-CC  MODULEs descriptors complete

Revision 1.4  2004/01/02 16:40:33  rasc
DSM-CC  INT/UNT descriptors complete
minor changes and fixes

Revision 1.3  2004/01/01 20:09:16  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.2  2003/12/27 14:35:00  rasc
dvb-t descriptors
DSM-CC: SSU Linkage/DataBroadcast descriptors

Revision 1.1  2003/12/26 23:27:39  rasc
DSM-CC  UNT section




*/




#include "dvbsnoop.h"
#include "dsmcc_misc.h"
#include "descriptors/descriptor.h"
#include "strings/dvb_str.h"
#include "strings/dsmcc_str.h"
#include "misc/output.h"
#include "misc/hexprint.h"



static int subDescriptor (u_char *b);





/*
 * -- dsmcc descriptor loops  (name, buffer)
 * --- P latform_descriptors
 * --- T arget_descriptors
 * --- O perational descriptors
 * -- return: len
 */

int dsmcc_pto_descriptor_loop (u_char *name, u_char *b)
{
   int loop_length;
   int len,i;


   out_nl (3,"%s_descriptor_loop:",name);
   indent (+1);

     outBit_Sx_NL (6,"reserved: ",		b,0,4);

     out (4,name);
     loop_length = outBit_Sx_NL (4,"_loop_length: ",	b,4,12);
     len = loop_length;
     b += 2;

     indent (+1);
     while (len > 0) {
	 i   = descriptor (b, DSMCC_INT_UNT);
	 b   += i;
	 len -= i;
	
     }

   indent (-2);
   return  (loop_length +2);
}






/*
 * ETSI TS 102 006 V1.2.1 (2002-10)
 * ISO/IEC 13818-6
 * This is a special descriptor loop
 */

int dsmcc_CompatibilityDescriptor(u_char *b)
{
   int  len;
   int  count;


   out_nl (3,"DSMCC_Compatibility Descriptor (loop):");
   indent (+1);
   len   = outBit_Sx_NL (4,"compatibilityDescriptorLength: ",	b, 0,16);
   count = outBit_Sx_NL (4,"DescriptorCount: ",			b,16,16);
   b += 4;
   len -= 4;


   while (count-- > 0) {
	int  subDesc_count;

	out_nl (4,"Descriptor (loop):");
	indent (+1);

   	outBit_S2x_NL (4,"descriptorType: ",		b, 0, 8,
			(char *(*)(u_long))dsmccStr_DescriptorType );
   	outBit_Sx_NL (4,"descriptorLength: ",		b, 8, 8);

   	outBit_S2x_NL (4,"specifierType: ",		b,16, 8,
			(char *(*)(u_long))dsmccStr_SpecifierType );
   	outBit_S2x_NL (4,"specifierData: ",		b,24,24,
			(char *(*)(u_long))dsmccStrOUI );
   	outBit_Sx_NL (4,"Model: ",			b,48,16);
   	outBit_Sx_NL (4,"Version: ",			b,64,16);

   	subDesc_count = outBit_Sx_NL (4,"SubDescriptorCount: ", b,80, 8);
	b    += 11;

	while (subDesc_count > 0) {
		int  i;

		out_nl (5,"SubDescriptor (loop):");
		indent (+1);
		i = subDescriptor (b);
   		indent (-1);
	}
   	indent (-1);
   }


   indent (-1);
   return len;
}





static int subDescriptor (u_char *b)

{
  int len;

  outBit_Sx_NL (5,"SubDescriptorType: ", 	b, 0, 8); 
  len = outBit_Sx_NL (5,"SubDescriptorlength: ",b, 8, 8);

  print_databytes (4,"Additional Information:", b+2, len);

  return len + 2;
}












/*
 * ISO/IEC 13818-6
 * dsmccMessageHeader() 
 */

int dsmcc_MessageHeader (int v, u_char *b, int len, int *msg_len,
				int *dsmccType, int *messageId)
{
   u_char *b_start = b;
   int    adapt_len;


	out_nl (v, "DSM-CC Message Header:");
	indent (+1);
  	outBit_Sx_NL (v,"protocolDiscriminator: ", 	b  , 0, 8);   // $$$ TODO table ? has to be 0x11 here

  	*dsmccType = outBit_S2x_NL (4,"dsmccType: ",	b+1, 0, 8,
			(char *(*)(u_long))dsmccStr_dsmccType);
  
  	*messageId = outBit_S2x_NL (v,"messageId: ", 	b+2, 0, 16,
			(char *(*)(u_long))dsmccStr_messageID);	
	dsmcc_print_transactionID_32 (v, b+4);
  	outBit_Sx_NL (v,"reserved: ",	 		b+8, 0,  8);
  	adapt_len = outBit_Sx_NL (v,"adaptionLength: ", b+9, 0,  8);
  	*msg_len  = outBit_Sx_NL (v,"messageLength: ", 	b+10,0, 16);
	b += 12;
	// len -= 12;

	if (adapt_len > 0) {
		int x;

		x = dsmcc_AdaptationHeader (v, b, adapt_len);
		b += x;
		// len -= x;
	}

	indent (-1);

	return b - b_start;
}









/*
 * ISO/IEC 13818-6
 * dsmccAdaptationHeader() 
 */

int dsmcc_AdaptationHeader (int v, u_char *b, int len)
{
   int  ad_type;
   int  len_org = len;

 
	out_nl (v, "Adaptation Header:");
 	ad_type = outBit_S2x_NL (4,"adaptationType: ",	b, 0, 8,
			(char *(*)(u_long))dsmccStr_adaptationType);
	b++;
	len--;

	out_NL (v);
	indent (+1);
	switch (ad_type) {

		case 0x01: 		// conditional Access
			dsmcc_ConditionalAccess (v, b, len);
			break;

		case 0x02: 		// user ID
			dsmcc_UserID (v, b, len);
			break;

		default:
  			print_databytes (v,"adaptationDataByte:", b, len);
			break;
	}
	indent (-1);
	out_NL (v);


	return len_org;
}




/*
 * ISO/IEC 13818-6
 * dsmccConditionalAccess() 
 */

int dsmcc_ConditionalAccess (int v, u_char *b, int len)
{
   int  len2;

	out_nl (v, "Conditional Acess:");
	indent (+1);
  	outBit_Sx_NL  (v,"reserved: ",	 	b  , 0,  8);
 	outBit_S2x_NL (v,"caSystemId: ",	b+1, 0, 16,
			(char *(*)(u_long)) dvbstrCASystem_ID);

  	len2 = outBit_Sx_NL  (v,"conditionalAccessLength: ", 	b+3, 0, 16);
	print_databytes (v,"conditionaAccessDataByte:", b+5, len2);

	indent (-1);
	return (5 + len2);
}




/*
 * ISO/IEC 13818-6
 * dsmccUserID () 
 */

int dsmcc_UserID (int v, u_char *b, int len)
{
	out_nl (v, "User ID:");
	indent (+1);
  	outBit_Sx_NL  (v,"reserved: ",	 	b  , 0,  8);

	print_databytes (v,"UserId:", b+1, 20);	// $$$ TODO detail decode??

	indent (-1);
	return 21;
}



/*
 * print transactionID detail
 * ISO/IEC 13818-6
 * TS 102 812 v1.2.1  B.2.7
 * split transactionID in parts and print
 */

int dsmcc_print_transactionID_32 (int v, u_char *b)
{
  	outBit_Sx_NL  (v,"transactionID: ", 		b,  0, 32);

  	outBit_S2x_NL (v,"  ==> originator: ", 		b,  0,  2,
			(char *(*)(u_long)) dsmccStr_transactionID_originator);
  	outBit_Sx_NL  (v,"  ==> version: ", 		b,  2, 14);
  	outBit_Sx_NL  (v,"  ==> identification: ", 	b, 16, 15);
  	outBit_Sx_NL  (v,"  ==> update toggle flag: ", 	b, 31,  1);

	return 4;
}





