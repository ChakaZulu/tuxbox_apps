/*
 * $Id: getservices.h,v 1.33 2002/04/10 18:36:21 obi Exp $
 */

#ifndef __getservices_h__
#define __getservices_h__

#include <ctype.h>
#include <ost/dmx.h>
#include <ost/frontend.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <map>
#include <string>
#include <vector>

#include "descriptors.h"
#include "eventserver.h"
#include "sdt.h"
#include "xml/xmltree.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define CONFIGDIR "/var/tuxbox/config"
#endif

#define max_num_apids 13
#define zapped_chan_is_nvod 0x80

#define NONE 0x0000
#define INVALID 0x1FFF

void ParseTransponders (XMLTreeNode *xmltransponder, uint8_t DiSEqC);
void ParseChannels (XMLTreeNode *node, uint16_t transport_stream_id, uint16_t original_network_id);
void FindTransponder (XMLTreeNode *root);
void LoadSortList ();
int LoadServices ();

// EVENTS...
extern CEventServer *eventServer;

typedef struct apid_struct
{
	dvb_pid_t pid;
	char desc[25];
	bool is_ac3;
	uint8_t component_tag;
} apid_struct;

typedef struct pids
{
        uint8_t count_vpids;
        dvb_pid_t vpid;
        uint8_t count_apids;
        apid_struct apids[max_num_apids];
        dvb_pid_t ecmpid;
	dvb_pid_t emmpid;
	dvb_pid_t pcrpid;
	dvb_pid_t pmtpid;
        dvb_pid_t vtxtpid;
} pids;

class CZapitChannel
{
	private:
		/* channel name */
		std::string name;

		/* pids of this channel */
		pids chanpids;

		/* set true when pids are set up */
		bool knowsPidsFlag;

		/* last selected audio channel */
		uint8_t audioChannel;

		/* number in channel list */
		uint16_t channelNumber;

		/* read only properties, set by constructor */
		uint16_t serviceId;
		uint16_t transportStreamId;
		uint16_t originalNetworkId;
		uint8_t serviceType;

	public:
		/* constructor */
		CZapitChannel (std::string p_name, uint16_t p_sid, uint16_t p_tsid, uint16_t p_onid, uint8_t p_service_type, uint16_t p_chan_nr = 0)
		{
			name = p_name;
			memset(&chanpids, 0, sizeof(chanpids));
			knowsPidsFlag = false;
			audioChannel = 0;
			serviceId = p_sid;
			transportStreamId = p_tsid;
			originalNetworkId = p_onid;
			serviceType = p_service_type;
			channelNumber = p_chan_nr;
		}

		/* get methods - read and write variables */
		std::string getName()		{ return name; }
		uint8_t getAudioChannel()	{ return audioChannel; }
		dvb_pid_t getAudioPid()		{ return chanpids.apids[audioChannel].pid; }
		dvb_pid_t getEcmPid()		{ return chanpids.ecmpid; }
		dvb_pid_t getEmmPid()		{ return chanpids.emmpid; }
		dvb_pid_t getPcrPid()		{ return chanpids.pcrpid; }
		dvb_pid_t getPmtPid()		{ return chanpids.pmtpid; }
		dvb_pid_t getVideoPid()		{ return chanpids.vpid; }
		pids getPids()			{ return chanpids; }
		bool knowsPids()		{ return knowsPidsFlag; }
		uint16_t getChannelNumber()	{ return channelNumber; }

		/* get methods - read only variables */
		uint16_t getServiceId()		{ return serviceId; }
		uint16_t getTransportStreamId()	{ return transportStreamId; }
		uint16_t getOriginalNetworkId()	{ return originalNetworkId; }
		uint8_t getServiceType()	{ return serviceType; }
		uint32_t getOnidSid()		{ return (originalNetworkId << 16) | serviceId; }

		/* set methods */
		void setName(std::string pName)			{ name = pName; }
		void setAudioChannel(uint8_t pAudioChannel)	{ audioChannel = pAudioChannel; }
		void setAudioPid(dvb_pid_t pAudioPid)		{ chanpids.apids[audioChannel].pid = pAudioPid; }
		void setEcmPid(dvb_pid_t pEcmPid)		{ chanpids.ecmpid = pEcmPid; }
		void setEmmPid(dvb_pid_t pEmmPid)		{ chanpids.emmpid = pEmmPid; }
		void setPcrPid(dvb_pid_t pPcrPid)		{ chanpids.pcrpid = pPcrPid; }
		void setPmtPid(dvb_pid_t pPmtPid)		{ chanpids.pmtpid = pPmtPid; }
		void setVideoPid(dvb_pid_t pVideoPid)		{ chanpids.vpid = pVideoPid; }
		void setPids(pids pPids)			{ chanpids = pPids; knowsPidsFlag = true; }
		void setChannelNumber(uint16_t pChannelNumber)	{ channelNumber = pChannelNumber; }

		/* cleanup methods */
		void resetPids()	{ memset(&chanpids, 0, sizeof(chanpids)); knowsPidsFlag = false; }
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

extern std::map<uint, transponder> transponders;

extern std::map<uint, CZapitChannel> allchans_tv;
extern std::map<uint, uint> numchans_tv;
extern std::map<std::string, uint> namechans_tv;

extern std::map<uint, CZapitChannel> allchans_radio;
extern std::map<uint, uint> numchans_radio;
extern std::map<std::string, uint> namechans_radio;

#endif /* __getservices_h__ */

