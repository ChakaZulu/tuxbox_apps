/*
$Id: hexprint.c,v 1.3 2003/11/26 16:27:46 rasc Exp $

 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


$Log: hexprint.c,v $
Revision 1.3  2003/11/26 16:27:46  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>



#include "hexprint.h"
#include "output.h"



/*
 -- global static data

*/


int HexPrintmode = 0;







/*
   -- print-modus setzen
*/

void setHexPrintMode (int i)

{
  HexPrintmode = i;
}






/*
  - print buffer as Hex Printout
*/

void printhex_buf (int verbose, u_char *buf, int n)

{

   switch (HexPrintmode) {

      case 0:
	return;
	break;

      case 1:
	printhexdump_buf (verbose,buf,n);
	break;

      case 2:
	printhexline_buf (verbose,buf,n);
	break;

      case 3:
	printasciiline_buf (verbose,buf,n);
	break;


      default:
	printhexdump_buf (verbose,buf,n);
	break;
   }

   return;
}




/*
 -- multi line dump
*/
void printhexdump_buf (int verbose, u_char *buf, int n)
{
 int i, j;
 u_char c;
 int WID=16;

j = 0;
while (j*WID < n) {

 out (verbose,"  %04x:  ",j*WID);
 for (i=0; i<WID; i++) {
   if ( (i+j*WID) >= n) break;
   c = buf[i+j*WID];
   out (verbose," %c ",isprint((int)c) ?c:'.');
 }
 out_NL (verbose);
 out (verbose,"  %04x:  ",j*WID);

 for (i=0; i<WID; i++) {
   if ( (i+j*WID) >= n) break;
   c = buf[i+j*WID];
   out (verbose,"%02x ",(int)c);
 }
 out_NL (verbose);
 
 j++;

}


}




/*
 -- single line dump
*/
void printhexline_buf (int verbose, u_char *buf, int n)
{
 int i;

 for (i=0; i<n; i++) {
   out (verbose,"%02x ",(int)buf[i]);
 }
 out_NL (verbose);
}




/*
 -- single line ascii
*/
void printasciiline_buf (int verbose, u_char *buf, int n)
{
 int    i;
 u_char c;

 for (i=0; i<n; i++) {
   c = buf[i];
   out (verbose,"%c",isprint((int)c) ?c:'.');
 }
 out_NL (verbose);
}


