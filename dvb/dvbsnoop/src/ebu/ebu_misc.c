/*
$Id: ebu_misc.c,v 1.1 2004/02/04 22:36:27 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)



 -- misc. routines for
 -- EBU data (see EN 300 472, EN 301 775, and some more)
 -- Teletext, VPS, WSS, closed caption, etc.




$Log: ebu_misc.c,v $
Revision 1.1  2004/02/04 22:36:27  rasc
more EBU/teletext stuff






*/




#include "dvbsnoop.h"
#include "ebu_misc.h"
#include "strings/dvb_str.h"
#include "misc/helper.h"
#include "misc/hexprint.h"
#include "misc/output.h"





// -- output EBU reserved, field_parity, line_offset
// -- length: 1 byte

void ebu_rfl_out (int v, u_char *b)
{
   	outBit_Sx_NL (6,"reserved: ",		b, 0, 2);
   	outBit_Sx_NL (v,"field_parity: ",	b, 2, 1);
   	outBit_Sx_NL (v,"line_offset: ",	b, 3, 5);
}












