#ifndef __descriptors__
#define __descriptors__

int stuffing_desc(char *buffer);
int linkage_desc(char *buffer);
int priv_data_desc(char *buffer);
int network_name_desc(char *buffer);
int service_list_desc(char *buffer);
int cable_deliv_system_desc(char *buffer, int tsid);
int sat_deliv_system_desc(char *buffer, int tsid, int diseqc);
int terr_deliv_system_desc(char *buffer);
int multilingual_network_name_desc(char *buffer);
int freq_list_desc(char *buffer);
int cell_list_desc(char *buffer);
int announcement_support_desc(char *buffer);
int service_name_desc(char *buffer, int sid, int tsid, int onid,bool scan_mode);
int bouquet_name_desc(char *buffer);
int country_availability_desc(char *buffer);
int nvod_ref_desc(char *buffer,int tsid,bool scan_mode);
int time_shift_service_desc(char *buffer);
int mosaic_desc(char *buffer);
int ca_ident_desc(char *buffer);
int telephone_desc(char *buffer);
int multilingual_service_name_desc(char *buffer);
int data_broadcast_desc(char *buffer);
int network_name_desc(char *buffer);
int cell_freq_list_desc(char *buffer);


#endif
