#ifndef __getservices__
#define __getservices__

#include <stdint.h>

#include <map>
#include <vector>
#include <string>

#include <ost/dmx.h>
#include <ost/frontend.h>

#include "eventserver.h"

#include "config.h"

// EVENTS...
extern CEventServer *eventServer;

struct channel {
	std::string name;
	time_t last_update;
	dvb_pid_t apid;
	dvb_pid_t ecmpid;
	dvb_pid_t pcrpid;
	dvb_pid_t pmt;
	dvb_pid_t vpid;
	uint16_t sid;
	uint16_t tsid;
	uint16_t onid;
	uint chan_nr;
	ushort service_type;

	uint32_t OnidSid ()
	{
		return (onid << 16) | sid;
	}

	channel(std::string p_name, time_t p_last_update,
			dvb_pid_t p_vpid, dvb_pid_t p_apid, dvb_pid_t p_pmtpid, dvb_pid_t p_ecmpid,
			uint16_t p_sid, uint16_t p_tsid, uint16_t p_onid, ushort p_service_type, uint p_cnr = 0, uint16_t p_pcrpid = 0x1FFF)
	{
		name = p_name;
		last_update = p_last_update;
		apid = p_apid;
		ecmpid = p_ecmpid;
		pcrpid = p_pcrpid;
		pmt = p_pmtpid;
		vpid = p_vpid;
		sid = p_sid;
		tsid = p_tsid;
		onid = p_onid;
		service_type = p_service_type;
		chan_nr = p_cnr;
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
	uint32_t onid_tsid;
} channel_msg_2;

typedef struct apid_struct {
	dvb_pid_t pid;
	char desc[25];
	bool is_ac3;
	uint8_t component_tag;
} apid_struct;

#define max_num_apids 13
#define no_ecmpid_found 0x0000
#define invalid_ecmpid_found 0x1FFF
#define zapped_chan_is_nvod 0x80

typedef struct pids {
        uint8_t count_vpids;
        dvb_pid_t vpid;
        uint8_t count_apids;
        apid_struct apids[max_num_apids];
        dvb_pid_t ecmpid;
        dvb_pid_t vtxtpid;
	dvb_pid_t pcrpid;
} pids;

extern std::map<uint, transponder>transponders;
extern std::map<uint, channel> allchans_tv;
extern std::map<uint, uint> numchans_tv;
extern std::map<std::string, uint> namechans_tv;
extern std::map<uint, channel> allchans_radio;
extern std::map<uint, uint> numchans_radio;
extern std::map<std::string, uint> namechans_radio;

#endif /* __getservices_h__ */

