/*
 * $Id: getservices.h,v 1.40 2002/05/12 01:56:18 obi Exp $
 */

#ifndef __getservices_h__
#define __getservices_h__

#include <ost/dmx.h>
#include <ost/frontend.h>
#include <stdint.h>
#include <string.h>

#include <iostream>
#include <string>

#include <eventserver.h>

#include <xml/xmltree.h>
#include <zapci/ci.h>
#include <zapsi/descriptors.h>
#include <zapsi/sdt.h>

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
void ParseChannels (XMLTreeNode *node, uint16_t transport_stream_id, uint16_t original_network_id, uint8_t DiSEqC);
void FindTransponder (XMLTreeNode *root);
void LoadSortList ();
int LoadServices ();

class CZapitAudioChannel
{
	public:
		dvb_pid_t pid;
		bool isAc3;
		std::string description;
		unsigned char componentTag;
};

class CZapitChannel
{
	private:
		/* channel name */
		std::string name;

		/* pids of this channel */
		CZapitAudioChannel * audioChannels[max_num_apids];
		dvb_pid_t pcrPid;
		dvb_pid_t pmtPid;
		dvb_pid_t teletextPid;
		dvb_pid_t videoPid;

		/* set true when pids are set up */
		bool pidsFlag;

		/* last selected audio channel */
		uint8_t currentAudioChannel;

		/* number of audio channels */
		uint8_t audioChannelCount;

		/* number in channel list */
		uint16_t channelNumber;

		/* read only properties, set by constructor */
		uint16_t serviceId;
		uint16_t transportStreamId;
		uint16_t originalNetworkId;
		uint8_t serviceType;
		uint8_t DiSEqC;

		/* the conditional access program map table of this channel */
		CCaPmt * caPmt;

	public:
		/* constructor, desctructor */
		CZapitChannel (std::string p_name, uint16_t p_sid, uint16_t p_tsid, uint16_t p_onid, uint8_t p_service_type, uint16_t p_chan_nr, uint8_t p_DiSEqC);
		~CZapitChannel ();

		/* get methods - read only variables */
		uint16_t getServiceId()		{ return serviceId; }
		uint16_t getTransportStreamId()	{ return transportStreamId; }
		uint16_t getOriginalNetworkId()	{ return originalNetworkId; }
		uint8_t getServiceType()	{ return serviceType; }
		uint8_t getDiSEqC()		{ return DiSEqC; }
		uint32_t getOnidSid()		{ return (originalNetworkId << 16) | serviceId; }
		uint32_t getTsidOnid()		{ return (transportStreamId << 16) | originalNetworkId; }

		/* get methods - read and write variables */
		std::string getName()		{ return name; }
		uint8_t getAudioChannelCount()	{ return audioChannelCount; }
		dvb_pid_t getPcrPid()		{ return pcrPid; }
		dvb_pid_t getPmtPid()		{ return pmtPid; }
		dvb_pid_t getTeletextPid()	{ return teletextPid; }
		dvb_pid_t getVideoPid()		{ return videoPid; }
		uint16_t getChannelNumber()	{ return channelNumber; }
		bool getPidsFlag()		{ return pidsFlag; }
		CCaPmt * getCaPmt()		{ return caPmt; }

		CZapitAudioChannel * getAudioChannel (uint8_t index = 0xFF);
		dvb_pid_t getAudioPid (uint8_t index = 0xFF);

		int addAudioChannel(dvb_pid_t pid, bool isAc3, string description, unsigned char componentTag);

		/* set methods */
		void setName(std::string pName)			{ name = pName; }
		void setAudioChannel(uint8_t pAudioChannel)	{ currentAudioChannel = pAudioChannel; }
		void setPcrPid(dvb_pid_t pPcrPid)		{ pcrPid = pPcrPid; }
		void setPmtPid(dvb_pid_t pPmtPid)		{ pmtPid = pPmtPid; }
		void setTeletextPid(dvb_pid_t pTeletextPid)	{ teletextPid = pTeletextPid; }
		void setVideoPid(dvb_pid_t pVideoPid)		{ videoPid = pVideoPid; }
		void setChannelNumber(uint16_t pChannelNumber)	{ channelNumber = pChannelNumber; }
		bool setPidsFlag()				{ pidsFlag = true; }
		void setCaPmt(CCaPmt * pCaPmt)			{ caPmt = pCaPmt; }

		/* cleanup methods */
		void resetPids();
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

typedef struct bouquet_msg_struct
{
	uint32_t bouquet_nr;
	char name[30];

} bouquet_msg;

typedef struct channel_msg_struct
{
	uint32_t chan_nr;
	char name[30];
	char mode;

} channel_msg;

typedef struct channel_msg_struct_2
{
	uint chan_nr;
	char name[30];
	char mode;
	uint32_t onid_tsid;

} channel_msg_2;

#endif /* __getservices_h__ */
