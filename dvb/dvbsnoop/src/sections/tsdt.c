/*
$Id: tsdt.c,v 1.4 2003/10/24 22:17:21 rasc Exp $

   -- TSDT section
   -- Transport Stream Description Section
   -- ISO 13818

   (c) rasc


$Log: tsdt.c,v $
Revision 1.4  2003/10/24 22:17:21  rasc
code reorg...

Revision 1.3  2003/10/19 13:54:25  rasc
-more table decoding

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/




#include "dvbsnoop.h"
#include "tsdt.h"
#include "descriptors/descriptor.h"
#include "strings/dvb_str.h"
#include "misc/output.h"
#include "misc/hexprint.h"



void decode_TSDT (u_char *b, int len)
{

	/* $$$$$$  TODO */

 out_nl (3,"TSDT-decoding....");
 out_nl (3," Transport Stream Description Section (MPEG2) --- not implemented");
 out_nl (3," Report, if you need it...");
 out_NL (3);

}


