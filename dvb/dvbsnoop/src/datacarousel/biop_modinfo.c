/*
$Id: biop_modinfo.c,v 1.4 2004/02/29 23:49:28 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)

 -- dsmcc  BIOP::ModuleInfo
 -- ISO/IEC 13818-6, ATSC a_47b




$Log: biop_modinfo.c,v $
Revision 1.4  2004/02/29 23:49:28  rasc
no message

Revision 1.3  2004/02/24 23:03:04  rasc
private data of DSMCC::DSI
BIOP::ServiceGatewayInformation()
IOP::IOR()

Revision 1.2  2004/02/20 23:13:17  rasc
BIOP:  TapUse

Revision 1.1  2004/02/20 22:21:57  rasc
DII complete (hopefully)
BIOP::ModuleInfo  (damned, who is spreading infos over several standards???)
maybe someone give me a hint on the selector_byte info!!!
some minor changes...






*/




#include "dvbsnoop.h"
#include "biop_modinfo.h"
#include "dsmcc_misc.h"

#include "misc/output.h"
#include "misc/hexprint.h"

#include "strings/dsmcc_str.h"
#include "strings/dvb_str.h"




/*
 * dsmcc BIOP::ModuleInfo(
 * ISO/IEC 13818-6  / ATSC a_47b
 * return: length used
 */


int BIOP_ModuleInfo (int v, u_char *b, u_int len_org)
{
   int   	len = len_org;
   int 		n1, i;


   	// -- due to some misbehavior of some service providers
   	// -- we do a simple plausi check for tap_counts_min_bytes > len
	i = getBits (b, 0, 96, 8);	// tap_counts
	i = i * 7;

	if (i > len_org) {	// this is no ModuleInfo
		print_databytes (v,"Data Bytes (non-standard):", b, len_org);
		return len_org;
	}




	out_nl (v, "BIOP::ModuleInfo:");

	indent (+1);
	outBit_S2Tx_NL (v,"ModuleTimeOut: ",	b,  0, 32, "(ms)");
	outBit_S2Tx_NL (v,"BlockTimeOut: ",	b, 32, 32, "(ms)");
	outBit_S2Tx_NL (v,"MinBlockTime: ",	b, 64, 32, "(ms)");
	n1 = outBit_Sx_NL (v,"taps_count: ",	b, 96,  8);

	b += 13,
	len -= 13;


	indent (+1);
	while (n1-- > 0) {
		int n2;

		out_NL (v);
		if (len <= 0) {
			out_nl (4, "... => strange len <= 0  and still tapcount > 0  (abort)");
			break;
		}

		outBit_Sx_NL (v,"Id: ",				b,  0, 16);
		outBit_S2x_NL(v,"Use: ",			b, 16, 16,
					(char *(*)(u_long))dsmccStrBIOP_TabUse );

		outBit_Sx_NL (v,"association_tag: ",		b, 32, 16);
		n2 = outBit_Sx_NL (v,"selector_length: ",	b, 48,  8);
		b += 7;
		len -= 7;

		if (n2 > 0) {
			// $$$ TODO who has some info to these selectorbytes ???
			print_databytes (v,"Selectorbytes:", b, n2);   // $$$ TODO  selector
		}
		// $$$ TODO  selector type ATSC
		// 0x0001 Message selector (defined in [DSM-CC]).    a_95  (where in DSM-CC defined?)
		// 0x109 TSFS selector (defined in this standard).   a_95
		//
		// ISO 13818-6:2000  5.6.1.1 Selector


		b += n2;
		len -= n2;

	}
	indent (-1);
	out_NL (v);


	n1 = outBit_Sx_NL (v,"userInfoLength: ",	b,   0,  8);
	// print_databytes (v,"UserInfoData:", b, n1); 

	dsmcc_CarouselDescriptor_Loop ("userInfo", b+1, n1);
	b   += 1+n1;
	len -= 1+n1;


	indent (-1);

	return len_org;
}







// BIOP::ModuleInfo::
// UserInfo
// The userInfo field contains a loop of descriptors. These are speci .ed in the
// DVB Data Broadcasting standard and/or this speci .cation.The MHP
// terminal shall support the compressed_module_descriptor (tag 0x09)used
// to signal that the module is transmitted in compressed form.The userInfo
// .eld may also contain a caching_priority_descriptor and one or more label_
// descriptors.  // DVB /This // spec.


// atsc a47
// The moduleInfoBytes field shall contain the BIOP::ModuleInfo
// structure. The BIOP::ModuleInfo structure provides additional
// delivery parameters and the Taps that are used to broadcast the
// Modules in the network. The syntax and semantics of the
// BIOP::ModuleInfo structure are shown below.

