/*
$Id: premiere_de.h,v 1.1 2004/11/03 21:01:02 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)


  -- Private Data Structures for:
  -- Premiere.de 



$Log: premiere_de.h,v $
Revision 1.1  2004/11/03 21:01:02  rasc
 - New: "premiere.de" private tables and descriptors (tnx to Peter.Pavlov, Premiere)
 - New: cmd option "-privateprovider <provider name>"
 - New: Private provider sections and descriptors decoding
 - Changed: complete restructuring of private descriptors and sections



*/



#ifndef _PREMIERE_DE_H
#define _PREMIERE_DE_H_


#include "private/userdefs.h"


void getPrivate_PremiereDE ( PRIV_SECTION_ID_FUNC **psect,
		PRIV_DESCR_ID_FUNC **pdesc);



#endif




