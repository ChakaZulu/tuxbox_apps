/*
$Id: dvb_descriptor.h,v 1.2 2003/10/19 21:05:53 rasc Exp $ 


  dvbsnoop
  (c) Rainer Scherg 2001-2003

  DVB Descriptors  ETSI 300 468


$Log: dvb_descriptor.h,v $
Revision 1.2  2003/10/19 21:05:53  rasc
- some datacarousell stuff started

Revision 1.1  2003/07/08 19:59:50  rasc
restructuring... some new, some fixes,
trying to include DSM-CC, Well someone a ISO13818-6 and latest version of ISO 18313-1 to spare?


*/


int   descriptorDVB (u_char *b);
void  descriptorDVB_any (u_char *b);

void  descriptorDVB_NetName (u_char *b);
void  descriptorDVB_ServList (u_char *b);
void  descriptorDVB_Stuffing (u_char *b);
void  descriptorDVB_SatDelivSys (u_char *b);
void  descriptorDVB_CableDelivSys (u_char *b);
void  descriptorDVB_VBI_Data (u_char *b);
void  descriptorDVB_VBI_Teletext (u_char *b);
void  descriptorDVB_BouquetName (u_char *b);
void  descriptorDVB_Service (u_char *b);
void  descriptorDVB_CountryAvail (u_char *b);
void  descriptorDVB_Linkage (u_char *b);
  void sub_descriptorDVB_Linkage0x0B (u_char *b, int len);
  void sub_descriptorDVB_Linkage0x0C (u_char *b, int len);
void  descriptorDVB_NVOD_Reference (u_char *b);
void  descriptorDVB_TimeShiftedService (u_char *b);
void  descriptorDVB_ShortEvent (u_char *b);
void  descriptorDVB_ExtendedEvent (u_char *b);
void  descriptorDVB_TimeShiftedEvent (u_char *b);
void  descriptorDVB_Component (u_char *b);
void  descriptorDVB_Mosaic (u_char *b);
void  descriptorDVB_StreamIdent (u_char *b);
void  descriptorDVB_CAIdentifier (u_char *b);
void  descriptorDVB_Content(u_char *b);
void  descriptorDVB_ParentalRating(u_char *b);
void  descriptorDVB_Teletext (u_char *b);
void  descriptorDVB_Telephone(u_char *b);
void  descriptorDVB_LocalTimeOffset (u_char *b);
void  descriptorDVB_Subtitling(u_char *b);
void  descriptorDVB_TerrestDelivSys (u_char *b);
void  descriptorDVB_MultilingNetworkName (u_char *b);
void  descriptorDVB_MultilingBouquetName (u_char *b);
void  descriptorDVB_MultilingServiceName (u_char *b);
void  descriptorDVB_MultilingComponent (u_char *b);
void  descriptorDVB_PrivateDataSpecifier (u_char *b);
void  descriptorDVB_ServiceMove (u_char *b);
void  descriptorDVB_FrequencyList (u_char *b);
void  descriptorDVB_ShortSmoothingBuffer(u_char *b);
void  descriptorDVB_PartialTransportStream(u_char *b);
void  descriptorDVB_DataBroadcast (u_char *b);
void  descriptorDVB_CASystem(u_char *b);
void  descriptorDVB_DataBroadcastID(u_char *b);
void  descriptorDVB_TransportStream(u_char *b);
void  descriptorDVB_DSNG(u_char *b);
void  descriptorDVB_PDC(u_char *b);
void  descriptorDVB_AC3(u_char *b);
void  descriptorDVB_AncillaryData(u_char *b);
void  descriptorDVB_CellList(u_char *b);
void  descriptorDVB_CellFrequencyLink(u_char *b);
void  descriptorDVB_AnnouncementSupport(u_char *b);
void  descriptorDVB_ApplicationSignalling(u_char *b);
void  descriptorDVB_AdaptionFieldData(u_char *b);
void  descriptorDVB_ServiceIdentifier(u_char *b);
void  descriptorDVB_ServiceAvailability(u_char *b);

