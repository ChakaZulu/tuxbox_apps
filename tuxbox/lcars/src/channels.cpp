/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: channels.cpp,v $
Revision 1.4  2001/12/12 15:23:55  TheDOC
Segfault after Scan-Bug fixed

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "channels.h"
#include "zap.h"
#include "help.h"
#include "tuner.h"
#include "settings.h"
#include "zap.h"
#include "tuner.h"
#include "pmt.h"
#include "pat.h"
#include "eit.h"

#include "config.h"

channels::channels(settings &set, pat &p1, pmt &p2) : setting(set), pat_obj(p1), pmt_obj(p2)
{
	cur_pos = -1;
}


channels::channels(settings &set, pat &p1, pmt &p2, eit *e, cam *c, hardware *h) : setting(set), pat_obj(p1), pmt_obj(p2)
{
	cur_pos = -1;
	eit_obj = e;
	cam_obj = c;
	hardware_obj = h;
}

void channels::setStuff(eit *e, cam *c, hardware *h)
{
	eit_obj = e;
	cam_obj = c;
	hardware_obj = h;
}

void channels::zapCurrentChannel(zap *zap_obj, tuner *tuner_obj)
{
	(*zap_obj).zap_allstop();
	tune(getCurrentTS(), tuner_obj);

	pat_obj.readPAT();

	ECM = 0;
		
	apid = 0;

	int tmp_pmt = pat_obj.getPMT(getCurrentSID());
		
	if (tmp_pmt != 0)
	{
		setCurrentPMT(pat_obj.getPMT(getCurrentSID()));
		pmt_data pmt_entry = (pmt_obj.readPMT(getCurrentPMT()));
		deleteCurrentAPIDs();
		number_components = 0;
		video_component = 0;
		for (int i = 0; i < pmt_entry.pid_counter; i++)
		{
			if (pmt_entry.type[i] == 0x02)
			{
				setCurrentVPID(pmt_entry.PID[i]);
				video_component = pmt_entry.component[i];
			}
			else if (pmt_entry.type[i] == 0x04 || pmt_entry.type[i] == 0x03)
			{
				addCurrentAPID(pmt_entry.PID[i]);
				component[number_components++] = pmt_entry.component[i];
			}
			else if (pmt_entry.type[i] == 0x06 && pmt_entry.subtype[i] == 1)
			{
				setCurrentTXT(pmt_entry.PID[i]);
			}	
			else if (pmt_entry.type[i] == 0x06 && pmt_entry.subtype[i] != 1)
			{
				addCurrentAPID(pmt_entry.PID[i], (bool) true);
				component[number_components++] = pmt_entry.component[i];
			}
			
			printf("type: %d - PID: %04x\n", pmt_entry.type[i], pmt_entry.PID[i]);
		}
	
		for (int i = 0; i < pmt_entry.ecm_counter; i++)
		{
			if (setting.getCAID() == pmt_entry.CAID[i])
				ECM = pmt_entry.ECM[i];
			printf("CAID: %04x - ECM: %04x\n", pmt_entry.CAID[i], pmt_entry.ECM[i]);
		}

		hardware_obj->useDD(getCurrentDD(0));
		if (getCurrentAPIDcount() == 1)
			(*zap_obj).zap_to(getCurrentVPID(), getCurrentAPID(0), ECM, getCurrentSID(), getCurrentONID(), getCurrentTS());
		else
			(*zap_obj).zap_to(getCurrentVPID(), getCurrentAPID(0), ECM, getCurrentSID(), getCurrentONID(), getCurrentTS(), getCurrentAPID(1));
		
		

		if (getCurrentAPIDcount() > 1)
			(*eit_obj).setAudioComponent(component[apid]);
		else
			(*eit_obj).setAudioComponent(-1);
	}
}

void channels::setCurrentOSDProgramInfo(osd *osd_obj)
{
	(*osd_obj).createProgramInfo();
	char text[100];
	sprintf(text, "COMMAND proginfo set_service_name %s", getCurrentServiceName().c_str());
	(*osd_obj).addCommand(text);
	sprintf(text, "COMMAND proginfo set_service_number %d", cur_pos);
	(*osd_obj).addCommand(text);
}

void channels::receiveCurrentEIT()
{
	memset (&now, 0, sizeof (struct event));
	memset (&next, 0, sizeof (struct event));
			
	char cmd_text[100];
	sprintf(cmd_text, "RECEIVE %d", getCurrentSID());
	(*eit_obj).addCommand(cmd_text);
}

void channels::setCurrentOSDProgramEIT(osd *osd_obj)
{
	if (getCurrentAPIDcount() > 1)
		(*osd_obj).setLanguage(audio_description);
	if (now.par_rating != 0)
		(*osd_obj).setParentalRating(now.par_rating);

	(*osd_obj).setNowDescription(now.event_name);
	(*osd_obj).setNextDescription(next.event_name);
	(*osd_obj).setNowTime(now.starttime);
	(*osd_obj).setNextTime(next.starttime);
}

void channels::updateCurrentOSDProgramEIT(osd *osd_obj)
{
	if (next.starttime <= time(0))
	{
		getCurrentEIT();
		setCurrentOSDProgramEIT(osd_obj);
	}
}

void channels::setCurrentOSDEvent(osd *osd_obj)
{
	(*osd_obj).setEPGEventName(now.event_name);
	(*osd_obj).setEPGEventShortText(now.event_short_text);
	(*osd_obj).setEPGEventExtendedText(now.event_extended_text);
	(*osd_obj).setEPGProgramName(getCurrentServiceName());
	(*osd_obj).setEPGstarttime(now.starttime);
	(*osd_obj).setEPGduration(now.duration);
}

void channels::zapCurrentAudio(int pid, zap *zap_obj)
{
	apid = pid;
	//hardware_obj->useDD(true);
	
	if (basic_channellist[cur_pos].DD[pid])
		printf("Dolby Digital ON ......................................\n");
	else
		printf("Dolby Digital OFF ......................................\n");
	hardware_obj->useDD(getCurrentDD(apid));
	(*zap_obj).zap_audio(getCurrentVPID(), getCurrentAPID(apid), ECM, getCurrentSID(), getCurrentONID());
	
	(*eit_obj).setAudioComponent(component[apid]);


}

void channels::updateCurrentOSDProgramAPIDDescr(osd *osd_obj)
{
	if (getCurrentAPIDcount() > 1)
		(*osd_obj).setLanguage(audio_description);
}

dvbchannel channels::getDVBChannel(int number)
{
	channel tmp_chan = basic_channellist[number];
	dvbchannel chan;

	memset (&chan, 0, sizeof(struct dvbchannel));

	std::multimap<int, struct transportstream>::iterator ts = basic_TSlist.find(tmp_chan.TS);
		
	chan.init[0] = 'D';
	chan.init[1] = 'V';
	chan.init[2] = 'S';
	chan.init[3] = 'O';
	chan.SID = tmp_chan.SID;
	chan.PMT = tmp_chan.PMT;
	chan.FREQU = (*ts).second.FREQU;
	chan.SYMBOL = (*ts).second.SYMBOL;
	chan.POLARIZATION = (*ts).second.POLARIZATION;
	chan.VPID = tmp_chan.VPID;
	chan.APID = tmp_chan.APID[0];
	chan.PCR = tmp_chan.PCR;
	chan.ECM = tmp_chan.ECM[0];
	chan.type = tmp_chan.type;
	chan.TS = tmp_chan.TS;
	for (int i = 0; i < 24; i++)
		chan.serviceName[i] = tmp_chan.serviceName[i];
	chan.AutoPIDPMT = 3;
	chan.ONID = tmp_chan.ONID;

	return chan;
	
}

void channels::addChannel()
{
	struct channel new_channel;
	printf("New Channel number %d\n", numberChannels());
	memset (&new_channel, 0, sizeof(struct channel));

	new_channel.channelnumber = numberChannels();
	basic_channellist.insert(basic_channellist.end(), new_channel);
	cur_pos = numberChannels() - 1;
}

void channels::addChannel(channel new_channel)
{
	new_channel.channelnumber = numberChannels();
	basic_channellist.insert(basic_channellist.end(), new_channel);
	cur_pos = numberChannels() - 1;
}

void channels::addDVBChannel(dvbchannel chan)
{
	struct channel tmp_chan;
	memset (&tmp_chan, 0, sizeof(struct channel));

	tmp_chan.SID = chan.SID;
	tmp_chan.PMT = chan.PMT;
	tmp_chan.VPID = chan.VPID;
	tmp_chan.APID[0] = chan.APID;
	tmp_chan.PCR = chan.PCR;
	tmp_chan.ECM[0] = chan.ECM;
	tmp_chan.CAID[0] = setting.getCAID();
	tmp_chan.type = chan.type;
	tmp_chan.TS = chan.TS;
	for (int i = 0; i < 24; i++)
		tmp_chan.serviceName[i] = chan.serviceName[i];
	tmp_chan.ONID = chan.ONID;
	
	basic_channellist.insert(basic_channellist.end(), tmp_chan);
	services_list.insert(std::pair<int, int>(chan.SID, basic_channellist.size() - 1));
	
	struct transportstream tmp_TS;
	memset (&tmp_TS, 0, sizeof(struct transportstream));
	
	tmp_TS.TS = chan.TS;
	tmp_TS.ONID = chan.ONID;
	tmp_TS.FREQU = chan.FREQU;
	tmp_TS.SYMBOL = chan.SYMBOL;
	tmp_TS.POLARIZATION = chan.POLARIZATION & 0x1;
	tmp_TS.FEC = chan.FEC;

	if (basic_TSlist.count(chan.TS) == 0)
	{
		basic_TSlist.insert(std::pair<int, struct transportstream>(tmp_TS.TS, tmp_TS));
	}


}

channel channels::getChannelByNumber(int number)
{
	return basic_channellist[number];
}

int channels::getChannelNumber(int TS, int ONID, int SID)
{
	printf("Wanted: TS: %x - ONID: %x - SID: %x\n", TS, ONID, SID);
	
	
	printf ("Found: %d\n",  services_list.count(SID));

	std::pair<std::multimap<int, int>::iterator, std::multimap<int, int>::iterator> ip = services_list.equal_range(SID);
	for (std::multimap<int, int>::iterator it = ip.first; it != ip.second; ++it)
	{
		int pos = (*it).second;
		setCurrentChannel(pos);
		printf("Checking Position %d - TS: %x - ONID: %x\n", pos, getCurrentTS(), getCurrentONID());
		if (getCurrentTS() == TS && getCurrentONID() == ONID)
			return pos;
		
	}
	
	return -1;
}

bool channels::setCurrentChannel(int channelnumber)
{
	printf("SetCurrentChannel to %d\n", channelnumber);
	if ((channelnumber > numberChannels() - 1) || (channelnumber < 0))
		return false;
	cur_pos = channelnumber;
	cur_pos_TS = basic_TSlist.find(basic_channellist[channelnumber].TS);
	return true;
}

void channels::setCurrentChannelViaSID(int SID)
{
	cur_pos = (*services_list.find(SID)).second;
}

void channels::setCurrentTS(int TS)
{
	printf("setCurrentTS to %d\n", TS);
	basic_channellist[cur_pos].TS = TS;
}

void channels::setCurrentONID(int ONID)
{
	printf("setCurrentONID to %d\n", ONID);
	basic_channellist[cur_pos].ONID = ONID;
}

void channels::setCurrentSID(int SID)
{
	printf("setCurrentSID to %d\n", SID);
	basic_channellist[cur_pos].SID = SID;
	services_list.insert(std::pair<int, int>(SID, cur_pos));
}

void channels::setCurrentPMT(int PMT)
{
	printf("setCurrentPMT to %d\n", PMT);
	basic_channellist[cur_pos].PMT = PMT;
}

void channels::setCurrentVPID(int VPID)
{
	printf("setCurrentVPID to %d \n", VPID);
	basic_channellist[cur_pos].VPID = VPID;
}

void channels::addCurrentAPID(int APID, int number = -1)
{
	if (number == -1)
		number = getCurrentAPIDcount();
	printf("addCurrentAPID %d to %04x\n", number, APID);
	basic_channellist[cur_pos].APID[number] = APID;
	basic_channellist[cur_pos].DD[number] = false;
}

void channels::addCurrentAPID(int APID, bool DD)
{
	addCurrentAPID(APID);
	basic_channellist[cur_pos].DD[getCurrentAPIDcount() - 1] = DD;
}

bool channels::getCurrentDD(int number = 0)
{
	return basic_channellist[cur_pos].DD[number];
}

void channels::setCurrentPCR(int PCR)
{
	basic_channellist[cur_pos].PCR = PCR;
}

void channels::setCurrentTXT(int TXT)
{
	basic_channellist[cur_pos].TXT = TXT;
}

void channels::addCurrentCA(int CAID, int ECM, int number = -1)
{
	printf("addCurrentCA to %d - %d\n", CAID, ECM);
	if (number == -1)
		number = getCurrentCAcount();
	basic_channellist[cur_pos].CAID[number] = CAID;
	basic_channellist[cur_pos].ECM[number] = ECM;
}

void channels::setCurrentEIT(int EIT)
{
	basic_channellist[cur_pos].EIT = EIT;
}

void channels::setCurrentType(int type)
{
	printf("setCurrentType to %d \n", type);
	basic_channellist[cur_pos].type = type;
}

void channels::addCurrentNVOD(int NVOD_TS, int NVOD_SID, int number = -1)
{
	printf("addCurrentNVOD to %d - %d\n", NVOD_TS, NVOD_SID);
	if (number == -1)
		number = basic_channellist[cur_pos].NVOD_count;
	basic_channellist[cur_pos].NVOD_TS[number] = NVOD_TS;
	basic_channellist[cur_pos].NVOD_SID[number] = NVOD_SID;
}

void channels::setCurrentNVODCount(int count)
{
	printf("setCurrentNVODCount to %d\n", count);
	basic_channellist[cur_pos].NVOD_count = count;	
}


void channels::setCurrentServiceName(std::string serviceName)
{
	printf("setCurrentServiceName\n");
	strcpy(basic_channellist[cur_pos].serviceName, serviceName.c_str());
}

void channels::setCurrentProviderName(std::string providerName)
{
	printf("setCurrentProviderName\n");
	strcpy(basic_channellist[cur_pos].providerName, providerName.c_str());
}

std::string channels::getServiceName(int channelnumber)
{
	int position = 0;
	char name[100];
	for (int i = 0; i < (int)strlen(basic_channellist[channelnumber].serviceName); i++)
	{
		if (basic_channellist[channelnumber].serviceName[i] != 135 && basic_channellist[channelnumber].serviceName[i] != 134)
			name[position++] = basic_channellist[channelnumber].serviceName[i];
	}
	name[position] = '\0';
	return name;
}

std::string channels::getShortServiceName(int channelnumber)
{
	int position = 0;
	char name[100];
	bool started = false;
	for (int i = 0; i < (int)strlen(basic_channellist[channelnumber].serviceName); i++)
	{
		if (started == true)
		{
			if (basic_channellist[channelnumber].serviceName[i] != 135)
				name[position++] = basic_channellist[channelnumber].serviceName[i];
			else
				started = false;
		}
		else if (basic_channellist[channelnumber].serviceName[i] == 134)
			started = true;
	}
	name[position] = '\0';

	if (position == 0)
		strcpy(name, basic_channellist[channelnumber].serviceName);

	// remove ugly characters:
	char clean_name[100];
	position = 0;
	for (int i = 0; i <= (int)strlen(name); i++)
	{
		if (name[i] != 5 && name[i] != 135 && name[i] != 134)
		{
			clean_name[position++] = name[i];
		}

	}

	return clean_name;
}

int channels::getCurrentTS()
{
	return basic_channellist[cur_pos].TS;
}

int channels::getCurrentONID()
{
	return basic_channellist[cur_pos].ONID;
}

int channels::getCurrentSID()
{
	return basic_channellist[cur_pos].SID;
}

int channels::getCurrentPMT()
{
	return basic_channellist[cur_pos].PMT;
}

int channels::getCurrentVPID()
{
	return basic_channellist[cur_pos].VPID;
}

int channels::getCurrentAPIDcount()
{
	int count = 0;
	while(basic_channellist[cur_pos].APID[count++] != 0)
		if (count > 3)
			break;
	count--;
	return count;
}

void channels::deleteCurrentAPIDs()
{
	for (int i = 0; i < 2; i++)
	{
		basic_channellist[cur_pos].APID[i] = 0;
	}
}

int channels::getCurrentAPID(int number = 0)
{
	return basic_channellist[cur_pos].APID[number];
}

int channels::getCurrentPCR()
{
	return basic_channellist[cur_pos].PCR;
}

int channels::getCurrentTXT()
{
	return basic_channellist[cur_pos].TXT;
}

int channels::getCurrentCAcount()
{
	int count = 0;
	while(basic_channellist[cur_pos].CAID[count++] != 0);
	return count;
}

int channels::getCurrentCAID(int number = 0)
{
	return basic_channellist[cur_pos].CAID[number];
}

int channels::getCurrentECM(int number = 0)
{
	return basic_channellist[cur_pos].ECM[number];
}

int channels::getCurrentEIT()
{
	return basic_channellist[cur_pos].EIT;
}

int channels::getCurrentType()
{
	return basic_channellist[cur_pos].type;
}

int channels::getCurrentNVODcount()
{
	return basic_channellist[cur_pos].NVOD_count;
}

int channels::getCurrentNVOD_TS(int number)
{
 	return basic_channellist[cur_pos].NVOD_TS[number];
}

int channels::getCurrentNVOD_SID(int number)
{
	return basic_channellist[cur_pos].NVOD_SID[number];
}

std::string channels::getCurrentServiceName()
{
	int position = 0;
	char name[100];
	for (int i = 0; i < (int)strlen(basic_channellist[cur_pos].serviceName); i++)
	{
		if (basic_channellist[cur_pos].serviceName[i] != 135 && basic_channellist[cur_pos].serviceName[i] != 134)
			name[position++] = basic_channellist[cur_pos].serviceName[i];
	}
	name[position] = '\0';
	return name;
}

std::string channels::getCurrentProviderName()
{
	std::string pname(basic_channellist[cur_pos].providerName);
	return pname;
}

bool channels::addTS(int TS, int ONID, int FREQU, int SYMBOL, int POLARIZATION = -1, int FEC = -1, int diseqc = 1)
{
	struct transportstream new_transportstream;

	memset (&new_transportstream, 0, sizeof(struct transportstream));
	if ((*basic_TSlist.find(TS)).second.FREQU == FREQU)
		return false;

	new_transportstream.TS = TS;
	new_transportstream.ONID = ONID;
	new_transportstream.FREQU = FREQU;
	new_transportstream.SYMBOL = SYMBOL;
	new_transportstream.POLARIZATION = POLARIZATION;
	new_transportstream.FEC = FEC;
	new_transportstream.diseqc = diseqc;

	basic_TSlist.insert(std::pair<int, struct transportstream>(TS, new_transportstream));
  
	return true;
}

int channels::getFrequency(int TS, int ONID)
{
	std::pair<std::multimap<int, struct transportstream>::iterator, std::multimap<int, struct transportstream>::iterator> ip = basic_TSlist.equal_range(TS);
	for (std::multimap<int, struct transportstream>::iterator it = ip.first; it != ip.second; ++it)
	{
		if ((*it).second.ONID == ONID)
			return (*it).second.FREQU;
	}
	
	return -1;
}

int channels::getSymbolrate(int TS, int ONID)
{
	std::pair<std::multimap<int, struct transportstream>::iterator, std::multimap<int, struct transportstream>::iterator> ip = basic_TSlist.equal_range(TS);
	for (std::multimap<int, struct transportstream>::iterator it = ip.first; it != ip.second; ++it)
	{
		if ((*it).second.ONID == ONID)
			return (*it).second.SYMBOL;
	}
	
	return -1;
}

int channels::getPolarization(int TS, int ONID)
{
	std::pair<std::multimap<int, struct transportstream>::iterator, std::multimap<int, struct transportstream>::iterator> ip = basic_TSlist.equal_range(TS);
	for (std::multimap<int, struct transportstream>::iterator it = ip.first; it != ip.second; ++it)
	{
		if ((*it).second.ONID == ONID)
			return (*it).second.POLARIZATION;
	}
	
	return -1;
}

int channels::getFEC(int TS, int ONID)
{
	std::pair<std::multimap<int, struct transportstream>::iterator, std::multimap<int, struct transportstream>::iterator> ip = basic_TSlist.equal_range(TS);
	for (std::multimap<int, struct transportstream>::iterator it = ip.first; it != ip.second; ++it)
	{
		if ((*it).second.ONID == ONID)
			return (*it).second.FEC;
	}
	
	return -1;
}

int channels::getDiseqc(int TS, int ONID)
{
	std::pair<std::multimap<int, struct transportstream>::iterator, std::multimap<int, struct transportstream>::iterator> ip = basic_TSlist.equal_range(TS);
	for (std::multimap<int, struct transportstream>::iterator it = ip.first; it != ip.second; ++it)
	{
		if ((*it).second.ONID == ONID)
			return (*it).second.diseqc;
	}
	
	return -1;
}

transportstream channels::getTS(int TS, int ONID)
{
	std::pair<std::multimap<int, struct transportstream>::iterator, std::multimap<int, struct transportstream>::iterator> ip = basic_TSlist.equal_range(TS);
	for (std::multimap<int, struct transportstream>::iterator it = ip.first; it != ip.second; ++it)
	{
		if ((*it).second.ONID == ONID)
			return (*it).second;
	}
	
	transportstream tmp_ts;

	tmp_ts.TS = -1;
	return tmp_ts;
}


void channels::dumpTS()
{
	for (std::multimap<int, struct transportstream>::iterator it = basic_TSlist.begin(); it != basic_TSlist.end(); ++it)
	{
		printf("TS: %d - FREQU: %ld - SYMBOL: %d - POL: %d - FEC: %d\n", (*it).second.TS, (*it).second.FREQU, (*it).second.SYMBOL, (*it).second.POLARIZATION, (*it).second.FEC);
	}
}


void channels::dumpChannels()
{
	for (std::vector<struct channel>::iterator it = basic_channellist.begin(); it != basic_channellist.end(); ++it)
	{
		printf("#%d TS: %04x - SID: %04x - Name: %s\n", (*it).channelnumber, (*it).TS, (*it).SID, (*it).serviceName);
	}
	printf("Das sind %d Kanäle\n", basic_channellist.size());
}

int channels::tune(int TS, tuner *tuner)
{
	std::multimap<int, struct transportstream>::iterator it = basic_TSlist.find(TS);

	(*tuner).tune((*it).second.FREQU, (*it).second.SYMBOL, (*it).second.POLARIZATION, (*it).second.FEC, (*cur_pos_TS).second.diseqc);
	return true;
}

bool channels::setNextTS()
{
	if (++cur_pos_TS != basic_TSlist.end())
	{
		return true;
	}
	return false;
}

int channels::tuneCurrentTS(tuner *tuner)
{
	(*tuner).tune((*cur_pos_TS).second.FREQU, (*cur_pos_TS).second.SYMBOL, (*cur_pos_TS).second.POLARIZATION, (*cur_pos_TS).second.FEC, (*cur_pos_TS).second.diseqc);

	return true;
}

void channels::saveChannels()
{
	int fd;
	printf("Writing\n");
	fd = open("/var/channels.dat", O_WRONLY|O_TRUNC|O_CREAT, 0666);
	for (std::vector<struct channel>::iterator it = basic_channellist.begin(); it != basic_channellist.end(); ++it)
	{
		channel test;
		test = (*it);
		write(fd, &test, sizeof(channel));
	}
	close(fd);
	fd = open("/var/transponders.dat", O_WRONLY|O_TRUNC|O_CREAT, 0666);
	for (std::multimap<int, struct transportstream>::iterator it = basic_TSlist.begin(); it != basic_TSlist.end(); ++it)
	{
		printf("%d\n", (*it).second.TS);
		write(fd, &((*it).second), sizeof(transportstream));
	}
	close(fd);
	printf("Written\n");
}

void channels::saveDVBChannels()
{
	FILE *fp;

	printf("Save File\n");
	fp = fopen(DATADIR "/lcars/lcars.dvb", "wb");
	for (std::vector<struct channel>::iterator it = basic_channellist.begin(); it != basic_channellist.end(); ++it)
	{
		dvbchannel chan;

		memset (&chan, 0, sizeof(struct dvbchannel));

		std::multimap<int, struct transportstream>::iterator ts = basic_TSlist.find((*it).TS);
		
		printf("SID: %x\n", (*it).SID);
		chan.init[0] = 'D';
		chan.init[1] = 'V';
		chan.init[2] = 'S';
		chan.init[3] = 'O';
		chan.SID = (*it).SID;
		chan.PMT = (*it).PMT;
		chan.TXT = (*it).TXT;
		chan.FREQU = (*ts).second.FREQU;
		chan.SYMBOL = (*ts).second.SYMBOL;
		chan.POLARIZATION = (*ts).second.POLARIZATION;
		chan.DISEQC = (*ts).second.diseqc;
		

		chan.FEC = (*ts).second.FEC;
		chan.VPID = (*it).VPID;
		chan.APID = (*it).APID[0];
		chan.PCR = (*it).PCR;
		for (int i = 0; i < 5; i++)
		{
			if (setting.getCAID() == (*it).CAID[i])
				chan.ECM = (*it).ECM[i];
		}
		chan.type = (*it).type;
		chan.TS = (*it).TS;
		for (int i = 0; i < 24; i++)
			chan.serviceName[i] = (*it).serviceName[i];

		chan.AutoPIDPMT = 3;
		chan.ONID = (*it).ONID;
		
		fwrite(&chan, sizeof(dvbchannel), 1, fp);
		printf("Size: %d\n", sizeof(dvbchannel));
	}
	fclose(fp);
}

void channels::loadDVBChannels()
{
	int fd;
	
	printf("Loading Channels\n");
	if ((fd = open(DATADIR "/lcars/lcars.dvb", O_RDONLY)) < 0)
	{
		printf("No channels available!\n");	
		return;
	}
	dvbchannel chan;
	int count = 0;
	while(read(fd, &chan, sizeof(dvbchannel)) > 0)
	{
		struct channel tmp_chan;
		memset (&tmp_chan, 0, sizeof(struct channel));

		tmp_chan.SID = chan.SID;
		tmp_chan.PMT = chan.PMT;
		tmp_chan.VPID = chan.VPID;
		tmp_chan.APID[0] = chan.APID;
		tmp_chan.PCR = chan.PCR;
		tmp_chan.ECM[0] = chan.ECM;
		tmp_chan.CAID[0] = setting.getCAID();
		tmp_chan.type = chan.type;
		tmp_chan.TS = chan.TS;
		tmp_chan.TXT = chan.TXT;
		for (int i = 0; i < 24; i++)
			tmp_chan.serviceName[i] = chan.serviceName[i];
		tmp_chan.ONID = chan.ONID;
				
		basic_channellist.insert(basic_channellist.end(), tmp_chan);
		services_list.insert(std::pair<int, int>(chan.SID, count++));

		struct transportstream tmp_TS;
		memset (&tmp_TS, 0, sizeof(struct transportstream));

		tmp_TS.TS = chan.TS;
		tmp_TS.ONID = chan.ONID;
		tmp_TS.FREQU = chan.FREQU;
		tmp_TS.SYMBOL = chan.SYMBOL;
		tmp_TS.POLARIZATION = chan.POLARIZATION & 0x1;
		tmp_TS.FEC = chan.FEC;
		tmp_TS.diseqc = chan.DISEQC;

		if (basic_TSlist.count(chan.TS) == 0)
		{
			basic_TSlist.insert(std::pair<int, struct transportstream>(tmp_TS.TS, tmp_TS));
		}
	}
	close(fd);
	printf("Channels loaded\n");
  }

void channels::loadChannels()
{
	int fd;
	fd = open("/var/channels.dat", O_RDONLY);
		
	int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, 0);

	printf("Channels: %d\n", size / sizeof(channel));
	for (int i = 0; i < (int)(size / sizeof(channel)); i++)
	{
		
		channel temp_chan;
		read(fd, &temp_chan, sizeof(channel));
	
		basic_channellist.insert(basic_channellist.end(), temp_chan);

	}
	close(fd);

	fd = open("/var/transponders.dat", O_RDONLY);
	size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, 0);

	printf("Transponders: %d\n", size / sizeof(transportstream));
	for (int i = 0; i < (int)(size / sizeof(transportstream)); i++)
	{
		
		struct transportstream temp_TS;
		read(fd, &temp_TS, sizeof(transportstream));

		basic_TSlist.insert(std::pair<int, struct transportstream>(temp_TS.TS, temp_TS));

	}
	close(fd);


}

