/*
$Id: tsdt.c,v 1.5 2003/10/29 20:54:57 rasc Exp $

   -- TSDT section
   -- Transport Stream Description Section
   -- ISO 13818

   (c) rasc


$Log: tsdt.c,v $
Revision 1.5  2003/10/29 20:54:57  rasc
more PES stuff, DSM descriptors, testdata



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


