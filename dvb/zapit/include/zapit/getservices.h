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
} channel_msg;

typedef struct pids{
	ushort count_vpids;
	uint vpid;
	ushort count_apids;
	uint apid[5];
	char *apid0_desc;
	char *apid1_desc;
	char *apid2_desc;
	char *apid3_desc;
	char *apid4_desc;
	uint ecmpid;
}pids;
