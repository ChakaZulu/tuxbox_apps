/*
 * $Id: channel.h,v 1.10 2002/09/11 20:12:42 thegoodguy Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
 *	& Steffen Hehn <mcclean@berlios.de>
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

#ifndef __channel_h__
#define __channel_h__

/* system */
#include <string>
#include <stdint.h>

/* zapit */
#include <zapci/ci.h>

typedef uint32_t t_channel_id;             // channel_id: (original_network_id << 16) | service_id

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
		std::vector <CZapitAudioChannel *> audioChannels;
		unsigned short pcrPid;
		unsigned short pmtPid;
		unsigned short teletextPid;
		unsigned short videoPid;

		/* set true when pids are set up */
		bool pidsFlag;

		/* last selected audio channel */
		unsigned char currentAudioChannel;

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
		CZapitChannel (std::string p_name, unsigned short p_sid, unsigned short p_tsid, unsigned short p_onid, unsigned char p_service_type, unsigned char p_DiSEqC);
		~CZapitChannel ();

		/* get methods - read only variables */
		unsigned short getServiceId()		{ return serviceId; }
		unsigned short getTransportStreamId()	{ return transportStreamId; }
		unsigned short getOriginalNetworkId()	{ return originalNetworkId; }
		unsigned char getServiceType()		{ return serviceType; }
		unsigned char getDiSEqC()		{ return DiSEqC; }
		t_channel_id getChannelID()		{ return (originalNetworkId << 16) | serviceId; }
		unsigned int getTsidOnid()		{ return (transportStreamId << 16) | originalNetworkId; }

		/* get methods - read and write variables */
		std::string getName()			{ return name; }
		unsigned char getAudioChannelCount()	{ return audioChannels.size(); }
		unsigned short getPcrPid()		{ return pcrPid; }
		unsigned short getPmtPid()		{ return pmtPid; }
		unsigned short getTeletextPid()		{ return teletextPid; }
		unsigned short getVideoPid()		{ return videoPid; }
		bool getPidsFlag()			{ return pidsFlag; }
		CCaPmt * getCaPmt()			{ return caPmt; }

		CZapitAudioChannel * getAudioChannel (unsigned char index = 0xFF);
		unsigned short getAudioPid (unsigned char index = 0xFF);
		unsigned char  getAudioChannelIndex()	{ return currentAudioChannel; }

		int addAudioChannel(unsigned short pid, bool isAc3, std::string description, unsigned char componentTag);

		/* set methods */
		void setName(std::string pName)				{ name = pName; }
		void setAudioChannel(unsigned char pAudioChannel)	{ if (pAudioChannel < audioChannels.size()) currentAudioChannel = pAudioChannel; }
		void setPcrPid(unsigned short pPcrPid)			{ pcrPid = pPcrPid; }
		void setPmtPid(unsigned short pPmtPid)			{ pmtPid = pPmtPid; }
		void setTeletextPid(unsigned short pTeletextPid)	{ teletextPid = pTeletextPid; }
		void setVideoPid(unsigned short pVideoPid)		{ videoPid = pVideoPid; }
		void setPidsFlag()					{ pidsFlag = true; }
		void setCaPmt(CCaPmt * pCaPmt)				{ caPmt = pCaPmt; }

		/* cleanup methods */
		void resetPids();
};

#endif /* __channel_h__ */
