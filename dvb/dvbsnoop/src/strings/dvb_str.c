/*
$Id: dvb_str.c,v 1.34 2004/01/05 02:03:42 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 http://dvbsnoop.sourceforge.net/

 (c) 2001-2004   Rainer.Scherg@gmx.de


  -- DVB-Strings
  -- dvb decoder helper functions






$Log: dvb_str.c,v $
Revision 1.34  2004/01/05 02:03:42  rasc
no message

Revision 1.33  2004/01/01 20:09:40  rasc
DSM-CC INT/UNT descriptors
PES-sync changed, TS sync changed,
descriptor scope
other changes

Revision 1.32  2003/12/30 14:05:38  rasc
just some annotations, so I do not forget these over Sylvester party...
(some alkohol may reformat parts of /devbrain/0 ... )
cheers!

Revision 1.31  2003/12/28 00:01:15  rasc
some minor changes/adds...

Revision 1.30  2003/12/27 18:17:18  rasc
dsmcc PES dsmcc_program_stream_descriptorlist

Revision 1.29  2003/12/27 00:21:17  rasc
dsmcc section tables

Revision 1.28  2003/11/26 19:55:34  rasc
no message

Revision 1.27  2003/11/25 00:17:11  rasc
-sync option, some TS and PES stuff;
dsm_addr inactive, may be wrong - due to missing ISO 13818-6

Revision 1.26  2003/11/24 23:52:18  rasc
-sync option, some TS and PES stuff;
dsm_addr inactive, may be wrong - due to missing ISO 13818-6

Revision 1.25  2003/11/24 14:16:07  obi
- corrected transport scrambling control bits according to ETSI ETR 289
- fixed lots of broken strings

Revision 1.24  2003/11/09 20:48:35  rasc
pes data packet (DSM-CC)

Revision 1.23  2003/11/07 16:33:32  rasc
no message

Revision 1.22  2003/11/01 21:40:28  rasc
some broadcast/linkage descriptor stuff

Revision 1.21  2003/10/29 20:54:57  rasc
more PES stuff, DSM descriptors, testdata

Revision 1.20  2003/10/27 22:43:50  rasc
carousel info descriptor and more

Revision 1.19  2003/10/26 23:00:43  rasc
fix

Revision 1.18  2003/10/25 19:11:50  rasc
no message

Revision 1.17  2003/10/21 19:54:43  rasc
no message

Revision 1.16  2003/10/19 22:22:57  rasc
- some datacarousell stuff started

Revision 1.15  2003/10/19 21:05:53  rasc
- some datacarousell stuff started

Revision 1.14  2003/10/19 13:54:25  rasc
-more table decoding

Revision 1.13  2003/10/17 19:04:11  rasc
- started more work on newer ISO 13818  descriptors
- some reorg/update work started

Revision 1.12  2003/10/17 18:16:54  rasc
- started more work on newer ISO 13818  descriptors
- some reorg work started

Revision 1.11  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162

Revision 1.10  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?

Revision 1.9  2003/06/24 23:51:03  rasc
bugfixes and enhancements

Revision 1.8  2002/08/17 20:36:12  obi
no more compiler warnings

Revision 1.7  2001/12/07 22:17:20  rasc
no message

Revision 1.6  2001/12/06 15:33:18  rasc
some small work on pespacket.c

Revision 1.5  2001/12/01 12:34:17  rasc
pespacket weitergestrickt, leider z.Zt. zuwenig Zeit um es richtig fertig zu machen.

Revision 1.4  2001/10/06 18:19:18  Toerli
Steuerzeichen entfernt. rasc wuerdest du mal bitte nen gescheiten unix-konformen Editor verwenden... windows editoren sind ungeeignet

Revision 1.3  2001/10/05 17:43:37  rasc
typo...

Revision 1.2  2001/10/02 21:52:44  rasc
- init der time_delta
- PES erweitert, PES arbeitet im read() noch nicht richtig!!
- muss tmbinc fragem, ob ich Mist baue, oder der Treiber (??)

Revision 1.1  2001/09/30 13:05:20  rasc
dvbsnoop v0.7  -- Commit to CVS


*/



#include "dvbsnoop.h"
#include "dvb_str.h"



typedef struct _STR_TABLE {
    u_int    from;          /* e.g. from id 1  */
    u_int    to;            /*      to   id 3  */
    u_char   *str;          /*      is   string xxx */
} STR_TABLE;




/*
  -- match id in range from STR_TABLE
*/

static char *findTableID (STR_TABLE *t, u_int id)

{

  while (t->str) {
    if (t->from <= id && t->to >= id)
       return t->str;
    t++;
  }

  return ">>ERROR: not (yet) defined... Report!<<";
}





/* -----------------------------------------  */



/*
  --  Table IDs (sections)
 ETSI EN 468   5.2
 EN 301 192
 TR 102 006
 RE 101 202
 ISO 13818-1
*/

char *dvbstrTableID (u_int id)

{
  STR_TABLE  TableIDs[] = {

 	// $$$ TODO DSM-CC  anyone a ISO 13818-6 tp spare???
 	// updated -- 2003-11-04
	// ATSC Table IDs could be included...
     {  0x00, 0x00,  "program_association_section" },
     {  0x01, 0x01,  "conditional_access_section" },
     {  0x02, 0x02,  "program_map_section" },
     {  0x03, 0x03,  "transport_stream_description_section" },
     {  0x04, 0x04,  "ISO_IEC_14496_scene_description_section" },	/* $$$ TODO */
     {  0x05, 0x05,  "ISO_IEC_14496_object_description_section" },	/* $$$ TODO */
      {  0x06, 0x37,  "ITU-T Rec. H.222.0|ISO/IEC13818 reserved" },
      {  0x38, 0x39,  "DSM-CC - reserved " },
     {  0x3a, 0x3a,  "DSM-CC - multiprotocol encapsulated data" },
     {  0x3b, 0x3b,  "DSM-CC - U-N messages (DSI or DII)" },
     {  0x3c, 0x3c,  "DSM-CC - Download Data Messages (DDB)" },    /* TR 101 202 */
     {  0x3d, 0x3d,  "DSM-CC - stream descriptorlist" },
     {  0x3e, 0x3e,  "DSM-CC - private data section (datagram)" },
      {  0x3f, 0x3f,  "DSM-CC - addressable sections" },	// $$$ TODO

     {  0x40, 0x40,  "network_information_section - actual network" },
     {  0x41, 0x41,  "network_information_section - other network" },
     {  0x42, 0x42,  "service_description_section - actual transport stream" },
     {  0x43, 0x45,  "reserved" },
     {  0x46, 0x46,  "service_description_section - other transport stream" },
     {  0x47, 0x49,  "reserved" },
     {  0x4A, 0x4A,  "bouquet_association_section" },
     {  0x4B, 0x4B,  "update notification table" },	/* TR 102 006 */
     {  0x4C, 0x4C,  "IP/MAC notification table [EN 301 192]" },  /* EN 192 */
     {  0x4D, 0x4D,  "reserved" },

     {  0x4E, 0x4E,  "event_information_section - actual transport stream, present/following" },
     {  0x4F, 0x4F,  "event_information_section - other transport stream, present/following" },
     {  0x50, 0x5F,  "event_information_section - actual transport stream, schedule" },
     {  0x60, 0x6F,  "event_information_section - other transport stream, schedule" },
     {  0x70, 0x70,  "time_date_section" },
     {  0x71, 0x71,  "running_status_section" },
     {  0x72, 0x72,  "stuffing_section" },
     {  0x73, 0x73,  "time_offset_section" },
     {  0x74, 0x74,  "AIT Application_information_section" }, /* MHP */
     {  0x75, 0x7D,  "reserved" },
     {  0x7E, 0x7E,  "discontinuity_information_section" },
     {  0x7F, 0x7F,  "selection_information_section" },
     {  0x80, 0x8F,  "DVB CA message section (EMM/ECM)" },   /* ITU-R BT.1300 ref. */
     {  0x90, 0xBF,  "User private" },
     {  0xC0, 0xFE,  "ATSC reserved" },		/* ETR 211e02 */
     {  0xFF, 0xFF,  "forbidden" },
     {  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}


/*
  -- Descriptor table tags
*/

char *dvbstrMPEGDescriptorTAG (u_int tag)

{
  STR_TABLE  Tags[] = {
// ISO 13818-1
     {  0x00, 0x01,  "Reserved" },
     {  0x02, 0x02,  "video_stream_descriptor" },
     {  0x03, 0x03,  "audio_stream_descriptor" },
     {  0x04, 0x04,  "hierarchy_descriptor" },
     {  0x05, 0x05,  "registration_descriptor" },
     {  0x06, 0x06,  "data_stream_alignment_descriptor" },
     {  0x07, 0x07,  "target_background_grid_descriptor" },
     {  0x08, 0x08,  "video_window_descriptor" },
     {  0x09, 0x09,  "CA_descriptor" },
     {  0x0A, 0x0A,  "ISO_639_language_descriptor" },
     {  0x0B, 0x0B,  "system_clock_descriptor" },
     {  0x0C, 0x0C,  "multiplex_buffer_utilization_descriptor" },
     {  0x0D, 0x0D,  "copyright_descriptor" },
     {  0x0E, 0x0E,  "maximum_bitrate_descriptor" },
     {  0x0F, 0x0F,  "private_data_indicator_descriptor" },
     {  0x10, 0x10,  "smoothing_buffer_descriptor" },
     {  0x11, 0x11,  "STD_descriptor" },
     {  0x12, 0x12,  "IBP_descriptor" },
          /* MPEG DSM-CC */
     {  0x13, 0x13,  "carousel_identifier_descriptor" },
     {  0x14, 0x14,  "association_tag_descriptor" },
     {  0x15, 0x15,  "deferred_association_tag_descriptor" },
     {  0x16, 0x16,  "ISO/IEC13818-6 Reserved" },
     /* $$$ TODO... vvvvvvv */
     	  /* DSM-CC stream descriptors */
     {  0x17, 0x17,  "NPT_reference_descriptor" },
     {  0x18, 0x18,  "NPT_endpoint_descriptor" },
     {  0x19, 0x19,  "stream_mode_descriptor" },
     {  0x1A, 0x1A,  "stream_event_descriptor" },
     /* $$$ TODO... ^^^^^^^^ */
          /* MPEG-4 descriptors */
     {  0x1B, 0x1B,  "MPEG4_video_descriptor" },
     {  0x1C, 0x1C,  "MPEG4_audio_descriptor" },
     {  0x1D, 0x1D,  "IOD_descriptor" },
     {  0x1E, 0x1E,  "SL_descriptor" },
     {  0x1F, 0x1F,  "FMC_descriptor" },
     {  0x20, 0x20,  "External_ES_ID_descriptor" },
     {  0x21, 0x21,  "MuxCode_descriptor" },
     {  0x22, 0x22,  "FMXBufferSize_descriptor" },
     {  0x23, 0x23,  "MultiplexBuffer_descriptor" },
     {  0x24, 0x24,  "FlexMuxTiming_descriptor" },

     {  0x25, 0x3F,  "ITU-T.Rec.H.222.0|ISO/IEC13818-1 Reserved" },

     {  0x40, 0xFF,  "Forbidden descriptor in MPEG context" },	// DVB Context
     {  0,0, NULL }
  };


  return findTableID (Tags, tag);
}
/*
  -- Descriptor table tags
*/

char *dvbstrDVBDescriptorTAG (u_int tag)

{
  STR_TABLE  Tags[] = {
     {  0x00, 0x3F,  "Forbidden descriptor in DVB context" },   // MPEG Context

// ETSI 300 468
     {  0x40, 0x40,  "network_name_descriptor" },
     {  0x41, 0x41,  "service_list_descriptor" },
     {  0x42, 0x42,  "stuffing_descriptor" },
     {  0x43, 0x43,  "satellite_delivery_system_descriptor" },
     {  0x44, 0x44,  "cable_delivery_system_descriptor" },
     {  0x45, 0x45,  "VBI_data_descriptor" },
     {  0x46, 0x46,  "VBI_teletext_descriptor" },
     {  0x47, 0x47,  "bouquet_name_descriptor" },
     {  0x48, 0x48,  "service_descriptor" },
     {  0x49, 0x49,  "country_availibility_descriptor" },
     {  0x4A, 0x4A,  "linkage_descriptor" },
     {  0x4B, 0x4B,  "NVOD_reference_descriptor" },
     {  0x4C, 0x4C,  "time_shifted_service_descriptor" },
     {  0x4D, 0x4D,  "short_event_descriptor" },
     {  0x4E, 0x4E,  "extended_event_descriptor" },
     {  0x4F, 0x4F,  "time_shifted_event_descriptor" },
     {  0x50, 0x50,  "component_descriptor" },
     {  0x51, 0x51,  "mosaic_descriptor" },
     {  0x52, 0x52,  "stream_identifier_descriptor" },
     {  0x53, 0x53,  "CA_identifier_descriptor" },
     {  0x54, 0x54,  "content_descriptor" },
     {  0x55, 0x55,  "parental_rating_descriptor" },
     {  0x56, 0x56,  "teletext_descriptor" },
     {  0x57, 0x57,  "telephone_descriptor" },
     {  0x58, 0x58,  "local_time_offset_descriptor" },
     {  0x59, 0x59,  "subtitling_descriptor" },
     {  0x5A, 0x5A,  "terrestrial_delivery_system_descriptor" },
     {  0x5B, 0x5B,  "multilingual_network_name_descriptor" },
     {  0x5C, 0x5C,  "multilingual_bouquet_name_descriptor" },
     {  0x5D, 0x5D,  "multilingual_service_name_descriptor" },
     {  0x5E, 0x5E,  "multilingual_component_descriptor" },
     {  0x5F, 0x5F,  "private_data_specifier_descriptor" },
     {  0x60, 0x60,  "service_move_descriptor" },
     {  0x61, 0x61,  "short_smoothing_buffer_descriptor" },
     {  0x62, 0x62,  "frequency_list_descriptor" },
     {  0x63, 0x63,  "partial_transport_stream_descriptor" },
     {  0x64, 0x64,  "data_broadcast_descriptor" },
     {  0x65, 0x65,  "CA_system_descriptor" },
     {  0x66, 0x66,  "data_broadcast_id_descriptor" },
     {  0x67, 0x67,  "transport_stream_descriptor" },
     {  0x68, 0x68,  "DSNG_descriptor" },
     {  0x69, 0x69,  "PDC_descriptor" },
     {  0x6A, 0x6A,  "AC3_descriptor" },
     {  0x6B, 0x6B,  "ancillary_data_descriptor" },
     {  0x6C, 0x6C,  "cell_list_descriptor" },
     {  0x6D, 0x6D,  "cell_frequency_list_descriptor" },
     {  0x6E, 0x6E,  "announcement_support_descriptor" },
     {  0x6F, 0x6F,  "application_signalling_descriptor" },
     {  0x70, 0x70,  "adaption_field_data_descriptor" },
     {  0x71, 0x71,  "service_identifier_descriptor" },
     {  0x72, 0x72,  "service_availability_descriptor" },
     {  0x73, 0x7F,  "reserved_descriptor" },
     {  0x80, 0xAF,  "ATSC reserved" },		/* ETR 211e02 */
     {  0xB0, 0xEF,  "User defined" },
     	{  0xf0, 0xf0,  "$$$ MHP Object Carousel" },	// $$$ TODO
     	{  0xf1, 0xf1,  "$$$ MHP Multiprotocol Encapsulation" },
	{  0xf2, 0xFE,  "reserved MHP" },

     {  0xFF, 0xFF,  "Forbidden" },
     {  0,0, NULL }
  };


  return findTableID (Tags, tag);
}


/*
 -- current_next_indicator
 -- ISO/IEC13818-1|ITU H.222.0
*/

char *dvbstrCurrentNextIndicator (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "valid next" },
     {  0x01, 0x01,  "valid now" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



/*
 -- delivery desctritor flags
 -- ETSI EN 468 6.2.12.1 ff
*/

char *dvbstrWEST_EAST_FLAG (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "WEST" },
     {  0x01, 0x01,  "EAST" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



char *dvbstrPolarisation_FLAG (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "linear - horizontal" },
     {  0x01, 0x01,  "linear - vertical" },
     {  0x02, 0x02,  "circular - left" },
     {  0x03, 0x03,  "circular - right" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}


char *dvbstrModulationSAT_FLAG (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not defined" },
     {  0x01, 0x01,  "QPSK" },
     {  0x02, 0x1F,  "reserved for future use" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



char *dvbstrModulationCable_FLAG (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not defined" },
     {  0x01, 0x01,  "16 QAM" },
     {  0x02, 0x02,  "32 QAM" },
     {  0x03, 0x03,  "64 QAM" },
     {  0x04, 0x04,  "128 QAM" },
     {  0x05, 0x05,  "256 QAM" },
     {  0x06, 0xFF,  "reserved for future use" },
     {  0,0, NULL }
  };


  return findTableID (Table, flag);
}



char *dvbstrFECinner_SCHEME (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not defined" },
     {  0x01, 0x01,  "1/2 conv. code rate" },
     {  0x02, 0x02,  "2/3 conv. code rate" },
     {  0x03, 0x03,  "3/4 conv. code rate" },
     {  0x04, 0x04,  "5/6 conv. code rate" },
     {  0x05, 0x05,  "7/8 conv. code rate" },
     {  0x06, 0x0E,  "reserved" },
     {  0x0F, 0x0F,  "No conv. coding" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



char *dvbstrFECouter_SCHEME (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not defined" },
     {  0x01, 0x01,  "no outer FEC coding" },
     {  0x02, 0x02,  "RS(204/188)" },
     {  0x03, 0x0F,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}





/*
  -- Linkage type descriptor
*/

char *dvbstrLinkage_TYPE (u_int flag)

{
  STR_TABLE  Table[] = {
	  /* -- updated 2003-10-19 */
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "information service" },
     {  0x02, 0x02,  "EPG service" },
     {  0x03, 0x03,  "CA replacement service" },
     {  0x04, 0x04,  "TS containing complete Network/Bouquet SI" },
     {  0x05, 0x05,  "service replacement service" },
     {  0x06, 0x06,  "data broadcast service" },
     {  0x07, 0x07,  "RCS Map" },
     {  0x08, 0x08,  "mobile handover service" },
     {  0x09, 0x09,  "system software update service" },
     {  0x0A, 0x0A,  "TS containing SSU BAT or NIT" },
//     {  0x0B, 0x7F,  "reserved" },   // own def...

     {  0x0B, 0x0B,  "IP/MAC Notification Table" },
     {  0x0C, 0x0C,  "Deferred IP/MAC Notification Table" },
     {  0x0D, 0x7F,  "reserved" },

     {  0x80, 0xFE,  "user defined" },
     {  0xFF, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



char *dvbstrHandover_TYPE (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "DVB hand-over to an identical service in a neighbouring country" },
     {  0x02, 0x02,  "DVB hand-over to local variation to same service" },
     {  0x03, 0x03,  "DVB hand-over to an associated service" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



char *dvbstrOrigin_TYPE (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "NIT" },
     {  0x01, 0x01,  "SDT" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



/*
 -- Service Link Descriptor
*/ 

char *dvbstrService_TYPE (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "digital television service" },
     {  0x02, 0x02,  "digital radio sound service" },
     {  0x03, 0x03,  "Teletext service" },
     {  0x04, 0x04,  "NVOD reference service" },
     {  0x05, 0x05,  "NVOD time-shifted service" },
     {  0x06, 0x06,  "mosaic service" },
     {  0x07, 0x07,  "PAL coded signal" },
     {  0x08, 0x08,  "SECAM coded signal" },
     {  0x09, 0x09,  "D/D2-MAC" },
     {  0x0A, 0x0A,  "FM-Radio" },
     {  0x0B, 0x0B,  "NTSC coded signal" },
     {  0x0C, 0x0C,  "data broadcast service" },
     {  0x0D, 0x0D,  "reserved for Common Interface Usage" },
     {  0x0E, 0x0E,  "RCS Map" },
     {  0x0F, 0x0F,  "RCS FLS" },
     {  0x10, 0x10,  "DVB  MHP service" },
     {  0x11, 0x7F,  "reserved" },
     {  0x80, 0xFE,  "User defined" },
     {  0xFF, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



/*
 -- Programm Map Table   Stream Type
*/

char *dvbstrStream_TYPE (u_int flag)

{
  /* ISO 13818-1  */

  STR_TABLE  Table[] = {
	  // -- updated 2003-10-17  from H.220
	  // -- updated 2003-11-04  from ATSC / ISO13818-6AMD1-2000
     {  0x00, 0x00,  "ITU-T | ISO-IE Reserved" },
     {  0x01, 0x01,  "ISO/IEC 11172 Video" },
     {  0x02, 0x02,  "ITU-T Rec. H.262 | ISO/IEC 13818-2 Video | ISO/IEC 11172-2 constr. parameter video stream" },
     {  0x03, 0x03,  "ISO/IEC 11172 Audio" },
     {  0x04, 0x04,  "ISO/IEC 13818-3 Audio" },
     {  0x05, 0x05,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private sections" },
     {  0x06, 0x06,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data" },
     {  0x07, 0x07,  "ISO/IEC 13512 MHEG" },
     {  0x08, 0x08,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A  DSM CC" },
     {  0x09, 0x09,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1/11172-1 auxiliary" },
     {  0x0A, 0x0A,  "ISO/IEC 13818-6 Multiprotocol encapsulation" },
     {  0x0B, 0x0B,  "ISO/IEC 13818-6 DSM-CC U-N Messages" },
     {  0x0C, 0x0C,  "ISO/IEC 13818-6 Stream Descriptors" },
     {  0x0D, 0x0D,  "ISO/IEC 13818-6 Sections (any type, including private data)" },
     {  0x0E, 0x0E,  "ISO/IEC 13818-1 auxiliary" },
     {  0x0F, 0x0F,  "ISO/IEC 13818-7 Audio with ADTS transport sytax" },
     {  0x10, 0x10,  "ISO/IEC 14496-2 Visual" },
     {  0x11, 0x11,  "ISO/IEC 14496-3 Audio with LATM transport syntax as def. in ISO/IEC 14496-3/AMD1" },
     {  0x12, 0x12,  "ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets" },
     {  0x13, 0x13,  "ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC 14496 sections" },
     {  0x14, 0x14,  "ISO/IEC 13818-6 DSM-CC synchronized download protocol" },

     {  0x15, 0x7F,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 reserved" },
     // $$$ ATSC ID Names could be includes...
     {  0x80, 0xFF,  "User private" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}



/*
 -- Audio Types (descriptor e.g. ISO 639)
*/

char *dvbstrAudio_TYPE (u_int flag)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "clean effects" },
     {  0x02, 0x02,  "hearing impaired" },
     {  0x03, 0x03,  "visual impaired commentary" },
     {  0x04, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}





/*
 -- CA-System Identifier  (ETSI ETR 162)
*/

char *dvbstrCASystem_ID (u_int id)

{
  STR_TABLE  Table[] = {
     // -- updated from dvb.org 2003-10-16
     {  0x0000, 0x0000,  "Reserved" },
     {  0x0001, 0x00FF,  "Standardized Systems" },
     {  0x0100, 0x01FF,  "Canal Plus (Seca/MediaGuard)" },
     {  0x0200, 0x02FF,  "CCETT" },
     {  0x0300, 0x03FF,  "MSG MediaServices GmbH" },
     {  0x0400, 0x04FF,  "Eurodec" },
     {  0x0500, 0x05FF,  "France Telecom (Viaccess)" },
     {  0x0600, 0x06FF,  "Irdeto" },
     {  0x0700, 0x07FF,  "Jerrold/GI/Motorola" },
     {  0x0800, 0x08FF,  "Matra Communication" },
     {  0x0900, 0x09FF,  "News Datacom (Videoguard)" },
     {  0x0A00, 0x0AFF,  "Nokia" },
     {  0x0B00, 0x0BFF,  "Norwegian Telekom (Conax)" },
     {  0x0C00, 0x0CFF,  "NTL" },
     {  0x0D00, 0x0DFF,  "Philips (Cryptoworks)" },
     {  0x0E00, 0x0EFF,  "Scientific Atlanta (Power VU)" },
     {  0x0F00, 0x0FFF,  "Sony" },
     {  0x1000, 0x10FF,  "Tandberg Television" },
     {  0x1100, 0x11FF,  "Thompson" },
     {  0x1200, 0x12FF,  "TV/COM" },
     {  0x1300, 0x13FF,  "HPT - Croatian Post and Telecommunications" },
     {  0x1400, 0x14FF,  "HRT - Croatian Radio and Television" },
     {  0x1500, 0x15FF,  "IBM" },
     {  0x1600, 0x16FF,  "Nera" },
     {  0x1700, 0x17FF,  "Beta Technik (Betacrypt)" },
     {  0x1800, 0x18FF,  "Kudelski SA"},
     {  0x1900, 0x19FF,  "Titan Information Systems"},
     {  0x2000, 0x20FF,  "Telefónica Servicios Audiovisuales"},
     {  0x2100, 0x21FF,  "STENTOR (France Telecom, CNES and DGA)"},
     {  0x2200, 0x22FF,  "Scopus Network Technologies"},
     {  0x2300, 0x23FF,  "BARCO AS"},
     {  0x2400, 0x24FF,  "StarGuide Digital Networks  "},
     {  0x2500, 0x25FF,  "Mentor Data System, Inc."},
     {  0x2600, 0x26FF,  "European Broadcasting Union"},
     {  0x4700, 0x47FF,  "General Instrument"},
     {  0x4800, 0x48FF,  "Telemann"},
     {  0x4900, 0x49FF,  "Digital TV Industry Alliance of China"},
     {  0x4A00, 0x4A0F,  "Tsinghua TongFang"},
     {  0x4A10, 0x4A1F,  "Easycas"},
     {  0x4A20, 0x4A2F,  "AlphaCrypt"},
     {  0x4A30, 0x4A3F,  "DVN Holdings"},
     {  0x4A40, 0x4A4F,  "Shanghai Advanced Digital Technology Co. Ltd. (ADT)"},
     {  0x4A50, 0x4A5F,  "Shenzhen Kingsky Company (China) Ltd"},
     {  0x4A60, 0x4A6F,  "@SKY"},
     {  0x4A70, 0x4A7F,  "DreamCrypt"},
     {  0x4A80, 0x4A8F,  "THALESCrypt"},
     {  0x4A90, 0x4A9F,  "Runcom Technologies"},
     {  0x4AA0, 0x4AAF,  "SIDSA"},
     {  0x4AB0, 0x4ABF,  "Beijing Compunicate Technology Inc."},
     {  0x4AC0, 0x4ACF,  "Latens Systems Ltd"},
     {  0,0, NULL }
  };

  return findTableID (Table, id);
}


/*
 -- Data Broadcast ID
 -- ETR 162
*/ 

char *dvbstrDataBroadcast_ID (u_int flag)

{
  STR_TABLE  Table[] = {
	  // -- upodated 2003-10-16
	  // { Data Broadcast ID, Data Broadcast ID,   "Data Broadcast Specification Name" },
	{ 0x0000, 0x0000,   "Reserved for future use" },
	{ 0x0001, 0x0001,   "Data pipe" },
	{ 0x0002, 0x0002,   "Asynchronous data stream" },
	{ 0x0003, 0x0003,   "Synchronous data stream" },
	{ 0x0004, 0x0004,   "Synchronised data stream" },
	{ 0x0005, 0x0005,   "Multi protocol encapsulation" },
	{ 0x0006, 0x0006,   "Data Carousel" },
	{ 0x0007, 0x0007,   "Object Carousel" },
	{ 0x0008, 0x0008,   "DVB ATM streams" },
	{ 0x0009, 0x0009,   "Higher Protocols based on asynchronous data streams" },
	// $$$ 0x0A = System Software Update   $$$ TODO TR 102 006
	{ 0x000A, 0x00ef,   "Reserved for future use by DVB" },
	{ 0x00F0, 0x00F0,   "MHP Object Carousel" },
	{ 0x00F1, 0x00F1,   "reserved for MHP Multi Protocol Encapsulation" },
	{ 0x00F2, 0x00Fe,   "Reserved for MHP use" },
	{ 0x00FF, 0x00FF,   "Reserved for future use by DVB" },
	{ 0x0100, 0x0100,   "Eutelsat Data Piping" },
	{ 0x0101, 0x0101,   "Eutelsat Data Streaming" },
	{ 0x0102, 0x0102,   "SAGEM IP encapsulation in MPEG-2 PES packets" },
	{ 0x0103, 0x0103,   "BARCO Data Broadcasting" },
	{ 0x0104, 0x0104,   "CyberCity Multiprotocol Encapsulation (New Media Communications Ltd.)" },
	{ 0x0105, 0x0105,   "CyberSat Multiprotocol Encapsulation (New Media Communications Ltd.)" },
	{ 0x0106, 0x0106,   "The Digital Network" },
	{ 0x0107, 0x0107,   "OpenTV Data Carousel" },
	{ 0x0108, 0x0108,   "Panasonic" },
	{ 0x0109, 0x0109,   "MSG MediaServices GmbH" },
	{ 0x010A, 0x010A,   "TechnoTrend" },
	{ 0x010B, 0x010B,   "Canal + Technologies system software download" },
	{ 0x0110, 0x0110,   "Televizja Polsat" },
	{ 0x0111, 0x0111,   "UK DTG" },
	{ 0x0112, 0x0112,   "SkyMedia" },
	{ 0x0113, 0x0113,   "Intellibyte DataBroadcasting" },
	{ 0x0114, 0x0114,   "TeleWeb Data Carousel" },
	{ 0x0115, 0x0115,   "TeleWeb Object Carousel" },
	{ 0x0116, 0x0116,   "TeleWeb" },
	{ 0x4444, 0x4444,   "4TV Data Broadcast" },
	{ 0x4E4F, 0x4E4F,   "Nokia IP based software delivery" },
	{ 0xBBB1, 0xBBB1,   "BBG Data Caroussel" },
	{ 0xBBB2, 0xBBB2,   "BBG Object Caroussel" },
	{ 0xBBBB, 0xBBBB,   "Bertelsmann Broadband Group" },
	{ 0xFFFF, 0xFFFF,   "Reserved for future use" },
     {  0,0, NULL }
  };

  return findTableID (Table, flag);
}





/*
 -- Network Identification coding (ETR 162)
*/

char *dvbstrOriginalNetwork_ID (u_int i)
{
  STR_TABLE  Table[] = {
	// -- updated 2003-10-16
	// -- { Original Network ID, Original Network ID,   "Description | Operator" },
	{ 0x0000, 0x0000,   "Reserved | Reserved" },
	{ 0x0001, 0x0001,   "Astra Satellite Network 19,2°E | Société Européenne des Satellites" },
	{ 0x0002, 0x0002,   "Astra Satellite Network 28,2°E | Société Européenne des Satellites" },
	{ 0x0003, 0x0019,   "Astra | Société Européenne des Satellites" },
	{ 0x001A, 0x001A,   "Quiero Televisión | Quiero Televisión" },
	{ 0x001B, 0x001B,   "RAI | RAI" },
	{ 0x001F, 0x001F,   "Europe Online Networks (EON) | Europe Online Networks S.A" },
	{ 0x0020, 0x0020,   "ASTRA | Société Européenne des Satellites" },
	{ 0x0021, 0x0026,   "Hispasat Network | Hispasat S.A." },
	{ 0x0027, 0x0027,   "Hispasat 30°W | Hispasat FSS" },
	{ 0x0028, 0x0028,   "Hispasat 30°W | Hispasat DBS" },
	{ 0x0029, 0x0029,   "Hispasat 30°W | Hispasat America" },
	{ 0x002E, 0x002E,   "Xantic | Xantic BU Broadband" },
	{ 0x002F, 0x002F,   "TVNZ Digital | TVNZ" },
	{ 0x0030, 0x0030,   "Canal+ Satellite Network | Canal+ SA (for Intelsat 601-325°E)" },
	{ 0x0031, 0x0031,   "Hispasat - VIA DIGITAL | Hispasat S.A." },
	{ 0x0032, 0x0034,   "Hispasat Network | Hispasat S.A." },
	{ 0x0035, 0x0035,   "Nethold Main Mux System | NetHold IMS" },
	{ 0x0036, 0x0036,   "TV Cabo | TV Cabo Portugal" },
	{ 0x0037, 0x0037,   "STENTOR | France Telecom, CNES and DGA" },
	{ 0x0038, 0x0038,   "OTE | Hellenic Telecommunications Organization S.A." },
	{ 0x0040, 0x0040,   "Croatian Post and Telecommunications | HPT Croatian Post and Telecommunications" },
	{ 0x0041, 0x0041,   "Mindport network | Mindport" },
	{ 0x0046, 0x0047,   "1 degree W | Telenor" },
	{ 0x0048, 0x0048,   "STAR DIGITAL | STAR DIGITAL A.S." },
	{ 0x0049, 0x0049,   "Sentech Digital Satellite | Sentech" },
	{ 0x0050, 0x0050,   "Croatian Radio and Television | HRT Croatian Radio and Television" },
	{ 0x0051, 0x0051,   "Havas | Havas" },
	{ 0x0052, 0x0052,   "Osaka Yusen Satellite | StarGuide Digital Networks" },
	{ 0x0054, 0x0054,   "Teracom Satellite | Teracom AB Satellite Services" },
	{ 0x0055, 0x0055,   "Sirius Satellite System European Coverage | NSAB (Teracom)" },
	{ 0x0058, 0x0058,   "(Thiacom 1 & 2 co-located 78.5°E) | UBC Thailand" },
	{ 0x005E, 0x005E,   "Sirius Satellite System Nordic Coverage | NSAB" },
	{ 0x005F, 0x005F,   "Sirius Satellite System FSS | NSAB" },
	{ 0x0060, 0x0060,   "MSG MediaServices GmbH | MSG MediaServices GmbH" },
	{ 0x0069, 0x0069,   "(Optus B3 156°E) | (Optus Communications)" },
	{ 0x0070, 0x0070,   "BONUM1; 36 Degrees East | NTV+" },
	{ 0x0073, 0x0073,   "(PanAmSat 4 68.5°E) | (Pan American Satellite System)" },
	{ 0x007D, 0x007D,   "Skylogic | Skylogic Italia" },
	{ 0x007E, 0x007E,   "Eutelsat Satellite System at 7°E | European Telecommunications Satellite Organization" },
	{ 0x007F, 0x007F,   "Eutelsat Satellite System at 7°E | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x0085, 0x0085,   "BetaTechnik | BetaTechnik" },
	{ 0x0090, 0x0090,   "National network | TDF" },
	{ 0x00A0, 0x00A0,   "National Cable Network | News Datacom" },
	{ 0x00A1, 0x00A5,   "News Satellite Network | News Datacom" },
	{ 0x00A6, 0x00A6,   "ART | ART" },
	{ 0x00A7, 0x00A7,   "Globecast | France Telecom" },
	{ 0x00A8, 0x00A8,   "Foxtel | Foxtel" },
	{ 0x00A9, 0x00A9,   "Sky New Zealand | Sky New Zealand" },
	{ 0x00B0, 0x00B3,   "TPS | La Télévision Par Satellite" },
	{ 0x00B4, 0x00B4,   "Telesat 107.3°W | Telesat Canada" },
	{ 0x00B5, 0x00B5,   "Telesat 111.1°W | Telesat Canada" },
	{ 0x00B6, 0x00B6,   "Telstra Saturn | TelstraSaturn Limited" },
	{ 0x00BA, 0x00BA,   "Satellite Express 6 (80°E) | Satellite Express" },
	{ 0x00C0, 0x00CD,   "Canal + | Canal+" },
	{ 0x00EB, 0x00EB,   "Eurovision Network | European Broadcasting Union" },
	{ 0x0100, 0x0100,   "ExpressVu | ExpressVu Inc." },
	{ 0x010D, 0x010D,   "Skylogic | Skylogic Italia" },
	{ 0x010E, 0x010E,   "Eutelsat Satellite System at 10°E | European Telecommunications Satellite Organization" },
	{ 0x010F, 0x010F,   "Eutelsat Satellite System at 10°E | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x0110, 0x0110,   "Mediaset | Mediaset" },
	{ 0x011F, 0x011F,   "visAvision Network | European Telecommunications Satellite Organization" },
	{ 0x013D, 0x013D,   "Skylogic | Skylogic Italia" },
	{ 0x013E, 0x013E,   "Eutelsat Satellite System 13°E | European Telecommunications Satellite Organization" },
	{ 0x013F, 0x013F,   "Eutelsat Satellite System at 13°E | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x016D, 0x016D,   "Skylogic | Skylogic Italia" },
	{ 0x016E, 0x016E,   "Eutelsat Satellite System at 16°E | European Telecommunications Satellite Organization" },
	{ 0x016F, 0x016F,   "Eutelsat Satellite System at 16°E | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x01F4, 0x01F4,   "MediaKabel B.V" },
	{ 0x022D, 0x022D,   "Skylogic | Skylogic Italia" },
	{ 0x022E, 0x022F,   "Eutelsat Satellite System at 21.5°E | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x026D, 0x026D,   "Skylogic | Skylogic Italia" },
	{ 0x026E, 0x026F,   "Eutelsat Satellite System at 25.5°E | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x029D, 0x029D,   "Skylogic | Skylogic Italia" },
	{ 0x029E, 0x029E,   "Eutelsat Satellite System at 29°E | European Telecommunications Satellite Organization" },
	{ 0x029F, 0x029F,   "Eutelsat Satellite System at 28.5°E | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x02BE, 0x02BE,   "Arabsat | Arabsat (Scientific Atlanta, Eutelsat)" },
	{ 0x033D, 0x033D,   "Skylogic at 33°E | Skylogic Italia" },
	{ 0x033E, 0x033f,   "Eutelsat Satellite System at 33°E | Eutelsat" },
	{ 0x036D, 0x036D,   "Skylogic | Skylogic Italia" },
	{ 0x036E, 0x036E,   "Eutelsat Satellite System at 36°E | European Telecommunications Satellite Organization" },
	{ 0x036F, 0x036F,   "Eutelsat Satellite System at 36°E | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x03E8, 0x03E8,   "Telia | Telia, Sweden" },
	{ 0x047D, 0x047D,   "Skylogic | Skylogic Italia" },
	{ 0x047E, 0x047f,   "Eutelsat Satellite System at 12.5°W | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x048D, 0x048D,   "Skylogic | Skylogic Italia" },
	{ 0x048E, 0x048E,   "Eutelsat Satellite System at 48°E | European Telecommunications Satellite Organization" },
	{ 0x048F, 0x048F,   "Eutelsat Satellite System at 48°E | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x052D, 0x052D,   "Skylogic | Skylogic Italia" },
	{ 0x052E, 0x052f,   "Eutelsat Satellite System at 8°W | EUTELSAT - European Telecommunications Satellite Organization" },
	{ 0x055D, 0x055D,   "Skylogic at 5°W | Skylogic Italia" },
	{ 0x055E, 0x055f,   "Eutelsat Satellite System at 5°W | Eutelsat" },
	{ 0x0600, 0x0600,   "UPC Satellite | UPC" },
	{ 0x0601, 0x0601,   "UPC Cable | UPC" },
	{ 0x0602, 0x0602,   "Tevel | Tevel Cable (Israel)" },
	{ 0x071D, 0x071D,   "Skylogic at 70.5°E | Skylogic Italia" },
	{ 0x071E, 0x071f,   "Eutelsat Satellite System at 70.5°E | Eutelsat" },
	{ 0x0800, 0x0801,   "Nilesat 101 | Nilesat" },
	{ 0x0880, 0x0880,   "MEASAT 1, 91.5°E | MEASAT Broadcast Network Systems SDN. BHD. (Kuala Lumpur, Malaysia)" },
	{ 0x0882, 0x0882,   "MEASAT 2, 91.5°E | MEASAT Broadcast Network Systems SDN. BHD. (Kuala Lumpur, Malaysia)" },
	{ 0x0883, 0x0883,   "MEASAT 2, 148.0°E | Hsin Chi Broadcast Company Ltd." },
	{ 0x088F, 0x088F,   "MEASAT 3 | MEASAT Broadcast Network Systems SDN. BHD. (Kuala Lumpur, Malaysia)" },
	{ 0x1000, 0x1000,   "Optus B3 156°E | Optus Communications" },
	{ 0x1001, 0x1001,   "DISH Network | Echostar Communications" },
	{ 0x1002, 0x1002,   "Dish Network 61.5 W | Echostar Communications" },
	{ 0x1003, 0x1003,   "Dish Network 83 W | Echostar Communications" },
	{ 0x1004, 0x1004,   "Dish Network 119 W | Echostar Communications" },
	{ 0x1005, 0x1005,   "Dish Network 121 W | Echostar Communications" },
	{ 0x1006, 0x1006,   "Dish Network 148 W | Echostar Communications" },
	{ 0x1007, 0x1007,   "Dish Network 175 W | Echostar Communications" },
	{ 0x1008, 0x1008,   "Dish Network W | Echostar Communications" },
	{ 0x1009, 0x1009,   "Dish Network X | Echostar Communications" },
	{ 0x100A, 0x100A,   "Dish Network Y | Echostar Communications" },
	{ 0x100B, 0x100B,   "Dish Network Z | Echostar Communications" },
	{ 0x1010, 0x1010,   "ABC TV | Australian Broadcasting Corporation" },
	{ 0x1011, 0x1011,   "SBS | SBS Australia" },
	{ 0x1012, 0x1012,   "Nine Network Australia | Nine Network Australia" },
	{ 0x1013, 0x1013,   "Seven Network Australia | Seven Network Limited" },
	{ 0x1014, 0x1014,   "Network TEN Australia | Network TEN Limited" },
	{ 0x1015, 0x1015,   "WIN Television Australia | WIN Television Pty Ltd" },
	{ 0x1016, 0x1016,   "Prime Television Australia | Prime Television Limited" },
	{ 0x1017, 0x1017,   "Southern Cross Broadcasting Australia | Southern Cross Broadcasting (Australia) Limited" },
	{ 0x1018, 0x1018,   "Telecasters Australia | Telecasters Australia Limited" },
	{ 0x1019, 0x1019,   "NBN Australia | NBN Limited" },
	{ 0x101A, 0x101A,   "Imparja Television Australia | Imparja Television Australia" },
	{ 0x101B, 0x101f,   "Reserved for Australian broadcasters | Reserved for Australian broadcasters" },
	{ 0x1100, 0x1100,   "GE Americom | GE American Communications" },
	{ 0x2000, 0x2000,   "Thiacom 1 & 2 co-located 78.5°E | Shinawatra Satellite" },
	{ 0x2024, 0x2024,   "Australian Digital Terrestrial Television | Australian Broadcasting Authority" },
	{ 0x2038, 0x2038,   "Belgian Digital Terrestrial Television | BIPT" },
	{ 0x20CB, 0x20CB,   "Czech Republic Digital Terrestrial Television | Czech Digital Group" },
	{ 0x20D0, 0x20D0,   "Danish Digital Terrestrial Television | National Telecom Agency Denmark" },
	{ 0x20E9, 0x20E9,   "Estonian Digital Terrestrial Television | Estonian National Communications Board" },
	{ 0x20F6, 0x20F6,   "Finnish Digital Terrestrial Television | Telecommunicatoins Administratoin Centre, Finland" },
	{ 0x2114, 0x2114,   "German Digital Terrestrial Television | IRT on behalf of the German DVB-T broadcasts" },
	{ 0x2174, 0x2174,   "Irish Digital Terrestrial Television | Irish Telecommunications Regulator" },
	{ 0x2178, 0x2178,   "Israeli Digital Terrestrial Television | BEZEQ (The Israel Telecommunication Corp Ltd.)" },
	{ 0x2210, 0x2210,   "Netherlands Digital Terrestrial Television | Nozema" },
	{ 0x22BE, 0x22BE,   "Singapore Digital Terrestrial Television | Singapore Broadcasting Authority" },
	{ 0x22D4, 0x22D4,   "Spanish Digital Terrestrial Television | Spanish Broadcasting Regulator" },
	{ 0x22F1, 0x22F1,   "Swedish Digital Terrestrial Television | Swedish Broadcasting Regulator" },
	{ 0x22F4, 0x22F4,   "Swiss Digital Terrestrial Television | OFCOM" },
	{ 0x233A, 0x233A,   "UK Digital Terrestrial Television | Independent Television Commission" },
	{ 0x3000, 0x3000,   "PanAmSat 4 68.5°E | Pan American Satellite System" },
	{ 0x5000, 0x5000,   "Irdeto Mux System | Irdeto Test Laboratories" },
	{ 0x616D, 0x616D,   "BellSouth Entertainment | BellSouth Entertainment, Atlanta, GA, USA" },
	{ 0x6600, 0x6600,   "UPC Satellite | UPC" },
	{ 0x6601, 0x6601,   "UPC Cable | UPC" },
	{ 0xF000, 0xF000,   "Small Cable networks | Small cable network network operators" },
	{ 0xF001, 0xF001,   "Deutsche Telekom | Deutsche Telekom AG" },
	{ 0xF010, 0xF010,   "Telefónica Cable | Telefónica Cable SA" },
	{ 0xF020, 0xF020,   "Cable and Wireless Communication | Cable and Wireless Communications" },
	{ 0xF100, 0xF100,   "Casema | Casema N.V." },
	{ 0xF750, 0xF750,   "Telewest Communications Cable Network | Telewest Communications Plc" },
	{ 0xF751, 0xF751,   "OMNE Communications | OMNE Communications Ltd." },
	{ 0xFBFC, 0xFBFC,   "MATAV | MATAV (Israel)" },
	{ 0xFBFD, 0xFBFD,   "Telia Kabel-TV | Telia, Sweden" },
	{ 0xFBFE, 0xFBFE,   "TPS | La Télévision Par Satellite" },
	{ 0xFBFF, 0xFBFF,   "Sky Italia | Sky Italia Spa." },
	{ 0xFC10, 0xFC10,   "Rhône Vision Cable | Rhône Vision Cable" },
	{ 0xFC41, 0xFC41,   "France Telecom Cable | France Telecom" },
	{ 0xFD00, 0xFD00,   "National Cable Network | Lyonnaise Communications" },
	{ 0xFE00, 0xFE00,   "TeleDenmark Cable TV | TeleDenmark" },
	{ 0xFEC0, 0xFEff,   "Network Interface Modules | Common Interface" },
	{ 0xFF00, 0xFFfe,   "Private_temporary_use | ETSI" },
     	{  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrNetworkIdent_ID (u_int i)

{
	// $$$ TODO same as OriginalNetworkID??
  STR_TABLE  Table[] = {
     {  0x0000, 0x0000,  "reserved" },
     {  0x0001, 0x0001,  "Astra Satellite Network 19.2E / Satellite / SES" },
     {  0x0002, 0x0002,  "Astra Satellite Network 28.2E / Satellite / SES" },
     {  0x0002, 0x0020,  "Astra Satellite Network / Satellite / SES" },
     {  0x0021, 0x0027,  "Hispasat Network / Satellite / Hispasat FSS" },
     {  0x0027, 0x0028,  "Hispasat 30W / Satellite / Hispasat FSS" },
     {  0x0028, 0x0028,  "Hispasat 30W / Satellite / Hispasat DBS" },
     {  0x0029, 0x0029,  "Hispasat 30W / Satellite / Hispasat America" },
     {  0x002A, 0x002A,  "Multicabal / Satellite / Multicanal" },
     {  0x0035, 0x0035,  "TV Africa / Satellite / Telemedia (PTY) Ltd." },
     {  0x0085, 0x0085,  "- / Satellite / Beta Technik" },
     {  0x013E, 0x013F,  "Eutelsat Satellite System 13.0E / Satellite / ETSO" },
     {  0x016E, 0x016F,  "Eutelsat Satellite System 16.0E / Satellite / ETSO" },
     {  0x022E, 0x022F,  "Eutelsat Satellite System 21.5E / Satellite / ETSO" },
     {  0x047E, 0x047F,  "Eutelsat Satellite System 12.5W / Satellite / ETSO" },
     {  0x052E, 0x052F,  "Eutelsat Satellite System  8.5W / Satellite / ETSO" },

//$$$ lots are missing
     {  0x0530, 0xFFFF,  "--> please lookup at http://www.dvb.org" },

     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
 -- Teletext type descriptor (ETSI EN 300 468  6.2.38)
*/

char *dvbstrTeletext_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "initial teletext page" },
     {  0x02, 0x02,  "teletext subtitle page" },
     {  0x03, 0x03,  "additional information page" },
     {  0x04, 0x04,  "program schedule page" },
     {  0x05, 0x05,  "teletext subtitle page for hearing impaired people" },
     {  0x06, 0x1F,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
 -- Terrestrial Bandwidth descriptor (ETSI EN 300 468  6.2.12.3)
*/

char *dvbstrTerrBandwidth_SCHEME (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "8 MHz" },
     {  0x01, 0x01,  "7 MHz" },
     {  0x02, 0x02,  "6 MHz" },
     {  0x03, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrTerrConstellation_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "QPSK" },
     {  0x01, 0x01,  "16-QAM" },
     {  0x02, 0x02,  "64-QAM" },
     {  0x03, 0x03,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrTerrHierarchy_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "non-hierarchical" },
     {  0x01, 0x01,  "alpha=1" },
     {  0x02, 0x02,  "alpha=2" },
     {  0x03, 0x03,  "alpha=4" },
     {  0x04, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrTerrCodeRate_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "1/2" },
     {  0x01, 0x01,  "2/3" },
     {  0x02, 0x02,  "3/4" },
     {  0x03, 0x03,  "5/6" },
     {  0x04, 0x04,  "7/8" },
     {  0x05, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrTerrGuardInterval_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "1/32" },
     {  0x01, 0x01,  "1/16" },
     {  0x02, 0x02,  "1/8" },
     {  0x03, 0x03,  "1/4" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrTerrTransmissionMode_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "2k mode" },
     {  0x01, 0x01,  "8k mode" },
     {  0x02, 0x03,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


/*
 -- Aspect Ratio  (e.g. Target Background Grid)
 -- ISO 13818-2  Table 6.3
*/

char *dvbstrAspectRatioInfo_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "forbidden" },
     {  0x01, 0x01,  " -- " },
     {  0x02, 0x02,  "3:4" },
     {  0x03, 0x03,  "9:16" },
     {  0x04, 0x04,  "1:2.21" },
     {  0x05, 0x0F,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
 -- Hierarchy Type  
 -- ISO 13818-1  Table 2.6.7
*/

char *dvbstrHierarchy_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "ITU-T Rec.H.262 | ISO/IEC 13818-2 Spatial Scalability" },
     {  0x02, 0x02,  "ITU-T Rec.H.262 | ISO/IEC 13818-2 SNR Scalability" },
     {  0x03, 0x03,  "ITU-T Rec.H.262 | ISO/IEC 13818-2 Temporal Scalability" },
     {  0x04, 0x04,  "ITU-T Rec.H.262 | ISO/IEC 13818-2 Data partioning" },
     {  0x05, 0x05,  "ISO/IEC 13818-3 Extension bitstream" },
     {  0x06, 0x06,  "ITU-T Rec.H.222.0 | ISO/IEC 13818-1 Private Stream" },
     {  0x07, 0x0E,  "reserved" },
     {  0x0F, 0x0F,  "Base layer" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
 -- Running Status  (SDT)  
 -- ETSI EN 300 468   5.2.3
*/

char *dvbstrRunningStatus_FLAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "not running" },
     {  0x02, 0x02,  "starts in a few seconds (e.g. for VCR)" },
     {  0x03, 0x03,  "pausing" },
     {  0x04, 0x04,  "running" },
     {  0x05, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
 -- Data Stream Alignment Type
 -- ISO 13818-1  2.6.11
*/

char *dvbstrDataStreamVIDEOAlignment_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "Slice or video access unit" },
     {  0x02, 0x02,  "video access unit" },
     {  0x03, 0x03,  "GOP or SEQ" },
     {  0x04, 0x04,  "SEQ" },
     {  0x05, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}


char *dvbstrDataStreamAUDIOAlignment_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "syncword" },
     {  0x02, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
  -- VBI Data Service ID
  -- ETSI EN 300 468   6.2.43
*/

char *dvbstrDataService_ID (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "EBU teletext" },
     {  0x02, 0x02,  "inverted teletext" },
     {  0x03, 0x03,  "reserved" },
     {  0x04, 0x04,  "VPS" },
     {  0x05, 0x05,  "WSS" },
     {  0x06, 0x06,  "Closed Caption" },
     {  0x07, 0x07,  "monochrome 4:2:2 samples" },
     {  0x08, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}





/*
  -- Stream Content & Component Type
  -- ETSI EN 300 468   6.2.7
*/

char *dvbstrStreamContent_Component_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     // streamComponentID << 8    | ComponentID
     {  0x0000, 0x00FF,  "reserved" },
     {  0x0100, 0x0100,  "reserved" },

     {  0x0101, 0x0101,  "video, 4:3  aspect ratio, 25 Hz" },
     {  0x0102, 0x0102,  "video, 16:9 aspect ratio with pan vectors, 25 Hz" },
     {  0x0103, 0x0103,  "video, 16:9 aspect ratio without pan vectors, 25 Hz" },
     {  0x0104, 0x0104,  "video, > 16:9 aspect ratio, 25 Hz" },
     {  0x0105, 0x0105,  "video, 4:3  aspect ratio, 30 Hz" },
     {  0x0106, 0x0106,  "video, 16:9 aspect ratio with pan vectors, 30 Hz" },
     {  0x0107, 0x0107,  "video, 16:9 aspect ratio without pan vectors, 30 Hz" },
     {  0x0108, 0x0108,  "video, > 16:9 aspect ratio, 30 Hz" },

     {  0x0109, 0x0109,  "high definition video, 4:3  aspect ratio, 25 Hz" },
     {  0x010A, 0x010A,  "high definition video, 16:9 aspect ratio with pan vectors, 25 Hz" },
     {  0x010B, 0x010B,  "high definition video, 16:9 aspect ratio without pan vectors, 25 Hz" },
     {  0x010C, 0x010C,  "high definition video, > 16:9 aspect ratio, 25 Hz" },
     {  0x010D, 0x010D,  "high definition video, 4:3  aspect ratio, 30 Hz" },
     {  0x010E, 0x010E,  "high definition video, 16:9 aspect ratio with pan vectors, 30 Hz" },
     {  0x010F, 0x010F,  "high definition video, 16:9 aspect ratio without pan vectors, 30 Hz" },
     {  0x0110, 0x0110,  "high definition video, > 16:9 aspect ratio, 30 Hz" },

     {  0x0111, 0x01AF,  "reserved" },
     {  0x01B0, 0x01FE,  "User defined" },
     {  0x01FF, 0x01FF,  "reserved" },
     {  0x0200, 0x0200,  "reserved" },

     {  0x0201, 0x0201,  "audio, single mono channel" },
     {  0x0202, 0x0202,  "audio, dual mono channel" },
     {  0x0203, 0x0203,  "audio, stereo (2 channels)" },
     {  0x0204, 0x0204,  "audio, multilingual, multi-channel)" },
     {  0x0205, 0x0205,  "audio, surround sound" },
     {  0x0206, 0x023F,  "reserved" },
     {  0x0240, 0x0240,  "audio description for visually impaired" },
     {  0x0241, 0x0241,  "audio for the hard of hearing" },

     {  0x0242, 0x02AF,  "reserved" },
     {  0x02B0, 0x02FE,  "User defined" },
     {  0x02FF, 0x02FF,  "reserved" },
     {  0x0300, 0x0300,  "reserved" },

     {  0x0301, 0x0301,  "EBU Teletext subtitles" },
     {  0x0302, 0x0302,  "associated Teletext" },
     {  0x0303, 0x0303,  "VBI data" },
     {  0x0304, 0x030F,  "reserved" },

     {  0x0310, 0x0310,  "DVB subtitles (normal) with no monitor aspect ratio critical" },
     {  0x0311, 0x0311,  "DVB subtitles (normal) for display 4:3 aspect ratio monitor" },
     {  0x0312, 0x0312,  "DVB subtitles (normal) for display 16:9 aspect ratio monitor" },
     {  0x0313, 0x0313,  "DVB subtitles (normal) for display 2.21:1 aspect ratio monitor" },
     {  0x0314, 0x031F,  "reserved" },
     {  0x0320, 0x0320,  "DVB subtitles (for the hard hearing) with no monitor aspect ratio critical" },
     {  0x0321, 0x0321,  "DVB subtitles (for the hard hearing) for display 4:3 aspect ratio monitor" },
     {  0x0322, 0x0322,  "DVB subtitles (for the hard hearing) for display 16:9 aspect ratio monitor" },
     {  0x0323, 0x0323,  "DVB subtitles (for the hard hearing) for display 2.21:1 aspect ratio monitor" },

     {  0x0324, 0x03AF,  "reserved" },
     {  0x03B0, 0x03FE,  "User defined" },
     {  0x03FF, 0x03FF,  "reserved" },
     {  0x0400, 0x0400,  "reserved" },
     {  0x0401, 0x047F,  "AC3 modes  (ERROR: to be defined more specific $$$)" },

     {  0x0480, 0x04FF,  "reserved" },
     {  0x0500, 0x0BFF,  "reserved" },
     {  0x0C00, 0x0FFF,  "User defined" },

     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
  -- Logical Cell Presentation Info
  -- ETSI EN 300 468   6.2.18
*/

char *dvbstrLogCellPresInfo_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "video" },
     {  0x02, 0x02,  "still picture (INTRA coded)" },
     {  0x03, 0x03,  "graphics/text" },
     {  0x04, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
  -- Cell Linkage Info
  -- ETSI EN 300 468   6.2.18
*/

char *dvbstrCellLinkageInfo_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "bouquet related" },
     {  0x02, 0x02,  "service related" },
     {  0x03, 0x03,  "other mosaic related" },
     {  0x04, 0x04,  "event related" },
     {  0x05, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
  -- Text Charset Types
  -- ETSI EN 300 468   ANNEX A
*/

char *dvbstrTextCharset_TYPE(u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "Latin/Cyrilic alphabet" },
     {  0x02, 0x02,  "Latin/Arabic alphabet" },
     {  0x03, 0x03,  "Latin/Greek alphabet" },
     {  0x04, 0x04,  "Latin/Hebrew alphabet" },
     {  0x05, 0x05,  "Latin alphabet no. 5" },
     {  0x06, 0x0F,  "reserved" },
     {  0x10, 0x10,  "ISO/IEC 8859  special table " },
     {  0x11, 0x11,  "ISO/IEC 10646-1 2Byte pairs Basic Multilingual Plane" },
     {  0x12, 0x12,  "Korean Charset KSC 5601" },
     {  0x13, 0x1F,  "reserved" },
     {  0x20, 0xFF,  "Latin alphabet" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
  -- Content Nibble Types (Content descriptor)
  -- ETSI EN 300 468   6.2.8
*/

char *dvbstrContentNibble_TYPE(u_int i)

{
  STR_TABLE  Table[] = {
     // ContenNibble_1 << 8    |  ContentNibble_2
     //  4 bit                 |       4 bit
     {  0x0000, 0x000F,  "reserved" },

     // Movie/Drama
     {  0x0100, 0x0100,  "movie/drama (general)" },
     {  0x0101, 0x0101,  "detective/thriller" },
     {  0x0102, 0x0102,  "adventure/western/war" },
     {  0x0103, 0x0103,  "science fiction/fantasy/horror" },
     {  0x0104, 0x0104,  "comedy" },
     {  0x0105, 0x0105,  "soap/melodram/folkloric" },
     {  0x0106, 0x0106,  "romance" },
     {  0x0107, 0x0107,  "serious/classical/religious/historical movie/drama" },
     {  0x0108, 0x0108,  "adult movie/drama" },

     {  0x0109, 0x010E,  "reserved" },
     {  0x010F, 0x010A,  "user defined" },

     // News Current Affairs
     {  0x0200, 0x0200,  "news/current affairs (general)" },
     {  0x0201, 0x0201,  "news/weather report" },
     {  0x0202, 0x0202,  "news magazine" },
     {  0x0203, 0x0203,  "documentary" },
     {  0x0204, 0x0204,  "discussion/interview/debate" },
     {  0x0205, 0x020E,  "reserved" },
     {  0x020F, 0x020F,  "user defined" },

     // Show Games show
     {  0x0300, 0x0300,  "show/game show (general)" },
     {  0x0301, 0x0301,  "game show/quiz/contest" },
     {  0x0302, 0x0302,  "variety show" },
     {  0x0303, 0x0303,  "talk show" },
     {  0x0304, 0x030E,  "reserved" },
     {  0x030F, 0x030F,  "user defined" },

     // Sports
     {  0x0400, 0x0400,  "sports (general)" },
     {  0x0401, 0x0401,  "special events" },
     {  0x0402, 0x0402,  "sports magazine" },
     {  0x0403, 0x0403,  "football/soccer" },
     {  0x0404, 0x0404,  "tennis/squash" },
     {  0x0405, 0x0405,  "team sports" },
     {  0x0406, 0x0406,  "athletics" },
     {  0x0407, 0x0407,  "motor sport" },
     {  0x0408, 0x0408,  "water sport" },
     {  0x0409, 0x0409,  "winter sport" },
     {  0x040A, 0x040A,  "equestrian" },
     {  0x040B, 0x040B,  "martial sports" },
     {  0x040C, 0x040E,  "reserved" },
     {  0x040F, 0x040F,  "user defined" },

     // Children/Youth
     {  0x0500, 0x0500,  "childrens's/youth program (general)" },
     {  0x0501, 0x0501,  "pre-school children's program" },
     {  0x0502, 0x0502,  "entertainment (6-14 year old)" },
     {  0x0503, 0x0503,  "entertainment (10-16 year old)" },
     {  0x0504, 0x0504,  "information/education/school program" },
     {  0x0505, 0x0505,  "cartoon/puppets" },
     {  0x0506, 0x050E,  "reserved" },
     {  0x050F, 0x050F,  "user defined" },

     // Music/Ballet/Dance 
     {  0x0600, 0x0600,  "music/ballet/dance (general)" },
     {  0x0601, 0x0601,  "rock/pop" },
     {  0x0602, 0x0602,  "serious music/classic music" },
     {  0x0603, 0x0603,  "folk/traditional music" },
     {  0x0604, 0x0604,  "jazz" },
     {  0x0605, 0x0605,  "musical/opera" },
     {  0x0606, 0x0606,  "ballet" },
     {  0x0607, 0x060E,  "reserved" },
     {  0x060F, 0x060F,  "user defined" },

     // Arts/Culture
     {  0x0700, 0x0700,  "arts/culture (without music, general)" },
     {  0x0701, 0x0701,  "performing arts" },
     {  0x0702, 0x0702,  "fine arts" },
     {  0x0703, 0x0703,  "religion" },
     {  0x0704, 0x0704,  "popular culture/traditional arts" },
     {  0x0705, 0x0705,  "literature" },
     {  0x0706, 0x0706,  "film/cinema" },
     {  0x0707, 0x0707,  "experimental film/video" },
     {  0x0708, 0x0708,  "broadcasting/press" },
     {  0x0709, 0x0709,  "new media" },
     {  0x070A, 0x070A,  "arts/culture magazine" },
     {  0x070B, 0x070B,  "fashion" },
     {  0x070C, 0x070E,  "reserved" },
     {  0x070F, 0x070F,  "user defined" },

     // Social/Political/Economics
     {  0x0800, 0x0800,  "social/political issues/economics (general)" },
     {  0x0801, 0x0801,  "magazines/reports/documentary" },
     {  0x0802, 0x0802,  "economics/social advisory" },
     {  0x0803, 0x0803,  "remarkable people" },
     {  0x0804, 0x080E,  "reserved" },
     {  0x080F, 0x080F,  "user defined" },

     // Education/Science/...
     {  0x0900, 0x0900,  "education/science/factual topics (general)" },
     {  0x0901, 0x0901,  "nature/animals/environment" },
     {  0x0902, 0x0902,  "technology/natural science" },
     {  0x0903, 0x0903,  "medicine/physiology/psychology" },
     {  0x0904, 0x0904,  "foreign countries/expeditions" },
     {  0x0905, 0x0905,  "social/spiritual science" },
     {  0x0906, 0x0906,  "further education" },
     {  0x0907, 0x0907,  "languages" },
     {  0x0908, 0x090E,  "reserved" },
     {  0x090F, 0x090F,  "user defined" },

     // Leisure hobies
     {  0x0A00, 0x0A00,  "leisure hobbies (general)" },
     {  0x0A01, 0x0A01,  "tourism/travel" },
     {  0x0A02, 0x0A02,  "handicraft" },
     {  0x0A03, 0x0A03,  "motoring" },
     {  0x0A04, 0x0A04,  "fitness & health" },
     {  0x0A05, 0x0A05,  "cooking" },
     {  0x0A06, 0x0A06,  "advertisement/shopping" },
     {  0x0A07, 0x0A07,  "gardening" },
     {  0x0A08, 0x0A0E,  "reserved" },
     {  0x0A0F, 0x0A0F,  "user defined" },

     {  0x0B00, 0x0B00,  "original language" },
     {  0x0B01, 0x0B01,  "black & white" },
     {  0x0B02, 0x0B02,  "unpublished" },
     {  0x0B03, 0x0B03,  "live broadcast" },
     {  0x0B04, 0x0B0E,  "reserved" },
     {  0x0B0F, 0x0B0F,  "user defined" },

     {  0x0C00, 0x0E0F,  "reserved" },
     {  0x0F00, 0x0F0F,  "user defined" },

     {  0,0, NULL }
  };

  return findTableID (Table, i);
}



/*
  -- Parental Rating Info
  -- ETSI EN 300 468   6.2.25
*/

char *dvbstrParentalRating_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "minimum age: 4 years" },
     {  0x02, 0x02,  "minimum age: 5 years" },
     {  0x03, 0x03,  "minimum age: 6 years" },
     {  0x04, 0x04,  "minimum age: 7 years" },
     {  0x05, 0x05,  "minimum age: 8 years" },
     {  0x06, 0x06,  "minimum age: 9 years" },
     {  0x07, 0x07,  "minimum age: 10 years" },
     {  0x08, 0x08,  "minimum age: 11 years" },
     {  0x09, 0x09,  "minimum age: 12 years" },
     {  0x0A, 0x0A,  "minimum age: 13 years" },
     {  0x0B, 0x0B,  "minimum age: 14 years" },
     {  0x0C, 0x0C,  "minimum age: 15 years" },
     {  0x0D, 0x0D,  "minimum age: 16 years" },
     {  0x0E, 0x0E,  "minimum age: 17 years" },
     {  0x0F, 0x0F,  "minimum age: 18 years" },
     {  0x10, 0xFF,  "defined by broadcaster" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
  -- Delivery System Coding Type
  -- ETSI EN 300 468   6.2.15
*/

char *dvbstrDelivSysCoding_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "undefined" },
     {  0x01, 0x01,  "satellite" },
     {  0x02, 0x02,  "cable" },
     {  0x03, 0x03,  "terrestrial" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}






/*
  -- Short Smoothing Buffer Size Type
  -- ETSI EN 300 468   6.2.29
*/

char *dvbstrShortSmoothingBufSize_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "1536 Bytes" },
     {  0x02, 0x02,  "reserved" },
     {  0x03, 0x03,  "reserved" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}



char *dvbstrShortSmoothingBufLeakRate_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "0.0009 Mbit/s" },
     {  0x02, 0x02,  "0.0018 Mbit/s" },
     {  0x03, 0x03,  "0.0036 Mbit/s" },
     {  0x04, 0x04,  "0.0072 Mbit/s" },
     {  0x05, 0x05,  "0.0108 Mbit/s" },
     {  0x06, 0x06,  "0.0144 Mbit/s" },
     {  0x07, 0x07,  "0.0216 Mbit/s" },
     {  0x08, 0x08,  "0.0288 Mbit/s" },
     {  0x09, 0x09,  "0.075 Mbit/s" },
     {  0x0A, 0x0A,  "0.5 Mbit/s" },
     {  0x0B, 0x0B,  "0.5625 Mbit/s" },
     {  0x0C, 0x0C,  "0.8437 Mbit/s" },
     {  0x0D, 0x0D,  "1.0 Mbit/s" },
     {  0x0E, 0x0E,  "1.1250 Mbit/s" },
     {  0x0F, 0x0F,  "1.5 Mbit/s" },
     {  0x10, 0x10,  "1.6875 Mbit/s" },
     {  0x11, 0x11,  "2.0 Mbit/s" },
     {  0x12, 0x12,  "2.25 Mbit/s" },
     {  0x13, 0x13,  "2.5 Mbit/s" },
     {  0x14, 0x14,  "3.0 Mbit/s" },
     {  0x15, 0x15,  "3.3750 Mbit/s" },
     {  0x16, 0x16,  "3.5 Mbit/s" },
     {  0x17, 0x17,  "4.0 Mbit/s" },
     {  0x18, 0x18,  "4.5 Mbit/s" },
     {  0x19, 0x19,  "5.0 Mbit/s" },
     {  0x1A, 0x1A,  "5.5 Mbit/s" },
     {  0x1B, 0x1B,  "6.0 Mbit/s" },
     {  0x1C, 0x1C,  "6.5 Mbit/s" },
     {  0x1D, 0x1D,  "6.75 Mbit/s" },
     {  0x1E, 0x1E,  "7.0 Mbit/s" },
     {  0x1F, 0x1F,  "7.5 Mbit/s" },
     {  0x20, 0x20,  "8.0 Mbit/s" },
     {  0x21, 0x21,  "9 Mbit/s" },
     {  0x22, 0x22,  "10 Mbit/s" },
     {  0x23, 0x23,  "11 Mbit/s" },
     {  0x24, 0x24,  "12 Mbit/s" },
     {  0x25, 0x25,  "13 Mbit/s" },
     {  0x26, 0x26,  "13.5 Mbit/s" },
     {  0x27, 0x27,  "14.0 Mbit/s" },
     {  0x28, 0x28,  "15 Mbit/s" },
     {  0x29, 0x29,  "16 Mbit/s" },
     {  0x2A, 0x2A,  "17 Mbit/s" },
     {  0x2B, 0x2B,  "18 Mbit/s" },
     {  0x2C, 0x2C,  "20 Mbit/s" },
     {  0x2D, 0x2D,  "22 Mbit/s" },
     {  0x2E, 0x2E,  "24 Mbit/s" },
     {  0x2F, 0x2F,  "26 Mbit/s" },
     {  0x30, 0x30,  "27 Mbit/s" },
     {  0x31, 0x31,  "28 Mbit/s" },
     {  0x32, 0x32,  "30 Mbit/s" },
     {  0x33, 0x33,  "32 Mbit/s" },
     {  0x34, 0x34,  "34 Mbit/s" },
     {  0x35, 0x35,  "36 Mbit/s" },
     {  0x36, 0x36,  "38 Mbit/s" },
     {  0x37, 0x37,  "40 Mbit/s" },
     {  0x38, 0x38,  "44 Mbit/s" },
     {  0x39, 0x39,  "48 Mbit/s" },
     {  0x3A, 0x3A,  "54 Mbit/s" },
     {  0x3B, 0x3B,  "72 Mbit/s" },
     {  0x3C, 0x3C,  "108 Mbit/s" },
     {  0x3D, 0x3F,  "reserved" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}





/*
  -- AC3 Component Type
  -- ETSI EN 300 468   ANNEX D
*/

char *dvbstrAC3Component_TYPE (u_int i)

{
  char *s = "ERROR:  TODO $$$  - AC3 Component type";


  return s;
}



/*
  -- Ancillary Data ID
  -- ETSI EN 300 468   6.2.1
*/

char *dvbstrAncillaryData_ID (u_int i)

{

 // $$$ coded in descriptor


  return NULL;
}



/*
  -- Announcement Type
  -- ETSI EN 300 468   6.2.2
*/

char *dvbstrAnnouncement_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "Emergency alarm" },
     {  0x01, 0x01,  "Road Traffic Flash" },
     {  0x02, 0x02,  "Public Transport Flash" },
     {  0x03, 0x03,  "Warning message" },
     {  0x04, 0x04,  "News flash" },
     {  0x05, 0x05,  "Weather flash" },
     {  0x06, 0x06,  "Event announcement" },
     {  0x07, 0x07,  "Personal call" },
     {  0x08, 0x0F,  "reserved" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}



char *dvbstrAnnouncementReference_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "Announcement is broadcast in the usual audio stream of the service" },
     {  0x01, 0x01,  "Announcement is broadcast in the separate audio stream that is part of the service" },
     {  0x02, 0x02,  "Announcement is broadcast by means of a different service within the same transport stream" },
     {  0x03, 0x03,  "Announcement is broadcast by means of a different service within a different transport stream" },
     {  0x04, 0x0F,  "reserved" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}





/*
 ------------------------------------------------------------------------
   Transport Stream  Stuff
 ------------------------------------------------------------------------
*/


/*
  -- Transport Stream PID  Table  ISO 13818-1  2.4.3.2
*/

char *dvbstrTSpid_ID (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x0000, 0x0000,  "Programm Association Table" },
     {  0x0001, 0x0001,  "Conditional Access Table" },
     {  0x0002, 0x000F,  "reserved" },
     {  0x0010, 0x1FFE,  "NIT, PMT or Elementary PID, etc." },
     {  0x1FFF, 0x1FFF,  "Null-packet" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}



/*
  -- Scrambling Control Table  ISO 13818-1
  -- Scrambling Control Table  ETSI ETR 289  5.1,
                               ETSI ETR 154  4.1.4.2.3
*/

char *dvbstrTS_ScramblingCtrl_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "No scrambling of TS packet payload" },
     {  0x01, 0x01,  "Reserved for future DVB use" },
     {  0x02, 0x02,  "TS packet scrambled with Even Key" },
     {  0x03, 0x03,  "TS packet scrambled with Odd Key" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}


/*
  -- Adaption Field Type  ISO 13818-1  2.4.3.2
*/

char *dvbstrTS_AdaptionField_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "no adaption_field, payload only" },
     {  0x02, 0x02,  "adaption_field only, no payload" },
     {  0x03, 0x03,  "adaption_field followed by payload" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}





/*
 ------------------------------------------------------------------------
  PES   Stuff
 ------------------------------------------------------------------------
*/


/*
  -- PES Stream_id  ISO 13818-1  2.4.3.6
*/

char *dvbstrPESstream_ID (u_int i)

{
  STR_TABLE  Table[] = {
     // on changes:  adapt dmx_pes.c!!!
     {  0x00, 0xBB,  "!!!unknown or PES stream not in sync... (!!!)" },
     {  0xBC, 0xBC,  "program_stream_map" },
     {  0xBD, 0xBD,  "private_stream_1" },
     {  0xBE, 0xBE,  "padding_stream" },
     {  0xBF, 0xBF,  "private_stream_2" },
     {  0xC0, 0xDF,  "ISO/IEC 13818-3 or ISO/IEC 11172-3 audio stream" },
     {  0xE0, 0xEF,  "ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream" },
     {  0xF0, 0xF0,  "ECM_stream" },
     {  0xF1, 0xF1,  "EMM_stream" },
     {  0xF2, 0xF2,  "ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex-A or ISO/IEC 13818-6_DSMCC stream" },
     {  0xF3, 0xF3,  "ISO/IEC 13522 stream" },
     {  0xF4, 0xF4,  "ITU-T Rec. H.222.1 type A" },
     {  0xF5, 0xF5,  "ITU-T Rec. H.222.1 type B" },
     {  0xF6, 0xF6,  "ITU-T Rec. H.222.1 type C" },
     {  0xF7, 0xF7,  "ITU-T Rec. H.222.1 type D" },
     {  0xF8, 0xF8,  "ITU-T Rec. H.222.1 type E" },
     {  0xF9, 0xF9,  "ancillary_stream" },
     {  0xFA, 0xFE,  "reserved data stream" },
     {  0xFF, 0xFF,  "program_stream_directory" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}



/*
  -- PES Scrambling CTRL   ISO 13818-1  2.4.3.7
  --  --> ETR 289  5.1
 */

char *dvbstrPESscrambling_ctrl_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not scrambled" },
     {  0x01, 0x01,  "Reserved for future DVB use" },
     {  0x02, 0x02,  "PES packet scrambled with Even Key" },
     {  0x03, 0x03,  "PES packet scrambled with Odd Key" },
     {  0,0, NULL }
  };


  return findTableID (Table, i);
}



/*
  -- Private Data Specifier Table  (from dvb.org)
  -- ETR 162
*/

char *dvbstrPrivateDataSpecifier_ID (u_int i)

{
  STR_TABLE  Table[] = {
	// { Private Data Value, Private Data Value // Organisation" },
  	// -- updated 2003-10-16
	{ 0x00000000, 0x00000000,   "Reserved" },
	{ 0x00000001, 0x00000001,   "SES" },
	{ 0x00000002, 0x00000002,   "BskyB 1" },
	{ 0x00000003, 0x00000003,   "BskyB 2" },
	{ 0x00000004, 0x00000004,   "BskyB 3" },
	{ 0x00000005, 0x00000005,   "ARD, ZDF, ORF" },
	{ 0x00000006, 0x00000006,   "Nokia Multimedia Network Terminals" },
	{ 0x00000007, 0x00000007,   "AT Entertainment Ltd." },
	{ 0x00000008, 0x00000008,   "TV Cabo Portugal  " },
	{ 0x00000009, 0x0000000D,   "Nagravision SA // Kudelski " },
	{ 0x0000000E, 0x0000000E,   "Valvision SA  " },
	{ 0x0000000F, 0x0000000F,   "Quiero Televisión  " },
	{ 0x00000010, 0x00000010,   "La Télévision Par Satellite (TPS)" },
	{ 0x00000011, 0x00000011,   "Echostar Communications" },
	{ 0x00000012, 0x00000012,   "Telia AB" },
	{ 0x00000013, 0x00000013,   "Viasat  " },
	{ 0x00000014, 0x00000014,   "Senda (Swedish Terrestrial TV )" },
	{ 0x00000015, 0x00000015,   "MediaKabel " },
	{ 0x00000016, 0x00000016,   "Casema  " },
	{ 0x00000017, 0x00000017,   "Humax Electronics Co. Ltd ." },
	{ 0x00000018, 0x00000018,   "@Sky  " },
	{ 0x00000019, 0x00000019,   "Singapore Digital Terrestrial Television  " },
	{ 0x0000001A, 0x0000001A,   "Télédiffusion de France (TDF)" },
	{ 0x0000001B, 0x0000001B,   "Intellibyte Inc." },
	{ 0x0000001C, 0x0000001C,   "Digital Theater Systems Ltd" },
	{ 0x0000001D, 0x0000001D,   "Finlux Ltd." },
	{ 0x0000001E, 0x0000001E,   "Sagem SA" },
	{ 0x00000020, 0x00000023,   "Lyonnaise Cable" },
	{ 0x00000025, 0x00000025,   "MTV Europe " },
	{ 0x00000026, 0x00000026,   "Pansonic  " },
	{ 0x00000027, 0x00000027,   "Mentor Data System, Inc ." },
	{ 0x00000028, 0x00000028,   "EACEM  " },
	{ 0x00000029, 0x00000029,   "NorDig  " },
	{ 0x0000002A, 0x0000002A,   "Intelsis Sistemas Inteligentes S.A ." },
	{ 0x0000002D, 0x0000002D,   "Alpha Digital Synthesis S.A." },
	{ 0x0000002F, 0x0000002F,   "Conax A.S." },
	{ 0x00000030, 0x00000030,   "Telenor" },
	{ 0x00000031, 0x00000031,   "TeleDenmark " },
	{ 0x00000035, 0x00000035,   "Europe Online Networks S.A ." },
	{ 0x00000038, 0x00000038,   "OTE  " },
	{ 0x00000039, 0x00000039,   "Telewizja Polsat  " },
	{ 0x000000A0, 0x000000A0,   "Sentech  " },
	{ 0x000000A1, 0x000000A1,   "TechniSat Digital GmbH  " },
	{ 0x000000BE, 0x000000BE,   "BetaTechnik" },
	{ 0x000000C0, 0x000000C0,   "Canal+" },
	{ 0x000000D0, 0x000000D0,   "Dolby Laboratories Inc." },
	{ 0x000000E0, 0x000000E0,   "ExpressVu Inc." },
	{ 0x000000F0, 0x000000F0,   "France Telecom, CNES and DGA (STENTOR)" },
	{ 0x00000100, 0x00000100,   "OpenTV" },
	{ 0x00000150, 0x00000150,   "Loewe Opta GmbH " },
	{ 0x00000600, 0x00000601,   "UPC 1  " },
	{ 0x00000ACE, 0x00000ACE,   "Ortikon Interactive Oy" },
	{ 0x00001000, 0x00001000,   "La Télévision Par Satellite (TPS )" },
	{ 0x000022D4, 0x000022D4,   "Spanish Broadcasting Regulator " },
	{ 0x000022F1, 0x000022F1,   "Swedish Broadcasting Regulator " },
	{ 0x0000233A, 0x0000233A,   "Independent Television Commission " },
	{ 0x00003200, 0x0000320f,   "Australian Terrestrial Television Networks" },
	{ 0x00006000, 0x00006000,   "News Datacom" },
	{ 0x00006001, 0x00006006,   "NDC " },
	{ 0x00362275, 0x00362275,   "Irdeto" },
	{ 0x004E544C, 0x004E544C,   "NTL" },
	{ 0x00532D41, 0x00532D41,   "Scientific Atlanta" },
	{ 0x00600000, 0x00600000,   "Rhône Vision Cable" },
	{ 0x44414E59, 0x44414E59,   "News Datacom (IL) 1" },
	{ 0x46524549, 0x46524549,   "News Datacom (IL) 1" },
	{ 0x46545600, 0x46545620,   "FreeTV " },
	{ 0x4A4F4A4F, 0x4A4F4A4F,   "MSG MediaServices GmbH  " },
	{ 0x4F545600, 0x4F5456ff,   "OpenTV " },
	{ 0x50484900, 0x504849ff,   "Philips DVS" },
	{ 0x53415053, 0x53415053,   "Scientific Atlanta" },
	{ 0x5347444E, 0x5347444E,   "StarGuide Digital Networks " },
	{ 0x56444700, 0x56444700,   "Vía Digital" },
	{ 0xBBBBBBBB, 0xBBBBBBBB,   "Bertelsmann Broadband Group  " },
	{ 0xECCA0001, 0xECCA0001,   "ECCA (European Cable Communications Association )" },
	{ 0xFCFCFCFC, 0xFCFCFCFC,   "France Telecom" },
     	{  0,0, NULL }
  };


  return findTableID (Table, i);
}




/*
  -- Country Code Table  (from dvb.org)
*/

char *dvbstrCountryCode_ID (u_int i)

{
  STR_TABLE  Table[] = {
	{ 0x0004, 0x0004,  "Afghanistan" },
	{ 0x0008, 0x0008,  "Albania" },
	{ 0x000a, 0x000a,  "Antarctica (the territory South of 60 deg S)" },
	{ 0x000c, 0x000c,  "Algeria" },
	{ 0x0010, 0x0010,  "American Samoa" },
	{ 0x0014, 0x0014,  "Andorra" },
	{ 0x0018, 0x0018,  "Angola" },
	{ 0x001c, 0x001c,  "Antigua and Barbuda" },
	{ 0x001f, 0x001f,  "Azerbaijan" },
	{ 0x0020, 0x0020,  "Argentina" },
	{ 0x0024, 0x0024,  "Australia" },
	{ 0x0028, 0x0028,  "Austria" },
	{ 0x002c, 0x002c,  "Bahamas" },
	{ 0x0030, 0x0030,  "Bahrain" },
	{ 0x0032, 0x0032,  "Bangladesh" },
	{ 0x0033, 0x0033,  "Armenia" },
	{ 0x0034, 0x0034,  "Barbados" },
	{ 0x0038, 0x0038,  "Belgium" },
	{ 0x003c, 0x003c,  "Bermuda" },
	{ 0x0040, 0x0040,  "Bhutan" },
	{ 0x0044, 0x0044,  "Bolivia" },
	{ 0x0046, 0x0046,  "Bosnia and Herzegowina [sic]" },
	{ 0x0048, 0x0048,  "Botswana" },
	{ 0x004a, 0x004a,  "Bouvet Island (Bouvetoya)" },
	{ 0x004c, 0x004c,  "Brazil" },
	{ 0x0054, 0x0054,  "Belize" },
	{ 0x0056, 0x0056,  "British Indian Ocean Territory (Chagos Archipelago)" },
	{ 0x005a, 0x005a,  "solomon Islands (was British Solomon Islands)" },
	{ 0x005c, 0x005c,  "British Virgin Islands" },
	{ 0x0060, 0x0060,  "Brunei Darussalam" },
	{ 0x0064, 0x0064,  "Bulgaria" },
	{ 0x0068, 0x0068,  "Myanmar (was Burma)" },
	{ 0x006c, 0x006c,  "Burundi" },
	{ 0x0070, 0x0070,  "Belarus" },
	{ 0x0074, 0x0074,  "Cambodia" },
	{ 0x0078, 0x0078,  "Cameroon" },
	{ 0x007c, 0x007c,  "Canada" },
	{ 0x0084, 0x0084,  "Cape Verde" },
	{ 0x0088, 0x0088,  "Cayman Islands" },
	{ 0x008c, 0x008c,  "Central African Republic" },
	{ 0x0090, 0x0090,  "sri Lanka" },
	{ 0x0094, 0x0094,  "Chad" },
	{ 0x0098, 0x0098,  "Chile" },
	{ 0x009c, 0x009c,  "China" },
	{ 0x009e, 0x009e,  "Taiwan" },
	{ 0x00a2, 0x00a2,  "Christmas Island" },
	{ 0x00a6, 0x00a6,  "Cocos (Keeling) Islands" },
	{ 0x00aa, 0x00aa,  "Colombia" },
	{ 0x00ae, 0x00ae,  "Comoros" },
	{ 0x00af, 0x00af,  "Mayotte" },
	{ 0x00b2, 0x00b2,  "Congo" },
	{ 0x00b4, 0x00b4,  "Congo (was Zaire)" },
	{ 0x00b8, 0x00b8,  "Cook Islands" },
	{ 0x00bc, 0x00bc,  "Costa Rica" },
	{ 0x00bf, 0x00bf,  "Hrvatska (Croatia)" },
	{ 0x00c0, 0x00c0,  "Cuba" },
	{ 0x00c4, 0x00c4,  "Cyprus" },
	{ 0x00cb, 0x00cb,  "Czech Republic" },
	{ 0x00cc, 0x00cc,  "Benin (was Dahomey)" },
	{ 0x00d0, 0x00d0,  "Denmark" },
	{ 0x00d4, 0x00d4,  "Dominica" },
	{ 0x00d6, 0x00d6,  "Dominican Republic" },
	{ 0x00da, 0x00da,  "Ecuador" },
	{ 0x00de, 0x00de,  "El Salvador" },
	{ 0x00e2, 0x00e2,  "Equatorial Guinea" },
	{ 0x00e7, 0x00e7,  "Ethiopia" },
	{ 0x00e8, 0x00e8,  "Eritrea" },
	{ 0x00e9, 0x00e9,  "Estonia" },
	{ 0x00ea, 0x00ea,  "Faeroe Islands" },
	{ 0x00ee, 0x00ee,  "Falkland Islands (Malvinas)" },
	{ 0x00ef, 0x00ef,  "south Georgia and the South Sandwich Islands" },
	{ 0x00f2, 0x00f2,  "Fiji" },
	{ 0x00f6, 0x00f6,  "Finland" },
	{ 0x00fa, 0x00fa,  "France" },
	{ 0x00fe, 0x00fe,  "French Guiana" },
	{ 0x0102, 0x0102,  "French Polynesia" },
	{ 0x0104, 0x0104,  "French Southern Territories" },
	{ 0x0106, 0x0106,  "Djibouti" },
	{ 0x010a, 0x010a,  "Gabon" },
	{ 0x010c, 0x010c,  "Georgia" },
	{ 0x010e, 0x010e,  "Gambia" },
	{ 0x0113, 0x0113,  "Palestinian Territory" },
	{ 0x0114, 0x0114,  "Germany" },
	{ 0x0120, 0x0120,  "Ghana" },
	{ 0x0124, 0x0124,  "Gibraltar" },
	{ 0x0128, 0x0128,  "Kiribati (was Gilbert Islands)" },
	{ 0x012c, 0x012c,  "Greece" },
	{ 0x0130, 0x0130,  "Greenland" },
	{ 0x0134, 0x0134,  "Grenada" },
	{ 0x0138, 0x0138,  "Guadaloupe" },
	{ 0x013c, 0x013c,  "Guam" },
	{ 0x0140, 0x0140,  "Guatemala" },
	{ 0x0144, 0x0144,  "Guinea" },
	{ 0x0148, 0x0148,  "Guyana" },
	{ 0x014c, 0x014c,  "Haiti" },
	{ 0x014e, 0x014e,  "Heard and McDonald Islands" },
	{ 0x0150, 0x0150,  "Holy See (Vatican City State)" },
	{ 0x0154, 0x0154,  "Honduras" },
	{ 0x0158, 0x0158,  "Hong Kong" },
	{ 0x015c, 0x015c,  "Hungary" },
	{ 0x0160, 0x0160,  "Iceland" },
	{ 0x0164, 0x0164,  "India" },
	{ 0x0168, 0x0168,  "Indonesia" },
	{ 0x016c, 0x016c,  "Iran" },
	{ 0x0170, 0x0170,  "Iraq" },
	{ 0x0174, 0x0174,  "Ireland" },
	{ 0x0178, 0x0178,  "Israel" },
	{ 0x017c, 0x017c,  "Italy" },
	{ 0x0180, 0x0180,  "Ivory Coast" },
	{ 0x0184, 0x0184,  "Jamaica" },
	{ 0x0188, 0x0188,  "Japan" },
	{ 0x018e, 0x018e,  "Kazakhstan" },
	{ 0x0190, 0x0190,  "Jordan" },
	{ 0x0194, 0x0194,  "Kenya" },
	{ 0x0198, 0x0198,  "Korea" },
	{ 0x019a, 0x019a,  "Korea" },
	{ 0x019e, 0x019e,  "Kuwait" },
	{ 0x01a1, 0x01a1,  "Kyrgyz Republic" },
	{ 0x01a2, 0x01a2,  "Lao People´s Democratic Republic" },
	{ 0x01a6, 0x01a6,  "Lebanon" },
	{ 0x01aa, 0x01aa,  "Lesotho" },
	{ 0x01ac, 0x01ac,  "Latvia" },
	{ 0x01ae, 0x01ae,  "Liberia" },
	{ 0x01b2, 0x01b2,  "Libyan Arab Jamahiriya" },
	{ 0x01b6, 0x01b6,  "Liechtenstein" },
	{ 0x01b8, 0x01b8,  "Lithuania" },
	{ 0x01ba, 0x01ba,  "Luxembourg" },
	{ 0x01be, 0x01be,  "Macau" },
	{ 0x01c2, 0x01c2,  "Madagascar" },
	{ 0x01c6, 0x01c6,  "Malawi" },
	{ 0x01ca, 0x01ca,  "Malaysia" },
	{ 0x01ce, 0x01ce,  "Maldives" },
	{ 0x01d2, 0x01d2,  "Mali" },
	{ 0x01d6, 0x01d6,  "Malta" },
	{ 0x01da, 0x01da,  "Martinique" },
	{ 0x01de, 0x01de,  "Mauritania" },
	{ 0x01e0, 0x01e0,  "Mauritius" },
	{ 0x01e4, 0x01e4,  "Mexico" },
	{ 0x01ec, 0x01ec,  "Monaco" },
	{ 0x01f0, 0x01f0,  "Mongolia" },
	{ 0x01f2, 0x01f2,  "Moldova" },
	{ 0x01f4, 0x01f4,  "Montserrat" },
	{ 0x01f8, 0x01f8,  "Morocco" },
	{ 0x01fc, 0x01fc,  "Mozambique" },
	{ 0x0200, 0x0200,  "Oman (was Muscat and Oman)" },
	{ 0x0204, 0x0204,  "Namibia" },
	{ 0x0208, 0x0208,  "Nauru" },
	{ 0x020c, 0x020c,  "Nepal" },
	{ 0x0210, 0x0210,  "Netherlands" },
	{ 0x0212, 0x0212,  "Netherlands Antilles" },
	{ 0x0215, 0x0215,  "Aruba" },
	{ 0x021c, 0x021c,  "New Caledonia" },
	{ 0x0224, 0x0224,  "Vanuatu (was New Hebrides)" },
	{ 0x022a, 0x022a,  "New Zealand" },
	{ 0x022e, 0x022e,  "Nicaragua" },
	{ 0x0232, 0x0232,  "Niger" },
	{ 0x0236, 0x0236,  "Nigeria" },
	{ 0x023a, 0x023a,  "Niue" },
	{ 0x023e, 0x023e,  "Norfolk Island" },
	{ 0x0242, 0x0242,  "Norway" },
	{ 0x0244, 0x0244,  "Northern Mariana Islands" },
	{ 0x0245, 0x0245,  "United States Minor Outlying Islands" },
	{ 0x0247, 0x0247,  "Micronesia" },
	{ 0x0248, 0x0248,  "Marshall Islands" },
	{ 0x0249, 0x0249,  "Palau" },
	{ 0x024a, 0x024a,  "Pakistan" },
	{ 0x024f, 0x024f,  "Panama" },
	{ 0x0256, 0x0256,  "Papua New Guinea" },
	{ 0x0258, 0x0258,  "Paraguay" },
	{ 0x025c, 0x025c,  "Peru" },
	{ 0x0260, 0x0260,  "Philippines" },
	{ 0x0264, 0x0264,  "Pitcairn Island" },
	{ 0x0268, 0x0268,  "Poland" },
	{ 0x026c, 0x026c,  "Portugal" },
	{ 0x0270, 0x0270,  "Guinea-Bissau" },
	{ 0x0272, 0x0272,  "East Timor (was Portuguese Timor)" },
	{ 0x0276, 0x0276,  "Puerto Rico" },
	{ 0x027a, 0x027a,  "Qatar" },
	{ 0x027e, 0x027e,  "Reunion" },
	{ 0x0282, 0x0282,  "Romania" },
	{ 0x0283, 0x0283,  "Russian Federation" },
	{ 0x0286, 0x0286,  "Rwanda" },
	{ 0x028e, 0x028e,  "st. Helena" },
	{ 0x0293, 0x0293,  "saint Kitts and Nevis" },
	{ 0x0294, 0x0294,  "Anguilla" },
	{ 0x0296, 0x0296,  "saint Lucia" },
	{ 0x029a, 0x029a,  "st. Pierre and Miquelon" },
	{ 0x029e, 0x029e,  "saint Vincent and the Grenadines" },
	{ 0x02a2, 0x02a2,  "san Marino" },
	{ 0x02a6, 0x02a6,  "sao Tome and Principe" },
	{ 0x02aa, 0x02aa,  "saudi Arabia" },
	{ 0x02ae, 0x02ae,  "senegal" },
	{ 0x02b2, 0x02b2,  "seychelles" },
	{ 0x02b6, 0x02b6,  "sierra Leone" },
	{ 0x02be, 0x02be,  "singapore" },
	{ 0x02bf, 0x02bf,  "slovakia (Slovak Republic)" },
	{ 0x02c0, 0x02c0,  "Viet Nam" },
	{ 0x02c1, 0x02c1,  "slovenia" },
	{ 0x02c2, 0x02c2,  "somalia" },
	{ 0x02c6, 0x02c6,  "south Africa" },
	{ 0x02cc, 0x02cc,  "Zimbabwe (was Southern Rhodesia)" },
	{ 0x02d4, 0x02d4,  "spain" },
	{ 0x02dc, 0x02dc,  "Western Sahara (was Spanish Sahara)" },
	{ 0x02e0, 0x02e0,  "sudan" },
	{ 0x02e4, 0x02e4,  "suriname" },
	{ 0x02e8, 0x02e8,  "svalbard & Jan Mayen Islands" },
	{ 0x02ec, 0x02ec,  "swaziland" },
	{ 0x02f0, 0x02f0,  "sweden" },
	{ 0x02f4, 0x02f4,  "switzerland" },
	{ 0x02f8, 0x02f8,  "syrian Arab Republic" },
	{ 0x02fa, 0x02fa,  "Tajikistan" },
	{ 0x02fc, 0x02fc,  "Thailand" },
	{ 0x0300, 0x0300,  "Togo" },
	{ 0x0304, 0x0304,  "Tokelau (Tokelau Islands)" },
	{ 0x0308, 0x0308,  "Tonga" },
	{ 0x030c, 0x030c,  "Trinidad and Tobago" },
	{ 0x0310, 0x0310,  "United Arab Emirates (was Trucial States)" },
	{ 0x0314, 0x0314,  "Tunisia" },
	{ 0x0318, 0x0318,  "Turkey" },
	{ 0x031b, 0x031b,  "Turkmenistan" },
	{ 0x031c, 0x031c,  "Turks and Caicos Islands" },
	{ 0x031e, 0x031e,  "Tuvalu (was part of Gilbert & Ellice Islands)" },
	{ 0x0320, 0x0320,  "Uganda" },
	{ 0x0324, 0x0324,  "Ukraine" },
	{ 0x0327, 0x0327,  "Macedonia" },
	{ 0x0332, 0x0332,  "Egypt" },
	{ 0x033a, 0x033a,  "United Kingdom of Great Britain & N. Ireland" },
	{ 0x0342, 0x0342,  "Tanzania" },
	{ 0x0348, 0x0348,  "United States of America" },
	{ 0x0352, 0x0352,  "US Virgin Islands" },
	{ 0x0356, 0x0356,  "Burkina Faso (was Upper Volta)" },
	{ 0x035a, 0x035a,  "Uruguay" },
	{ 0x035c, 0x035c,  "Uzbekistan" },
	{ 0x035e, 0x035e,  "Venezuela" },
	{ 0x036c, 0x036c,  "Wallis and Futuna Islands" },
	{ 0x0372, 0x0372,  "samoa" },
	{ 0x0377, 0x0377,  "Yemen" },
	{ 0x037b, 0x037b,  "ex Yugoslavia" },
	{ 0x037e, 0x037e,  "Zambia" },
	{ 0x0384, 0x0384,  "Scandinavia" },
	{ 0x0385, 0x0385,  "North America (Canada, Carribbean,Mexico,United States of America)" },
	{ 0x0386, 0x0386,  "All countries" },
	{ 0x0387, 0x0387,  "South America" },
	{ 0x0388, 0x0388,  "Latin America" },
	{ 0x0389, 0x0389,  "Europe" },
	{ 0x0389, 0x0389,  "Middle East" },
	{ 0x038a, 0x038a,  "North Africa" },
	{ 0x038b, 0x038b,  "Oceania (Australia, New Zealand, Melanesia, Micronesia, Polynesia)" },

     	{  0,0, NULL }
  };


  return findTableID (Table, i);
}




/*
  -- Bouquet ID Table (from dvb.org)
  -- ETR 162
*/

char *dvbstrBouquetTable_ID (u_int i)

{
  STR_TABLE  Table[] = {
	// -- updated 2003-10-16
	// -- { Bouquet ID, Bouquet ID,   "Name | Country Code | Operator" },
	{ 0x0000, 0x0000,   "Reserved | 902 | Reserved" },
	{ 0x002F, 0x002F,   "TVNZ Digital | 902 | TNVZ  " },
	{ 0x0030, 0x0030,   "TT Data Services | 902 | TechnoTrend AG" },
	{ 0x0031, 0x0031,   "Balon | 100 | Interactive Technologies PLC" },
	{ 0x006E, 0x006E,   "Europe Online Networks (EON) | 902 | Europe Online Networks S.A  " },
	{ 0x006F, 0x006F,   "WRN D-Radiosat | 902 | WRN (World Radio Network )" },
	{ 0x0070, 0x007f,   "Eutelsat Satellite System n° 1 | 902 | EUTELSAT - European Telecommunications Satellite Organization " },
	{ 0x0080, 0x0080,   "Digital Platform DIGITURK1 | 902 | Digital Platform  " },
	{ 0x0081, 0x0081,   "TV Polsat | 902 | Telewizja Polsat  " },
	{ 0x0082, 0x0082,   "TV Cabo Portugal | 902 | TV Cabo Portugal  " },
	{ 0x0083, 0x0083,   "Dijital Yayýn Pazarlama ve Ticaret A.Þ. | 902 | Dijital Yayýn Pazarlama ve Ticaret A.Þ ." },
	{ 0x0084, 0x0084,   "Digital Platform DIGITURK2 | 902 | DIGITURK" },
	{ 0x0130, 0x013f,   "Eutelsat Service Guide | 902 | Eutelsat" },
	{ 0x061F, 0x061F,   "BellSouth Entertainment | 902 | BellSouth Entertainment, Atlanta, GA, USA" },
	{ 0x1000, 0x101f,   "BskyB | 902 | British Sky Broadcasting" },
	{ 0x1020, 0x103f,   "DISH Network | 902 | Echostar Communications" },
	{ 0x1040, 0x107f,   "ARD | 902 | ARD" },
	{ 0x1080, 0x109f,   "ZDF | 902 | ZDF" },
	{ 0x10A0, 0x10bf,   "ORF | 902 | ORF" },
	{ 0x10C0, 0x10C0,   "NTV+ | 902 | NTV +" },
	{ 0x10C1, 0x10C1,   "RTL Television | 902 | RTL Television " },
	{ 0x10D1, 0x10D7,   "Primacom | 276 | Primacom A.G ." },
	{ 0x10D8, 0x10Db,   "Viasat | 900 | Viasat  " },
	{ 0x10DC, 0x10Df,   "Teracom | 900 | Teracom AB  " },
	{ 0x1500, 0x150f,   "ExpressVu | 902 | ExpressVu Inc." },
	{ 0x2000, 0x2000,   "Kaleidascope Multichoice | 902 | Filmnet" },
	{ 0x2001, 0x2001,   "Osaka Yusen | 902 | StarGuide Networks " },
	{ 0x2010, 0x2013,   "WIZJATV | 902 | AT Entertainment Ltd." },
	{ 0x2100, 0x212f,   "TSA | 902 | Telefónica Servicios Audiovisuales" },
	{ 0x2130, 0x2130,   "Galaxis | 905 | Galaxis Technology AG  " },
	{ 0x3000, 0x300f,   "TPS | 902 | La Télévision Par Satellite" },
	{ 0x3010, 0x3015,   "Sentech | 902 | Sentech" },
	{ 0x3100, 0x3100,   "STENTOR | 902 | France Telecom, CNES and DGA" },
	{ 0x3200, 0x320f,   "Australian Digital Television | 36 | Australian Terrestrial Television Networks" },
	{ 0x322B, 0x322B,   "Telstra Saturn Satellite | 902 | TelstraSaturn Limited  " },
	{ 0x332B, 0x332B,   "Telstra Saturn Cable | 907 | TelstraSaturn Limited  " },
	{ 0x3622, 0x3622,   "Irdeto Bouquet of Download data Services | 902 | Irdeto" },
	{ 0x3623, 0x3623,   "To be defined (see Wim Mooij) | 902 | Mindport " },
	{ 0x3800, 0x3800,   "OTE | 902 | OTE  " },
	{ 0x4000, 0x4000,   "HPT | 902 | HPT" },
	{ 0x4001, 0x4001,   "visAvision | 905 | European Telecommunications Satellite Organization" },
	{ 0x4010, 0x4010,   "HRT | 902 | HRT" },
	{ 0x4040, 0x407f,   "OpenTV | 902 | OpenTV Inc ." },
	{ 0x5000, 0x501f,   "BetaTechnik | 902 | BetaTechnik" },
	{ 0x6000, 0x60bf,   "NDC | 902 | News Datacom" },
	{ 0x60C0, 0x60ff,   "NDS | 0 | NDS" },
	{ 0x6180, 0x61ff,   "Information Network Centre (INC) | 156 | Information Network Centre (China)" },
	{ 0x6600, 0x6601,   "UPC | 902 | UPC  " },
	{ 0x7000, 0x700f,   "MediaServices | 902 | MSG MediaServices GmbH  " },
	{ 0xBBB1, 0xBBBb,   "BBG | 902 | Bertelsmann Broadband Group" },
	{ 0xBBBC, 0xBBBC,   "SISAL | 905 | SISAL  " },
	{ 0xC000, 0xC01f,   "Canal+ | 902 | Canal +" },
	{ 0xFC00, 0xFCff,   "France Telecom | 902 | France Telecom" },
	{ 0xFD08, 0xFD08,   "Xtra Music | 902 | Xtra Music " },
	{ 0xFFFF, 0xFFFF,   "?????? whatever this is..." },

     	{  0,0, NULL }
  };


  return findTableID (Table, i);
}







/*
  -- Trick Mode Control
  -- ISO 13818-1
*/

char *dvbstrPESTrickModeControl (u_int i)

{
  STR_TABLE  Table[] = {
	{ 0x0, 0x0,   "fast forward" },
	{ 0x1, 0x1,   "slow motion" },
	{ 0x2, 0x2,   "freeze frame" },
	{ 0x3, 0x3,   "fast reverse" },
	{ 0x4, 0x4,   "slow reverse" },
	{ 0x5, 0x7,   "reserved" },
     	{  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
  -- Data Identifier 
  -- ETR 162 [3] and EN 300 472 [6]).
*/

char *dvbstrPESDataIdentifier (u_int i)

{
  STR_TABLE  Table[] = {
	{ 0x00, 0x0F,   "reserved" },
	{ 0x10, 0x1F,   "EBU data EN 300 472" },
	{ 0x20, 0x20,   "DVB subtitling EN 300 743" },
	{ 0x21, 0x21,   "DVB synchronous data stream" },
	{ 0x22, 0x22,   "DVB synchronized data stream" },
	{ 0x23, 0x7F,   "reserved" },
	{ 0x80, 0xFF,   "user defined" },
     	{  0,0, NULL }
  };

  return findTableID (Table, i);
}








/* Annotation:

  --- $$$ TODO: store table strings in external text files, to be
  ---           more flexible

*/

