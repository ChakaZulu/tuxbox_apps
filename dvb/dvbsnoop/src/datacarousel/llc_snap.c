/*
$Id: llc_snap.c,v 1.1 2003/11/26 20:02:31 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de




$Log: llc_snap.c,v $
Revision 1.1  2003/11/26 20:02:31  rasc
LLC-SNAP



*/


#include "dvbsnoop.h"
#include "llc_snap.h"
#include "misc/hexprint.h"
#include "misc/output.h"






/*
  -- decode LLC/SNAP
  -- ISO 8802, RFC 1042
  -- return: len
 */


int  llc_snap (int v, u_char *b)

{

  out_nl (v,"LLC/SNAP:");
  indent (+1);
  out_nl (v,"LLC:");
  outBit_Sx_NL (v," DSAP: ",  		b, 0, 8);
  outBit_Sx_NL (v," SSAP: ",  		b, 8, 8);
  outBit_Sx_NL (v," Control: ", 	b,16, 8);
  
  out_nl (v,"SNAP:");
  outBit_Sx_NL (v," Protocol ID/Ord Code: ", 	b,24,24);
  outBit_Sx_NL (v," EtherType: ", 		b,48,16);
  indent (-1);

  return 8;

}



