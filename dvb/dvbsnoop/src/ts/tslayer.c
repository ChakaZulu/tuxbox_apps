/*
$Id: tslayer.c,v 1.7 2003/12/07 23:36:13 rasc Exp $

   -- Transport Stream Decode/Table section

   
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de





$Log: tslayer.c,v $
Revision 1.7  2003/12/07 23:36:13  rasc
pidscan on transponder
- experimental(!)

Revision 1.6  2003/11/26 16:27:48  rasc
- mpeg4 descriptors
- simplified bit decoding and output function

Revision 1.5  2003/11/24 23:52:18  rasc
-sync option, some TS and PES stuff;
dsm_addr inactive, may be wrong - due to missing ISO 13818-6

Revision 1.4  2003/10/24 22:17:24  rasc
code reorg...

Revision 1.3  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/




#include "dvbsnoop.h"
#include "tslayer.h"
#include "strings/dvb_str.h"
#include "misc/output.h"
#include "misc/hexprint.h"





void decodeTS_buf (u_char *b, int len, int pid)
{
 /* IS13818-1  2.4.3.2  */

 typedef struct  _TS_PackLayer {
    u_int      sync_byte;
    u_int      transport_error_indicator;		
    u_int      payload_unit_start_indicator;		
    u_int      transport_priority;		
    u_int      pid;		
    u_int      transport_scrambling_control;		
    u_int      adaption_field_control;		
    u_int      continuity_counter;		

    // adaption field
    // or N ... data bytes

 } TS_PackLayer;




 TS_PackLayer  t;
 int           n;



 t.sync_byte 			 = b[0];

 if (t.sync_byte != 0x47) {
     out_nl (3,"!!! SyncByte is wrong (= no TS)!!!\n");
     // $$$ return;
 }
 if (len != 188) {
   out_nl (3,"Fixed packet length is wrong (not 188)!!!\n");
 }
 
 t.transport_error_indicator	 = getBits (b, 0, 8, 1);
 t.payload_unit_start_indicator	 = getBits (b, 0, 9, 1);
 t.transport_priority		 = getBits (b, 0,10, 1);
 t.pid				 = getBits (b, 0,11, 13);
 t.transport_scrambling_control	 = getBits (b, 0,24, 2);
 t.adaption_field_control	 = getBits (b, 0,26, 2);
 t.continuity_counter		 = getBits (b, 0,28, 4);



 out_SB_NL (3,"Sync-Byte: ",t.sync_byte);
 out_SB_NL (3,"Transport_error_indicator: ",t.transport_error_indicator);
 out_SB_NL (3,"Payload_unit_start_indicator: ",t.payload_unit_start_indicator);
 out_SB_NL (3,"transport_priority: ",t.transport_priority);
 out_S2W_NL(3,"PID: ",t.pid, dvbstrTSpid_ID (t.pid) );

 out_S2B_NL (3,"transport_scrambling_control: ",
	t.transport_scrambling_control,
	dvbstrTS_ScramblingCtrl_TYPE (t.transport_scrambling_control) );

 out_S2B_NL (3,"adaption_field_control: ",
	t.adaption_field_control,
	dvbstrTS_AdaptionField_TYPE (t.adaption_field_control) );

 out_SB_NL (3,"continuity_counter: ",t.continuity_counter);


 len -= 4;
 b   += 4;


 if (t.adaption_field_control & 0x2) {
    indent (+1);

    n = ts_adaption_field (b);

    b   += n;
    len -= n;

    indent (-1);
 }

 
 if (t.adaption_field_control & 0x1) {
    indent (+1);

    out_nl (3,"Data-Bytes: (len: %d)",len);
    printhexdump_buf (4, b,len); 

    b   += n;
    len -= n;

    indent (-1);
 }

}






int ts_adaption_field (u_char  *b)

{

 typedef struct  _TS_AdaptionField {
    u_int      adaption_field_length;

    // if length > 0
    u_int      discontinuity_indicator;
    u_int      random_access_indicator;
    u_int      elementary_stream_priority_indicator;
    u_int      PCR_flag;
    u_int      OPCR_flag;
    u_int      splicing_point_flag;
    u_int      transport_private_data_flag;
    u_int      adaption_field_extension_flag;

    // PCR_flag == 1
    u_long     program_clock_reference_baseH;
    u_long     program_clock_reference_baseL;
    u_int      reserved1;
    u_int      program_clock_reference_extension;
   
    // OPCR_flag == 1
    u_long     original_program_clock_reference_baseH;
    u_long     original_program_clock_reference_baseL;
    u_int      reserved2;
    u_int      original_program_clock_reference_extension;

    // splicing_point_flag == 1
    u_int      splice_countdown;

    // transport_private_data_flag == 1
    u_int      transport_private_data_length;
	// N   data bytes

    // adaption_field_extension_flag == 1
   
	// N2  adaption_field_extension  

    // N stuffing bytes...



 } TS_AdaptionField;


 TS_AdaptionField  a;
 int               len,n;



 a.adaption_field_length   	 		= b[0];

 out_SB_NL (5,"Adaption_field_length: ",a.adaption_field_length);
 b  += 1;
 len = a.adaption_field_length;

 if (a.adaption_field_length == 0)
    return 1;


 a.discontinuity_indicator			= getBits (b, 0, 0, 1);
 a.random_access_indicator			= getBits (b, 0, 1, 1);
 a.elementary_stream_priority_indicator		= getBits (b, 0, 2, 1);
 a.PCR_flag					= getBits (b, 0, 3, 1);
 a.OPCR_flag					= getBits (b, 0, 4, 1);
 a.splicing_point_flag				= getBits (b, 0, 5, 1);
 a.transport_private_data_flag			= getBits (b, 0, 6, 1);
 a.adaption_field_extension_flag		= getBits (b, 0, 7, 1);

 b   += 1;
 len -= 1;

 out_SB_NL (3,"discontinuity_indicator: ",a.discontinuity_indicator);
 out_SB_NL (3,"random_access_indicator: ",a.random_access_indicator);
 out_SB_NL (3,"elementary_stream_priotity_indicator: ",a.elementary_stream_priority_indicator);
 out_SB_NL (3,"PCR_flag: ",a.PCR_flag);
 out_SB_NL (3,"OPCR_flag: ",a.OPCR_flag);
 out_SB_NL (3,"splicing_point_flag: ",a.splicing_point_flag);
 out_SB_NL (3,"transport_private_data_flag: ",a.transport_private_data_flag);
 out_SB_NL (3,"adaption_field_extension_flag: ",a.adaption_field_extension_flag);

  if (a.PCR_flag) {
     a.program_clock_reference_baseH		= getBits (b, 0, 0,  1);
     a.program_clock_reference_baseL		= getBits (b, 0, 1, 32);
     a.reserved1				= getBits (b, 0,33,  6);
     a.program_clock_reference_extension	= getBits (b, 0,39,  9);

     b   += 6;
     len -= 6;

     out_nl (3,"program_clock_reference_base: 0x%01lx%08lx",
     	a.program_clock_reference_baseH, a.program_clock_reference_baseL);
     out_SB_NL (6,"reserved: ",a.reserved1);
     out_nl (3,"program_clock_reference_extension: 0x%03lx",
     	a.program_clock_reference_extension);
  }


  if (a.OPCR_flag) {
     a.original_program_clock_reference_baseH	= getBits (b, 0, 0,  1);
     a.original_program_clock_reference_baseL	= getBits (b, 0, 1, 32);
     a.reserved2				= getBits (b, 0,33,  6);
     a.original_program_clock_reference_extension = getBits (b, 0,39,  9);

     b   += 6;
     len -= 6;

     out_nl (3,"original_program_clock_reference_base: 0x%01lx%08lx",
     	a.original_program_clock_reference_baseH,
	a.original_program_clock_reference_baseL);
     out_SB_NL (6,"reserved: ",a.reserved2);
     out_nl (3,"original_program_clock_reference_extension: 0x%03lx",
     	a.original_program_clock_reference_extension);
  }


  if (a.splicing_point_flag) {
     a.splice_countdown 			= b[0];

     b   += 1;
     len -= 1;

     out_SB_NL (3,"splice_countdown: ",a.splice_countdown);
  }


  if (a.transport_private_data_flag) {
     a.transport_private_data_length		= b[0];

     out_SB_NL (5,"transport_private_data_length: ",
	a.transport_private_data_length);
     out_nl (4,"Transport_private_data:");
     printhexdump_buf (4,b+1,a.transport_private_data_length);

     n = 1 + a.transport_private_data_length;
     b   += n;
     len -= n;
  }


   // Extension

  if (a.adaption_field_extension_flag) {
      indent (+1);

      n = ts_adaption_field_extension (b);
      b   += n;
      len -= n;

      indent (-1);
  }


   // Stuffing bytes

   if (len > 0) {
     out_nl (5,"(Stuffing_bytes length: %d) ",len);
     out_nl (4,"Stuffing_bytes:");
     printhexdump_buf (4,b,len);
   }


 return (a.adaption_field_length + 1);
}






int ts_adaption_field_extension (u_char  *b)

{

 typedef struct  _TS_AdaptionExtField {
    u_int      adaption_field_extension_length;
    u_int      ltw_flag;
    u_int      piecewise_rate_flag;
    u_int      seamless_splice_flag;
    u_int      reserved1;

    // if ltwflag == 1
    u_int      ltw_valid_flag;
    u_int      ltw_offset;

    // if piecewise_rate_flag
    u_int      reserved2;
    u_long     piecewise_rate;

    // if seamless_splice_flag
    u_int      splice_type;
    u_int      DTS_next_AU32_30;
    u_int      marker_bit1;
    u_int      DTS_next_AU29_15;
    u_int      marker_bit2;
    u_int      DTS_next_AU14_0;
    u_int      marker_bit3;

    // N ... Reserved

 } TS_AdaptionExtField;


 TS_AdaptionExtField  a;
 int                  len;


 a.adaption_field_extension_length  		= b[0];

 out_SB_NL (5,"Adaption_field_extension_length: ",
	a.adaption_field_extension_length);
 b  += 1;
 if (a.adaption_field_extension_length == 0)
    return 1;

 len = a.adaption_field_extension_length;


  a.ltw_flag				= getBits (b, 0, 0,  1);
  a.piecewise_rate_flag			= getBits (b, 0, 1,  1);
  a.seamless_splice_flag		= getBits (b, 0, 2,  1);
  a.reserved1				= getBits (b, 0, 3,  5);

  b   += 1;
  len -= 1;


  out_SB_NL (3,"ltw_flag: ",a.ltw_flag);
  out_SB_NL (3,"piecewise_rate_flag: ",a.piecewise_rate_flag);
  out_SB_NL (3,"seamless_splice_flag: ",a.seamless_splice_flag);
  out_SB_NL (6,"reserved: ",a.reserved1);


  if (a.ltw_flag) {
     a.ltw_valid_flag			= getBits (b, 0, 0,  1);
     a.ltw_offset			= getBits (b, 0, 1, 15);

     b   += 2;
     len -= 2;
  
     out_SB_NL (3,"ltw_valid_flag: ",a.ltw_valid_flag);
     out_SW_NL (3,"ltw_offset: ",a.ltw_offset);
  }

  if (a.piecewise_rate_flag) {
     a.reserved2			= getBits (b, 0, 0,  2);
     a.piecewise_rate			= getBits (b, 0, 2,  22);

     b   += 3;
     len -= 3;

     out_SB_NL (6,"reserved: ",a.reserved2);
     out_SL_NL (3,"piecewise_rate: ",a.piecewise_rate);
  }

  if (a.seamless_splice_flag) {
     a.splice_type			= getBits (b, 0, 0,  4);
     a.DTS_next_AU32_30			= getBits (b, 0, 4,  3);
     a.marker_bit1			= getBits (b, 0, 7,  1);
     a.DTS_next_AU29_15			= getBits (b, 0, 8, 15);
     a.marker_bit2			= getBits (b, 0,23,  1);
     a.DTS_next_AU14_0			= getBits (b, 0,24, 15);
     a.marker_bit3			= getBits (b, 0,39,  1);

     b   += 5;
     len -= 5;

//$$$ TODO types display ??
     out_SB_NL (3,"splice_type: ",a.splice_type);
     out_SB_NL (3,"DTS_next_AU[32..30]: ",a.DTS_next_AU32_30);
     out_SB_NL (3,"maker_bit: ",a.marker_bit1);
     out_SB_NL (3,"DTS_next_AU[29..15]: ",a.DTS_next_AU29_15);
     out_SB_NL (3,"maker_bit: ",a.marker_bit2);
     out_SB_NL (3,"DTS_next_AU[14..0]: ",a.DTS_next_AU14_0);
     out_SB_NL (3,"maker_bit: ",a.marker_bit3);
  }


  if (len > 0) {
     out_nl (6,"(Reserved Bytes length: %d) ",len);
     out_nl (6,"Reserved bytes:");
     printhexdump_buf (6,b,len);
  }


 return a.adaption_field_extension_length + 1;
}
