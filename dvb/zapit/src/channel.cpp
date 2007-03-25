/*
 * $Id: channel.cpp,v 1.21 2007/03/25 15:06:04 Arzka Exp $
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
#include <zapit/debug.h>

CZapitChannel::CZapitChannel(const std::string & p_name, t_service_id p_sid, t_transport_stream_id p_tsid, t_original_network_id p_onid, unsigned char p_service_type, unsigned char p_DiSEqC, t_satellite_position p_satellite_position, frequency_kHz_t p_frequency)
{
	name = p_name;
	service_id = p_sid;
	transport_stream_id = p_tsid;
	original_network_id = p_onid;
	serviceType = p_service_type;
	DiSEqC = p_DiSEqC;
	satellitePosition = p_satellite_position;
	frequency = p_frequency;
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

int CZapitChannel::addAudioChannel(const unsigned short pid, const bool isAc3, const std::string & description, const unsigned char componentTag)
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
	audioChannels.push_back(tmp);
	return 0;
}

void CZapitChannel::resetPids(void)
{
	std::vector<CZapitAudioChannel *>::iterator aI;

	for (aI = audioChannels.begin(); aI != audioChannels.end(); aI++)
		delete *aI;

	audioChannels.clear();
	currentAudioChannel = 0;

	std::vector<CZapitAbsSub *>::iterator subI;
	for (subI = channelSubs.begin(); subI != channelSubs.end(); subI++){
	    delete *subI;
	}
	channelSubs.clear();
	currentSub = 0;

	pcrPid = 0;
	pmtPid = 0;
	teletextPid = 0;
	videoPid = 0;
	privatePid = 0;

	pidsFlag = false;
}

void CZapitChannel::addTTXSubtitle(const unsigned int pid, const std::string langCode, const unsigned char magazine_number, const unsigned char page_number, const bool impaired)
{
    CZapitTTXSub* oldSub = 0;
    CZapitTTXSub* tmpSub = 0;
    unsigned char mag_nr = magazine_number ? magazine_number : 8;

    std::vector<CZapitAbsSub*>::iterator subI;
    // Check if it already exists
    for (subI=channelSubs.begin(); subI!=channelSubs.end();subI++){
	if ((*subI)->thisSubType==CZapitAbsSub::TTX){
	    tmpSub=reinterpret_cast<CZapitTTXSub*>(*subI);
            if (tmpSub->ISO639_language_code == langCode) {
	        oldSub = tmpSub;
                if (tmpSub->pId==pid &&
		    tmpSub->teletext_magazine_number==mag_nr &&
		    tmpSub->teletext_page_number==page_number &&
		    tmpSub->hearingImpaired==impaired) {
                    // It is already there, do nothing
		    return;
                }
                // No need to iterate more
                break;
	    }
	}
    }

    DBG("TTXSub: PID=0x%04x, lang=%3.3s, page=%1X%02X", 
        pid, langCode.c_str(), mag_nr, page_number);

    if (oldSub) {
        tmpSub=oldSub;
    } else {
        tmpSub = new CZapitTTXSub();
        channelSubs.push_back(tmpSub);
    }
    tmpSub->pId=pid;
    tmpSub->ISO639_language_code=langCode;
    tmpSub->teletext_magazine_number=mag_nr;
    tmpSub->teletext_page_number=page_number;
    tmpSub->hearingImpaired=impaired;

    setPidsUpdated();  // To notify clients if pids have changed
}

void CZapitChannel::addDVBSubtitle(const unsigned int pid, const std::string langCode, const unsigned char subtitling_type, const unsigned short composition_page_id, const unsigned short ancillary_page_id)
{
    CZapitDVBSub* oldSub = 0;
    CZapitDVBSub* tmpSub = 0;
    std::vector<CZapitAbsSub*>::iterator subI;
    for (subI=channelSubs.begin(); subI!=channelSubs.end();subI++){
	if ((*subI)->thisSubType==CZapitAbsSub::DVB){
	    tmpSub=reinterpret_cast<CZapitDVBSub*>(*subI);
	    if (tmpSub->ISO639_language_code==langCode) {
                oldSub = tmpSub;
                if (tmpSub->pId==pid &&
		    tmpSub->subtitling_type==subtitling_type &&
		    tmpSub->composition_page_id==composition_page_id &&
		    tmpSub->ancillary_page_id==ancillary_page_id) {

                    return;
                 }
		 break;
	    }
	}
    }

    DBG("DVBSub: PID=0x%04x, lang=%3.3s, cpageid=%04x, apageid=%04x",
        pid, langCode.c_str(), composition_page_id, ancillary_page_id);

    if (oldSub) {
        tmpSub = oldSub;
    } else {
        tmpSub = new CZapitDVBSub();
        channelSubs.push_back(tmpSub);
    }

    tmpSub->pId=pid;
    tmpSub->ISO639_language_code=langCode;
    tmpSub->subtitling_type=subtitling_type;
    tmpSub->composition_page_id=composition_page_id;
    tmpSub->ancillary_page_id=ancillary_page_id;

    setPidsUpdated();
}

CZapitAbsSub* CZapitChannel::getChannelSub(int index)
{
    CZapitAbsSub* retval = NULL;

    if ((index < 0) && (currentSub < getSubtitleCount())){
	retval = channelSubs[currentSub];
    } else { 
        if ((index >= 0) && (index < (int)getSubtitleCount())) {
	    retval = channelSubs[index];
	}
    }
    return retval;
}

void CZapitChannel::setChannelSub(int subIdx)
{
    if (subIdx < (int)channelSubs.size()){
	currentSub=subIdx;
    }
}

int CZapitChannel::getChannelSubIndex(void)
{
    return currentSub < getSubtitleCount() ? currentSub : -1;
}
