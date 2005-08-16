/*
 * $Id: channel.h,v 1.29 2005/08/16 21:59:54 metallica Exp $
 *
 * (C) 2002 Steffen Hehn <mcclean@berlios.de>
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __zapit_channel_h__
#define __zapit_channel_h__

/* system */
#include <string>
#include <inttypes.h>

/* zapit */
#include "ci.h"
#include "types.h"

class CZapitAudioChannel
{
	public:
		unsigned short		pid;
		bool			isAc3;
		std::string		description;
		unsigned char		componentTag;
};

class CZapitChannel
{
	private:
		/* channel name */
		std::string name;

		/* pids of this channel */
		std::vector <CZapitAudioChannel *> audioChannels;
		unsigned short			pcrPid;
		unsigned short			pmtPid;
		unsigned short			teletextPid;
		unsigned short			videoPid;
		unsigned short			privatePid;

		/* set true when pids are set up */
		bool pidsFlag;

		/* last selected audio channel */
		unsigned char			currentAudioChannel;

		/* read only properties, set by constructor */
		t_service_id			service_id;
		t_transport_stream_id		transport_stream_id;
		t_original_network_id		original_network_id;
		t_network_id			network_id;
		unsigned char			DiSEqC;
		t_satellite_position		satellitePosition;
		/* used for Eutelsat 13e dup transponders */
		frequency_kHz_t			frequency;

		/* read/write properties (write possibility needed by scan) */
		unsigned char			serviceType;

		/* the conditional access program map table of this channel */
		CCaPmt * 			caPmt;

	public:
		/* constructor, desctructor */
		CZapitChannel(const std::string & p_name, t_service_id p_sid, t_transport_stream_id p_tsid, t_original_network_id p_onid, unsigned char p_service_type, unsigned char p_DiSEqC, t_satellite_position p_satellite_position, frequency_kHz_t frequency);
		~CZapitChannel(void);

		/* get methods - read only variables */
		t_network_id		getNetworkID(void)		const { return network_id; }
		t_service_id		getServiceId(void)         	const { return service_id; }
		t_transport_stream_id	getTransportStreamId(void) 	const { return transport_stream_id; }
		t_original_network_id	getOriginalNetworkId(void) 	const { return original_network_id; }
		unsigned char        	getServiceType(void)       	const { return serviceType; }
		unsigned char        	getDiSEqC(void)            	const { return DiSEqC; }
		t_channel_id            getChannelID(void)              const { return CREATE_CHANNEL_ID; }
		transponder_id_t        getTransponderId(void)          const { return CREATE_TRANSPONDER_ID_FROM_FREQUENCY_SATELLITEPOSITION_ORIGINALNETWORK_TRANSPORTSTREAM_ID(frequency, satellitePosition,original_network_id,transport_stream_id); }
		
		/* get methods - read and write variables */
		const std::string	getName(void)			const { return name; }
		t_satellite_position	getSatellitePosition(void)	const { return satellitePosition; }
		frequency_kHz_t		getFrequency(void)       	const { return frequency; }
		unsigned char 		getAudioChannelCount(void)	{ return audioChannels.size(); }
		unsigned short		getPcrPid(void)			{ return pcrPid; }
		unsigned short		getPmtPid(void)			{ return pmtPid; }
		unsigned short		getTeletextPid(void)		{ return teletextPid; }
		unsigned short		getVideoPid(void)		{ return videoPid; }
		unsigned short		getPrivatePid(void)		{ return privatePid; }
		bool			getPidsFlag(void)		{ return pidsFlag; }
		CCaPmt *		getCaPmt(void)			{ return caPmt; }

		CZapitAudioChannel * 	getAudioChannel(unsigned char index = 0xFF);
		unsigned short 		getAudioPid(unsigned char index = 0xFF);
		unsigned char  		getAudioChannelIndex(void)	{ return currentAudioChannel; }

		int addAudioChannel(const unsigned short pid, const bool isAc3, const std::string & description, const unsigned char componentTag);

		/* set methods */
		void setServiceType(const unsigned char pserviceType)	{ serviceType = pserviceType; }
		inline void setName(const std::string pName)            { name = pName; }
		void setAudioChannel(unsigned char pAudioChannel)	{ if (pAudioChannel < audioChannels.size()) currentAudioChannel = pAudioChannel; }
		void setPcrPid(unsigned short pPcrPid)			{ pcrPid = pPcrPid; }
		void setPmtPid(unsigned short pPmtPid)			{ pmtPid = pPmtPid; }
		void setTeletextPid(unsigned short pTeletextPid)	{ teletextPid = pTeletextPid; }
		void setVideoPid(unsigned short pVideoPid)		{ videoPid = pVideoPid; }
		void setPrivatePid(unsigned short pPrivatePid)		{ privatePid = pPrivatePid; }
		void setPidsFlag(void)					{ pidsFlag = true; }
		void setCaPmt(CCaPmt *pCaPmt)				{ caPmt = pCaPmt; }
		/* cleanup methods */
		void resetPids(void);
};

#endif /* __zapit_channel_h__ */
