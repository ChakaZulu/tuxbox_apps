/*
$Id: mpeg2_video.c,v 1.2 2005/11/23 23:06:10 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2005   Rainer.Scherg@gmx.de  (rasc)


 ISO 13818-2



$Log: mpeg2_video.c,v $
Revision 1.2  2005/11/23 23:06:10  rasc
ISO13818-2  MPEG2 sequence header

Revision 1.1  2005/11/10 00:07:18  rasc
 - New: PS MPEG2 UserData + GOP, DVB-S2 fix





*/




#include "dvbsnoop.h"
#include "mpeg2_video.h"
#include "strings/dvb_str.h"
#include "misc/hexprint.h"
#include "misc/helper.h"
#include "misc/output.h"









/*
   -- MPEG User Data
   -- ISO 13818-2
   -- Sync and streamID already displayed
*/

void MPEG2_decodeUserData (u_char *b, int len)
{

   // outBit_Sx_NL (3,"packet_start_code: ",	b, 0, 24);
   // outBit_S2x_NL(3,"Stream_id: ",		b, 24, 8,
   // 		   (char *(*)(u_long))dvbstrPESstream_ID );
 
   print_databytes (4,"User data:", b+4, len-4);
}
 



/*
   -- MPEG GOP, Group of Pictures
   -- ISO 13818-2
   -- Sync and streamID already displayed
*/

void MPEG2_decodeGroupOfPictures (u_char *b, int len)
{

   // outBit_Sx_NL (3,"packet_start_code: ",	b, 0, 24);
   // outBit_S2x_NL(3,"Stream_id: ",		b, 24, 8,
   // 		   (char *(*)(u_long))dvbstrPESstream_ID );
   // len -= 4;

   b += 4;

   // outBit_Sx_NL (4,"time_code: ",		b,  0, 25); 
   out_nl (4,"time_code:");
   indent (+1);
   	outBit_Sx_NL (4,"drop_frame_flag: ",	b,  0,  1);
   	outBit_Sx_NL (4,"time_code_hours: ",	b,  1,  5);
   	outBit_Sx_NL (4,"time_code_minutes: ",	b,  6,  6);
   	outBit_Sx_NL (4,"marker_bit: ",		b, 12,  1);
   	outBit_Sx_NL (4,"time_code_seconds: ",	b, 13,  6);
   	outBit_Sx_NL (4,"time_code_pictures: ",	b, 19,  6);
   indent (-1);

   outBit_Sx_NL (4,"closed_gop: ",		b, 25,  1);
   outBit_Sx_NL (4,"broken_link: ",		b, 26,  1);

}




/*
   -- MPEG sequence header
   -- ISO 13818-2
   -- Sync and streamID already displayed
*/

void MPEG2_decodeSequenceHeader (u_char *b, int len)
{

  int liqm;
  int nliqm;
  int bo;	// bit offset;

   // outBit_Sx_NL (3,"packet_start_code: ",	b, 0, 24);
   // outBit_S2x_NL(3,"Stream_id: ",		b, 24, 8,
   // 		   (char *(*)(u_long))dvbstrPESstream_ID );
   // len -= 4;

   b += 4;

   outBit_Sx_NL (4,"horizontal_size_value: ",		b,  0,  12);
   outBit_Sx_NL (4,"vertical_size_value: ",		b, 12,  12);
   outBit_S2x_NL(4,"aspect_ratio_information: ",	b, 24,   4,
    		   (char *(*)(u_long))dvbstrAspectRatioInfo_FLAG);
   outBit_S2x_NL(4,"frame_rate_code: ",			b, 28,   4,
    		   (char *(*)(u_long))dvbstrMPEG_FrameRateCode);

   outBit_Sx_NL (4,"bit_rate_value: ",			b, 32,  18);
   outBit_Sx_NL (4,"marker_bit: ",			b, 50,   1);
   outBit_Sx_NL (4,"vbv_buffer_size_value: ",		b, 51,  10);
   outBit_Sx_NL (4,"contraint_parameters_flag: ",	b, 61,   1);

   liqm  = outBit_Sx_NL (4,"load_intra_quantiser_matrix: ",	b, 62,   1);
   bo = 63;
   if (liqm) {
	   // 8x[64]
	   print_BitMatrix (4,"intra_quantiser_matrix: ", b, bo,  8,64);
	   bo += 64;
   }

   nliqm = outBit_Sx_NL (4,"load_non_intra_quantiser_matrix: ",	b, bo,   1);
   bo++;
   if (nliqm) {
	   // 8x[64]
	   print_BitMatrix (4,"non_intra_quantiser_matrix: ", b, bo,  8,64);
   }



}
 






