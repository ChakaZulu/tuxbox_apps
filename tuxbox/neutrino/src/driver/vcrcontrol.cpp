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

#include <driver/vcrcontrol.h>

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
#include <sys/un.h>


#include <daemonc/remotecontrol.h>
#include <zapit/client/zapittools.h>

extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */


#define SA struct sockaddr
#define SAI struct sockaddr_in

CVCRControl* CVCRControl::getInstance()
{
	static CVCRControl* vcrControl = NULL;

	if(!vcrControl)
	{
		vcrControl = new CVCRControl();
		printf("[neutrino] vcrControl Instance created\n");
	}
	else
	{
		//printf("[neutrino] vcrControl Instace requested\n");
	}
	return vcrControl;
}

//-------------------------------------------------------------------------
CVCRControl::CVCRControl()
{
	Device=NULL;
}

//-------------------------------------------------------------------------
CVCRControl::~CVCRControl()
{
	if(Device)
		delete Device;
}
//-------------------------------------------------------------------------

void CVCRControl::setDeviceOptions(CDeviceInfo *deviceInfo)
{
	if (Device)
	{
		if (Device->getDeviceType() == DEVICE_FILE)
		{
			CFileDevice * device = (CFileDevice *) Device;
			CFileDeviceInfo * serverinfo = (CFileDeviceInfo *) deviceInfo;

			if (!(serverinfo->Directory.empty()))
				device->Directory = serverinfo->Directory;
			device->SplitSize = serverinfo->SplitSize;
			device->StreamAllAudioPids = serverinfo->StreamAllAudioPids;

			device->StopPlayBack = serverinfo->StopPlayBack;
			device->StopSectionsd = serverinfo->StopSectionsd;
		}
		else if(Device->getDeviceType() == DEVICE_SERVER)
		{
			CServerDevice * device = (CServerDevice *) Device;
			CServerDeviceInfo *serverinfo = (CServerDeviceInfo *) deviceInfo;

			if (!(serverinfo->ServerAddress.empty()))
				device->ServerAddress = serverinfo->ServerAddress;
			if (serverinfo->ServerPort > 0)
				device->ServerPort = serverinfo->ServerPort;

			device->StopPlayBack = serverinfo->StopPlayBack;
			device->StopSectionsd = serverinfo->StopSectionsd;
		}
		else if(Device->getDeviceType() == DEVICE_VCR)
		{
			CVCRDevice * device = (CVCRDevice *) Device;
			CVCRDeviceInfo *serverinfo = (CVCRDeviceInfo *) deviceInfo;

			device->SwitchToScart = serverinfo->SwitchToScart;
		}

		if (!(deviceInfo->Name.empty()))
			Device->Name = deviceInfo->Name;
	}
}
//-------------------------------------------------------------------------

void CVCRControl::unregisterDevice()
{
	if(Device)
	{
		delete Device;
		Device = NULL;
	}
}

//-------------------------------------------------------------------------
bool CVCRControl::registerDevice(CVCRDevices deviceType, CDeviceInfo *deviceInfo)
{
	if(Device)
		unregisterDevice();

	if(deviceType == DEVICE_SERVER)
	{
		CServerDevice * device =  new CServerDevice();
		Device = (CDevice*) device;
		setDeviceOptions(deviceInfo);
		printf("CVCRControl registered new server device: %s\n",device->Name.c_str());
	}
	else if(deviceType == DEVICE_VCR)
	{
		CVCRDevice * device = new CVCRDevice();
		Device = (CDevice*) device;
		setDeviceOptions(deviceInfo);
		printf("CVCRControl registered new vcr device: %s\n",device->Name.c_str());
	}
	else if(deviceType == DEVICE_FILE)
	{
		CFileDevice * device = new CFileDevice();
		Device = (CDevice*) device;
		setDeviceOptions(deviceInfo);
		printf("CVCRControl registered new file device: %s\n",device->Name.c_str());
	}
	else
		return false;

	return true;
}

//-------------------------------------------------------------------------
bool CVCRControl::Record(const CTimerd::RecordingInfo * const eventinfo)
{
	int mode = g_Zapit->isChannelTVChannel(eventinfo->channel_id) ? NeutrinoMessages::mode_tv : NeutrinoMessages::mode_radio;
	return Device->Record(eventinfo->channel_id, mode, eventinfo->epgID, eventinfo->apids); 
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
bool CVCRControl::CVCRDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string & apids)
{
	printf("Record channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %llx, apids %s mode \n",
	       channel_id,
	       epgid,
	       apids.c_str());
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
	if(apids != "") //selbiges für apid
	{
		unsigned short apid = atoi(apids.c_str()); // only user first apid
		CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo ();
		if(si.apid != apid)
		{
			CZapitClient::responseGetPIDs pids;
			g_Zapit->getPIDS (pids);
			unsigned int i;
			for(i=0;i<pids.APIDs.size();i++)
			{
				if(pids.APIDs[i].pid==apid)
					g_Zapit->setAudioChannel(i);
			}
			// nicht gefunden, dann 1.
			if(i==pids.APIDs.size())
				g_Zapit->setAudioChannel(0);
		}
	}
	else
		g_Zapit->setAudioChannel(0); //sonst apid 0, also auf jeden fall ac3 aus !!!

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

	if(StopPlayBack && g_Zapit->isPlayBackActive())	// wenn playback gestoppt werden soll und noch läuft
		g_Zapit->stopPlayBack();					// dann playback stoppen

	if(StopSectionsd)								// wenn sectionsd gestoppt werden soll
		g_Sectionsd->setPauseScanning(true);		// sectionsd stoppen

	g_Zapit->setRecordMode( true );					// recordmode einschalten
}

std::string CVCRControl::CFileAndServerDevice::getCommandString(const CVCRCommand command, const t_channel_id channel_id, const event_id_t epgid, const std::string & apids) const
{
	char tmp[40];
	std::string apids10;
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
	
//		sprintf(tmp,"%u", g_RemoteControl->current_PIDs.PIDs.vpid );
	CZapitClient::responseGetPIDs pids;
	g_Zapit->getPIDS (pids);
	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo ();
//		sprintf(tmp,"%u", g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
	//Apids
/*		for(int i=0 ; i < MAX_APIDS ; i++)
		sApids[i]="";*/
	if (apids.empty())
	{
		sprintf(tmp,"%u", si.apid);
		apids10=tmp;
	}
	else
	{
		unsigned int index=0,pos=0;
		apids10="";
		while(pos!=std::string::npos)
		{
			pos = apids.find(' ', index);
			if(pos!=std::string::npos)
			{
				sprintf(tmp, "%ld ", strtol(apids.substr(index,pos-index).c_str(),NULL,16));
				index=pos+1;
			}
			else
			{
				sprintf(tmp, "%ld", strtol(apids.substr(index).c_str(),NULL,16));
			}
			apids10 += tmp;
		}
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
	extMessage += apids10;
	extMessage += "\">\n";
	// super hack :-), der einfachste weg an die apid descriptions ranzukommen
	g_RemoteControl->current_PIDs = pids;
	g_RemoteControl->processAPIDnames();
//		bool apidFound=false;
	for(unsigned int i= 0; i< pids.APIDs.size(); i++)
	{
		extMessage += "\t\t\t<audio pid=\"";
		sprintf(tmp, "%u", pids.APIDs[i].pid);
		extMessage += tmp;
		extMessage += "\" name=\"";
		extMessage += ZapitTools::UTF8_to_UTF8XML(g_RemoteControl->current_PIDs.APIDs[i].desc);
		extMessage += "\"/>\n";
/*			if(pids.APIDs[i].pid==apid)
			apidFound=true;*/
	}
/*		if(!apidFound)
		{
		// add spec apid to available
		extMessage +="            <audio pid=\"" + extAudioPID + "\" name=\"" + extAudioPID  + "\"/>\n";
		}*/
	extMessage += 
		"\t\t</audiopids>\n"
		"\t</record>\n"
		"</neutrino>\n";

	return extMessage;
}








bool CVCRControl::CFileDevice::Stop()
{
	printf("Stop\n");

	bool return_value = (::stop_recording() == STREAM2FILE_OK);

	RestoreNeutrino();

	deviceState = CMD_VCR_STOP;

	return return_value;
}

bool CVCRControl::CFileDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string & apids) 
{
	printf("Record channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %llx, apids %s mode %d\n",
	       channel_id,
	       epgid,
	       apids.c_str(),
	       mode);

	CutBackNeutrino(channel_id, mode);

#define MAXPIDS		64
	unsigned short pids[MAXPIDS];
	unsigned int numpids;
	unsigned int pos;

	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo();
	if (si.vpid != 0)
	{
		pids[0] = si.vpid;
		numpids = 1;
	}
	else
	{
		/* no video pid */
		numpids = 0;
	}
	
	if (StreamAllAudioPids)
	{
		CZapitClient::responseGetPIDs allpids;
		g_Zapit->getPIDS(allpids);
	    	for(unsigned int i = 0; i < allpids.APIDs.size(); i++)
		{
			pids[numpids++] = allpids.APIDs[i].pid;
		}
	}
	else
	{
		if (apids.empty())
		{
			pids[numpids++] = si.apid;
		}
		else
		{
			unsigned int index = 0;
			pos = 0;
			
			while(pos != std::string::npos)
			{
				pos = apids.find(' ', index);
				if(pos != std::string::npos)
				{
					pids[numpids++] = strtol(apids.substr(index,pos-index).c_str(),NULL,16);
					index = pos+1;
				}
				else
				{
					pids[numpids++] = strtol(apids.substr(index).c_str(),NULL,16);
				}
			}
		}
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

	std::string ext_channel_name = g_Zapit->getChannelName(channel_id);
	if (!(ext_channel_name.empty()))
	{
		strcpy(&(filename[pos]), ext_channel_name.c_str());
		char * p_act = &(filename[pos]);
		do {
			p_act += strcspn(p_act, "/ \"%&-\t`'´!,:;");
			if (*p_act)
			{
				*p_act++ = '_';
			}
		} while (*p_act);
								
		strcat(filename, "_");
	}

	pos = strlen(filename);
	if (epgid != 0)
	{
		CSectionsdClient sdc;
		CShortEPGData epgdata;
		if (sdc.getEPGidShort(epgid, &epgdata))
		{
			if (!(epgdata.title.empty()))
			{
#warning fixme sectionsd should deliver data in UTF-8 format
//				strcpy(&(filename[pos]), Latin1_to_UTF8(epgdata.title).c_str());
// all characters with code >= 128 will be discarded anyway
				strcpy(&(filename[pos]), epgdata.title.c_str());
				char * p_act = &(filename[pos]);
				do {
					p_act +=  strcspn(p_act, "/ \"%&-\t`'~<>!,:;?^°$\\=*#@¤|");
					if (*p_act) {
						*p_act++ = '_';
					}
				} while (*p_act);
				
				p_act = &(filename[pos]);
				do
				{
					if ((unsigned char) (*p_act) >= 128) {
						*p_act = '_';
					}
				} while (*p_act++);
				
				strcat(filename, "_");
			}
		}
	}

	pos = strlen(filename);
	time_t t = time(NULL);
	strftime(&(filename[pos]), sizeof(filename) - pos - 1, "%Y%m%d_%H%M%S", localtime(&t));

	stream2file_error_msg_t error_msg = ::start_recording(filename,
							      getCommandString(CMD_VCR_RECORD, channel_id, epgid, apids).c_str(),
							      ((unsigned long long)SplitSize) * 1048576ULL,
							      numpids,
							      pids,
							      g_settings.misc_option[MISC_SETTING_SPTS_MODE]);

	if (error_msg == STREAM2FILE_OK)
	{
		deviceState = CMD_VCR_RECORD;
		return true;
	}
	else
	{
		RestoreNeutrino();

		printf("[vcrcontrol] stream2file error code: %d\n", error_msg);
#warning FIXME: Use better error message
		DisplayErrorMessage(g_Locale->getText(
						      error_msg == STREAM2FILE_BUSY ? "streaming.busy" :
						      error_msg == STREAM2FILE_INVALID_DIRECTORY ? "streaming.dir_not_writable" :
						      "streamingserver.noconnect"
						      )); // UTF-8

		return false;
	}
}






//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Stop()
{
	printf("Stop\n");

	bool return_value = sendCommand(CMD_VCR_STOP);

	RestoreNeutrino();

	return return_value;
}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Record(const t_channel_id channel_id, int mode, const event_id_t epgid, const std::string & apids) 
{
	printf("Record channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epg: %llx, apids %s mode %d\n",
	       channel_id,
	       epgid,
	       apids.c_str(),
	       mode);

	CutBackNeutrino(channel_id, mode);

	if(!sendCommand(CMD_VCR_RECORD,channel_id,epgid,apids))
	{
		RestoreNeutrino();

		DisplayErrorMessage(g_Locale->getText("streamingserver.noconnect")); // UTF-8

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
bool CVCRControl::CServerDevice::sendCommand(CVCRCommand command, const t_channel_id channel_id, const event_id_t epgid, const std::string & apids)
{
	printf("Send command: %d channel_id: "
	       PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
	       " epgid: %llx\n",
	       command,
	       channel_id,
	       epgid);
	if(serverConnect())
	{
		std::string extMessage = getCommandString(command, channel_id, epgid, apids);

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
