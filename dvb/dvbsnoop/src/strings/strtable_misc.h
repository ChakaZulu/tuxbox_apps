/*
$Id: strtable_misc.h,v 1.1 2004/07/24 11:47:08 rasc Exp $ 


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de



*/


#ifndef __STRTABLE_MISC_H
#define __STRTABLE_MISC_H


#include "dvbsnoop.h"



typedef struct _STR_TABLE {
    u_int    from;          /* e.g. from id 1  */
    u_int    to;            /*      to   id 3  */
    u_char   *str;          /*      is   string xxx */
} STR_TABLE;




char *findTableID (STR_TABLE *t, u_int id);


#endif




