#ifndef __getservices__
#define __getservices__

typedef struct channelstruct *chanptr;

typedef struct channelstruct {
	uint chan_nr;
	char *name;
	time_t last_update;
	uint vpid;
	uint apid;
	uint pmt;
	uint frequency;
	uint symbolrate;
	ushort Polarity;
	uint Diseqc;
	ushort Fec;
	uint ecmpid;
	uint onid;
	uint tsid;
	chanptr next;
	chanptr prev;
} channel;

typedef struct channel_msg_struct {
	uint chan_nr;
	char name[30];
	char mode;
} channel_msg;

typedef struct channel_msg_struct_2 {
	uint chan_nr;
	char name[30];
	char mode;
    unsigned int onid_tsid;
} channel_msg_2;

typedef struct pids{
	ushort count_vpids;
	uint vpid;
	ushort count_apids;
	uint apid[5];
	char apid_desc[5][30];
	uint ecmpid;
}pids;

#endif



