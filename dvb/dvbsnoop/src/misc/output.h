/*
$Id: output.h,v 1.9 2004/01/02 16:40:37 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de (rasc)





$Log: output.h,v $
Revision 1.9  2004/01/02 16:40:37  rasc
DSM-CC  INT/UNT descriptors complete
minor changes and fixes

Revision 1.8  2004/01/01 20:09:26  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.7  2003/11/26 23:54:48  rasc
-- bugfixes on Linkage descriptor

Revision 1.6  2003/11/26 19:55:33  rasc
no message

Revision 1.5  2003/11/26 16:27:46  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.4  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/


#ifndef __OUTPUT_H
#define __OUTPUT_H 1


void indent (int v);
void setVerboseLevel (int v);
int  getVerboseLevel ();
void out(int verbose, const char *msgfmt,...);
void out_nl(int verbose, const char *msgfmt,...);
void out_nl2(int verbose);
void print_indent(void);


#define out_NL(v)      out_nl2(v)


// L = long     (32 bit)   output
// T = TriByte  (24 bit)
// W = Word     (16 bit)
// B = Byte     ( 8 bit) 

/* out "128 (0x80)"  */
#define out_L(v,hex)   out((v),"%lu (0x%08lx)",(hex),(hex))
#define out_T(v,hex)   out((v),"%lu (0x%06x)",(hex),(hex))
#define out_W(v,hex)   out((v),"%u (0x%04x)",(hex),(hex))
#define out_B(v,hex)   out((v),"%u (0x%02x)",(hex),(hex))


/* out "String 128 (=0x80)"  */
#define out_SL(v,str,hex)   out((v),"%s%lu (0x%08lx)",(str),(hex),(hex))
#define out_ST(v,str,hex)   out((v),"%s%lu (0x%06x)",(str),(hex),(hex))
#define out_SW(v,str,hex)   out((v),"%s%u (0x%04x)",(str),(hex),(hex))
#define out_SB(v,str,hex)   out((v),"%s%u (0x%02x)",(str),(hex),(hex))


/* out "String 128 (=0x80)\n"  */
#define out_SL_NL(v,str,hex)   out_nl((v),"%s%lu (0x%08lx)",(str),(hex),(hex))
#define out_ST_NL(v,str,hex)   out_nl((v),"%s%lu (0x%06x)",(str),(hex),(hex))
#define out_SW_NL(v,str,hex)   out_nl((v),"%s%u (0x%04x)",(str),(hex),(hex))
#define out_SB_NL(v,str,hex)   out_nl((v),"%s%u (0x%02x)",(str),(hex),(hex))


/* out "String 128 (=0x80)  [=string]\n"  */
#define out_S2L_NL(v,str,hex,str2)   out_nl((v),"%s%lu (0x%08lx)  [= %s]",(str),(hex),(hex),(str2))
#define out_S2T_NL(v,str,hex,str2)   out_nl((v),"%s%lu (0x%06x)  [= %s]",(str),(hex),(hex),(str2))
#define out_S2W_NL(v,str,hex,str2)   out_nl((v),"%s%u (0x%04x)  [= %s]",(str),(hex),(hex),(str2))
#define out_S2B_NL(v,str,hex,str2)   out_nl((v),"%s%u (0x%02x)  [= %s]",(str),(hex),(hex),(str2))




#endif


