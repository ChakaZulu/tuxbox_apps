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
$Log: channels.h,v $
Revision 1.5  2002/03/03 22:57:59  TheDOC
lcars 0.20

Revision 1.3  2001/12/17 01:00:33  tux
scan.cpp fix

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.4  2001/12/12 15:23:55  TheDOC
Segfault after Scan-Bug fixed

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef CHANNELS_H
#define CHANNELS_H

#include <string>
#include <vector>
#include <map>

#include "tuner.h"
#include "pat.h"
#include "pmt.h"
#include "settings.h"
#include "zap.h"
#include "osd.h"
#include "eit.h"
#include "cam.h"
#include "hardware.h"

enum
{
	CHANNEL, LINKAGE, NVOD
};

struct channel
{
	int channelnumber; // equals the vector-number... don't know, if i need that, yet ;)
	int TS; // Transport-Stream
	int ONID;
	int SID;
	int PMT;
	int VPID;
	int APID[4];
	bool DD[4];
	int PCR;
	int CAID[5];
	int ECM[5];
	int TXT;
	int EIT;
	int type; // 1->video, 2->radio, 4->NVOD-reference
	int NVOD_count;
	int NVOD_TS[10]; // Nur bei type = 4 - TSID des NVOD-services
	int NVOD_SID[10]; // Nur bei type = 4 - SID des NVOD-services
	char serviceName[100];
	char providerName[100];
	int number_perspectives;
	linkage perspective[20];
};

struct dvbchannel
{
	unsigned char init[4];
	unsigned short SID;
	unsigned short PMT;
	unsigned short FREQU;
	unsigned short SYMBOL;
	unsigned char FEC;
	unsigned char unknown;
	unsigned char POLARIZATION; // 0=V, 1=H
	unsigned char DISEQC;
	unsigned short VPID;
	unsigned short APID;
	unsigned short PCR;
	unsigned short AC3;
	unsigned short ECM;
	unsigned char flags;
	unsigned char type;
	unsigned short TXT;
	unsigned short TS;
	unsigned char serviceName[24];
	unsigned char AutoPIDPMT;
	unsigned char providerIndex;
	unsigned char parental;
	unsigned char countrycode;
	unsigned char linkage;
	unsigned char favourite;
	unsigned short ONID;
};

struct transportstream
{
	int TS;
	int ONID;
	long FREQU;
	int SYMBOL;
	int POLARIZATION; // 0->H, 1->V -- Nur Sat
	int FEC;
	int diseqc;
};

class channels
{
	std::vector<struct channel> basic_channellist; // the list of channels
	std::multimap<int, struct transportstream> basic_TSlist; // the list of transportstreams
	std::multimap<int, int> services_list; // services multimap pointing to basic_channellist-entries
	int cur_pos; // position for getChannel/setChannel
	std::multimap<int, struct transportstream>::iterator cur_pos_TS;
	settings *setting;
	pat *pat_obj;
	pmt *pmt_obj;
	eit *eit_obj;
	cam *cam_obj;
	osd *osd_obj;
	zap *zap_obj;
	tuner *tuner_obj;
	hardware *hardware_obj;
	event now, next;
	char audio_description[20];
	int ECM, apid;
	int video_component, component[10], number_components;
	int curr_perspective;
	int current_mode;
	int old_TS;
public:	
	channels(settings *setting, pat *p1, pmt *p2, eit *e, cam *c, hardware *h, osd *o, zap *z, tuner *t);
	channels(settings *setting, pat *p1, pmt *p2);

	void setStuff(eit *e, cam *c, hardware *h, osd *o, zap *z, tuner *t);

	// multiperspective-stuff

	bool currentIsMultiPerspective();
	int currentNumberPerspectives();
	void parsePerspectives();
	void setPerspective(int number);

	// end multiperspective-stuff
	
	void zapCurrentChannel();
	void setCurrentOSDProgramInfo(osd *osd_obj);
	void receiveCurrentEIT();
	void setCurrentOSDProgramEIT(osd *osd_obj);
	void setCurrentOSDEvent(osd *osd_obj);
	void updateCurrentOSDProgramEIT(osd *osd_obj);
	void zapCurrentAudio(int apid);
	void updateCurrentOSDProgramAPIDDescr(osd *osd_obj);

	event getCurrentNow() { return now; }
	event getCurrentNext() { return next; }
	
	void clearChannels() { basic_channellist.clear(); basic_TSlist.clear(); }

	void addChannel(); // Adds a channel at the end and sets current position to new channel
	void addChannel(channel new_channel);
	void addDVBChannel(dvbchannel tmp_channel);

	void setCurrentTS(int TS);
	void setCurrentONID(int ONID);
	void setCurrentSID(int SID);
	void setCurrentPMT(int PMT);
	void setCurrentVPID(int VPID);
	void addCurrentAPID(int APID, int number = -1);
	void addCurrentAPID(int APID, bool DD);
	void deleteCurrentAPIDs();
	void setCurrentPCR(int PCR);
	void setCurrentTXT(int TXT);
	void addCurrentCA(int CAID, int ECM, int number = -1);
	void setCurrentEIT(int EIT);
	void setCurrentType(int type);
	void clearCurrentNVODs() { basic_channellist[cur_pos].NVOD_count = 0; }
	void addCurrentNVOD(int NVOD_TS, int NVOD_SID, int number = -1);
	void setCurrentNVODCount(int count);
	void setCurrentServiceName(std::string serviceName);
	void setCurrentProviderName(std::string serviceName);

	int numberChannels() { return basic_channellist.size(); } // Returns number of Channels
	int numberTransponders() { return basic_TSlist.size(); }
	bool setCurrentChannel(int channelnumber); // false if setChannel failed
	void setCurrentChannelViaSID(int SID);
	int getCurrentChannelNumber() { return cur_pos; } // returns the currently set channelnumber / -1 if not set

	int getChannelNumber(int TS, int ONID, int SID);
	channel getChannelByNumber(int number);
	void updateChannel(int number, channel channel_data);

	dvbchannel getDVBChannel(int number);

	std::string getServiceName(int channelnumber);
	std::string getShortServiceName(int channelnumber);

	int getCurrentTS();
	int getCurrentONID();
	int getCurrentSID();
	int getCurrentPMT();
	int getCurrentVPID();
	int getCurrentAPIDcount();
	int getCurrentAPID(int number = 0);
	bool getCurrentDD(int number = 0);
	int getCurrentPCR();
	int getCurrentCAcount();
	int getCurrentCAID(int number = 0);
	int getCurrentECM(int number = 0);
	int getCurrentEIT();
	int getCurrentTXT();
	int getCurrentType();
	int getType(int number) { return basic_channellist[number].type; };
	int getCurrentNVODcount();
	int getCurrentNVOD_TS(int number);
	int getCurrentNVOD_SID(int number);
	std::string getCurrentServiceName();
	std::string getCurrentProviderName();

	bool addTS(int TS, int ONID, int FREQU, int SYMBOL, int POLARIZATION = -1, int FEC = -1, int diseqc = 1);
	int getFrequency(int TS) { return (*basic_TSlist.find(TS)).second.FREQU; }
	int getSymbolrate(int TS) { return (*basic_TSlist.find(TS)).second.SYMBOL; }
	int getPolarization(int TS) { return (*basic_TSlist.find(TS)).second.POLARIZATION; }
	int getFEC(int TS) { return (*basic_TSlist.find(TS)).second.FEC; }
	int getDiseqc(int TS) { return (*basic_TSlist.find(TS)).second.diseqc; }
	
	int getFrequency(int TS, int ONID);
	int getSymbolrate(int TS, int ONID);
	int getPolarization(int TS, int ONID);
	int getFEC(int TS, int ONID);
	int getDiseqc(int TS, int ONID);
	transportstream getTS(int TS, int ONID);

	int tune(int TS, tuner *tuner);
	int tuneCurrentTS(tuner *tuner);

	void setBeginTS() { cur_pos_TS = basic_TSlist.begin(); }
	bool setNextTS();
	void clearTS() { basic_TSlist.clear(); }
	int getCurrentSelectedTS() { return (*cur_pos_TS).second.TS; }
	int getCurrentSelectedONID() { return (*cur_pos_TS).second.ONID; }
	int getCurrentFrequency() { return (*cur_pos_TS).second.FREQU; }
	int getCurrentSymbolrate() { return (*cur_pos_TS).second.SYMBOL; }
	int getCurrentPolarization() { return (*cur_pos_TS).second.POLARIZATION; }
	int getCurrentFEC() { return (*cur_pos_TS).second.FEC; }
	int getCurrentDiseqc() { return (*cur_pos_TS).second.diseqc; }
	

	void dumpTS();
	void dumpChannels();

	void saveDVBChannels();
	void loadDVBChannels();
};

#endif
