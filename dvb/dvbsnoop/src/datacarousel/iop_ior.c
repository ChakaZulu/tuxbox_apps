/*
$Id: iop_ior.c,v 1.1 2004/02/24 23:03:04 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)

 -- dsmcc  Interoperable Object Reference (IOR)
 -- TR 101 202 v1.2.1  4.7.5.2




$Log: iop_ior.c,v $
Revision 1.1  2004/02/24 23:03:04  rasc
private data of DSMCC::DSI
BIOP::ServiceGatewayInformation()
IOP::IOR()





*/




#include "dvbsnoop.h"
#include "iop_ior.h"
#include "dsmcc_misc.h"

#include "misc/output.h"
#include "misc/hexprint.h"

#include "strings/dsmcc_str.h"
#include "strings/dvb_str.h"




/*
 * dsmcc IOP::IOR ()
 * TR 101 202 v 1.2.1
 * return: length used
 */


int IOP_IOR (int v, u_char *b)
{
   u_char       *b_start = b;
   int		i,x;
   u_long 	n1,n2;



	out_nl (v, "IOP::IOR:");
	indent (+1);


	n1 = outBit_Sx_NL (v,"type_id_length: ",	b,  0, 32);
	print_text_UTF8 (v, "type_id: ", b+4, n1);	// $$$ TODO
	b += 4+n1;


	// alignment gap  (CDR alignment rule), should be 0xFF
	x = n1 % 4;
	if (x) {
		print_databytes (v,"alignment_gap:", b, 4-x);
		b += 4-x;
	}


	n1 = outBit_Sx_NL (v,"taggedProfiles_count: ",	b,  0, 32);
	b += 4;
	for (i=0; i < n1; i++) {
		//  IOP_taggedProfile 

		outBit_S2x_NL (v,"profileId_tag: ",		b,   0, 32,
				(char *(*)(u_long)) dsmccStrIOP_ProfileID );
		n2 = outBit_Sx_NL (v,"profile_data_length: ",	b,  32, 32);

		// e.g. BIOPProfileBody
		// e.g. LiteOptionsProfileBody
		print_databytes (v,"profile_data:", b+8, n2);   // $$$ TODO
		
		b += 8+n2;
	}


	indent (-1);

	return b - b_start;
}









