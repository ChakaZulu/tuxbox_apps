/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MOVIEBROWSER

#include <driver/vcrcontrol.h>

#ifdef MOVIEBROWSER
#include <gui/movieinfo.h>
#endif /* MOVIEBROWSER */

#include <driver/encoding.h>
#include <driver/stream2file.h>

#include <gui/widget/messagebox.h>

#include <irsend/irsend.h>

#include <global.h>
#include <neutrino.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <errno.h>

#include <daemonc/remotecontrol.h>
#include <zapit/client/zapittools.h>

extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_init();
extern "C" void tuxtxt_start(int tpid);
extern "C" int  tuxtxt_stop();
extern "C" void tuxtxt_close();
#endif
extern "C" {
#include <driver/genpsi.h>
}

#define SA struct sockaddr
#define SAI struct sockaddr_in

static CVCRControl vcrControl;

CVCRControl * CVCRControl::getInstance()
{
	return &vcrControl;
}

//-------------------------------------------------------------------------
CVCRControl::CVCRControl()
{
	Device = NULL;
}

//-------------------------------------------------------------------------
CVCRControl::~CVCRControl()
{
	unregisterDevice();
}

//-------------------------------------------------------------------------
void CVCRControl::unregisterDevice()
{
	if (Device)
	{
		delete Device;
		Device = NULL;
	}
}

//-------------------------------------------------------------------------
void CVCRControl::registerDevice(CDevice * const device)
{
	unregisterDevice();
	
	Device = device;
}

//-------------------------------------------------------------------------
bool CVCRControl::Record(const CTimerd::RecordingInfo * const eventinfo)
{
	int mode = g_Zapit->isChannelTVChannel(eventinfo->channel_id) ? NeutrinoMessages::mode_tv : NeutrinoMessages::mode_radio;

#ifdef MOVIEBROWSER
	return Device->Record(eventinfo->channel_id, mode, eventinfo->epgID, eventinfo->epgTitle, eventinfo->apids,eventinfo->epg_starttime); 
#else /* MOVIEBROWSER */
	return Device->Record(eventinfo->channel_id, mode, eventinfo->epgID, eventinfo->epgTitle, eventinfo->apids); 
#endif /* MOVIEBROWSER */
}

//-------------------------------------------------------------------------
void CVCRControl::CDevice::getAPIDs(const unsigned char ap, APIDList & apid_list)
{
//	(strstr(g_RemoteControl->current_PIDs.APIDs[i].desc, "(AC3)") == NULL))
	unsigned char apids=ap;
	if (apids == TIMERD_APIDS_CONF)
		apids = g_settings.recording_audio_pids_default;
	apid_list.clear();
	CZapitClient::responseGetPIDs allpids;
	g_Zapit->getPIDS(allpids);
	// assume smallest apid ist std apid
	if (apids & TIMERD_APIDS_STD)
	{
		uint apid_min=UINT_MAX;
		uint apid_min_idx=0;
		for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
			{
				apid_min = allpids.APIDs[i].pid;
				apid_min_idx = i;
			}
		}
		if (apid_min != UINT_MAX)
		{
			APIDDesc a = {apid_min, apid_min_idx, false};
			apid_list.push_back(a);
		}		
	}
	if (apids & TIMERD_APIDS_ALT)
	{
		uint apid_min=UINT_MAX;
		uint apid_min_idx=0;
		for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
			{
				apid_min = allpids.APIDs[i].pid;
				apid_min_idx = i;
			}
		}
		for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			if (allpids.APIDs[i].pid != apid_min && !allpids.APIDs[i].is_ac3)
			{
				APIDDesc a = {allpids.APIDs[i].pid, i, false};
				apid_list.push_back(a);
			}
		}		
	}
	if (apids & TIMERD_APIDS_AC3)
	{
		for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			if (allpids.APIDs[i].is_ac3)
			{
				APIDDesc a = {allpids.APIDs[i].pid, i, true};
				apid_list.push_back(a);
			}
		}
	}
	// no apid selected use standard
	if (apid_list.empty() && !allpids.APIDs.empty())
	{	
		uint apid_min=UINT_MAX;
		uint apid_min_idx=0;
		for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			if (allpids.APIDs[i].pid < apid_min && !allpids.APIDs[i].is_ac3)
			{
				apid_min = allpids.APIDs[i].pid;
				apid_min_idx = i;
			}
		}
		if (apid_min != UINT_MAX)
		{
			APIDDesc a = {apid_min, apid_min_idx, false};
			apid_list.push_back(a);
		}		
		for(APIDList::iterator it = apid_list.begin(); it != apid_list.end(); it++)
			printf("Record APID 0x%X %d\n",it->apid, it->ac3);

	}
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Stop()
{
	deviceState = CMD_VCR_STOP;

	if(last_mode != NeutrinoMessages::mode_scart)
	{
		g_RCInput->postMsg( NeutrinoMessages::VCR_OFF, 0 );
		g_RCInput->postMsg( NeutrinoMessages::CHANGEMODE , last_mode);
	}
	CIRSend irs("stop");
	return irs.Send();
}

//-------------------------------------------------------------------------
#ifdef MOVIEBROWSER
bool CVCRControl::CVCRDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string& epgTitle, unsigned char apids,const time_t epg_time)
#else /* MOVIEBROWSER */
bool CVCRControl::CVCRDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string& epgTitle, unsigned char apids)
#endif /* MOVIEBROWSER */
{
	printf("Record channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %llx, apids 0x%X mode \n",
	       channel_id,
	       epgid,
	       apids);
	// leave menu (if in any)
	g_RCInput->postMsg( CRCInput::RC_timeout, 0 );
	
	last_mode = CNeutrinoApp::getInstance()->getMode();
	if(mode != last_mode)
	{
		CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , mode | NeutrinoMessages::norezap );
	}
	
	if(channel_id != 0)		// wenn ein channel angegeben ist
	{
		if(g_Zapit->getCurrentServiceID() != channel_id)	// und momentan noch nicht getuned ist
		{
			g_Zapit->zapTo_serviceID(channel_id);		// dann umschalten
		}
	}
	if(! (apids & TIMERD_APIDS_STD)) // nicht std apid
	{
		APIDList apid_list;
		getAPIDs(apids,apid_list);
		if(!apid_list.empty())
		{
			if(!apid_list.begin()->ac3)
				g_Zapit->setAudioChannel(apid_list.begin()->index);
			else
				g_Zapit->setAudioChannel(0); //sonst apid 0, also auf jeden fall ac3 aus !
		}
		else
			g_Zapit->setAudioChannel(0); //sonst apid 0, also auf jeden fall ac3 aus !
	}
	else
		g_Zapit->setAudioChannel(0); //sonst apid 0, also auf jeden fall ac3 aus !

	if(SwitchToScart)
	{
		// Auf Scart schalten
		CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::VCR_ON, 0 );
		// Das ganze nochmal in die queue, da obiges RC_timeout erst in der naechsten ev. loop ausgeführt wird
		// und dann das menu widget das display falsch rücksetzt
		g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
	}

	deviceState = CMD_VCR_RECORD;
	// Send IR
	CIRSend irs("record");
	return irs.Send();
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Pause()
{
	CIRSend irs("pause");
	return irs.Send();
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Resume()
{
	CIRSend irs("resume");
	return irs.Send();
}

//-------------------------------------------------------------------------
void CVCRControl::CFileAndServerDevice::RestoreNeutrino(void)
{
	if (!g_Zapit->isPlayBackActive() && 
	    CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_standby)
		g_Zapit->startPlayBack();
	g_Sectionsd->setPauseScanning(false);
	g_Zapit->setRecordMode( false );
	// alten mode wieder herstellen (ausser wen zwischenzeitlich auf oder aus sb geschalten wurde)
	if(CNeutrinoApp::getInstance()->getMode() != last_mode && 
	   CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_standby &&
	   last_mode != NeutrinoMessages::mode_standby)
		g_RCInput->postMsg( NeutrinoMessages::CHANGEMODE , last_mode);

/*	if(last_mode == NeutrinoMessages::mode_standby &&
		CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_standby )
	{
		//Wenn vorher und jetzt standby, dann die zapit wieder auf sb schalten
		g_Zapit->setStandby(true);
	}*/
#ifndef TUXTXT_CFG_STANDALONE
	if(g_settings.tuxtxt_cache)
	{
		int vtpid=g_RemoteControl->current_PIDs.PIDs.vtxtpid;
		tuxtxt_init();
		if(vtpid)
			tuxtxt_start(vtpid);
	}
#endif
}

void CVCRControl::CFileAndServerDevice::CutBackNeutrino(const t_channel_id channel_id, const int mode)
{
	if (channel_id != 0) // wenn ein channel angegeben ist
	{
		last_mode = CNeutrinoApp::getInstance()->getMode();
		if (mode != last_mode && (last_mode != NeutrinoMessages::mode_standby || mode != CNeutrinoApp::getInstance()->getLastMode()))
		{
			CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , mode | NeutrinoMessages::norezap );
			// Wenn wir im Standby waren, dann brauchen wir fürs streamen nicht aufwachen...
			if(last_mode == NeutrinoMessages::mode_standby)
				CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_standby);
		}
		// Wenn im SB dann müssen wir die zapit aufwecken
/*		if(last_mode == NeutrinoMessages::mode_standby)
		{
			g_Zapit->setStandby(false);
		}*/
		if(g_Zapit->getCurrentServiceID() != channel_id)	// und momentan noch nicht getuned ist
		{
			g_Zapit->zapTo_serviceID(channel_id);		// dann umschalten
		}
		if(last_mode == NeutrinoMessages::mode_standby)
		{
			sleep(1); // Wait for zapit to come alive
			g_Zapit->muteAudio(false); // god knows why this is neccessary, it wont work without
			g_Zapit->muteAudio(true);
		}
	}
#ifndef TUXTXT_CFG_STANDALONE
	if(g_settings.tuxtxt_cache)
	{
		tuxtxt_stop();
		tuxtxt_close();
	}
#endif
	if(StopPlayBack && g_Zapit->isPlayBackActive())	// wenn playback gestoppt werden soll und noch läuft
		g_Zapit->stopPlayBack();					// dann playback stoppen

	if(StopSectionsd)								// wenn sectionsd gestoppt werden soll
		g_Sectionsd->setPauseScanning(true);		// sectionsd stoppen

	g_Zapit->setRecordMode( true );					// recordmode einschalten
}

#ifdef MOVIEBROWSER  		

std::string CVCRControl::CFileAndServerDevice::getMovieInfoString(const CVCRCommand command, const t_channel_id channel_id, const event_id_t epgid, unsigned char apids,const time_t epg_time)
{
	std::string extMessage;
	CMovieInfo cMovieInfo;
	MI_MOVIE_INFO movieInfo;
	std::string info1, info2;

	cMovieInfo.clearMovieInfo(&movieInfo);
	CZapitClient::responseGetPIDs pids;
	g_Zapit->getPIDS (pids);
	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo ();

	std::string tmpstring = g_Zapit->getChannelName(channel_id);
	if (tmpstring.empty())
		movieInfo.epgChannel = "unknown";
	else
		movieInfo.epgChannel = ZapitTools::UTF8_to_UTF8XML(tmpstring.c_str());

	tmpstring = "not available";
	if (epgid != 0)
	{
//#define SHORT_EPG
#ifdef SHORT_EPG
		CSectionsdClient sdc;
		CShortEPGData epgdata;
		
		if (sdc.getEPGidShort(epgid, &epgdata))
		{
#warning fixme sectionsd should deliver data in UTF-8 format
			tmpstring = Latin1_to_UTF8(epgdata.title);
			info1 = Latin1_to_UTF8(epgdata.info1);
			info2 = Latin1_to_UTF8(epgdata.info2);
		}
#else
		CSectionsdClient sdc;
		CEPGData epgdata;
		if (sdc.getEPGid(epgid, epg_time,&epgdata))
		{
#warning fixme sectionsd should deliver data in UTF-8 format
			tmpstring = Latin1_to_UTF8(epgdata.title);
			info1 = Latin1_to_UTF8(epgdata.info1);
			info2 = Latin1_to_UTF8(epgdata.info2);
			
			movieInfo.parentalLockAge = epgdata.fsk;
			if(epgdata.contentClassification.size() > 0 )
				movieInfo.genreMajor = epgdata.contentClassification[0];
				
			movieInfo.length = epgdata.epg_times.dauer	/ 60;
				
			printf("fsk:%d, Genre:%d, Dauer: %d\r\n",movieInfo.parentalLockAge,movieInfo.genreMajor,movieInfo.length);	
		}
#endif
	}
	movieInfo.epgTitle = 	ZapitTools::UTF8_to_UTF8XML(tmpstring.c_str());
	movieInfo.epgId = 		channel_id;
	movieInfo.epgInfo1 = 	ZapitTools::UTF8_to_UTF8XML(info1.c_str());
	movieInfo.epgInfo2 = 	ZapitTools::UTF8_to_UTF8XML(info2.c_str());
	movieInfo.epgEpgId =  	epgid ;
	movieInfo.epgMode = 	g_Zapit->getMode();
	movieInfo.epgVideoPid = si.vpid;

	EPG_AUDIO_PIDS audio_pids;
	// super hack :-), der einfachste weg an die apid descriptions ranzukommen
	g_RemoteControl->current_PIDs = pids;
	g_RemoteControl->processAPIDnames();

	for(unsigned int i= 0; i< pids.APIDs.size(); i++)
	{
		audio_pids.epgAudioPid = pids.APIDs[i].pid;
		audio_pids.epgAudioPidName = ZapitTools::UTF8_to_UTF8XML(g_RemoteControl->current_PIDs.APIDs[i].desc);
		movieInfo.audioPids.push_back(audio_pids);
	}
	movieInfo.epgVTXPID = si.vtxtpid;

	cMovieInfo.encodeMovieInfoXml(&extMessage,movieInfo);
	
	movieInfo.audioPids.clear();

	return extMessage;
}
#endif /* MOVIEBROWSER */

std::string CVCRControl::CFileAndServerDevice::getCommandString(const CVCRCommand command, const t_channel_id channel_id, const event_id_t epgid, const std::string& epgTitle, unsigned char apids)
{
	char tmp[40];
	std::string apids_selected;
	const char * extCommand;
//		std::string extAudioPID= "error";
	std::string info1, info2;
	std::string extMessage = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n\n<neutrino commandversion=\"1\">\n\t<record command=\"";
	switch(command)
	{
	case CMD_VCR_RECORD:
		extCommand = "record";
		break;
	case CMD_VCR_STOP:
		extCommand = "stop";
		break;
	case CMD_VCR_PAUSE:
		extCommand = "pause";
		break;
	case CMD_VCR_RESUME:
		extCommand = "resume";
		break;
	case CMD_VCR_AVAILABLE:
		extCommand = "available";
		break;
	case CMD_VCR_UNKNOWN:
	default:
		extCommand = "unknown";
		printf("[CVCRControl] Unknown Command\n");
	}

	extMessage += extCommand;
	extMessage += 
		"\">\n"
		"\t\t<channelname>";
	
	CZapitClient::responseGetPIDs pids;
	g_Zapit->getPIDS (pids);
	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo ();

	APIDList apid_list;
	getAPIDs(apids,apid_list);
	apids_selected="";
	for(APIDList::iterator it = apid_list.begin(); it != apid_list.end(); it++)
	{
		if(it != apid_list.begin())
			apids_selected += " ";
		sprintf(tmp, "%u", it->apid);
		apids_selected += tmp; 
	}
	
	std::string tmpstring = g_Zapit->getChannelName(channel_id);
	if (tmpstring.empty())
		extMessage += "unknown";
	else
		extMessage += ZapitTools::UTF8_to_UTF8XML(tmpstring.c_str());
	
	extMessage += "</channelname>\n\t\t<epgtitle>";
	
//		CSectionsdClient::responseGetCurrentNextInfoChannelID current_next;
	tmpstring = "not available";
	if (epgid != 0)
	{
		CSectionsdClient sdc;
		CShortEPGData epgdata;
		if (sdc.getEPGidShort(epgid, &epgdata))
		{
#warning fixme sectionsd should deliver data in UTF-8 format
			tmpstring = Latin1_to_UTF8(epgdata.title);
			info1 = Latin1_to_UTF8(epgdata.info1);
			info2 = Latin1_to_UTF8(epgdata.info2);
		}
		else if (!epgTitle.empty())
		{
			tmpstring = epgTitle;
		}
	}
	else if (!epgTitle.empty())
	{
		tmpstring = epgTitle;
	}
	extMessage += ZapitTools::UTF8_to_UTF8XML(tmpstring.c_str());
	
	extMessage += "</epgtitle>\n\t\t<id>";
	
	sprintf(tmp, PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS, channel_id);
	extMessage += tmp;
	
	extMessage += "</id>\n\t\t<info1>";
	extMessage += ZapitTools::UTF8_to_UTF8XML(info1.c_str());
	extMessage += "</info1>\n\t\t<info2>";
	extMessage += ZapitTools::UTF8_to_UTF8XML(info2.c_str());
	extMessage += "</info2>\n\t\t<epgid>";
	sprintf(tmp, "%llu", epgid);
	extMessage += tmp;
	extMessage += "</epgid>\n\t\t<mode>";
	sprintf(tmp, "%d", g_Zapit->getMode());
	extMessage += tmp;
	extMessage += "</mode>\n\t\t<videopid>";
	sprintf(tmp, "%u", si.vpid);
	extMessage += tmp;
	extMessage += "</videopid>\n\t\t<audiopids selected=\"";
	extMessage += apids_selected;
	extMessage += "\">\n";
	// super hack :-), der einfachste weg an die apid descriptions ranzukommen
	g_RemoteControl->current_PIDs = pids;
	g_RemoteControl->processAPIDnames();
	for(unsigned int i= 0; i< pids.APIDs.size(); i++)
	{
		extMessage += "\t\t\t<audio pid=\"";
		sprintf(tmp, "%u", pids.APIDs[i].pid);
		extMessage += tmp;
		extMessage += "\" name=\"";
		extMessage += ZapitTools::UTF8_to_UTF8XML(g_RemoteControl->current_PIDs.APIDs[i].desc);
		extMessage += "\"/>\n";
	}
	extMessage += 
		"\t\t</audiopids>\n"
		"\t\t<vtxtpid>";
	sprintf(tmp, "%u", si.vtxtpid);
	extMessage += tmp;
	extMessage +=
		"</vtxtpid>\n"
		"\t</record>\n"
		"</neutrino>\n";

	return extMessage;
}








bool CVCRControl::CFileDevice::Stop()
{
	printf("Stop\n");

	bool return_value = (::stop_recording() == STREAM2FILE_OK);

	int actmode=g_Zapit->PlaybackState(); // get actual decoder mode
	if ((actmode == 1) && (!g_settings.misc_spts)) // actual mode is SPTS and settings require PES
		g_Zapit->PlaybackPES(); // restore PES mode

	RestoreNeutrino();

	deviceState = CMD_VCR_STOP;

	return return_value;
}

#ifdef MOVIEBROWSER
bool CVCRControl::CFileDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string &epgTitle, unsigned char apids,const time_t epg_time) 
#else /* MOVIEBROWSER */
bool CVCRControl::CFileDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string &epgTitle, unsigned char apids) 
#endif /* MOVIEBROWSER */
{
	printf("Record channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %llx, apids 0x%X mode %d\n",
	       channel_id,
	       epgid,
	       apids,
	       mode);

	CutBackNeutrino(channel_id, mode);

	int repeatcount=0;
	int actmode=g_Zapit->PlaybackState(); // get actual decoder mode
	bool sptsmode=g_settings.misc_spts;   // take default from settings

	// aviaEXT is loaded, actual mode is not SPTS and switchoption is set , only in tvmode
	if ((actmode == 0) && g_settings.recording_in_spts_mode && mode == 1)
	{
		g_Zapit->PlaybackSPTS();
		while ((repeatcount++ < 10) && (g_Zapit->PlaybackState() != 1)) {
			sleep(1); 
		}
		sptsmode = true;
	}
	else if(mode==2)
	{
		if(actmode== 1)
		{
			g_Zapit->PlaybackPES();
			while ((repeatcount++ < 10) && (g_Zapit->PlaybackState() != 0)) {
				sleep(1); 
			}
		}
		sptsmode = false;
	}

#define MAXPIDS		64
	unsigned short pids[MAXPIDS];
	unsigned int numpids;
	unsigned int pos;

	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo();
	if (si.vpid != 0)
	{
		pids[0] = si.vpid;
		numpids = 1;
		if(sptsmode)
			transfer_pids(si.vpid,0x00,0);
	}
	else
	{
		/* no video pid */
		numpids = 0;
	}
	APIDList apid_list;
	getAPIDs(apids,apid_list);
	for(APIDList::iterator it = apid_list.begin(); it != apid_list.end(); it++)
	{
		pids[numpids++] = it->apid;
		if(sptsmode)
			transfer_pids(it->apid,0x01, it->ac3 ? 1 : 0);
	}
	if(!apid_list.empty())
		g_Zapit->setAudioChannel(apid_list.begin()->index);
	
	CZapitClient::responseGetPIDs allpids;
	g_Zapit->getPIDS(allpids);
	if ((StreamVTxtPid) && (si.vtxtpid != 0))
	{
		pids[numpids++] = si.vtxtpid;
	}

	if ((StreamPmtPid) && (si.pmtpid != 0))
	{
		pids[numpids++] = si.pmtpid;
	}

	char filename[512]; // UTF-8

	// Create filename for recording
	pos = Directory.size();
	strcpy(filename, Directory.c_str());
	
	if ((pos == 0) ||
	    (filename[pos - 1] != '/'))
	{
		filename[pos] = '/';
		pos++;
		filename[pos] = '\0';
	}
	
	time_t t = time(NULL);
	
	if (FilenameTemplate.empty())
	{
		if (g_settings.recording_epg_for_filename)
			FilenameTemplate = "%c_%i_";
		FilenameTemplate += "%d_%t";
	}
	
	// %c == channel, %i == info, %d == date, %t == time
	std::string expandedTemplate;
	if (CreateTemplateDirectories)
	{	
		expandedTemplate = FilenameTemplate;
	} else
	{
		expandedTemplate = std::string(basename(FilenameTemplate.c_str()));
	}
	unsigned int searchPos = std::string::npos;
	unsigned int startAt = 0;
	unsigned int dataLength = 0;
	char buf[256];
	buf[255] = '\0';
	
	if (g_settings.recording_epg_for_filename) {
		appendChannelName(buf,255,channel_id);
		dataLength = strlen(buf);
		
		while ((searchPos = expandedTemplate.find("%c",startAt)) != std::string::npos) {
			expandedTemplate.erase(searchPos,2);
			expandedTemplate.insert(searchPos,buf);
			startAt = searchPos + dataLength;
		}
		startAt = 0;
		appendEPGInfo(buf, 255, epgid, epgTitle);
		dataLength = strlen(buf);
		while ((searchPos = expandedTemplate.find("%i",startAt)) != std::string::npos) {
			expandedTemplate.erase(searchPos,2);
			expandedTemplate.insert(searchPos,buf);
			startAt = searchPos + dataLength;
		}
	}
	
	strftime(buf,11,"%Y-%m-%d",localtime(&t));
	dataLength = strlen(buf);
	startAt = 0;
	while ((searchPos = expandedTemplate.find("%d",startAt)) != std::string::npos) {
		expandedTemplate.erase(searchPos,2);
		expandedTemplate.insert(searchPos,buf);
		startAt = searchPos + dataLength;
	}
	
	strftime(buf,7,"%H%M%S",localtime(&t));
	dataLength = strlen(buf);
	startAt = 0;
	while ((searchPos = expandedTemplate.find("%t",startAt)) != std::string::npos) {
		expandedTemplate.erase(searchPos,2);
		expandedTemplate.insert(searchPos,buf);
		startAt = searchPos + dataLength;
	}
	//printf("[CFileDevice] filename: %s, expandedTemplate: %s\n",filename,expandedTemplate.c_str());
	
	strncpy(&(filename[pos]),expandedTemplate.c_str(),511-pos);

	stream2file_error_msg_t error_msg;
	if (CreateTemplateDirectories && !createRecordingDir(filename))
	{
		error_msg = STREAM2FILE_INVALID_DIRECTORY;
	} else
	{
		error_msg = ::start_recording(filename,
#ifdef MOVIEBROWSER  		
					      getMovieInfoString(CMD_VCR_RECORD, channel_id, epgid, apids,epg_time).c_str(),
#else /* MOVIEBROWSER */
					      getCommandString(CMD_VCR_RECORD, channel_id, epgid, epgTitle, apids).c_str(),
#endif /* MOVIEBROWSER */
					      Use_O_Sync,
					      Use_Fdatasync,
					      ((unsigned long long)SplitSize) * 1048576ULL,
					      numpids,
					      pids,
					      sptsmode,
					      RingBuffers);
	}
	CreateTemplateDirectories = true;
	if (error_msg == STREAM2FILE_OK)
	{
		deviceState = CMD_VCR_RECORD;
		return true;
	}
	else
	{
		RestoreNeutrino();

		printf("[CFileDevice] stream2file error code: %d\n", error_msg);
#warning FIXME: Use better error message
		DisplayErrorMessage(g_Locale->getText(
						      error_msg == STREAM2FILE_BUSY ? LOCALE_STREAMING_BUSY :
						      error_msg == STREAM2FILE_INVALID_DIRECTORY ? LOCALE_STREAMING_DIR_NOT_WRITABLE :
						      LOCALE_STREAMINGSERVER_NOCONNECT
						      )); // UTF-8

		return false;
	}
}

void CVCRControl::CFileDevice::appendEPGInfo(char *buf, unsigned int size, const event_id_t epgid, const std::string& epgTitleTimer) {
	
	CSectionsdClient sdc;
	CShortEPGData epgdata;
	std::string epgTitle;
	if (size > 0)
		buf[0] = '\0';
	if (sdc.getEPGidShort(epgid, &epgdata))
		epgTitle = epgdata.title;
	else
		epgTitle = epgTitleTimer;

	if (!(epgTitle.empty()) && epgTitle.size() < size)
	{
#warning fixme sectionsd should deliver data in UTF-8 format
//				strcpy(&(filename[pos]), Latin1_to_UTF8(epgdata.title).c_str());
// all characters with code >= 128 will be discarded anyway
		strcpy(buf, epgTitle.c_str());
		char * p_act = buf;
		do {
			p_act +=  strcspn(p_act, "/ \"%&-\t`'~<>!,:;?^°$\\=*#@¤|");
			if (*p_act) {
				*p_act++ = '_';
			}
		} while (*p_act);
		
		p_act = buf;
		do
		{
			if ((unsigned char) (*p_act) >= 128) {
				*p_act = '_';
			}
		} while (*p_act++);
	}
}

void CVCRControl::CFileDevice::appendChannelName(char *buf, unsigned int size, const t_channel_id channel_id) {
	
	if (size > 0)
		buf[0] = '\0';
	std::string ext_channel_name = g_Zapit->getChannelName(channel_id);
	if (ext_channel_name.size() < size)
	{
		strcpy(buf, UTF8_TO_FILESYSTEM_ENCODING(ext_channel_name.c_str()));
		
		char * p_act = buf;
		do {
			p_act += strcspn(p_act, "/ \"%&-\t`'´!,:;");
			if (*p_act)
			{
				*p_act++ = '_';
			}
		} while (*p_act);
	}
}



bool CVCRControl::CFileDevice::createRecordingDir(const char *filename) 
{
	//printf("[CFileDevice] trying to create directory %s\n",filename);
	char *pos;
	unsigned int start = 0;
	while ((pos = strchr(&(filename[start]),'/')) != NULL) {
		if (pos == &filename[0])
		{
			start = 1;
			continue;
		}
		*pos = '\0';
		start = strlen(filename)+1;
		struct stat statInfo;
		if (stat(filename,&statInfo) == -1)
		{
			if (errno == ENOENT)
			{	
				if (mkdir(filename,0000) == 0)
				{
					mode_t mode = strtoul(g_settings.recording_dir_permissions[0],(char**)NULL,8);
					if (chmod(filename,mode) != 0)
					{
						perror("[CFileDevice] chmod:");
						*pos = '/';
						return false;
					}
				} else
				{
					perror("[CFileDevice] mkdir");
					*pos = '/';
					return false;
				}
				
			} else {
				perror("[CFileDevice] stat");
				*pos = '/';
				return false;
			}
		} else {
			if (!S_ISDIR(statInfo.st_mode)) {
				printf("[CFileDevice] cannot create directory %s\n",filename);
				*pos = '/';
				return false;
			}
		}		
		*pos = '/';
	}
	return true;
}
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Stop()
{
	printf("Stop\n");

	bool return_value = sendCommand(CMD_VCR_STOP);

	int actmode=g_Zapit->PlaybackState(); // get actual decoder mode
	if ((actmode == 1) && (!g_settings.misc_spts)) // actual mode is SPTS and settings require PES
	{
		int repeatcount=0;
		g_Zapit->PlaybackPES(); // restore PES mode
		while ((repeatcount++ < 10) && (g_Zapit->PlaybackState() != 0)) {
			sleep(1); 
		}
	}

	RestoreNeutrino();

	return return_value;
}

//-------------------------------------------------------------------------
#ifdef MOVIEBROWSER
bool CVCRControl::CServerDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string &epgTitle, unsigned char apids,const time_t epg_time) 
#else /* MOVIEBROWSER */
bool CVCRControl::CServerDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string &epgTitle, unsigned char apids) 
#endif /* MOVIEBROWSER */
{
	printf("Record channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %s(%llx), apids 0x%X mode %d\n",
	       channel_id,
			 epgTitle.c_str(),
	       epgid,
	       apids,
	       mode);

	CutBackNeutrino(channel_id, mode);

	int repeatcount=0;
	int actmode=g_Zapit->PlaybackState() ; // get actual decoder mode

	// aviaEXT is loaded, actual mode is not SPTS and switchoption is set , only in tvmode
	if ((actmode == 0)  && g_settings.recording_in_spts_mode && mode == 1)
	{
		g_Zapit->PlaybackSPTS();
		while ((repeatcount++ < 10) && (g_Zapit->PlaybackState() != 1)) {
			sleep(1); 
		}
	}
	else if(mode==2  && actmode== 1)
	{
			g_Zapit->PlaybackPES();
			while ((repeatcount++ < 10) && (g_Zapit->PlaybackState() != 0)) {
				sleep(1); 
			}
	}

	if(!sendCommand(CMD_VCR_RECORD, channel_id, epgid, epgTitle, apids))
	{
		RestoreNeutrino();

		DisplayErrorMessage(g_Locale->getText(LOCALE_STREAMINGSERVER_NOCONNECT));

		return false;
	}
	else
		return true;
}


//-------------------------------------------------------------------------
void CVCRControl::CServerDevice::serverDisconnect()
{
	close(sock_fd);
}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::sendCommand(CVCRCommand command, const t_channel_id channel_id, const event_id_t epgid, const std::string& epgTitle, unsigned char apids)
{
	printf("Send command: %d channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %s(%llx)\n",
	       command,
	       channel_id,
			 epgTitle.c_str(),
	       epgid);
	if(serverConnect())
	{
		std::string extMessage = getCommandString(command, channel_id, epgid, epgTitle, apids);

		printf("sending to vcr-client:\n\n%s\n", extMessage.c_str());
		write(sock_fd, extMessage.c_str() , extMessage.length() );

		serverDisconnect();

		deviceState = command;
		return true;
	}
	else
		return false;

}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::serverConnect()
{

	printf("connect to server: %s:%d\n",ServerAddress.c_str(),ServerPort);

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SAI servaddr;
	memset(&servaddr,0,sizeof(SAI));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(ServerPort);
	inet_pton(AF_INET, ServerAddress.c_str(), &servaddr.sin_addr);


	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("[cvcr] -  cannot connect to streamingserver\n");
		return false;
	}

	return true;
}
//-------------------------------------------------------------------------
