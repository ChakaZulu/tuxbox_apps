/*
 * $Id: channel.cpp,v 1.15 2003/05/07 18:28:10 digi_casi Exp $
 *
 * (C) 2002 by Steffen Hehn <mcclean@berlios.de>
 * (C) 2002, 2003 by Andreas Oberritter <obi@tuxbox.org>
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

#include <zapit/channel.h>

CZapitChannel::CZapitChannel (std::string p_name, t_service_id p_sid, t_transport_stream_id p_tsid, t_original_network_id p_onid, unsigned char p_service_type, unsigned char p_DiSEqC, std::string p_satellite, int32_t p_satellite_position)
{
	name = p_name;
	service_id = p_sid;
	transport_stream_id = p_tsid;
	original_network_id = p_onid;
	serviceType = p_service_type;
	DiSEqC = p_DiSEqC;
	satellite = p_satellite;
	satellitePosition = p_satellite_position;

	caPmt = NULL;
	resetPids();
}

CZapitChannel::~CZapitChannel(void)
{
	resetPids();

	if (caPmt)
		delete caPmt;
}

CZapitAudioChannel *CZapitChannel::getAudioChannel(unsigned char index)
{
	CZapitAudioChannel *retval = NULL;

	if ((index == 0xFF) && (currentAudioChannel < getAudioChannelCount()))
		retval = audioChannels[currentAudioChannel];
	else if (index < getAudioChannelCount())
		retval = audioChannels[index];

	return retval;
}

unsigned short CZapitChannel::getAudioPid(unsigned char index)
{
	unsigned short retval = 0;

	if ((index == 0xFF) && (currentAudioChannel < getAudioChannelCount()))
		retval = audioChannels[currentAudioChannel]->pid;
	else if (index < getAudioChannelCount())
		retval = audioChannels[index]->pid;

	return retval;
}

int CZapitChannel::addAudioChannel(unsigned short pid, bool isAc3, std::string description, unsigned char componentTag)
{
	std::vector <CZapitAudioChannel *>::iterator aI;

	for (aI = audioChannels.begin(); aI != audioChannels.end(); aI++)
		if ((* aI)->pid == pid)
			return -1;

	CZapitAudioChannel *tmp = new CZapitAudioChannel();
	tmp->pid = pid;
	tmp->isAc3 = isAc3;
	tmp->description = description;
	tmp->componentTag = componentTag;
	audioChannels.insert(audioChannels.end(), tmp);
	return 0;
}

void CZapitChannel::resetPids(void)
{
	std::vector<CZapitAudioChannel *>::iterator aI;

	for (aI = audioChannels.begin(); aI != audioChannels.end(); aI++)
		delete *aI;

	audioChannels.clear();
	currentAudioChannel = 0;

	pcrPid = 0;
	pmtPid = 0;
	teletextPid = 0;
	videoPid = 0;

	pidsFlag = false;
}

