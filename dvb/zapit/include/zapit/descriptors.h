#ifndef __descriptors__
#define __descriptors__

int stuffing_desc(char *buffer, FILE *logfd);
int linkage_desc(char *buffer, FILE *logfd);
int priv_data_desc(char *buffer, FILE *logfd);
int network_name_desc(char *buffer, FILE *logfd);
int service_list_desc(char *buffer, FILE *logfd);
int cable_deliv_system_desc(char *buffer, int tsid, FILE *logfd);
int sat_deliv_system_desc(char *buffer, int tsid, int diseqc, FILE *logfd);
int terr_deliv_system_desc(char *buffer, FILE *logfd);
int multilingual_network_name_desc(char *buffer, FILE *logfd);
int freq_list_desc(char *buffer, FILE *logfd);
int cell_list_desc(char *buffer, FILE *logfd);
int announcement_support_desc(char *buffer, FILE *logfd);
int service_name_desc(char *buffer, int sid, int tsid, int onid,bool scan_mode, FILE *logfd);
int bouquet_name_desc(char *buffer, FILE *logfd);
int country_availability_desc(char *buffer, FILE *logfd);
int nvod_ref_desc(char *buffer,int tsid,bool scan_mode, FILE *logfd);
int time_shift_service_desc(char *buffer, FILE *logfd);
int mosaic_desc(char *buffer, FILE *logfd);
int ca_ident_desc(char *buffer, FILE *logfd);
int telephone_desc(char *buffer, FILE *logfd);
int multilingual_service_name_desc(char *buffer, FILE *logfd);
int data_broadcast_desc(char *buffer, FILE *logfd);
int network_name_desc(char *buffer, FILE *logfd);
int cell_freq_list_desc(char *buffer, FILE *logfd);


#endif
