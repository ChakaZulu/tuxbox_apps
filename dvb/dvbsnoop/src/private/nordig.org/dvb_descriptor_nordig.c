/*
$Id: dvb_descriptor_nordig.c,v 1.1 2008/08/29 20:00:06 obi Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2006   Rainer.Scherg@gmx.de (rasc)



 -- Private DVB Descriptors  nordig.org



$Log: dvb_descriptor_nordig.c,v $
Revision 1.1  2008/08/29 20:00:06  obi
decode NorDig logic(al) channel descriptor (e.g. used by unitymedia)

Revision 1.4  2006/04/04 17:16:54  rasc
finally fix typo in premiere descriptor name

Revision 1.3  2006/01/02 18:24:16  rasc
just update copyright and prepare for a new public tar ball

Revision 1.2  2004/11/04 19:21:11  rasc
Fixes and changes on "premiere.de" private sections
Cleaning up "premiere.de" private descriptors (should be final now)

Revision 1.1  2004/11/03 21:01:02  rasc
 - New: "premiere.de" private tables and descriptors (tnx to Peter.Pavlov, Premiere)
 - New: cmd option "-privateprovider <provider name>"
 - New: Private provider sections and descriptors decoding
 - Changed: complete restructuring of private descriptors and sections


*/


#include "dvbsnoop.h"
#include "dvb_descriptor_nordig.h"
#include "strings/dvb_str.h"
#include "misc/hexprint.h"
#include "misc/output.h"




/*
 *
 * Private DVB descriptors
 * User Space: nordig.org
 *
 */





/*
   0x83  Logic Channel Descriptor
   NorDig-Basic ver 1.0.1, 01.07.2003
*/

void descriptor_PRIVATE_NordigORG_LogicChannelDescriptor (u_char *b)
{
	unsigned int tag = b[0];
	unsigned int len = b[1];
	unsigned int i;

	out_nl(4, "--> NorDig Logic Channel Descriptor ");

	b += 2;

	indent(+1);

	for (i = 0; i < len; i += 4) {
		unsigned int service_id = (b[i] << 8) | b[i + 1];
		unsigned int visible_service_flag = (b[i + 2] >> 7) & 1;
		unsigned int reserved = (b[i + 2] >> 6) & 1;
		unsigned int logic_channel_number = ((b[i + 2] << 8) | b[i + 3]) & 0x3fff;

		out_NL(5);
		out_SW_NL(5, "service_id: ", service_id);
		out_SB_NL(5, "visible_service_flag: ", visible_service_flag);
		out_SB_NL(5, "reserved: ", reserved);
		out_SW_NL(5, "logic_channel_number: ", logic_channel_number);
	}

	indent(-1);
	out_NL(4);
}

