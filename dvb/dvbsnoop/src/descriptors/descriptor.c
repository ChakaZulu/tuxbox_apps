/*
$Id: descriptor.c,v 1.5 2002/09/29 13:01:35 wjoost Exp $

 -- Descriptor Section
 -- (c) rasc


 -- all descriptors are returning their length used in buffer

$Log: descriptor.c,v $
Revision 1.5  2002/09/29 13:01:35  wjoost
kleiner Fehler

Revision 1.4  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.3  2001/12/01 12:34:31  rasc
pespacket weitergestrickt, leider z.Zt. zuwenig Zeit um es richtig fertig zu machen.

Revision 1.2  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/


#include "dvbsnoop.h"
#include "descriptor.h"
#include "hexprint.h"







/*
  determine descriptor type and print it...
  return byte length
*/

int  descriptor  (u_char *b)

{
 int len;



  len = ((int)b[1]) + 2;

  // nothing to print here?
  if (getVerboseLevel() < 4) return len;


 // here, to same some typings... 
 // because it's in each descriptor
 indent (+1);
 out_NL (4);
 out_S2B_NL (4,"DescriptorTag: ",b[0], dvbstrDescriptorTAG(b[0]));
 out_SW_NL  (5,"Descriptor_length: ",b[1]);

 if (b[1] == 0) {
    out_nl (0,"  ==> ERROR: Descriptor_length == 0!!!  (abort)"); 
    exit (0);
 }

 // print hex buf of descriptor
 printhex_buf (9, b,b[1]+2);


  switch (b[0]) {
     case 0x02:  descriptor_VideoStream  (b);  break;
     case 0x03:  descriptor_AudioStream  (b);  break;
     case 0x04:  descriptor_Hierarchy  (b);  break;
     case 0x05:  descriptor_Registration  (b);  break;
     case 0x06:  descriptor_DataStreamAlignment (b);  break;
     case 0x07:  descriptor_TargetBackgroundGrid (b);  break;
     case 0x08:  descriptor_VideoWindow (b);  break;
     case 0x09:  descriptor_CA  (b);  break;
     case 0x0A:  descriptor_ISO639_Lang  (b);  break;
     case 0x0B:  descriptor_SystemClock (b);  break;
     case 0x0C:  descriptor_MultiplexBufUtil (b);  break;
     case 0x0D:  descriptor_Copyright  (b);  break;
     case 0x0E:  descriptor_MaxBitrate  (b);  break;
     case 0x0F:  descriptor_PrivateDataIndicator  (b);  break;
     case 0x10:  descriptor_SmoothingBuf  (b);  break; 
     case 0x11:  descriptor_STD  (b);  break; 
     case 0x12:  descriptor_IBP  (b);  break; 

     case 0x40:  descriptor_NetName (b);  break;
     case 0x41:  descriptor_ServList (b);  break;
     case 0x42:  descriptor_Stuffing (b);  break;
     case 0x43:  descriptor_SatDelivSys (b);  break;
     case 0x44:  descriptor_CableDelivSys (b);  break;
     case 0x45:  descriptor_VBI_Data (b);  break;
     case 0x46:  descriptor_VBI_Teletext (b);  break;
     case 0x47:  descriptor_BouquetName (b);  break;
     case 0x48:  descriptor_Service (b);  break;
     case 0x49:  descriptor_CountryAvail (b);  break;
     case 0x4A:  descriptor_Linkage (b);  break;
     case 0x4B:  descriptor_NVOD_Reference (b);  break;
     case 0x4C:  descriptor_TimeShiftedService (b);  break;
     case 0x4D:  descriptor_ShortEvent (b);  break;
     case 0x4E:  descriptor_ExtendedEvent (b);  break;
     case 0x4F:  descriptor_TimeShiftedEvent(b);  break;
     case 0x50:  descriptor_Component(b);  break;
     case 0x51:  descriptor_Mosaic(b);  break;
     case 0x52:  descriptor_StreamIdent (b);  break;
     case 0x53:  descriptor_CAIdentifier (b);  break;
     case 0x54:  descriptor_Content (b);  break;
     case 0x55:  descriptor_ParentalRating(b);  break;
     case 0x56:  descriptor_Teletext (b);  break;
     case 0x57:  descriptor_Telephone (b);  break;
     case 0x58:  descriptor_LocalTimeOffset (b);  break;
     case 0x59:  descriptor_Subtitling (b);  break;
     case 0x5A:  descriptor_TerrestDelivSys (b);  break;
     case 0x5B:  descriptor_MultilingNetworkName (b);  break;
     case 0x5C:  descriptor_MultilingBouquetName (b);  break;
     case 0x5D:  descriptor_MultilingServiceName (b);  break;
     case 0x5E:  descriptor_MultilingComponent (b);  break;
     case 0x5F:  descriptor_PrivateDataSpecifier (b);  break;
     case 0x60:  descriptor_ServiceMove (b);  break;
     case 0x61:  descriptor_ShortSmoothingBuffer (b);  break;
     case 0x62:  descriptor_FrequencyList (b);  break;
     case 0x63:  descriptor_PartialTransportStream(b);  break;
     case 0x64:  descriptor_DataBroadcast(b);  break;
     case 0x65:  descriptor_CASystem(b);  break;
     case 0x66:  descriptor_DataBroadcastID(b);  break;
     case 0x67:  descriptor_TransportStream(b);  break;
     case 0x68:  descriptor_DSNG(b);  break;
     case 0x69:  descriptor_PDC(b);  break;
     case 0x6A:  descriptor_AC3(b);  break;
     case 0x6B:  descriptor_AncillaryData(b);  break;
     case 0x6C:  descriptor_CellList(b);  break;
     case 0x6D:  descriptor_CellFrequencyLink(b);  break;
     case 0x6E:  descriptor_AnnouncementSupport(b);  break;


     default: 
	if (b[0] < 0x80) {
	    out_nl (0,"  ----> ERROR: unimplemented descriptor, Report!");
	}
	descriptor_any (b);
	break;
  } 

  indent (-1);

  return len;   // (descriptor total length)
}



/*
  Any  descriptor  (Basic Descriptor output)
*/

void descriptor_any (u_char *b)

{
 /* ETSI 300 468 */

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
  well known descriptors
   --------------------------------------------------------------- */


/*
  0x02  VideoStream  descriptor 
  ISO 13818-1   2.6.2
*/

void descriptor_VideoStream (u_char *b)

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

void descriptor_AudioStream (u_char *b)

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

void descriptor_Hierarchy (u_char *b)

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

void descriptor_Registration (u_char *b)

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
 out_nl (4,"add. info:");
 printhexdump_buf (4, b+6, d.descriptor_length -4);

}





/*
  0x06  DataStreamAlignment descriptor 
  ISO 13818-1   2.6.11
*/

void descriptor_DataStreamAlignment (u_char *b)

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

}







/*
  0x07 Target Background Grid  descriptor
  ISO 13818-1 2.6.12
*/

void descriptor_TargetBackgroundGrid (u_char *b)

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

void descriptor_VideoWindow (u_char *b)

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

void descriptor_CA (u_char *b)

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

void descriptor_ISO639_Lang (u_char *b)

{
 /* ISO 13818-1   2.6.19 */

 typedef struct  _descISO639 {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // N ISO639 desc

 } descISO639;

 typedef struct  _descISO639List {
    u_char      ISO_639_language_code[3];
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
    strncpy (d2.ISO_639_language_code, b, 3);	
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
  ISO 13818-1   2.6.20
*/

void descriptor_SystemClock (u_char *b)

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
  ISO 13818-1   2.6.22
*/

void descriptor_MultiplexBufUtil  (u_char *b)

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
  ISO 13818-1   2.6.24
*/

void descriptor_Copyright  (u_char *b)

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
  ISO 13818-1   2.6.26
*/

void descriptor_MaxBitrate (u_char *b)

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

void descriptor_PrivateDataIndicator  (u_char *b)

{
   descriptor_any (b);
}





/*
  0x10  Smoothing Buffer  descriptor 
  ISO 13818-1   2.6.30
*/

void descriptor_SmoothingBuf  (u_char *b)

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

void descriptor_STD (u_char *b)

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

void descriptor_IBP  (u_char *b)

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

















/*
  0x40 NetName  descriptor  (network name descriptor)
*/

void descriptor_NetName (u_char *b)

{
 /* ETSI 300 468 */

 typedef struct  _descNND {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_char     *network_name;   // with controls


 } descNND;

 descNND  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];
 // d.network_name to be assigned 


 out (4,"Network_name:  ");
 b += 2;

 print_name (4, b,d.descriptor_length);
 out_NL (4);

}




/*
  0x41 Service List Descriptor 
*/

void descriptor_ServList (u_char *b)

{
 /* ETSI 300 468  6.2.31 */

 typedef struct  _descServList {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // N ... Service ID & Types

 } descServList;


 typedef struct _descServList2 {
    u_int      service_id;
    u_int      service_type;
 } descServList2;


 descServList  d;
 descServList2 d2;
 int           len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 len = d.descriptor_length - 2;
 b += 2;

 indent (+1);
 while (len > 0) {
   d2.service_id 		= getBits (b, 0, 0, 16);
   d2.service_type 		= getBits (b, 0, 16, 8);
   b   += 3;
   len -= 3;
   

    out_S2W_NL (4,"Service_ID: ",d2.service_id,
	   " --> refers to PMS program_number");
    out_S2B_NL (4,"Service_type: ",d2.service_type,
	   dvbstrService_TYPE(d2.service_type));
    out_NL (4);

 }
 indent (-1);


}





/*
  0x42  Stuffing descriptor 
  ETSI EN 300 468  
*/

void descriptor_Stuffing (u_char *b)

{
  descriptor_any (b);
}





/*
  0x43 SatDelivSys  descriptor  (Satellite delivery system descriptor)
*/

void descriptor_SatDelivSys (u_char *b)

{
 /* ETSI 300 468    6.2.12.2*/

 typedef struct  _descSDS {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_long     frequency;
    u_int      orbital_position;
    u_int      west_east_flag;
    u_int      polarisation;
    u_int      modulation;
    u_long     symbol_rate;
    u_int      FEC_inner;

 } descSDS;

 descSDS  d;
 //int      i;




 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 d.frequency			 = getBits (b, 0, 16, 32);
 d.orbital_position		 = getBits (b, 0, 48, 16);
 d.west_east_flag		 = getBits (b, 0, 64, 1);
 d.polarisation			 = getBits (b, 0, 65, 2);
 d.modulation			 = getBits (b, 0, 67, 5);
 d.symbol_rate			 = getBits (b, 0, 72, 28);
 d.FEC_inner			 = getBits (b, 0, 100, 4);


 out_nl (4,"Frequency: %lu (= %3lx.%05lx GHz)",d.frequency,
	 d.frequency >> 20, d.frequency & 0x000FFFFF );
 out_nl (4,"Orbital_position: %u (= %3x.%01x)",d.orbital_position,
	 d.orbital_position >> 4, d.orbital_position & 0x000F);

 out_S2B_NL (4,"West_East_flag: ",d.west_east_flag,
	 dvbstrWEST_EAST_FLAG(d.west_east_flag));

 out_S2B_NL (4,"Polarisation: ",d.polarisation,
	 dvbstrPolarisation_FLAG(d.polarisation));

 out_S2B_NL (4,"Modulation (Sat): ",d.modulation,
	 dvbstrModulationSAT_FLAG(d.modulation));

 out_nl (4,"Symbol_rate: %u (= %3x.%04x)",d.symbol_rate,
	 d.symbol_rate >> 16, d.symbol_rate & 0x0000FFFF );

 out_S2B_NL (4,"FEC_inner: ",d.FEC_inner,
	 dvbstrFECinner_SCHEME (d.FEC_inner));


}





/*
  0x44 CableDelivSys  descriptor  (Cable delivery system descriptor)
*/

void descriptor_CableDelivSys (u_char *b)

{
 /* ETSI 300 468    6.2.12.1*/

 typedef struct  _descCDS {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_long     frequency;
    u_int      FEC_outer;
    u_int      reserved_1;
    u_int      modulation;
    u_long     symbol_rate;
    u_int      FEC_inner;

 } descCDS;

 descCDS  d;
 //int      i;




 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 d.frequency			 = getBits (b, 0, 16, 32);
 d.reserved_1			 = getBits (b, 0, 48, 12);
 d.FEC_outer			 = getBits (b, 0, 60, 4);
 d.modulation			 = getBits (b, 0, 64, 8);
 d.symbol_rate			 = getBits (b, 0, 72, 28);
 d.FEC_inner			 = getBits (b, 0, 100, 4);


 out_nl (4,"Frequency: %lu (= %3lx.%05lx MHz)",d.frequency,
	 d.frequency >> 16, d.frequency & 0x0000FFFF );

 out_S2B_NL (4,"FEC_outer: ",d.FEC_outer,
	 dvbstrFECouter_SCHEME (d.FEC_outer));

 out_SB_NL (6,"reserved_1: ",d.reserved_1);


 out_S2B_NL (4,"Modulation (Cable): ",d.modulation,
	 dvbstrModulationCable_FLAG(d.modulation));

 out_nl (4,"  Symbol_rate: %u (= %4x.%04x)",d.symbol_rate,
	 d.symbol_rate >> 16, d.symbol_rate & 0x0000FFFF );

 out_S2B_NL (4,"FEC_inner: ",d.FEC_inner,
	 dvbstrFECinner_SCHEME (d.FEC_inner));

}






/*
  0x45  VBI Data  descriptor 
  ETSI EN 300 468  2.2.42
*/

void descriptor_VBI_Data  (u_char *b)

{

 typedef struct  _descVBIData {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // N ... DataList 1
 } descVBIData;

 typedef struct  _descVBIData2 {
    u_int      data_service_id;		
    u_int      data_service_descriptor_length;		

    //    N ... DataList 3
    // or N ... reserved bytes
 } descVBIData2;

 typedef struct  _descVBIData3 {
    u_int      reserved_1;		
    u_int      field_parity;		
    u_int      line_offset;		
 } descVBIData3;



 descVBIData   d;
 descVBIData2  d2;
 descVBIData3  d3;
 int           len1,len2;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 b += 2;
 len1 = d.descriptor_length;


 indent (+1);
 while (len1 > 0) {

    d2.data_service_id		       = b[0];
    d2.data_service_descriptor_length   = b[1];

    out_NL (4); 
    out (4, "Data_service_id: %u  [= %s]", d2.data_service_id,
		dvbstrDataService_ID(d2.data_service_id));
          out_nl (4," --> refers to PMS program_number");

    out_SB_NL (5,"Data_service_descriptor_length: ",
		d2.data_service_descriptor_length);


    b    += 2;
    len1 -= 2;
    len2  = d2.data_service_descriptor_length;

    if (d2.data_service_id >= 1 && d2.data_service_id <= 7) {

       indent (+1);
       while (len2 > 0) {
           d3.reserved_1		 = getBits (b, 0, 0, 2);
           d3.field_parity		 = getBits (b, 0, 2, 1);
           d3.line_offset		 = getBits (b, 0, 3, 5);

           out_NL (4);
           out_SB_NL (6,"reserved_1: ",d3.reserved_1);
           out_SB_NL (4,"field_parity: ",d3.field_parity);
           out_SB_NL (4,"line_offset: ",d3.line_offset);
       } 
       indent (-1);

    } else {

       out_nl (6,"Reserved Data:");
       printhexdump_buf (6, b,len2);

    }


 }
 indent (-1);


}






/*
  0x46  VBI teletext descriptor 
  ETSI EN 300 468   6.2.43
*/

void descriptor_VBI_Teletext (u_char *b)

{
  descriptor_Teletext (b);
}






/*
  0x47  Bouquet Name  descriptor 
  ETSI EN 300 468    6.2.3
*/

void descriptor_BouquetName  (u_char *b)

{

 typedef struct  _descBouquetName {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // N   ... char name

 } descBouquetName;


 descBouquetName d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 out (4,"BouquetName: ");
 print_name (4, b+2,d.descriptor_length);
 out_NL (4);

}







/*
  0x48  Service  descriptor 
  ETSI EN 300 468   6.2.30
*/

void descriptor_Service  (u_char *b)

{

 typedef struct  _descService {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      service_type;
    u_int      service_provider_name_length;

    // N   char 
    // char   *service_provider_name;

    u_int      service_name_length;

    // N2  char
    // char   *service_name;

 } descService;


 descService d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.service_type			 = getBits (b, 0, 16, 8);
 d.service_provider_name_length  = getBits (b, 0, 24, 8);


 b += 4;

 out_S2B_NL (4,"Service_type: ",d.service_type,
	dvbstrService_TYPE(d.service_type));
 out_SB_NL (5,"Service_provider_name_length: ",d.service_provider_name_length);
 out (4,"Service_provider_name: ");
        print_name (4, b,d.service_provider_name_length);
 out_NL (4);
 

 b += d.service_provider_name_length;
 d.service_name_length		  = getBits (b, 0, 0, 8);
 b += 1;

 out_SW_NL (5,"Service_name_length: ",d.service_name_length);
 out (4,"Service_name: ");
        print_name (4, b,d.service_name_length);
 out_NL (4);

}





/*
  0x49  Country Availibility  descriptor 
  ETSI EN 300 468   6.2.9
*/

void descriptor_CountryAvail  (u_char *b)

{

 typedef struct  _descCountryAvail {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      country_availability_flag;
    u_int      reserved_1;

    //  N   countrycodes[3]

 } descCountryAvail;


 descCountryAvail d;
 int              len;
 u_char           country_code[3];



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.country_availability_flag	 = getBits (b, 0, 16, 1);
 d.reserved_1			 = getBits (b, 0, 17, 7);

 
 out_SB_NL (4,"country_availability_flag: ",d.country_availability_flag);
 out_SB_NL (6,"reserved_1: ",d.reserved_1);

 b  += 3;
 len = d.descriptor_length - 1;

 indent (+1);
 while (len > 0) {
    strncpy (country_code, b, 3);
    b   += 3;
    len -= 3;

    out_nl (4,"Country:  %3.3s",country_code);
 }
 indent (-1);

}







/*
  0x4A  Linkage  descriptor  
*/

void descriptor_Linkage (u_char *b)

{
 /* ETSI 300 468   6.2.16 */

 typedef struct  _descLinkage {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      transport_stream_id;
    u_int      original_network_id;
    u_int      service_id;
    u_int      linkage_type;

    // if linkage_type == 8
    // the following field are conditional!!
    u_int      handover_type;
    u_int      reserved_1; 
    u_int      origin_type;
    u_int      network_id;
    u_int      initial_service_id;

 } descLinkage;


 descLinkage  d;
 int          len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.transport_stream_id		 = getBits (b, 0, 16, 16);
 d.original_network_id		 = getBits (b, 0, 32, 16);
 d.service_id			 = getBits (b, 0, 48, 16);
 d.linkage_type			 = getBits (b, 0, 64, 8);


 out_SW_NL  (4,"Transport_stream_ID: ",d.transport_stream_id);
 out_S2W_NL (4,"Original_network_ID: ",d.original_network_id,
	dvbstrNetworkIdent_ID(d.original_network_id));
 out_S2W_NL (4,"Service_ID: ",d.service_id,
      " --> refers to PMS program_number");
 out_S2B_NL (4,"Linkage_type: ",d.linkage_type,
	dvbstrLinkage_TYPE (d.linkage_type));

 len = d.descriptor_length - 7;
 b  += 7 + 2;

 if (d.linkage_type != 0x08) {
    // private data
    out_nl (4,"Private data:"); 
    printhexdump_buf (4, b,len);
 } else {
    d.handover_type		= getBits (b, 0, 0, 4);
    d.reserved_1		= getBits (b, 0, 4, 3);
    d.origin_type		= getBits (b, 0, 7, 1);
    b   += 1;
    len -= 1;
    
    out_S2B_NL (4,"Handover_type: ",d.handover_type,
	dvbstrHandover_TYPE(d.handover_type));

    out_SB_NL (6,"reserved_1: ",d.reserved_1);

    out_S2B_NL (4,"Origin_type: ",d.origin_type,
	dvbstrOrigin_TYPE(d.origin_type));

    if (   d.handover_type == 0x01
        || d.handover_type == 0x02
        || d.handover_type == 0x03) {
        d.network_id		= getBits (b, 0, 0, 16);
        out_S2W_NL (4,"Network_ID: ",d.network_id,
		dvbstrNetworkIdent_ID(d.network_id));
        b   += 1;
        len -= 1;
    } 
    if (d.origin_type == 0x00) {
        d.initial_service_id	= getBits (b, 0, 0, 16);
        out_SW_NL (4,"Initial_service_ID: ",d.initial_service_id);
        b   += 1;
        len -= 1;
    }

    if (len > 0) {
      // private data 
       out_nl (4,"Private data:"); 
       printhexdump_buf (4, b,len);
    }
    

 } //if


}






/*
  0x4B  NVOD Reference  descriptor 
  ETSI EN 300 468  6.2.23
*/

void descriptor_NVOD_Reference  (u_char *b)

{

 typedef struct  _descNVODRef {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // N ...  Ref2
 } descNVODRef;

 typedef struct  _descNVODRef2 {
    u_int      transport_stream_id;
    u_int      original_network_id;
    u_int      service_id;
 } descNVODRef2;


 descNVODRef  d;
 descNVODRef2 d2;
 int          len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 b  += 2;
 len = d.descriptor_length;

 indent (+1);
 while (len > 0) {
    d2.transport_stream_id	 = getBits (b, 0, 0, 16);
    d2.original_network_id	 = getBits (b, 0, 16, 16);
    d2.service_id		 = getBits (b, 0, 32, 16);

    len -= 6;
    b   += 6;

    out_SW_NL  (4,"Transport_stream_ID: ",d2.transport_stream_id);
    out_S2W_NL (4,"Original_network_ID: ",d2.original_network_id,
	  dvbstrNetworkIdent_ID(d2.original_network_id));
    out_S2W_NL (4,"Service_ID: ",d2.service_id,
        " --> refers to PMS program_number");
    out_NL (4);
 }
 indent (-1);

}







/*
  0x4C  Time Shifted Service   descriptor 
  ETSI EN 300 468     6.2.40
*/

void descriptor_TimeShiftedService  (u_char *b)

{

 typedef struct  _descTimShiftServ {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      reference_service_id;

 } descTimShiftServ;


 descTimShiftServ  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.reference_service_id		 = getBits (b, 0, 16, 16);

 out_SW_NL (4,"Reference_service_ID: ",d.reference_service_id);

}







/*
  0x4D  Short Event  descriptor 
  ETSI EN 300 468     6.2.33
*/

void descriptor_ShortEvent  (u_char *b)

{

 typedef struct  _descShortEvent {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_char     ISO639_2_language_code[3];
    u_int      event_name_length;

    // N   char event_name

    u_int      text_length;

    // N2  char  text char

 } descShortEvent;


 descShortEvent d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 strncpy (d.ISO639_2_language_code, b+2, 3);	
 out_nl (4,"  ISO639_2_language_code:  %3.3s", d.ISO639_2_language_code);


 d.event_name_length		 = getBits (b, 5, 0, 8);
 b += 6;

 out_SB_NL (5,"Event_name_length: ",d.event_name_length);
 out       (4,"Event_name: ");
	print_name (4, b,d.event_name_length);
	out_NL (4);

 b += d.event_name_length;

 d.text_length			 = getBits (b, 0, 0, 8);
 b += 1;
 out_SB_NL (5,"Text_length: ",d.text_length);
 	out (4,"Text: ");
 	print_name (4, b,d.text_length);
	out_NL (4);

}






/*
  0x4E  Extended Event  descriptor 
  ETSI EN 300 468     6.2.14
*/

void descriptor_ExtendedEvent  (u_char *b)

{

 typedef struct  _descExtendedEvent {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      descriptor_number;
    u_int      last_descriptor_number;
    u_char     ISO639_2_language_code[3];
    u_int      length_of_items;

    // N   Ext. Events List

    u_int      text_length;
    // N4  char  text char
 } descExtendedEvent;


 typedef struct  _descExtendedEvent2 {
    u_int      item_description_length;
    //  N2   descriptors
    u_int      item_length;
    //  N3   chars
 } descExtendedEvent2;


 descExtendedEvent    d;
 descExtendedEvent2   d2;
 int                  len1, lenB;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.descriptor_number		 = getBits (b, 0, 16, 4);
 d.last_descriptor_number	 = getBits (b, 0, 20, 4);
 strncpy (d.ISO639_2_language_code, b+3, 3);	
 d.length_of_items		 = getBits (b, 0, 48, 8);


 out_SB_NL (4,"Descriptor_number: ",d.descriptor_number);
 out_SB_NL (4,"Last_descriptor_number: ",d.last_descriptor_number);
 out_nl    (4,"ISO639_2_language_code:  %3.3s", d.ISO639_2_language_code);
 out_SB_NL (5,"Length_of_items: ",d.length_of_items);


 b   += 7;
 lenB = d.descriptor_length - 5;
 len1 = d.length_of_items;

 indent (+1);
 while (len1 > 0) {
 
   d2.item_description_length	 = getBits (b, 0, 0, 8);
   out_NL (4);
   out_SB_NL (5,"Item_description_length: ",d2.item_description_length);
   out_nl (4,"Item_description: ");
      print_name (4, b+1, d2.item_description_length);
	out_NL (4);

   b += 1 + d2.item_description_length;
   

   d2.item_length	 	 = getBits (b, 0, 0, 8);
   out_SB_NL (5,"Item_length: ",d2.item_length);
   out (4,"Item: ");
      print_name (4, b+1, d2.item_length);
	out_NL (4);

   b += 1 + d2.item_length;

   len1 -= (2 + d2.item_description_length + d2.item_length);
   lenB -= (2 + d2.item_description_length + d2.item_length);

 }
 out_NL (4);
 indent (-1);



   d.text_length		 = getBits (b, 0, 0, 8);
   b += 1;
   lenB -= 1;


   out_SB_NL (5,"Text_length: ",d.text_length);
   out (4,"Text: ");
	print_name (4, b,d.text_length);
	out_NL (4);


}







/*
  0x4F  Time Shifted Event  descriptor 
  ETSI EN 300 468     6.2.39
*/

void descriptor_TimeShiftedEvent  (u_char *b)

{

 typedef struct  _descTimeShiftedEvent {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      reference_service_id;
    u_int      reference_event_id;

 } descTimeShiftedEvent;


 descTimeShiftedEvent d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.reference_service_id		 = getBits (b, 0, 16, 16);
 d.reference_event_id		 = getBits (b, 0, 32, 16);

 
 out_SW_NL (4,"Reference_service_id: ",d.reference_service_id);
 out_SW_NL (4,"Reference_event_id: ",d.reference_service_id);

}







/*
  0x50  Component descriptor 
  ETSI EN 300 468     6.2.7
*/

void descriptor_Component  (u_char *b)

{

 typedef struct  _descComponent {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      reserved_1;
    u_int      stream_content;
    u_int      component_type;
    u_int      component_tag;
    u_char     ISO639_2_language_code[3];

    // N2  char Text

 } descComponent;


 descComponent d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.reserved_1			 = getBits (b, 0, 16, 4);
 d.stream_content		 = getBits (b, 0, 20, 4);
 d.component_type		 = getBits (b, 0, 24, 8);
 d.component_tag		 = getBits (b, 0, 32, 8);
 strncpy (d.ISO639_2_language_code, b+5, 3);	

 
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_SB_NL (4,"Stream_content: ",d.stream_content);
 out_SB_NL (4,"Component_type: ",d.component_type);
 out_nl    (4,"   == Content&Component: (= %s)",
      dvbstrStreamContent_Component_TYPE(
	(d.stream_content << 8) | d.component_type ) );

 out_SB_NL (4,"Component_tag: ",d.component_tag);
 out_nl    (4,"ISO639_2_language_code:  %3.3s", d.ISO639_2_language_code);

 out       (4,"Component-Description: ");
	print_name (4, b+8,d.descriptor_length - 6);
 	out_NL (4);

}








/*
  0x51  Mosaic  descriptor 
  ETSI EN 300 468     6.2.18
*/

void descriptor_Mosaic  (u_char *b)

{

 typedef struct  _descMosaic {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      mosaic_entry_point;
    u_int      number_of_horizontal_elementary_cells;
    u_int      reserved_1;
    u_int      number_of_vertical_elementary_cells;

    // N    desc Mosaic2

 } descMosaic;

 typedef struct  _descMosaic2 {
    u_int      logical_cell_id;
    u_int      reserved_1;
    u_int      logical_cell_presentation_info;
    u_int      elementary_cell_field_length;

    // N2   desc Mosaic3

    u_int      cell_linkage_info;
    //  conditional data !! (cell_linkage_info)
    u_int   bouquet_id;
    u_int   original_network_id;
    u_int   transport_stream_id;
    u_int   service_id;
    u_int   event_id;
 } descMosaic2;

 typedef struct  _descMosaic3 {
    u_int      reserved_1;
    u_int      elementary_cell_id;
 } descMosaic3;




 descMosaic    d;
 descMosaic2   d2;
 descMosaic3   d3;
 int           len1,len2;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.mosaic_entry_point                    = getBits (b, 0, 16,  1);
 d.number_of_horizontal_elementary_cells = getBits (b, 0, 17,  3);
 d.reserved_1                            = getBits (b, 0, 20,  1);
 d.number_of_vertical_elementary_cells   = getBits (b, 0, 21,  3);

 
 out_SB_NL (4,"Mosaic_entry_point: ",d.mosaic_entry_point);
 out_nl    (4,"Number_of_horizontal_elementary_cells: %u  (= %d cells)",
	d.number_of_horizontal_elementary_cells,
	d.number_of_horizontal_elementary_cells + 1);
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_nl    (4,"  Number_of_vertical_elementary_cells: %u  (= %d cells)",
	d.number_of_vertical_elementary_cells,
	d.number_of_vertical_elementary_cells + 1);

 len1 = d.descriptor_length - 1;
 b   += 3;

 indent(+1);
 while (len1 > 0) {

    d2.logical_cell_id                       = getBits (b, 0,  0,  6);
    d2.reserved_1                            = getBits (b, 0,  6,  7);
    d2.logical_cell_presentation_info        = getBits (b, 0, 13,  3);
    d2.elementary_cell_field_length          = getBits (b, 0, 16,  8);


    out_NL (4);
    out_SB_NL  (4,"Logical_cell_ID: ",d2.logical_cell_id);
    out_SB_NL  (6,"reserved_1: ",d2.reserved_1);
    out_S2B_NL (4,"Logical_cell_presentation_info: ",
	d2.logical_cell_presentation_info,
	dvbstrLogCellPresInfo_TYPE(d2.logical_cell_presentation_info) );
    out_SB_NL (5,"Elementary_cell_field_length: ",
	d2.elementary_cell_field_length);

    b    += 3;
    len2  = d2.elementary_cell_field_length;
    len1 -= (len2 + 3);

    indent (+1);
    while (len2 > 0) {
       d3.reserved_1                         = getBits (b, 0,  0,  2);
       d3.elementary_cell_id                 = getBits (b, 0,  2,  6);

       len2 -= 1;
       b    += 1;

       out_NL (4);	
       out_SB_NL (6,"reserved_1: ",d3.reserved_1);
       out_SB_NL (4,"Elementary_cell_ID: ",d3.elementary_cell_id);

    }  //len2
    indent (-1);


    d2.cell_linkage_info                     = getBits (b, 0,  0,  8);
    b    += 1;
    len1 -= 1;
    out_S2B_NL (4,"Cell_linkage_info: ",
		d2.cell_linkage_info,
		dvbstrCellLinkageInfo_TYPE (d2.cell_linkage_info));


    /*   conditional fields!! */

    switch (d2.cell_linkage_info) {

      case 0x01:
	d2.bouquet_id		 = getBits (b, 0, 0, 16);
	b    += 2;
	len1 -= 2;
	out_SW_NL (4,"Bouquet_ID: ",d2.bouquet_id);
//$$ do bouquet_id
	break;

      case 0x02:
      case 0x03:
      case 0x04:
 	d2.transport_stream_id		 = getBits (b, 0, 0, 16);
	d2.original_network_id		 = getBits (b, 0, 16, 16);
	d2.service_id			 = getBits (b, 0, 32, 16);
	b    += 6;
	len1 -= 6;

 	out_SW_NL  (4,"Transport_stream_ID: ",d2.transport_stream_id);
	out_S2W_NL (4,"Original_network_ID: ",d2.original_network_id,
	    dvbstrNetworkIdent_ID(d2.original_network_id));
	out_S2W_NL (4,"Service_ID: ",d2.service_id,
          " --> refers to PMS program_number");


	if (d2.cell_linkage_info == 0x03)
		out_nl (4,"  --> Service referce to mosaic service");

	if (d2.cell_linkage_info == 0x04) {
		out_nl (4,"  --> Service referce to event");

 		d2.event_id	 = getBits (b, 0, 0, 16);
		b    += 2;
		len1 -= 2;
 		out_SW_NL (4,"Event_ID: ",d2.event_id);
	}

	break;

    } // switch
    

 } // while len1
 indent(-1);

}









/*
  0x52  Stream Identifier descriptor
  ETSI EN 300 468  6.2.34

*/

void descriptor_StreamIdent (u_char *b)

{

 typedef struct  _descStreamIdent {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      component_tag;		
 } descStreamIdent;


 descStreamIdent  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.component_tag		 = getBits (b,0,16,8);


 out_SB_NL (4,"Component_tag: ",d.component_tag);

}








/*
  0x53  CA Identifier  descriptor 
  ETSI EN 300 468   6.2.4
*/

void descriptor_CAIdentifier  (u_char *b)

{

 typedef struct  _descCAIdent {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // N   CA_SysIDs

 } descCAIdent;


 descCAIdent d;
 u_int       CA_system_ID;
 int         len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 len = d.descriptor_length; 
 b  += 2;

 indent (+1);
 while (len > 0) {

   CA_system_ID 		 = getBits (b,0,0,16);

   out_S2W_NL (4,"CA_system_ID: ",CA_system_ID,
      dvbstrCASystem_ID(CA_system_ID));

   b   += 2;
   len -= 2;
 }
 indent (-1);

}









/*
  0x54  Content  descriptor 
  ETSI EN 300 468     6.2.8
*/

void descriptor_Content  (u_char *b)

{

 typedef struct  _descContent {
    u_int      descriptor_tag;
    u_int      descriptor_length;		
 } descContent;

 typedef struct  _descContent2 {
    u_int      content_nibble_level_1;
    u_int      content_nibble_level_2;
    u_int      user_nibble_1;
    u_int      user_nibble_2;
 } descContent2;


 descContent   d;
 descContent2  d2;
 int           len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 len = d.descriptor_length;
 b  += 2;
 
 indent (+1);
 while ( len > 0) {
    d2.content_nibble_level_1	 = getBits (b,0, 0,4);
    d2.content_nibble_level_2	 = getBits (b,0, 4,4);
    d2.user_nibble_1		 = getBits (b,0, 8,4);
    d2.user_nibble_2		 = getBits (b,0,12,4);

    b   += 2;
    len -= 2;
 
    out_SB_NL (4,"Content_nibble_level_1: ", d2.content_nibble_level_1);
    out_SB_NL (4,"Content_nibble_level_2: ", d2.content_nibble_level_2);
    out_nl    (4,"   [= %s]", dvbstrContentNibble_TYPE (
	(d2.content_nibble_level_1 << 8) | d2.content_nibble_level_2) );

    out_SB_NL (4,"User_nibble_1: ", d2.user_nibble_1);
    out_SB_NL (4,"User_nibble_2: ", d2.user_nibble_2);
    out_NL (4);
 }
 indent (-1);

}











/*
  0x55  Parental Rating  descriptor 
  ETSI EN 300 468     6.2.25
*/

void descriptor_ParentalRating (u_char *b)

{

 typedef struct  _descParentalRating {
    u_int      descriptor_tag;
    u_int      descriptor_length;		
 } descParentalRating;

 typedef struct  _descParentalRating2 {
    u_char     country_code[3];
    u_int      rating;		
 } descParentalRating2;



 descParentalRating   d;
 descParentalRating2  d2;
 int                  len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 len = d.descriptor_length;
 b  += 2;

 indent (+1);
 while (len > 0) {
    strncpy (d2.country_code, b, 3);	
    d2.rating			 = getBits (b,0,24,8);

    b += 4;
    len -= 4;

    out_nl     (4,"Country_code:  %3.3s", d2.country_code);
    out_S2B_NL (4,"Rating:  ", d2.rating,
	dvbstrParentalRating_TYPE (d2.rating));
    out_NL (4);

 }
 indent (-1);
 
}









/*
  -- 0x56 Teletext descriptor
*/

void descriptor_Teletext (u_char *b)

{
 /* ETSI EN 300 468   6.2.38 */

 typedef struct  _descTeletext {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // N TeleTextList desc

 } descTeletext;

 typedef struct  _descTeletextList {
    u_char      ISO_639_language_code[3];
    u_int      teletext_type;
    u_int      teletext_magazine_number;
    u_int      teletext_page_number;
 } descTeletextList;


 descTeletext      d;
 descTeletextList  d2;
 int               len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 len = d.descriptor_length;
 b  += 2;

 indent (+1);
 while ( len > 0) {
    strncpy (d2.ISO_639_language_code, b, 3);	
    d2.teletext_type		= getBits (b,0,24,5);
    d2.teletext_magazine_number	= getBits (b,0,29,3);
    d2.teletext_page_number	= getBits (b,0,32,8);

    b += 5;
    len -= 5;

    out_nl     (4,"ISO639_language_code:  %3.3s", d2.ISO_639_language_code);
    out_S2B_NL (4,"Teletext_tye: ", d2.teletext_type,
	dvbstrTeletext_TYPE (d2.teletext_type));

    out_SB_NL (4,"Teletext_magazine_number: ",d2.teletext_magazine_number);
    out_SB_NL (4,"Teletext_page_number: ",d2.teletext_page_number);
    out_NL (4);
 }
 indent (-1);

}





/*
  0x57  Telephone  descriptor 
  ETSI EN 300 468     6.2.27
*/

void descriptor_Telephone  (u_char *b)

{

 typedef struct  _descTelephone {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      reserved_1;
    // $$$ ToDO

 } descTelephone;


 descTelephone  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];



 descriptor_any (b);
 out_nl (4," ==> ERROR: Telephone descriptor not implemented, Report!");


}







/*
  0x58  Local Time Offset  descriptor 
  ETSI EN 300 468     6.2.17
*/

void descriptor_LocalTimeOffset  (u_char *b)

{

 typedef struct  _descLocalTimeOffset {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    // N  Descriptor
 } descLocalTimeOffset;

 typedef struct  _descLocalTimeOffset2 {
   u_char        country_code[3];
   u_int         country_region_id;
   u_int         reserved_1;
   u_int         local_time_offset_polarity;
   u_int         local_time_offset;
   u_int         time_of_change_MJD;
   u_int         time_of_change_UTC;
   u_int         next_time_offset;
 } descLocalTimeOffset2;


 descLocalTimeOffset   d;
 descLocalTimeOffset2  d2;
 int                   len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 len = d.descriptor_length;
 b  += 2;
 
 indent (+1);
 while (len > 0) {

    strncpy (d2.country_code, b, 3);	
    d2.country_region_id	 = getBits (b, 0, 24,  6);
    d2.reserved_1		 = getBits (b, 0, 30,  1);
    d2.local_time_offset_polarity = getBits (b, 0, 31,  1);
    d2.local_time_offset	 = getBits (b, 0, 32, 16);
    d2.time_of_change_MJD	 = getBits (b, 0, 48, 16);
    d2.time_of_change_UTC	 = getBits (b, 0, 64, 24);
    d2.next_time_offset		 = getBits (b, 0, 88, 16);

    len -= 13;
    b   += 13;

    out_nl    (4,"Country_code:  %3.3s", d2.country_code);
    out_SB_NL (4,"Country_region_ID: ",d2.country_region_id);
    out_SB_NL (6,"reserved_1: ",d2.reserved_1);

    out_nl    (4,"local_time_offset_polarity: %u  (= %s to UTC)",
	d2.local_time_offset_polarity,
	(d2.local_time_offset_polarity) ? "minus" : "plus");

    out_nl    (4,"Local_time_offset: %02x:%02x",
	d2.local_time_offset >> 8, d2.local_time_offset & 0xFF);

    out       (4,"Time_of_change: ");
      print_time40 (4, d2.time_of_change_MJD,d2.time_of_change_UTC);
      out_NL (4);

    out_nl    (4,"Next_time_offset: %02x:%02x ",
	d2.next_time_offset >> 8, d2.next_time_offset & 0xFF);

    out_NL(4);

 }
 indent (-1);


}






/*
  0x59  Subtitling  descriptor 
  ETSI EN 300 468     6.2.36
*/

void descriptor_Subtitling  (u_char *b)

{

 typedef struct  _descSubTitling {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

 } descSubTitling;

 typedef struct  _descSubTitling2 {
    u_char     ISO_639_language_code[3];
    u_int      subtitling_type;
    u_int      composition_page_id;
    u_int      ancillary_page_id;
 } descSubTitling2;


 descSubTitling   d;
 descSubTitling2  d2;
 int              len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 len = d.descriptor_length;
 b  += 2;

 indent (+1);
 while ( len > 0) {
    strncpy (d2.ISO_639_language_code, b, 3);	
    d2.subtitling_type		= getBits (b,0,24, 8);
    d2.composition_page_id	= getBits (b,0,32,16);
    d2.ancillary_page_id	= getBits (b,0,48,16);

    b   += 8;
    len -= 8;

    out_nl  (4,"  ISO639_language_code:  %3.3s", d2.ISO_639_language_code);
    out_S2B_NL (4,"Subtitling_type: ", d2.subtitling_type,
	dvbstrStreamContent_Component_TYPE (
	    (0x03 << 8) | d2.subtitling_type));

    out_SW_NL (4,"Composition_page_id: ",d2.composition_page_id);
    out_SW_NL (4,"Ancillary_page_id: ",d2.ancillary_page_id);
    out_NL (4);
 }
 indent (-1);

}








/*
  0x5A TerrestDelivSys  descriptor  (Terrestrial delivery system descriptor)
*/

void descriptor_TerrestDelivSys (u_char *b)

{
 /* ETSI 300 468    6.2.12.1*/

 typedef struct  _descCDS {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_long     centre_frequency;
    u_int      bandwidth;
    u_int      reserved_1;
    u_int      constellation;
    u_long     hierarchy_information;
    u_int      code_rate_HP_stream;
    u_int      code_rate_LP_stream;
    u_int      guard_interval;
    u_int      transmission_mode; 
    u_int      other_frequency_flag;
    u_int      reserved_2;

 } descTDS;

 descTDS  d;
 //int      i;




 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 d.centre_frequency		 = getBits (b, 0, 16, 32);
 d.bandwidth			 = getBits (b, 0, 48, 3);
 d.reserved_1			 = getBits (b, 0, 51, 5);
 d.constellation		 = getBits (b, 0, 56, 2);
 d.hierarchy_information	 = getBits (b, 0, 58, 3);
 d.code_rate_HP_stream		 = getBits (b, 0, 61, 3);
 d.code_rate_LP_stream		 = getBits (b, 0, 64, 3);
 d.guard_interval		 = getBits (b, 0, 67, 2);
 d.transmission_mode		 = getBits (b, 0, 69, 2);
 d.other_frequency_flag		 = getBits (b, 0, 71, 1);
 d.reserved_2			 = getBits (b, 0, 72, 32);


 out_nl (4,"Center frequency: 0x%08x (= %lu Hz)",d.centre_frequency,
	 d.centre_frequency * 10 );
 out_S2B_NL (4,"Bandwidth: ",d.bandwidth,
	 dvbstrTerrBandwidth_SCHEME (d.bandwidth));
 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_S2B_NL (4,"Constellation: ",d.constellation,
	 dvbstrTerrConstellation_FLAG(d.constellation));
 out_S2B_NL (4,"Hierarchy information: ",d.hierarchy_information,
	 dvbstrTerrHierarchy_FLAG(d.hierarchy_information));
 out_S2B_NL (4,"Code_rate_HP_stream: ",d.code_rate_HP_stream,
	 dvbstrTerrCodeRate_FLAG(d.code_rate_HP_stream));
 out_S2B_NL (4,"Code_rate_LP_stream: ",d.code_rate_LP_stream,
	 dvbstrTerrCodeRate_FLAG(d.code_rate_LP_stream));
 out_S2B_NL (4,"Guard_interval: ",d.guard_interval,
	 dvbstrTerrGuardInterval_FLAG(d.guard_interval));
 out_S2B_NL (4,"Transmission_mode: ",d.transmission_mode,
	 dvbstrTerrTransmissionMode_FLAG(d.transmission_mode));
 out_SB_NL (4,"Other_frequency_flag: ",d.other_frequency_flag);
 out_SL_NL (6,"reserved_2: ",d.reserved_2);


}







/*
  0x5B  Multilingual Network Name  descriptor 
  ETSI EN 300 468     6.2.21
*/

void descriptor_MultilingNetworkName (u_char *b)

{

 typedef struct  _descMultiNetName {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    //  N .. List2

 } descMultiNetName;

 typedef struct  _descMultiNetName2 {
    u_char     ISO639_2_language_code[3];
    u_int      network_name_length;

    //  N2 ..  char

 } descMultiNetName2;



 descMultiNetName   d;
 descMultiNetName2  d2;
 int                len1;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 b += 2;
 len1 = d.descriptor_length;

 indent (+1);
 while (len1 > 0 ) {

    strncpy (d2.ISO639_2_language_code, b, 3);	
    d2.network_name_length	 = getBits (b, 0, 24, 8);

    out_nl    (4,"ISO639_2_language_code:  %3.3s", d2.ISO639_2_language_code);
    out_SB_NL (5,"Network_name_length: ",d2.network_name_length);
    out       (4,"Network_name: ");
	print_name (4, b+4,d2.network_name_length);
 	out_NL (4);
    out_NL (4);

    len1 -= (4 + d2.network_name_length);
    b    +=  4 + d2.network_name_length;

 }
 indent (-1);

}








/*
  0x5C  Multilingual Bouquet Name  descriptor 
  ETSI EN 300 468     6.2.19
*/

void descriptor_MultilingBouquetName (u_char *b)

{

 typedef struct  _descMultiBouqName {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    //  N .. List2

 } descMultiBouqName;

 typedef struct  _descMultiBouqName2 {
    u_char     ISO639_2_language_code[3];
    u_int      bouquet_name_length;

    //  N2 ..  char

 } descMultiBouqName2;



 descMultiBouqName   d;
 descMultiBouqName2  d2;
 int                 len1;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 b += 2;
 len1 = d.descriptor_length;

 indent (+1);
 while (len1 > 0 ) {

    strncpy (d2.ISO639_2_language_code, b, 3);	
    d2.bouquet_name_length	 = getBits (b, 0, 24, 8);

    out_nl    (4,"ISO639_2_language_code:  %3.3s", d2.ISO639_2_language_code);
    out_SB_NL (5,"Bouquet_name_length: ",d2.bouquet_name_length);
    out       (4,"Bouquet_name: ");
	print_name (4, b+4,d2.bouquet_name_length);
 	out_NL (4);
    out_NL (4);

    len1 -= (4 + d2.bouquet_name_length);
    b    +=  4 + d2.bouquet_name_length;

 }
 indent (-1);

}






/*
  0x5D  Multilingual Service Name  descriptor 
  ETSI EN 300 468     6.2.22
*/

void descriptor_MultilingServiceName (u_char *b)

{

 typedef struct  _descMultiServiceName {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    //  N .. List2

 } descMultiServiceName;

 typedef struct  _descMultiServiceName2 {
    u_char     ISO639_2_language_code[3];
    u_int      service_provider_name_length;

    //  N2 ..  char

    u_int      service_name_length;

    //  N3 ..  char

 } descMultiServiceName2;



 descMultiServiceName   d;
 descMultiServiceName2  d2;
 int                    len1;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 b += 2;
 len1 = d.descriptor_length;

 indent (+1);
 while (len1 > 0 ) {

    strncpy (d2.ISO639_2_language_code, b, 3);	
    d2.service_provider_name_length	 = getBits (b, 0, 24, 8);

    out_nl    (4,"ISO639_2_language_code:  %3.3s", d2.ISO639_2_language_code);
    out_SB_NL (5,"Service_provider_name_length: ",d2.service_provider_name_length);
    out       (4,"Service_provider_name: ");
	print_name (4, b+4,d2.service_provider_name_length);
 	out_NL (4);

    len1 -= (4 + d2.service_provider_name_length);
    b    +=  4 + d2.service_provider_name_length;


    d2.service_name_length	 = getBits (b, 0, 24, 8);
    out_SB_NL (5,"Service_name_length: ",d2.service_name_length);
    out       (4,"Service_name: ");
	print_name (4, b+4,d2.service_name_length);
 	out_NL (4);

    len1 -= (4 + d2.service_name_length);
    b    +=  4 + d2.service_name_length;

    out_NL (4);

 }
 indent (-1);

}







/*
  0x5E  Multilingual Component  descriptor 
  ETSI EN 300 468     6.2.20
*/

void descriptor_MultilingComponent (u_char *b)

{

 typedef struct  _descMultiComponent {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      component_tag;

    //  N .. List2

 } descMultiComponent;

 typedef struct  _descMultiComponent2 {
    u_char     ISO639_2_language_code[3];
    u_int      text_description_length;

    //  N2 ..  char

 } descMultiComponent2;



 descMultiComponent   d;
 descMultiComponent2  d2;
 int                  len1;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];


 d.component_tag 		 = b[2];

 out_SB_NL (4,"Component_tag: ",d.component_tag);

 b += 3;
 len1 = d.descriptor_length + 1;

 indent (+1);
 while (len1 > 0 ) {

    strncpy (d2.ISO639_2_language_code, b, 3);	
    d2.text_description_length	 = getBits (b, 0, 24, 8);

    out_nl    (4,"ISO639_2_language_code:  %3.3s", d2.ISO639_2_language_code);
    out_SB_NL (5,"Text_description_length: ",d2.text_description_length);
    out       (4,"Text_description: ");
	print_name (4, b+4,d2.text_description_length);
 	out_NL (4);
    out_NL (4);

    len1 -= (4 + d2.text_description_length);
    b    +=  4 + d2.text_description_length;

 }
 indent (-1);

}








/*
  0x5F  Private Data Specifier  descriptor 
  ETSI EN 300 468     6.2.28
*/

void descriptor_PrivateDataSpecifier (u_char *b)

{

 typedef struct  _descPrivDataSpec {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_long     private_data_specifier;

 } descPrivDataSpec;


 descPrivDataSpec d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.private_data_specifier	 = getBits (b, 0, 16, 32);

 
 out_SL_NL (4,"Private_data_specifier: ",d.private_data_specifier);

}







/*
  0x60  Service Move  descriptor 
  ETSI EN 300 468     6.2.32
*/

void descriptor_ServiceMove  (u_char *b)

{

 typedef struct  _descServMove {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      new_original_network_id;
    u_int      new_transport_stream_id;
    u_int      new_service_id;

 } descServMove;


 descServMove    d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.new_original_network_id	 = getBits (b, 0, 16, 16);
 d.new_transport_stream_id	 = getBits (b, 0, 32, 16);
 d.new_service_id		 = getBits (b, 0, 48, 16);



  out_S2W_NL (4,"New_original_network_ID: ",d.new_original_network_id,
	dvbstrNetworkIdent_ID(d.new_original_network_id));
  out_SW_NL  (4,"New_transport_stream_ID: ",d.new_transport_stream_id);
  out_SW_NL (4,"Service_ID: ",d.new_service_id);

}








/*
  0x61  Short Smoothing Buffer  descriptor 
  ETSI EN 300 468     6.2.29
*/

void descriptor_ShortSmoothingBuffer  (u_char *b)

{

 typedef struct  _descSSBuf {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      sb_size;
    u_int      sb_leak_rate;

 } descSSBuf;


 descSSBuf  d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.sb_size			 = getBits (b, 0, 16, 2);
 d.sb_leak_rate			 = getBits (b, 0, 16, 6);

 
 out_S2B_NL (4,"sb_size: ", d.sb_size,
	dvbstrShortSmoothingBufSize_TYPE (d.sb_size) );
 out_S2B_NL (4,"sb_leak_rate: ", d.sb_leak_rate,
	dvbstrShortSmoothingBufLeakRate_TYPE (d.sb_leak_rate) );

 out_nl (6,"Reserved:");
   printhexdump_buf (6, b+3,d.descriptor_length-1);


}










/*
  0x62  Frequency List descriptor 
  ETSI EN 300 468     6.2.15
*/

void descriptor_FrequencyList  (u_char *b)

{

 typedef struct  _descFreqList {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      reserved_1;
    u_int      coding_type;
 } descFreqList;

 typedef struct  _descFreqList2 {
    u_long     centre_frequency;
 } descFreqList2;


 descFreqList   d;
 descFreqList2  d2;
 int            len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.reserved_1			 = getBits (b, 0, 16, 6);
 d.coding_type			 = getBits (b, 0, 22, 2);

 

 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_S2B_NL (4,"coding_type: ",d.coding_type,
	dvbstrDelivSysCoding_TYPE(d.coding_type));


 b += 3;
 len = d.descriptor_length - 1;
 indent (+1);
 while (len > 0) {

    d2.centre_frequency		 = getBits (b, 0, 0, 32);

    out (4,"Centre_frequency: %08lx  ",d2.centre_frequency);
    switch (d.coding_type) {
	case 0x01:
 	  out_nl (4,"(= %3lx.%05lx GHz)",
	    d2.centre_frequency >> 20, d2.centre_frequency & 0x000FFFFF );
	  break;

	case 0x02:
 	  out_nl (4,"(= %3lx.%05lx MHz)",
	    d2.centre_frequency >> 16, d2.centre_frequency & 0x0000FFFF );
	  break;

	case 0x03:
 	  out_nl (4,"(= %lu Hz)", d2.centre_frequency * 10 );
	  break;
    }

    len -= 4;
    b   += 4;
 }
 indent (-1);


}







/*
  0x63  Partial Transport Stream Descriptor
  ETSI EN 300 468     6.2.26
*/

void descriptor_PartialTransportStream  (u_char *b)

{

 typedef struct  _descPartTranspStream {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      reserved_1;
    u_long     peak_rate;
    u_int      reserved_2;
    u_long     minimum_overall_smoothing_rate;
    u_int      reserved_3;
    u_int      maximum_overall_smoothing_buffer;

 } descPartTranspStream;


 descPartTranspStream   d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.reserved_1			 = getBits (b, 0, 16, 2);
 d.peak_rate			 = getBits (b, 0, 18, 22);
 d.reserved_2			 = getBits (b, 0, 40, 2);
 d.minimum_overall_smoothing_rate= getBits (b, 0, 42, 22);
 d.reserved_3			 = getBits (b, 0, 64, 2);
 d.maximum_overall_smoothing_buffer= getBits (b, 0, 66, 14);

 

 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out_nl    (4,"peak_rate: 0x%06lx  (= %lu bits/sec)",
	d.peak_rate,d.peak_rate/400);

 out_SB_NL (6,"reserved_2: ",d.reserved_2);
 out_nl    (4,"minimum_overall_smoothing_rate: 0x%06lx  (= %lu bits/sec)",
	d.minimum_overall_smoothing_rate,
	d.minimum_overall_smoothing_rate/400);

 out_SB_NL (6,"reserved_3: ",d.reserved_3);
 out_nl    (4,"maximum_overall_smoothing_buffer: 0x%04x  (= %lu bits/sec)",
	d.maximum_overall_smoothing_buffer,
	d.maximum_overall_smoothing_buffer/400);


}









/*
  0x64  DataBroadcast  descriptor 
  ETSI EN 300 468    6.2.10
*/

void descriptor_DataBroadcast (u_char *b)

{

 typedef struct  _descDataBroadcast {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      data_broadcast_id;
    u_int      component_tag;
    u_int      selector_length;

    // N   bytes

    u_char     ISO639_2_language_code[3];
    u_int      text_length;

    // N2  char 

 } descDataBroadcast;


 descDataBroadcast d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.data_broadcast_id		 = getBits (b, 0, 16, 16);
 d.component_tag		 = getBits (b, 0, 32, 8);
 d.selector_length		 = getBits (b, 0, 40, 8);

 
 out_S2W_NL (4,"Data_broadcast_ID: ",d.data_broadcast_id,
	dvbstrNetworkIdent_ID(d.data_broadcast_id));

 out_SB_NL (4,"Component_tag: ",d.component_tag);
 out_SB_NL (5,"Selector_length: ",d.selector_length);
 out_nl    (4,"Selector-Bytes:");
 b += 6;
 printhexdump_buf (4,  b, d.selector_length);

 b += d.selector_length;
 strncpy (d.ISO639_2_language_code, b, 3);	
 d.text_length			 = getBits (b, 0, 24, 8);

 out_nl    (4,"ISO639_2_language_code:  %3.3s", d.ISO639_2_language_code);
 out_SB_NL (5,"Text_length: ",d.text_length);
 out       (4,"Text: ");
	print_name (4, b+4,d.text_length);
 	out_NL (4);

}








/*
  0x65  CA System descriptor 
  ETSI EN 300 468     6.2.x
*/

void descriptor_CASystem (u_char *b)

{


  descriptor_any (b);

}






/*
  0x66  Data Broadcast ID  descriptor 
  ETSI EN 300 468     6.2.11
*/

void descriptor_DataBroadcastID  (u_char *b)

{

 typedef struct  _descDataBroadcastID {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      data_broadcast_id;

    //  N ... id_selector bytes
 } descDataBroadcastID;


 descDataBroadcastID   d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.data_broadcast_id		 = getBits (b, 0, 16, 16);

 
 out_S2W_NL (4,"Data_broadcast_ID: ",d.data_broadcast_id,
	dvbstrNetworkIdent_ID(d.data_broadcast_id));

 out_nl (4,"ID_selector_bytes: ");
     printhexdump_buf (4, b+4, d.descriptor_length-4);

}





/*
  0x67  Transport Stream  descriptor 
  ETSI EN 300 468     6.2.41
*/

void descriptor_TransportStream  (u_char *b)

{

 typedef struct  _descTransportStream {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    //   N ... bytes
 } descTransportStream;


 descTransportStream   d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 
 out_nl (4,"Transport-stream-bytes: ");
     printhexdump_buf (4, b+2, d.descriptor_length);

}








/*
  0x68  DSNG  descriptor 
  ETSI EN 300 468     6.2.13
*/

void descriptor_DSNG  (u_char *b)

{

 typedef struct  _descDSNG {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    //  N ... bytes
 } descDSNG;


 descDSNG   d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 out_nl (4,"DSNG-bytes: ");
     printhexdump_buf (4, b+2, d.descriptor_length);

}



/*
  0x69  PDC  descriptor 
  ETSI EN 300 468     6.2.27
*/

void descriptor_PDC  (u_char *b)

{

 typedef struct  _descPDC {
    u_int      descriptor_tag;
    u_int      descriptor_length;


    u_int      reserved_1;
    u_long     programme_identification_label;
    // ... splits in
    u_int     day;
    u_int     month;
    u_int     hour;
    u_int     minute;

 } descPDC;


 descPDC   d;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.reserved_1			 = getBits (b, 0, 16, 4);
 d.programme_identification_label = getBits (b, 0, 20, 20);

    d.day     = getBits (b,0,20,5);
    d.month   = getBits (b,0,25,4);
    d.hour    = getBits (b,0,29,5);
    d.minute  = getBits (b,0,34,6);


 out_SB_NL (6,"reserved_1: ",d.reserved_1);
 out       (4,"Programme_identification_label: 0x%05lx ",
	d.programme_identification_label);
 out       (4,"[= month: %d  day= %d   hour=%d  min=%d]",
	d.month, d.day, d.hour, d.minute);

}





/*
  0x6A  AC-3  descriptor 
  ETSI EN 300 468    ANNEX E 
*/

void descriptor_AC3  (u_char *b)

{

 typedef struct  _descAC3 {
    u_int      descriptor_tag;
    u_int      descriptor_length;

    u_int      AC3_type_flag;
    u_int      bsid_flag;
    u_int      mainid_flag;
    u_int      asvc_flag;
    u_int      reserved_1;

    // conditional vars
    u_int      AC3_type;
    u_int      bsid_type;
    u_int      mainid_type;
    u_int      asvc_type;

    // N ...  bytes add info

 } descAC3;


 descAC3   d;
 int       len;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.AC3_type_flag		 = getBits (b, 0, 16, 1);
 d.bsid_flag			 = getBits (b, 0, 17, 1);
 d.mainid_flag			 = getBits (b, 0, 18, 1);
 d.asvc_flag			 = getBits (b, 0, 19, 1);
 d.reserved_1			 = getBits (b, 0, 20, 4);



 out_SB_NL (4,"AC3_type_flag: ",d.AC3_type_flag);
 out_SB_NL (4,"bsid_flag: ",d.bsid_flag);
 out_SB_NL (4,"mainid_flag: ",d.mainid_flag);
 out_SB_NL (4,"asvc_flag: ",d.asvc_flag);
 out_SB_NL (6,"reserved_1: ",d.reserved_1);

 b   += 3;
 len  = d.descriptor_length - 2;

 if (d.AC3_type_flag) {
     d.AC3_type			 = b[0];
     b++;
     len--;
     out_SB_NL (4,"AC3_type: ",d.AC3_type);
 }

 if (d.bsid_flag) {
     d.bsid_flag		 = b[0];
     b++;
     len--;
     out_SB_NL (4,"bsid_flag: ",d.bsid_flag);
 }

 if (d.mainid_flag) {
     d.mainid_flag		 = b[0];
     b++;
     len--;
     out_SB_NL (4,"mainid_flag: ",d.mainid_flag);
 }

 if (d.asvc_flag) {
     d.asvc_flag		 = b[0];
     b++;
     len--;
     out_SB_NL (4,"asvc_flag: ",d.asvc_flag);
 }

 out_nl (4,"Additional info:");
    printhexdump_buf (4, b, len);

}





/*
  0x6B  Ancillary Data  descriptor 
  ETSI EN 300 468     6.2.1
*/

void descriptor_AncillaryData  (u_char *b)

{

 typedef struct  _descAncillaryData {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      ancillary_data_identifier;

 } descAncillaryData;


 descAncillaryData  d;
 u_int              i;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.ancillary_data_identifier	 = b[2];

 out_SB_NL (4,"Ancillary_data_identifier: ",
	d.ancillary_data_identifier);


 // $$$ the following should normally in dvbStrAncillaryData...()

 i = d.ancillary_data_identifier;
 indent (+1);
   if (i & 0x01) out_nl (4,"[= DVD-Video Ancillary Data]");
   if (i & 0x02) out_nl (4,"[= Extended Ancillary Data]");
   if (i & 0x04) out_nl (4,"[= Announcement Switching Data]");
   if (i & 0x08) out_nl (4,"[= DAB Ancillary Data]");
   if (i & 0x10) out_nl (4,"[= Scale Factor Error Check]");
   if (i & 0xE0) out_nl (4,"[= reserved ]");
 indent (-1);


}




/*
  0x6C  Cell List descriptor 
  ETSI EN 300 468     6.2.6
*/

void descriptor_CellList  (u_char *b)

{

 descriptor_any (b);
 out_nl (4," ==> ERROR: CellList descriptor not implemented, Report!");


}




/*
  0x6D  Cell Frequency Link descriptor 
  ETSI EN 300 468     6.2.5
*/

void descriptor_CellFrequencyLink  (u_char *b)

{

 descriptor_any (b);
 out_nl (4," ==> ERROR: CellFrequencyLink descriptor not implemented, Report!");

}





/*
  0x6E  Announcement Support descriptor 
  ETSI EN 300 468     6.2.2
*/

void descriptor_AnnouncementSupport (u_char *b)

{

 typedef struct  _descAnnouncementSupport {
    u_int      descriptor_tag;
    u_int      descriptor_length;		

    u_int      announcement_support_indicator;
    // N .. Announcement 2

 } descAnnouncementSupport;

 typedef struct  _descAnnouncementSupport2 {
    u_int      announcement_type;
    u_int      reserved_1;
    u_int      reference_type;
    // conditional data
    u_int      original_network_id;
    u_int      transport_stream_id;
    u_int      service_id;
    u_int      component_tag;


 } descAnnouncementSupport2;



 descAnnouncementSupport   d;
 descAnnouncementSupport2  d2;
 int                       len;
 int                       i;



 d.descriptor_tag		 = b[0];
 d.descriptor_length       	 = b[1];

 d.announcement_support_indicator = getBits (b, 0, 16, 16);
 b   += 4;
 len = d.descriptor_length - 4;


 out_SW_NL (4,"Announcement_support_indicator: ",
	d.announcement_support_indicator);

    i = d.announcement_support_indicator;
    // $$$ should be in dvbstr...()
    indent (+1);
      if (i & 0x01) out_nl (4,"[= Emergency alarm]");
      if (i & 0x02) out_nl (4,"[= Road Traffic Flash]");
      if (i & 0x04) out_nl (4,"[= Public Transport Flash]");
      if (i & 0x08) out_nl (4,"[= Warning message]");
      if (i & 0x10) out_nl (4,"[= News flash]");
      if (i & 0x20) out_nl (4,"[= Weather flash]");
      if (i & 0x40) out_nl (4,"[= Event announcement]");
      if (i & 0x80) out_nl (4,"[= Personal call]");
      if (i & 0xFF00) out_nl (4,"[= reserved ]");
    indent (-1);
    out_NL (4);

    

 indent (+1);
 while (len > 0) {
    d2.announcement_type	 = getBits (b, 0,  0,  4);
    d2.reserved_1		 = getBits (b, 0,  4,  1);
    d2.reference_type		 = getBits (b, 0,  5,  3);

    b   += 1;
    len -= 1;

    out_S2B_NL (4,"Announcement_type: ",d2.announcement_type,
	dvbstrAnnouncement_TYPE (d2.announcement_type) );
    out_SB_NL  (6,"reserved_1: ",d2.reserved_1);
    out_S2B_NL (4,"reference_type: ",d2.reference_type,
	dvbstrAnnouncementReference_TYPE (d2.reference_type) );


    i = d2.reference_type;
    if (i == 1 || i == 2 || i == 3) {
       d2.original_network_id	 = getBits (b, 0,  0, 16);
       d2.transport_stream_id	 = getBits (b, 0, 16, 16);
       d2.service_id		 = getBits (b, 0, 32, 16);
       d2.component_tag		 = getBits (b, 0, 48,  8);

       b   += 7;
       len -= 7;

       out_S2W_NL (4,"Original_network_ID: ",d2.original_network_id,
           dvbstrNetworkIdent_ID(d2.original_network_id));
       out_SW_NL  (4,"Transport_stream_ID: ",d2.transport_stream_id);
       out_SW_NL  (4,"Service_ID: ",d2.service_id);
       out_SB_NL  (4,"Component_tag: ",d2.component_tag);

    } // if


 } // while
 indent (-1); 

}





