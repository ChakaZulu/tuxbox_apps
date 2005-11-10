/*
$Id: mpeg2_video.c,v 1.1 2005/11/10 00:07:18 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2005   Rainer.Scherg@gmx.de  (rasc)


 ISO 13818-2



$Log: mpeg2_video.c,v $
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
 









