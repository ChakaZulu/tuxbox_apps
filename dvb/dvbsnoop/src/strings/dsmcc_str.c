/*
$Id: dsmcc_str.c,v 1.8 2003/12/17 23:15:05 rasc Exp $


 DVBSNOOP

 a dvb sniffer  and mpeg2 stream analyzer tool
 mainly for me to learn about dvb streams, mpeg2, mhp, dsm-cc, ...

 http://dvbsnoop.sourceforge.net/

 (c) 2001-2003   Rainer.Scherg@gmx.de


 -- DSM-CC -Strings



$Log: dsmcc_str.c,v $
Revision 1.8  2003/12/17 23:15:05  rasc
PES DSM-CC  ack and control commands  according ITU H.222.0 Annex B

Revision 1.7  2003/11/29 23:11:43  rasc
no message

Revision 1.6  2003/11/26 23:54:49  rasc
-- bugfixes on Linkage descriptor

Revision 1.5  2003/11/01 21:40:27  rasc
some broadcast/linkage descriptor stuff

Revision 1.4  2003/10/29 20:54:57  rasc
more PES stuff, DSM descriptors, testdata

Revision 1.3  2003/10/26 21:36:20  rasc
private DSM-CC descriptor Tags started,
INT-Section completed..

Revision 1.2  2003/10/25 19:11:50  rasc
no message

Revision 1.1  2003/10/16 19:02:28  rasc
some updates to dvbsnoop...
- small bugfixes
- tables updates from ETR 162


*/



#include "dvbsnoop.h"
#include "dsmcc_str.h"



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
  -- DSM-CC  Descriptors
  -- Private Tag Space  (DII, DSI)
  -- see EN 192
 */

char *dsmccStrDSMCC_DataCarousel_DescriptorTAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "type_descriptor" },
     {  0x02, 0x02,  "name_descriptor" },
     {  0x03, 0x03,  "info_descriptor" },
     {  0x04, 0x04,  "module_link_descriptor" },
     {  0x05, 0x05,  "CRC32_descriptor" },
     {  0x06, 0x06,  "location_descriptor" },
     {  0x07, 0x07,  "estimated_download_time_descriptor" },
     {  0x08, 0x08,  "group_link_descriptor" },
     {  0x09, 0x09,  "compressed_module_descriptor" },
     {  0x0A, 0x0A,  "subgroup_association_descriptor" },
     {  0x0B, 0x6F,  "reserved for future use by DVB" },
     {  0x70, 0x7F,  "reserved MHP" },
     {  0x80, 0xFF,  "private_descriptor" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}




/*
  -- DSM-CC  INT (UNT, SSU-UNT) Descriptors
  -- Private INT, UNT, SSU-UNT Tag Space
  -- see EN 192
 */

char *dsmccStrDSMCC_INT_UNT_DescriptorTAG (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "scheduling_descriptor" },
     {  0x02, 0x02,  "update_descriptor" },
     {  0x03, 0x03,  "ssu_location_descriptor" },
     {  0x04, 0x04,  "message_descriptor" },
     {  0x05, 0x05,  "ssu_event_name_descriptor" },
     {  0x06, 0x06,  "target_smartcard_descriptor" },
     {  0x07, 0x07,  "target_MAC_address_descriptor" },
     {  0x08, 0x08,  "target_serial_number_descriptor" },
     {  0x09, 0x09,  "target_IP_address_descriptor" },
     {  0x0A, 0x0A,  "target_IPv6_address_descriptor" },
     {  0x0B, 0x0B,  "ssu_subgroup_association_descriptor" },
     {  0x0C, 0x0C,  "IP/MAC_platform_name_descriptor" },
     {  0x0D, 0x0D,  "IP/MAC_platform_provider_name_descriptor" },
     {  0x0E, 0x0E,  "target_MAC_address_range_descriptor" },
     {  0x0F, 0x0F,  "target_IP_slash_descriptor" },
     {  0x10, 0x10,  "target_IP_source_slash_descriptor" },
     {  0x11, 0x11,  "target_IPv6_slash_descriptor" },
     {  0x12, 0x12,  "target_IPv6_source_slash_descriptor" },
     {  0x13, 0x13,  "ISP_access_mode_descriptor" },
     {  0x14, 0x3F,  "reserved" },
     //     {  0x40, 0x7F,  "DVB-SI scope" },  Telphone, private_data_spec
     {  0x80, 0xFE,  "user_private_descriptor" },
     {  0xFF, 0xFF,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}









/*
  --  MHP Organisations
*/

char *dsmccStrMHPOrg (u_int id)

{
  STR_TABLE  TableIDs[] = {
	// --{ MHP Organisation ID, MHP Organisation ID,   "Organisation Supplying MHP Applications" },
	{ 0x0000, 0x0000,   "Reserved" },
	{ 0x0001, 0x0001,   "MTV Oy" },
	{ 0x0002, 0x0002,   "Digita Oy" },
	{ 0x0003, 0x0003,   "NRK" },
	{ 0x0004, 0x0004,   "Premiere Medien GmbH & Co KG" },
	{ 0x0005, 0x0005,   "Platco Oy" },
	{ 0x0006, 0x0006,   "NOB" },
	{ 0x0007, 0x0007,   "Sofia Digital Oy" },
	{ 0x0008, 0x0008,   "YLE (Finnish Broadcasting Company)" },
	{ 0x0009, 0x0009,   "IRT (Institut für Rundfunktechnik GmbH)" },
	{ 0x000A, 0x000A,   "Cardinal Information Systems Ltd" },
	{ 0x000B, 0x000B,   "Mediaset s.p.a." },
	{ 0x000C, 0x000C,   "Ortikon Interactive Oy" },
	{ 0x000D, 0x000D,   "Austrian Broadcastion Corporation (ORF)" },
	{ 0x000E, 0x000E,   "Strategy & Technology Ltd" },
	{ 0x000F, 0x000F,   "Canal+ Technologies" },
	{ 0x0010, 0x0010,   "TV2Nord Digital" },
	{ 0x0011, 0x0011,   "Zweites Deutsches Fernsehen - ZDF" },
	{ 0x0012, 0x0012,   "SCIP AG" },
	{ 0x0013, 0x0013,   "ARD" },
	{ 0x0014, 0x0014,   "Sveng.com" },
	{ 0x0015, 0x0015,   "UniSoft Corporation" },
	{ 0x0016, 0x0016,   "Microsoft Corp" },
	{ 0x0017, 0x0017,   "Nokia" },
	{ 0x0018, 0x0018,   "SWelcom Oy" },
	{ 0x0019, 0x0019,   "Fraunhofer Institut Medienkommunikation - IMK" },
	{ 0x001A, 0x001A,   "RTL NewMedia GmbH" },
	{ 0x001B, 0x001B,   "Fraunhofer FOKUS" },
	{ 0x001C, 0x001C,   "TwonkyVision GmbH" },
	{ 0x001D, 0x001D,   "Gist Communications" },
	{ 0x001E, 0x001E,   "Televisió de Catalunya SA" },
     {  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}




/*
  --  Action Type (Linkage) EN 301 192 7.6.x
*/

char *dsmccStrAction_Type (u_int id)

{
  STR_TABLE  TableIDs[] = {
	{ 0x00, 0x00,   "reserved" },
	{ 0x01, 0x01,   "location of IP/MAC streams in DVB networks" },
	{ 0x02, 0xff,   "reserved" },
      {  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}



/*
  --  Processing Order (INT)   EN 301 192
*/

char *dsmccStrProcessing_order (u_int id)

{
  STR_TABLE  TableIDs[] = {
	{ 0x00, 0x00,   "first action" },
	{ 0x01, 0xfe,   "subsequent actions (ascending)" },
	{ 0xff, 0xff,   "no ordering implied" },
      {  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}



/*
  --  Payload scrambling control (INT)   EN 301 192
  --  Address scrambling control (INT)   EN 301 192
*/

char *dsmccStrPayload_scrambling_control (u_int id)

{
  STR_TABLE  TableIDs[] = {
	{ 0x00, 0x00,   "unscrambled" },
	{ 0x01, 0x03,   "defined by service" },
      {  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}


char *dsmccStrAddress_scrambling_control (u_int id)

{
  STR_TABLE  TableIDs[] = {
	{ 0x00, 0x00,   "unscrambled" },
	{ 0x01, 0x03,   "defined by service" },
      {  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}




/*
 * -- LinkageDescriptor0x0C Table_type  EN301192
 *
 */

char *dsmccStrLinkage0CTable_TYPE (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "not defined" },
     {  0x01, 0x01,  "NIT" },
     {  0x02, 0x02,  "BAT" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}





/*
 * -- MultiProtocolEncapsulationMACAddressRangeField
 * -- EN 301 192
 */

char *dsmccStrMultiProtEncapsMACAddrRangeField (u_int i)

{
  STR_TABLE  Table[] = {
     {  0x00, 0x00,  "reserved" },
     {  0x01, 0x01,  "6" },
     {  0x02, 0x02,  "6,5" },
     {  0x03, 0x03,  "6,5,4" },
     {  0x04, 0x04,  "6,5,4,3" },
     {  0x05, 0x05,  "6,5,4,3,2" },
     {  0x06, 0x06,  "6,5,4,3,2,1" },
     {  0x07, 0x07,  "reserved" },
     {  0,0, NULL }
  };

  return findTableID (Table, i);
}







/*
  --  Platform ID    ETR 162
*/

char *dsmccStrPlatform_ID (u_int id)

{
  STR_TABLE  TableIDs[] = {
	  /* $$$ TODO   ... */
	{ 0x000000, 0x000000,   "" },
      {  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}




/*
  --  Carousel Type ID    EN 301 192
*/

char *dsmccStrCarouselType_ID (u_int id)

{
  STR_TABLE  TableIDs[] = {
	{ 0x00, 0x00,   "reserved" },
	{ 0x01, 0x01,   "one layer carousel" },
	{ 0x02, 0x02,   "two layer carousel" },
	{ 0x03, 0x03,   "reserved" },
      	{  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}




/*
  --  Higher Protocol ID    EN 301 192
*/

char *dsmccStrHigherProtocol_ID (u_int id)

{
  STR_TABLE  TableIDs[] = {
	{ 0x00, 0x00,   "reserved" },
	{ 0x01, 0x01,   "dGNSS data" },
	{ 0x02, 0x02,   "TPEG" },
	{ 0x03, 0x0F,   "reserved" },
      	{  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}



/*
  --  Update Type Table    TR 102 006
*/

char *dsmccStrUpdateType_ID (u_int id)

{
  STR_TABLE  TableIDs[] = {
	{ 0x00, 0x00,   "proprietary update solution" },
	{ 0x01, 0x01,   "standard update carousel via broadcast" },
	{ 0x02, 0x02,   "system software update with notification table (UNT) via broadcast" },
	{ 0x03, 0x03,   "system software update using return channel with UNT" },
	{ 0x04, 0x0F,   "reserved" },
      	{  0,0, NULL }
  };


  return findTableID (TableIDs, id);
}




/*
  --  OUI  Table    LLC-SNAP  IEEE 802
  --  real table is very large!!
  --  see: http://standards.ieee.org/regauth/oui/index.shtml
*/

char *dsmccStrOUI  (u_int id)

{
	return (char *) "http://standards.ieee.org/regauth/oui/";
}





/*
  --  Command_ID
  --  e.g. from ITU H.222.0 Annex B
  --  
*/

char *dsmccStr_Command_ID  (u_int id)

{
  STR_TABLE  TableIDs[] = {
	{ 0x00, 0x00,   "forbidden" },
	{ 0x01, 0x01,   "control" },
	{ 0x02, 0x02,   "Acknownledge" },
	{ 0x03, 0xFF,   "reserved" },
      	{  0,0, NULL }
  };

  return findTableID (TableIDs, id);
}



/*
  --  Command_ID
  --  e.g. from ITU H.222.0 Annex B
  --  
*/

char *dsmccStr_SelectMode_ID  (u_int id)

{
  STR_TABLE  TableIDs[] = {
	{ 0x00, 0x00,   "forbidden" },
	{ 0x01, 0x01,   "storage" },
	{ 0x02, 0x02,   "retrieval" },
	{ 0x03, 0x1F,   "reserved" },
      	{  0,0, NULL }
  };

  return findTableID (TableIDs, id);
}



/*
  --  direction_indicator
  --  e.g. from ITU H.222.0 Annex B
  --  
*/

char *dsmccStr_DirectionIndicator (u_int id)

{
  STR_TABLE  TableIDs[] = {
	{ 0x00, 0x00,   "Forward" },
	{ 0x01, 0x01,   "Backward" },
      	{  0,0, NULL }
  };

  return findTableID (TableIDs, id);
}











