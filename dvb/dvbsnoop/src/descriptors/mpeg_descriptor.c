/*
$Id: mpeg_descriptor.c,v 1.2 2003/09/09 05:12:45 obi Exp $

  dvbsnoop
  (c) Rainer Scherg 2001-2003

  MPEG Descriptors  ISO/IEC 13818-2
  all descriptors are returning their length used in buffer


$Log: mpeg_descriptor.c,v $
Revision 1.2  2003/09/09 05:12:45  obi
print format identifier of registration descriptor in ascii.
looks quite strange but is nice to see :)

Revision 1.1  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/


#include "dvbsnoop.h"
#include "mpeg_descriptor.h"
#include "hexprint.h"







/*
  determine MPEG descriptor type and print it...
  return byte length
*/

int  descriptorMPEG  (u_char *b)

{
 int len;
 int id;


  id  =  (int) b[0];
  len = ((int) b[1]) + 2;

  out_NL (4);
  out_S2B_NL (4,"MPEG-DescriptorTag: ",id, dvbstrMPEGDescriptorTAG(id));
  out_SW_NL  (5,"Descriptor_length: ",b[1]);

  // empty ??
  len = ((int)b[1]) + 2;
  if (b[1] == 0)
	 return len;

  // print hex buf of descriptor
  printhex_buf (9, b,len);


  switch (b[0]) {
     case 0x02:  descriptorMPEG_VideoStream  (b);  break;
     case 0x03:  descriptorMPEG_AudioStream  (b);  break;
     case 0x04:  descriptorMPEG_Hierarchy  (b);  break;
     case 0x05:  descriptorMPEG_Registration  (b);  break;
     case 0x06:  descriptorMPEG_DataStreamAlignment (b);  break;
     case 0x07:  descriptorMPEG_TargetBackgroundGrid (b);  break;
     case 0x08:  descriptorMPEG_VideoWindow (b);  break;
     case 0x09:  descriptorMPEG_CA  (b);  break;
     case 0x0A:  descriptorMPEG_ISO639_Lang  (b);  break;
     case 0x0B:  descriptorMPEG_SystemClock (b);  break;
     case 0x0C:  descriptorMPEG_MultiplexBufUtil (b);  break;
     case 0x0D:  descriptorMPEG_Copyright  (b);  break;
     case 0x0E:  descriptorMPEG_MaxBitrate  (b);  break;
     case 0x0F:  descriptorMPEG_PrivateDataIndicator  (b);  break;
     case 0x10:  descriptorMPEG_SmoothingBuf  (b);  break; 
     case 0x11:  descriptorMPEG_STD  (b);  break; 
     case 0x12:  descriptorMPEG_IBP  (b);  break; 

     default: 
	if (b[0] < 0x80) {
	    out_nl (0,"  ----> ERROR: unimplemented descriptor (mpeg context), Report!");
	}
	descriptorMPEG_any (b);
	break;
  } 


  return len;   // (descriptor total length)
}





/*
  Any  descriptor  (Basic Descriptor output)
  ETSI 300 468 
*/

void descriptorMPEG_any (u_char *b)

{

 typedef struct  _descANY {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // private data bytes

 } descANY;


 descANY  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 out_nl (4,"Descriptor-Data:");
 printhexdump_buf (4,b+2,d.descriptor_length);

}









/* --------------------------------------------------------------- 
  well known MPEG descriptors
   --------------------------------------------------------------- */


/*
  0x02  VideoStream  descriptor 
  ISO 13818-1   2.6.xx
*/

void descriptorMPEG_VideoStream (u_char *b)

{

 typedef struct  _descVidStream {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      multiple_frame_rate_flag;
    u_int      frame_rate_code;
    u_int      MPEG_1_only_flag;
    u_int      constrained_parameter_flag;
    u_int      still_picture_flag;

    // if MPEG_1_only_flag == 1

    u_int	profile_and_level_indication;
    u_int	chroma_format;
    u_int	frame_rate_extension_flag;
    u_int	reserved_1;

 } descVidStream;


 descVidStream  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.multiple_frame_rate_flag	 = getBits (b, 0, 16, 1);
 d.frame_rate_code		 = getBits (b, 0, 17, 4);
 d.MPEG_1_only_flag		 = getBits (b, 0, 21, 1);
 d.constrained_parameter_flag	 = getBits (b, 0, 22, 1);
 d.still_picture_flag		 = getBits (b, 0, 23, 1);

 
 out_SB_NL (4,"multiple_frame_rate_flag: ",d.multiple_frame_rate_flag);
 out_SW_NL (4,"frame_rate_code: ",d.frame_rate_code);
 out_SB_NL (4,"MPEG_1_only_flag: ",d.MPEG_1_only_flag);
 out_SB_NL (4,"constrained_parameter_flag: ",d.constrained_parameter_flag);
 out_SB_NL (4,"still_picture_flag: ",d.still_picture_flag);


 if (d.MPEG_1_only_flag == 1) {
    d.profile_and_level_indication	= getBits (b, 0, 24, 8);
    d.chroma_format			= getBits (b, 0, 32, 2);
    d.frame_rate_extension_flag		= getBits (b, 0, 34, 1);
    d.reserved_1			= getBits (b, 0, 35, 5);

    out_SB_NL (4,"profile_and_level_indication: ",d.profile_and_level_indication);
    out_SB_NL (4,"chroma_format: ",d.chroma_format);
    out_SB_NL (4,"frame_rate_extension_flag: ",d.frame_rate_extension_flag);
    out_SB_NL (6,"reserved_1: ",d.reserved_1);
 } 

}




/*
  0x03  AudioStream  descriptor 
  ISO 13818-1   2.6.4
*/

void descriptorMPEG_AudioStream (u_char *b)

{

 typedef struct  _descAudioStream {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      free_format_flag;
    u_int      ID;
    u_int      layer;
    u_int      variable_rate_audio_indicator;
    u_int      reserved_1;

 } descAudioStream;


 descAudioStream  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.free_format_flag		 = getBits (b, 0, 16, 1);
 d.ID				 = getBits (b, 0, 17, 1);
 d.layer			 = getBits (b, 0, 18, 2);
 d.variable_rate_audio_indicator = getBits (b, 0, 20, 1);
 d.reserved_1			 = getBits (b, 0, 21, 3);

 
 out_SB_NL (4,"free_format_flag: ",d.free_format_flag);
 out_SB_NL (4,"ID: ",d.ID);
 out_SB_NL (4,"layer: ",d.layer);
 out_SB_NL (4,"variable_rate_audio_indicator: ",d.variable_rate_audio_indicator);
 out_SB_NL (6,"reserved_1: ",d.reserved_1);

}




/*
  0x04 Hierarchy  descriptor
  ISO 13818-1 2.6.6
*/

void descriptorMPEG_Hierarchy (u_char *b)

{
 typedef struct  _descHierarchy {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      reserved_1;
    u_int      hierarchy_type;
    u_int      reserved_2;
    u_int      hierarchy_layer_index;
    u_int      reserved_3;
    u_int      hierarchy_embedded_layer_index;
    u_int      reserved_4;
    u_int      hierarchy_channel;

 } descHierarchy;


 descHierarchy  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.reserved_1			 = getBits (b, 0, 16, 4);
 d.hierarchy_type 		 = getBits (b, 0, 20, 4);
 d.reserved_2			 = getBits (b, 0, 24, 2);
 d.hierarchy_layer_index	 = getBits (b, 0, 26, 6);
 d.reserved_3			 = getBits (b, 0, 32, 2);
 d.hierarchy_embedded_layer_index = getBits (b, 0, 34, 6);
 d.reserved_4			 = getBits (b, 0, 40, 2);
 d.hierarchy_channel		 = getBits (b, 0, 42, 6);


 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_S2B_NL (4,"hierarchy_type: ",d.hierarchy_type,
	dvbstrHierarchy_TYPE(d.hierarchy_type));
 out_SB_NL (6,"reserved_2: ",d.reserved_2);
 out_SB_NL (4,"hierarchy_layer_index: ",d.hierarchy_layer_index);
 out_SB_NL (6,"reserved_3: ",d.reserved_3);
 out_SB_NL (4,"hierarchy_embedded_layer_index: ",d.hierarchy_embedded_layer_index);
 out_SB_NL (6,"reserved_4: ",d.reserved_4);
 out_SB_NL (4,"hierarchy_channel: ",d.hierarchy_channel);

}




/*
  0x05 Registration  descriptor
  ISO 13818-1 2.6.8
*/

void descriptorMPEG_Registration (u_char *b)

{
 typedef struct  _descRegistration {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_long      format_identifier;
    // N   add. info

 } descRegistration;


 descRegistration  d;


 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.format_identifier		 = getBits (b, 0, 16, 32);


 out_S2L_NL (4,"format_identifier: ",d.format_identifier,"see: SC29");
 indent(+1);
 printasciiline_buf (4, b+2, 4);
 indent(-1);
 out_nl (4,"add. info:");
 printhexdump_buf (4, b+6, d.descriptor_length -4);

}





/*
  0x06  DataStreamAlignment descriptor 
  ISO 13818-1   2.6.11
*/

void descriptorMPEG_DataStreamAlignment (u_char *b)

{

 typedef struct  _descDataStreamAlignment {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      alignment_type;

 } descDataStreamAlignment;


 descDataStreamAlignment d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.alignment_type		 = b[2];


 
 out_SB_NL (4,"alignment_type: ",d.alignment_type);
 out (4,"   as VideoStream:  (= %s)",
	dvbstrDataStreamVIDEOAlignment_TYPE(d.alignment_type));
 out (4,"   as AudioStream:  (= %s)",
	dvbstrDataStreamAUDIOAlignment_TYPE(d.alignment_type));
 out_NL (4);

}







/*
  0x07 Target Background Grid  descriptor
  ISO 13818-1 2.6.12
*/

void descriptorMPEG_TargetBackgroundGrid (u_char *b)

{
 typedef struct  _descTBGG {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      horizontal_size;
    u_int      vertical_size;
    u_int      aspect_ratio_information;


 } descTBGG;


 descTBGG  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.horizontal_size		 = getBits (b, 0, 16, 14);
 d.vertical_size		 = getBits (b, 0, 30, 14);
 d.aspect_ratio_information	 = getBits (b, 0, 44, 4);

 out_S2W_NL (4,"horizontal_size: ",d.horizontal_size,"pixel");
 out_S2W_NL (4,"vertical_size: ",d.vertical_size,"pixel");
 out_S2B_NL  (4,"aspect_ratio_information: ",
	d.aspect_ratio_information,
	dvbstrAspectRatioInfo_FLAG(d.aspect_ratio_information));

}




/*
  0x08 Video Window  descriptor
  ISO 13818-1 2.6.14
*/

void descriptorMPEG_VideoWindow (u_char *b)

{
 typedef struct  _descVidWin {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      horizontal_offset;
    u_int      vertical_offset;
    u_int      window_priority;


 } descVidWin;


 descVidWin  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.horizontal_offset		 = getBits (b, 0, 16, 14);
 d.vertical_offset		 = getBits (b, 0, 30, 14);
 d.window_priority		 = getBits (b, 0, 44, 4);

 out_S2W_NL (4,"horizontal_offset: ",d.horizontal_offset,"pixel");
 out_S2W_NL (4,"vertical_offset: ",d.vertical_offset,"pixel");
 out_S2B_NL (4,"window_priority: ",d.window_priority,"(15 = highest)");

}





/*
  0x09 CA descriptor  (condition access)
*/

void descriptorMPEG_CA (u_char *b)

{
 /* IS13818-1  2.6.1 */

 typedef struct  _descCA {
    u_int      descriptor_tag;
    u_int      descriptor_length;		
    u_int      CA_system_ID;
    u_int      reserved;
    u_int      CA_PID;

    // private data bytes

 } descCA;


 descCA  d;
 int     len;


 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];
 d.CA_system_ID			 = getBits (b, 0, 16, 16);
 d.reserved			 = getBits (b, 0, 32, 3);
 d.CA_PID			 = getBits (b, 0, 35, 13);



 out_S2W_NL (4,"CA_system_ID: ",d.CA_system_ID,
     dvbstrCASystem_ID(d.CA_system_ID));
 out_SB_NL  (6,"reserved: ",d.reserved);
 out_SW_NL  (4,"CA_PID: ",d.CA_PID);

 len = d.descriptor_length-4;
 if (len > 0) {
    out_nl (4,"Private-Data:");
    printhexdump_buf (4, b+6,len);
 }

}






/*
  -- 0x0A ISO 639 Language descriptor
*/

void descriptorMPEG_ISO639_Lang (u_char *b)

{
 /* ISO 13818-1   2.6.19 */

 typedef struct  _descISO639 {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // N ISO639 desc

 } descISO639;

 typedef struct  _descISO639List {
    u_char      ISO_639_language_code[4];
    u_int      audio_type;
 } descISO639List;


 descISO639      d;
 descISO639List  d2;
 int             len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 len = d.descriptor_length;
 b  += 2;
 indent (+1);
 while ( len > 0) {
    getISO639_3 (d2.ISO_639_language_code, b);	
    d2.audio_type		= getBits (b,0,24,8);

    b += 4;
    len -= 4;

    out_nl (4,"ISO639_language_code:  %3.3s", d2.ISO_639_language_code);
    out_S2B_NL (4,"Audio_type: ", d2.audio_type,
	dvbstrAudio_TYPE (d2.audio_type));
    out_NL(4);
 }
 indent (-1);

}






/*
  0x0B  System Clock  descriptor 
  ISO 13818-1   2.6.xx
*/

void descriptorMPEG_SystemClock (u_char *b)

{

 typedef struct  _descSysClock {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      external_clock_reference_indicator;
    u_int      reserved_1;
    u_int      clock_accuracy_integer;
    u_int      clock_accuracy_exponent;
    u_int      reserved_2;

 } descSysClock;


 descSysClock d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.external_clock_reference_indicator = getBits (b, 0, 16, 1);
 d.reserved_1			 = getBits (b, 0, 17, 1);
 d.clock_accuracy_integer	 = getBits (b, 0, 18, 6);
 d.clock_accuracy_exponent	 = getBits (b, 0, 24, 3);
 d.reserved_2			 = getBits (b, 0, 27, 5);

 
 out_SB_NL (4,"external_clock_reference_indicatior: ",
	d.external_clock_reference_indicator);
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_SB_NL (4,"clock_accuracy_integer: ",d.clock_accuracy_integer);
 out_SB_NL (4,"clock_accuracy_exponent: ",d.clock_accuracy_exponent);
 out_nl    (4,"    == : %u * 10^(-%u) ",
		d.clock_accuracy_integer,d.clock_accuracy_exponent);
 out_SB_NL (6,"reserved_2: ",d.reserved_2);

}






/*
  0x0C  Multiplex Buffer Utilization  descriptor 
  ISO 13818-1   2.6.xx
*/

void descriptorMPEG_MultiplexBufUtil  (u_char *b)

{

 typedef struct  _descMBU {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      bound_valid_flag;
    u_int      LTW_offset_lower_bound;
    u_int      reserved_1;
    u_int      LTW_offset_upper_bound;

 } descMBU;


 descMBU  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.bound_valid_flag		 = getBits (b, 0, 16, 1);
 d.LTW_offset_lower_bound	 = getBits (b, 0, 17, 15);
 d.reserved_1			 = getBits (b, 0, 32, 1);
 d.LTW_offset_upper_bound	 = getBits (b, 0, 33, 15);

 
 out_SB_NL (4,"bound_valid_flag: ",d.bound_valid_flag);
 out_S2W_NL (4,"LTW_offset_lower_bound: ",d.LTW_offset_lower_bound,
	"(27 MHz/300 clock periods)");
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_S2W_NL (4,"LTW_offset_upper_bound: ",d.LTW_offset_upper_bound,
	"(27 MHz/300 clock periods)");

}








/*
  0x0D  Copyright  descriptor 
  ISO 13818-1   2.6.xx
*/

void descriptorMPEG_Copyright  (u_char *b)

{

 typedef struct  _descCopyright {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_long     copyright_identifier;

    // add. info

 } descCopyright;


 descCopyright d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.copyright_identifier		 = getBits (b, 0, 16, 32);

 
 out_S2L_NL (4,"copyright_identifier: ",d.copyright_identifier,"see: SC29");
 out_nl (4,"add. info:");
 printhexdump_buf (4, b+6, d.descriptor_length -4);

}











/*
  0x0E  Maximum Bitrate  descriptor 
  ISO 13818-1   2.6.xx
*/

void descriptorMPEG_MaxBitrate (u_char *b)

{

 typedef struct  _descMaxBitrate {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      reserved_1;
    u_long     maximum_bitrate;

 } descMaxBitrate;


 descMaxBitrate d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.reserved_1			 = getBits (b, 0, 16, 2);
 d.maximum_bitrate		 = getBits (b, 0, 18, 22);

 
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_nl    (4,"maximum_bitrate: %lu (0x%08lx)  (= %lu Bytes/sec)",
	d.maximum_bitrate, d.maximum_bitrate,
	d.maximum_bitrate * 50);

}




/*
  0x0F  Private Data Indicator descriptor 
  ISO 13818-1   2.6.x
*/

void descriptorMPEG_PrivateDataIndicator  (u_char *b)

{
   descriptorMPEG_any (b);
}





/*
  0x10  Smoothing Buffer  descriptor 
  ISO 13818-1   2.6.30
*/

void descriptorMPEG_SmoothingBuf  (u_char *b)

{

 typedef struct  _descSmoothingBuf {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      reserved_1;
    u_long     sb_leak_rate;
    u_int      reserved_2;
    u_long     sb_size;



 } descSmoothingBuf;


 descSmoothingBuf d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.reserved_1			 = getBits (b, 0, 16, 2);
 d.sb_leak_rate			 = getBits (b, 0, 18, 22);
 d.reserved_2			 = getBits (b, 0, 40, 2);
 d.sb_size			 = getBits (b, 0, 42, 22);

 
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_nl    (4,"sb_leak_rate: %lu  (= %lu bits/sec)",
	d.sb_leak_rate,d.sb_leak_rate/400);
 out_SB_NL (6,"reserved_2: ",d.reserved_2);
 out_nl    (4,"sb_size: %lu  bytes", d.sb_size);

}









/*
  0x11  STD    descriptor 
  ISO 13818-1   2.6.32
*/

void descriptorMPEG_STD (u_char *b)

{

 typedef struct  _descSTD {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      reserved_1;
    u_int      leak_valid_flag;

 } descSTD;


 descSTD d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.reserved_1			 = getBits (b, 0, 16, 7);
 d.leak_valid_flag		 = getBits (b, 0, 23, 1);

 
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_SB_NL (4,"leak_valid_flag: ",d.leak_valid_flag);

}










/*
  0x12  IBP  descriptor 
  ISO 13818-1   2.6.34
*/

void descriptorMPEG_IBP  (u_char *b)

{

 typedef struct  _descIBP {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      closed_gop_flag;
    u_int      identical_gop_flag;
    u_int      max_gop_length;

 } descIBP;


 descIBP d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.closed_gop_flag		 = getBits (b, 0, 16, 1);
 d.identical_gop_flag		 = getBits (b, 0, 17, 1);
 d.max_gop_length		 = getBits (b, 0, 18, 14);

 
 out_SB_NL (4,"Closed_gop_flag: ",d.closed_gop_flag);
 out_SB_NL (4,"Identical_gop_flag: ",d.identical_gop_flag);
 out_SW_NL (4,"Max_gop_length: ",d.max_gop_length);

}




