/*
$Id: output.h,v 1.4 2003/07/08 19:59:50 rasc Exp $

  dvbsnoop
  (c) Rainer Scherg 2001-2003

  Output Module

$Log: output.h,v $
Revision 1.4  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/

void indent (int v);
void setVerboseLevel (int v);
int  getVerboseLevel ();
void out(int verbose, char *msgfmt,...);
void out_nl(int verbose, char *msgfmt,...);
void out_nl2(int verbose);
void print_indent(void);


#define out_NL(v)      out_nl2(v)


/* out "128 (0x80)"  */
#define out_L(v,hex)   out((v),"%lu (0x%08lx)",(hex),(hex))
#define out_W(v,hex)   out((v),"%u (0x%04x)",(hex),(hex))
#define out_B(v,hex)   out((v),"%u (0x%02x)",(hex),(hex))


/* out "String 128 (=0x80)"  */
#define out_SL(v,str,hex)   out((v),"%s%lu (0x%08lx)",(str),(hex),(hex))
#define out_SW(v,str,hex)   out((v),"%s%u (0x%04x)",(str),(hex),(hex))
#define out_SB(v,str,hex)   out((v),"%s%u (0x%02x)",(str),(hex),(hex))


/* out "String 128 (=0x80)\n"  */
#define out_SL_NL(v,str,hex)   out_nl((v),"%s%lu (0x%08lx)",(str),(hex),(hex))
#define out_SW_NL(v,str,hex)   out_nl((v),"%s%u (0x%04x)",(str),(hex),(hex))
#define out_SB_NL(v,str,hex)   out_nl((v),"%s%u (0x%02x)",(str),(hex),(hex))


/* out "String 128 (=0x80)  [=string]\n"  */
#define out_S2L_NL(v,str,hex,str2)   out_nl((v),"%s%lu (0x%08lx)  [= %s]",(str),(hex),(hex),(str2))
#define out_S2W_NL(v,str,hex,str2)   out_nl((v),"%s%u (0x%04x)  [= %s]",(str),(hex),(hex),(str2))
#define out_S2B_NL(v,str,hex,str2)   out_nl((v),"%s%u (0x%02x)  [= %s]",(str),(hex),(hex),(str2))



