/*
$Id: dmx_error.c,v 1.1 2004/01/02 00:00:37 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de  (rasc)


 -- simple error output for DMX & file I/O




$Log: dmx_error.c,v $
Revision 1.1  2004/01/02 00:00:37  rasc
error output for buffer overflow



*/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>



void  IO_error (char *str)
{
   char *s;

   s = (str) ? str : "";

   switch (errno) {

	case EOVERFLOW:
		fprintf (stderr,"Error: %s: Buffer overflow, stream bandwidth to high\n",s);
		break;

	default:
		fprintf (stderr,"Error: ");
		perror(s);
		break;

   }

}
