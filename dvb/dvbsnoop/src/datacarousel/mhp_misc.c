/*
$Id: mhp_misc.c,v 1.1 2004/02/10 22:57:52 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)




$Log: mhp_misc.c,v $
Revision 1.1  2004/02/10 22:57:52  rasc
MHP descriptor, missing DVB descriptor done



*/




#include "dvbsnoop.h"
#include "mhp_misc.h"
#include "strings/dvb_str.h"
#include "strings/dsmcc_str.h"
#include "misc/output.h"




/*
 *  used in AIT and in descriptors
 *  ETSI TS 102 812
 */

int  mhp_application_identifier (int  v, u_char *b)
{
 	outBit_Sx_NL  (v,"organisation_id: ",	b,  0, 32);
 	outBit_S2x_NL (v,"appliction_id: ",	b, 32, 16,
			(char *(*)(u_long)) dsmccStrMHP_application_id );
	return 6;
}




