/*
  -- dvb decoder helper functions
*/

char *dvbstrTableID (u_int id);
char *dvbstrDescriptorTAG (u_int tag);
char *dvbstrWEST_EAST_FLAG (u_int tag);
char *dvbstrPolarisation_FLAG (u_int tag);
char *dvbstrModulationSAT_FLAG (u_int tag);
char *dvbstrFECinner_SCHEME (u_int tag);
char *dvbstrFECouter_SCHEME (u_int tag);
char *dvbstrLinkage_TYPE(u_int tag);
char *dvbstrHandover_TYPE(u_int tag);
char *dvbstrOrigin_TYPE(u_int tag);
char *dvbstrService_TYPE(u_int tag);
char *dvbstrStream_TYPE(u_int tag);
char *dvbstrAudio_TYPE(u_int tag);
char *dvbstrCASystem_ID(u_int id);
char *dvbstrNetworkIdent_ID(u_int id);
char *dvbstrTeletext_TYPE(u_int id);
char *dvbstrTerrBandwidth_SCHEME(u_int id);
char *dvbstrTerrConstellation_FLAG(u_int id);
char *dvbstrTerrHierarchy_FLAG(u_int id);
char *dvbstrTerrCodeRate_FLAG(u_int id);
char *dvbstrTerrGuardInterval_FLAG(u_int id);
char *dvbstrTerrTransmissionMode_FLAG(u_int id);
char *dvbstrAspectRatioInfo_FLAG(u_int id);
char *dvbstrHierarchy_TYPE(u_int id);
char *dvbstrRunningStatus_FLAG (u_int id);
char *dvbstrDataStreamVIDEOAlignment_TYPE (u_int id);
char *dvbstrDataStreamAUDIOAlignment_TYPE (u_int id);
char *dvbstrDataService_ID (u_int id);
char *dvbstrContent_Component_TYPE (u_int id);
char *dvbstrLogCellPresInfo_TYPE(u_int id);
char *dvbstrCellLinkageInfo_TYPE(u_int id);
char *dvbstrTextCharset_TYPE(u_int id);
char *dvbstrContentNibble_TYPE(u_int id);
char *dvbstrParentalRating_TYPE(u_int id);
char *dvbstrDelivSysCoding_TYPE(u_int id);
char *dvbstrShortSmoothingBufSize_TYPE(u_int id);
char *dvbstrShortSmoothingBufLeakRate_TYPE(u_int id);
char *dvbstrAC3Component_TYPE(u_int id);
char *dvbstrAncillaryData_ID(u_int id);
char *dvbstrAnnouncement_TYPE(u_int id);
char *dvbstrAnnouncementReference_TYPE(u_int id);


/* -- Transport Stream Stuff */

char *dvbstrTSpid_ID (u_int id);
char *dvbstrTS_AdaptionField_TYPE (u_int id);
char *dvbstrTS_ScramblingCtrl_TYPE (u_int id);

/* -- PES stuff */

char *dvbstrPESstream_ID(u_int id);



// Todo: 
// Bouquet IDs (ETR 162)





