/*
 * $Id: getservices.h,v 1.41 2002/05/13 14:56:51 obi Exp $
 */

#ifndef __getservices_h__
#define __getservices_h__

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

void ParseTransponders (XMLTreeNode * xmltransponder, unsigned char DiSEqC);
void ParseChannels (XMLTreeNode * node, unsigned short transport_stream_id, unsigned short original_network_id, unsigned char DiSEqC);
void FindTransponder (XMLTreeNode * root);
void LoadSortList ();
int LoadServices ();

class CZapitAudioChannel
{
	public:
		unsigned short pid;
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
		unsigned short pcrPid;
		unsigned short pmtPid;
		unsigned short teletextPid;
		unsigned short videoPid;

		/* set true when pids are set up */
		bool pidsFlag;

		/* last selected audio channel */
		unsigned char currentAudioChannel;

		/* number of audio channels */
		unsigned char audioChannelCount;

		/* number in channel list */
		unsigned short channelNumber;

		/* read only properties, set by constructor */
		unsigned short serviceId;
		unsigned short transportStreamId;
		unsigned short originalNetworkId;
		unsigned char serviceType;
		unsigned char DiSEqC;

		/* the conditional access program map table of this channel */
		CCaPmt * caPmt;

	public:
		/* constructor, desctructor */
		CZapitChannel (std::string p_name, unsigned short p_sid, unsigned short p_tsid, unsigned short p_onid, unsigned char p_service_type, unsigned short p_chan_nr, unsigned char p_DiSEqC);
		~CZapitChannel ();

		/* get methods - read only variables */
		unsigned short getServiceId()		{ return serviceId; }
		unsigned short getTransportStreamId()	{ return transportStreamId; }
		unsigned short getOriginalNetworkId()	{ return originalNetworkId; }
		unsigned char getServiceType()		{ return serviceType; }
		unsigned char getDiSEqC()		{ return DiSEqC; }
		unsigned int getOnidSid()		{ return (originalNetworkId << 16) | serviceId; }
		unsigned int getTsidOnid()		{ return (transportStreamId << 16) | originalNetworkId; }

		/* get methods - read and write variables */
		std::string getName()			{ return name; }
		unsigned char getAudioChannelCount()	{ return audioChannelCount; }
		unsigned short getPcrPid()		{ return pcrPid; }
		unsigned short getPmtPid()		{ return pmtPid; }
		unsigned short getTeletextPid()		{ return teletextPid; }
		unsigned short getVideoPid()		{ return videoPid; }
		unsigned short getChannelNumber()	{ return channelNumber; }
		bool getPidsFlag()			{ return pidsFlag; }
		CCaPmt * getCaPmt()			{ return caPmt; }

		CZapitAudioChannel * getAudioChannel (unsigned char index = 0xFF);
		unsigned short getAudioPid (unsigned char index = 0xFF);

		int addAudioChannel(unsigned short pid, bool isAc3, string description, unsigned char componentTag);

		/* set methods */
		void setName(std::string pName)				{ name = pName; }
		void setAudioChannel(unsigned char pAudioChannel)	{ currentAudioChannel = pAudioChannel; }
		void setPcrPid(unsigned short pPcrPid)			{ pcrPid = pPcrPid; }
		void setPmtPid(unsigned short pPmtPid)			{ pmtPid = pPmtPid; }
		void setTeletextPid(unsigned short pTeletextPid)	{ teletextPid = pTeletextPid; }
		void setVideoPid(unsigned short pVideoPid)		{ videoPid = pVideoPid; }
		void setChannelNumber(unsigned short pChannelNumber)	{ channelNumber = pChannelNumber; }
		void setPidsFlag()					{ pidsFlag = true; }
		void setCaPmt(CCaPmt * pCaPmt)				{ caPmt = pCaPmt; }

		/* cleanup methods */
		void resetPids();
};

struct transponder
{
	unsigned short transport_stream_id;
	FrontendParameters feparams;
	unsigned char polarization;
	unsigned char DiSEqC;
	unsigned short original_network_id;

	transponder (unsigned short p_transport_stream_id, FrontendParameters p_feparams)
	{
		transport_stream_id = p_transport_stream_id;
		feparams = p_feparams;
		polarization = 0;
		DiSEqC = 0;
		original_network_id = 0;
	}

	transponder (unsigned short p_transport_stream_id, FrontendParameters p_feparams, unsigned short p_polarization, unsigned char p_DiSEqC, unsigned short p_original_network_id)
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
	unsigned int bouquet_nr;
	char name[30];

} bouquet_msg;

typedef struct channel_msg_struct
{
	unsigned int chan_nr;
	char name[30];
	char mode;

} channel_msg;

typedef struct channel_msg_struct_2
{
	unsigned int chan_nr;
	char name[30];
	char mode;
	unsigned int onid_tsid;

} channel_msg_2;

#endif /* __getservices_h__ */
