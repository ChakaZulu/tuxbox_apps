/*
 * $Id: channel.cpp,v 1.2 2002/05/13 18:24:00 McClean Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
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
	unsigned char i;

	name = p_name;
	serviceId = p_sid;
	transportStreamId = p_tsid;
	originalNetworkId = p_onid;
	serviceType = p_service_type;
	channelNumber = p_chan_nr;
	DiSEqC = p_DiSEqC;

	caPmt = NULL;

	for (i = 0; i < MAX_AUDIO_PIDS; i++)
	{
		audioChannels[i] = NULL;
	}

	currentAudioChannel = 0;
	audioChannelCount = 0;

	pcrPid = 0;
	pmtPid = 0;
	teletextPid = 0;
	videoPid = 0;

	pidsFlag = false;
}

CZapitChannel::~CZapitChannel ()
{
	unsigned char i;

	for (i = 0; i < audioChannelCount; i++)
	{
		delete audioChannels[i];
	}

	delete caPmt;
}

CZapitAudioChannel * CZapitChannel::getAudioChannel (unsigned char index)
{
	if (index == 0xFF)
	{
		return audioChannels[currentAudioChannel];
	}

	if (index < audioChannelCount)
	{
		return audioChannels[index];
	}

	return NULL;
}

unsigned short CZapitChannel::getAudioPid (unsigned char index = 0xFF)
{
	if (index == 0xFF)
	{
		return audioChannels[currentAudioChannel]->pid;
	}
	if (index < audioChannelCount)
	{
		return audioChannels[index]->pid;
	}

	return 0;
}

int CZapitChannel::addAudioChannel(unsigned short pid, bool isAc3, std::string description, unsigned char componentTag)
{
	unsigned char i;

	for (i = 0; i < audioChannelCount; i++)
	{
		if (audioChannels[i]->pid == pid)
		{
			return -1;
		}
	}

	audioChannels[audioChannelCount] = new CZapitAudioChannel();
	audioChannels[audioChannelCount]->pid = pid;
	audioChannels[audioChannelCount]->isAc3 = isAc3;
	audioChannels[audioChannelCount]->description = description;
	audioChannels[audioChannelCount]->componentTag = componentTag;
	audioChannelCount++;

	return 0;
}

void CZapitChannel::resetPids()
{
	unsigned char i;

	for (i = 0; i < audioChannelCount; i++)
	{
		delete audioChannels[i];
		audioChannels[i] = NULL;
	}

	currentAudioChannel = 0;
	audioChannelCount = 0;

	pcrPid = 0;
	pmtPid = 0;
	teletextPid = 0;
	videoPid = 0;

	pidsFlag = false;
}

