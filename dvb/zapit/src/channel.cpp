/*
 * $Id: channel.cpp,v 1.6 2002/09/09 08:51:32 thegoodguy Exp $
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

#include "channel.h"

CZapitChannel::CZapitChannel (std::string p_name, unsigned short p_sid, unsigned short p_tsid, unsigned short p_onid, unsigned char p_service_type, unsigned short p_chan_nr, unsigned char p_DiSEqC)
{
	iscopy = false;
	name = p_name;
	serviceId = p_sid;
	transportStreamId = p_tsid;
	originalNetworkId = p_onid;
	serviceType = p_service_type;
	channelNumber = p_chan_nr;
	DiSEqC = p_DiSEqC;

	caPmt = NULL;
	resetPids();
}

CZapitChannel::~CZapitChannel ()
{
	if (iscopy) return;

	resetPids();

	if (caPmt)
	{
		delete caPmt;
	}
}

CZapitAudioChannel * CZapitChannel::getAudioChannel (unsigned char index)
{
	CZapitAudioChannel* retval = NULL;

	if ((index == 0xFF) && (currentAudioChannel < getAudioChannelCount()))
	{
		retval = audioChannels[currentAudioChannel];
	}
	else if (index < getAudioChannelCount())
	{
		retval = audioChannels[index];
	}

	return retval;
}

unsigned short CZapitChannel::getAudioPid (unsigned char index)
{
	unsigned short retval = 0;

	if ((index == 0xFF) && (currentAudioChannel < getAudioChannelCount()))
	{
		retval = audioChannels[currentAudioChannel]->pid;
	}
	else if (index < getAudioChannelCount())
	{
		retval = audioChannels[index]->pid;
	}

	return retval;
}

int CZapitChannel::addAudioChannel (unsigned short pid, bool isAc3, std::string description, unsigned char componentTag)
{
	std::vector <CZapitAudioChannel *>::iterator aI;

	for (aI = audioChannels.begin(); aI != audioChannels.end(); aI++)
	{
		if ((* aI)->pid == pid)
		{
			return -1;
		}
	}

	CZapitAudioChannel * tmp = new CZapitAudioChannel();
	tmp->pid = pid;
	tmp->isAc3 = isAc3;
	tmp->description = description;
	tmp->componentTag = componentTag;
	audioChannels.insert(audioChannels.end(), tmp);

	return 0;
}

void CZapitChannel::resetPids()
{
	std::vector<CZapitAudioChannel *>::iterator aI;

	for (aI = audioChannels.begin(); aI != audioChannels.end(); aI++)
	{
		delete * aI;
	}

	audioChannels.clear();
	currentAudioChannel = 0;

	pcrPid = 0;
	pmtPid = 0;
	teletextPid = 0;
	videoPid = 0;

	pidsFlag = false;
}

