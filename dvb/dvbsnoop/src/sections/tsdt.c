/*
$Id: tsdt.c,v 1.1 2001/09/30 13:05:20 rasc Exp $

   -- TSDT section
   -- Transport Stream Description Section
   -- ISO 13818

   (c) rasc


$Log: tsdt.c,v $
Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/




#include "dvbsnoop.h"
#include "tsdt.h"



void decode_TSDT (u_char *b, int len)
{

 out_nl (3,"TSDT-decoding....");
 out_nl (3," Transport Stream Description Section (MPEG2) --- not implemented");
 out_nl (3," Report, if you need it...");
 out_NL (3);

}


