/*
$Id: dsmcc_grpinfind.c,v 1.1 2004/02/24 23:14:35 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)

 -- dsmcc GroupInfoIndication()
 -- (e.g. part of DSI private data)



$Log: dsmcc_grpinfind.c,v $
Revision 1.1  2004/02/24 23:14:35  rasc
DSI:: DSMCC::GroupInfoIndication






*/




#include "dvbsnoop.h"
#include "dsmcc_grpinfind.h"
#include "dsmcc_misc.h"
#include "misc/output.h"
#include "misc/hexprint.h"





/*
 * ISO/IEC 13818-6
 * EN 301 192
 * dsmcc GroupInfoIndication()
 */


int dsmcc_GroupInfoIndication (int v, u_char *b, u_int len)
{
   int   	len_org = len;
   int		n_groups;
   int 		i;
   int		len2;




	out_nl (v,"GroupInfoIndication:");
	indent (+1);
		n_groups = outBit_Sx_NL (v,"NumberOfGroups: ",	b,  0, 16);
		b += 2;
		len -= 2;

		for (i=0; i < n_groups; i++) {
			out_NL (v);
			out_nl (v, "Group (%d):",i);
			indent (+1);

			outBit_Sx_NL (v,"GroupId: ",	b,  0, 32);
			outBit_Sx_NL (v,"GroupSize: ",	b, 32, 32);
			b += 8;
			len -= 8;

			// GroupCompatibility()
			len2 = dsmcc_CompatibilityDescriptor (b);
			b += len2;
			len -= len2;

			len2 = outBit_Sx_NL (v,"GroupInfoLength: ",	b,  0, 16);
			// print_databytes (v, "GroupInfoBytes: ", b+2, len2);   // $$$ TODO 
			dsmcc_CarouselDescriptor_Loop ("GroupInfo", b+2, len2);
			b += 2+len2;
			len -= 2+len2;


			indent (-1);
		}

		// $$$ passt das hier?
		len2 = outBit_Sx_NL (v,"PrivateDataLength: ",	b,  0, 16);
		print_private_data (v, b+2, len2);   // $$$ What to do here?
		// b += 2+len2;
		// len -= 2+len2;

	indent (-1);
	out_NL (v);

	return len_org;
}



// EN 301 192:
// 8.1.2 DownloadServerInitiate message
// Supergroups in DSI





