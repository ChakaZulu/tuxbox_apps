#ifndef __getservices__
#define __getservices__

#include <stdint.h>

#include <map>
#include <vector>
#include <string>

#include <ost/frontend.h>

#include "eventserver.h"

#include "config.h"

// EVENTS...
extern CEventServer *eventServer;

struct channel {
  std::string name;
  time_t last_update;
  uint vpid;
  uint apid;
  uint pmt;
  uint ecmpid;
  uint sid;
  uint tsid;
  uint onid;
  uint chan_nr;
  ushort service_type;
  uint pcrpid;

  unsigned int OnidSid(){return( (onid << 16)|sid);}
  channel(std::string Name, time_t Last_update, uint Vpid, uint Apid, uint Pmt, uint Ecmpid, uint Sid, uint Tsid, uint Onid, ushort Service_type, uint cnr=0, uint Pcrpid=0x1fff)
  {
    name = Name;
    last_update = Last_update;
    vpid = Vpid;
    apid = Apid;
    pmt = Pmt;
    ecmpid = Ecmpid;
    sid = Sid;
    tsid = Tsid;
    onid = Onid;
    service_type = Service_type;
    chan_nr = cnr;
    pcrpid = Pcrpid;
      }
};

struct transponder
{
	uint16_t transport_stream_id;
	FrontendParameters feparams;
	uint8_t polarization;
	uint8_t DiSEqC;
	uint16_t original_network_id;

	transponder(uint16_t p_transport_stream_id, FrontendParameters p_feparams)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = 0;
		DiSEqC = 0;
		original_network_id = 0;
	}

	transponder(uint16_t p_transport_stream_id, FrontendParameters p_feparams, uint16_t p_polarization, uint8_t p_DiSEqC, uint16_t p_original_network_id)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = p_polarization;
		DiSEqC = p_DiSEqC;
		original_network_id = p_original_network_id;
	}
};

typedef struct bouquet_msg_struct {
        uint bouquet_nr;
        char name[30];
} bouquet_msg;

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

typedef struct apid_struct_2 {
    uint    pid;
    char    desc[25];
    int     is_ac3;
    int     component_tag;
}apid_struct;

#define max_num_apids 13
#define no_ecmpid_found 0x10000
#define invalid_ecmpid_found 0x10001

#define zapped_chan_is_nvod 0x80

typedef struct pids{
        ushort count_vpids;
        uint vpid;
        ushort count_apids;
        apid_struct apids[max_num_apids];
        uint ecmpid;
        uint vtxtpid;
		uint pcrpid;
}pids;


extern std::map<uint, transponder>transponders;
extern std::map<uint, channel> allchans_tv;
extern std::map<uint, uint> numchans_tv;
extern std::map<std::string, uint> namechans_tv;
extern std::map<uint, channel> allchans_radio;
extern std::map<uint, uint> numchans_radio;
extern std::map<std::string, uint> namechans_radio;



#endif
