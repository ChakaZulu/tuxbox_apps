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
#include <sys/un.h>

#include <global.h>
#include <neutrinoMessages.h>

#include <gui/widget/messagebox.h>
#include <liblircdclient.h>

#include "vcrcontrol.h"
#include "neutrino.h"

#define LIRCDIR "/var/tuxbox/config/"

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
	if(Device)
	{
		if(Device->deviceType == DEVICE_SERVER)
		{
			CServerDevice * device = (CServerDevice *) Device;
			CServerDeviceInfo *serverinfo = (CServerDeviceInfo *) deviceInfo;
			if(serverinfo->Name.length() > 0)
				device->Name = serverinfo->Name;
			if(serverinfo->ServerAddress.length() > 0)
				device->ServerAddress = serverinfo->ServerAddress;
			if(serverinfo->ServerPort > 0)
				device->ServerPort = serverinfo->ServerPort;
			device->StopPlayBack = serverinfo->StopPlayBack;
			device->StopSectionsd = serverinfo->StopSectionsd;
		}
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
		printf("CVCRControl registered new serverdevice: %s\n",device->Name.c_str());
		return true;
	}
	else if(deviceType == DEVICE_VCR)
	{
		CVCRDevice * device = new CVCRDevice();
		device->Name = deviceInfo->Name;
		Device = (CDevice*) device;
		printf("CVCRControl registered new vcrdevice: %s\n",device->Name.c_str());
		return true;
	}
	else
		return false;
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
	return ParseFile(LIRCDIR "stop.lirc");
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::ParseFile(string filename)
{
	ifstream inp;
	char buffer[101];
	int wait_time;
	inp.open(filename.c_str(),ifstream::in);
	if( inp.is_open() )
	{
		CLircdClient lirc;
		if(lirc.Connect())
		{
			while(inp.good())
			{
				inp.getline(buffer,100);
				if(buffer[0]!=0)
				{
					string line = buffer;
					if(line.substr(0,4)=="WAIT" || line.substr(0,4)=="wait")
					{
						sscanf(line.substr(5).c_str(),"%d",&wait_time);
						if(wait_time > 0)
							usleep(wait_time*1000);
					}
					else
					{
						int duration=0;
						unsigned int space_pos=line.find(" ");
						if(space_pos!=string::npos)
						{
							sscanf(line.substr(space_pos+1).c_str(),"%d",&duration);
						}
						if(duration > 0)
							lirc.SendUsecs(Name, line.substr(0,space_pos).c_str(),duration*1000);
						else
							lirc.SendOnce(Name, buffer);
					}
				}
			}
			lirc.Disconnect();
			return true;
		}
		inp.close();
	}
	else
		printf("konnte datei %s nicht oeffnen\n",filename.c_str());
	return false;
}
//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Record(const t_channel_id channel_id, int mode, unsigned long long epgid, uint apid)
{
	printf("Record channel_id: %x epg: %llx, apid %u mode \n", channel_id, epgid, apid);
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
	if(apid !=0) //selbiges für apid
	{
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

	// Auf Scart schalten
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::VCR_ON, 0 );
	// Das ganze nochmal in die queue, da obiges RC_timeout erst in der naechsten ev. loop ausgeführt wird
	// und dann das menu widget das display falsch rücksetzt
	g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
	deviceState = CMD_VCR_RECORD;
	// Send IR
	return ParseFile(LIRCDIR "record.lirc");
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Pause()
{
	return ParseFile(LIRCDIR "pause.lirc");
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Resume()
{
	return ParseFile(LIRCDIR "resume.lirc");
}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Stop()
{
	printf("Stop\n"); 
	if(!g_Zapit->isPlayBackActive())
		g_Zapit->startPlayBack();
	g_Sectionsd->setPauseScanning(false);
	g_Zapit->setRecordMode( false );
	if(last_mode != NeutrinoMessages::mode_scart)
	{
		g_RCInput->postMsg( NeutrinoMessages::VCR_OFF, 0 );
		g_RCInput->postMsg( NeutrinoMessages::CHANGEMODE , last_mode);
	}
	if(sendCommand(CMD_VCR_STOP))
		return true;
	else
		return false;
}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Record(const t_channel_id channel_id, int mode, unsigned long long epgid, uint apid) 
{
	printf("Record channel_id: %x epg: %llx, apid %u\n", channel_id, epgid, apid);
	if(channel_id != 0)		// wenn ein channel angegeben ist
	{
		last_mode = CNeutrinoApp::getInstance()->getMode();
		if(mode != last_mode)
			CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , mode | NeutrinoMessages::norezap );
		if(g_Zapit->getCurrentServiceID() != channel_id)	// und momentan noch nicht getuned ist
		{
			g_Zapit->zapTo_serviceID(channel_id);		// dann umschalten
		}
	}

	if(StopPlayBack && g_Zapit->isPlayBackActive())	// wenn playback gestoppt werden soll und noch läuft
		g_Zapit->stopPlayBack();					// dann playback stoppen

	if(StopSectionsd)								// wenn sectionsd gestoppt werden soll
		g_Sectionsd->setPauseScanning(true);		// sectionsd stoppen

	g_Zapit->setRecordMode( true );					// recordmode einschalten

	if(!sendCommand(CMD_VCR_RECORD,channel_id,epgid,apid))
	{
		if(!g_Zapit->isPlayBackActive())			// wenn command nicht gesendet werden konnte
			g_Zapit->startPlayBack();				// dann alles rueckgaengig machen
		g_Sectionsd->setPauseScanning(false);
		g_Zapit->setRecordMode( false );

		ShowMsg ( "messagebox.info", g_Locale->getText("streamingserver.noconnect"), CMessageBox::mbrBack, CMessageBox::mbBack, "error.raw" );

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
bool CVCRControl::CServerDevice::sendCommand(CVCRCommand command, const t_channel_id channel_id, unsigned long long epgid, uint apid)
{
	printf("Send command: %d channel_id: %x epgid: %llx\n",command, channel_id, epgid);
	if(serverConnect())
	{
		char tmp[40];
		string extCommand="unknown";
		string ext_channel_id = "error";
		string ext_channel_name = "unknown";
		string extEpgid="error";
		string extMode="error";
		string extVideoPID="error";
		string extAudioPID="error";
		string extEPGTitle="not available";
		sprintf(tmp,"%u", channel_id);
		ext_channel_id = tmp;
		sprintf(tmp,"%llu", epgid);
		extEpgid = tmp;
//		sprintf(tmp,"%u", g_RemoteControl->current_PIDs.PIDs.vpid );
		CZapitClient::responseGetPIDs pids;
		g_Zapit->getPIDS (pids);
		CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo ();
		sprintf(tmp,"%u", si.vdid );
		extVideoPID = tmp;
//		sprintf(tmp,"%u", g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
		if(apid==0)
			apid=(uint)si.apid;
		sprintf(tmp,"%u", apid);
		extAudioPID = tmp;
		sprintf(tmp,"%d",g_Zapit->getMode ());
		extMode = tmp;

		CZapitClient::BouquetChannelList channellist;     
		g_Zapit->getChannels(channellist);
		CZapitClient::BouquetChannelList::iterator channel = channellist.begin();
		for(; channel != channellist.end();channel++)
		{
			if(channel->channel_id==channel_id)
			{
				ext_channel_name=channel->name;
				break;
			}
		}

		CSectionsdClient sections;
		sectionsd::responseGetCurrentNextInfoChannelID current_next;
		if(sections.getCurrentNextServiceKey(channel_id, current_next))
		{
			extEPGTitle=current_next.current_name;
		}

		switch(command)
		{
			case CMD_VCR_RECORD: extCommand="record";
				break;
			case CMD_VCR_STOP: extCommand="stop";
				break;
			case CMD_VCR_PAUSE: extCommand="pause";
				break;
			case CMD_VCR_RESUME: extCommand="resume";
				break;
			case CMD_VCR_AVAILABLE: extCommand="available";
				break;
			case CMD_VCR_UNKNOWN:
			default:
				printf("CVCRControl: Unknown Command\n");
		}

		string extMessage = "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n\n";
		extMessage +="<neutrino commandversion=\"1\">\n";
		extMessage +="    <record command=\"" + extCommand + "\">\n";
		extMessage +="        <channelname>" + ext_channel_name + "</channelname>\n";
		extMessage +="        <epgtitle>" + extEPGTitle + "</epgtitle>\n";
		extMessage +="        <onidsid>" + ext_channel_id + "</onidsid>\n";
		extMessage +="        <epgid>" + extEpgid + "</epgid>\n";
		extMessage +="        <mode>" + extMode + "</mode>\n";
		extMessage +="        <videopid>" + extVideoPID + "</videopid>\n";
		extMessage +="        <audiopids selected=\"" + extAudioPID + "\">\n";
		// super hack :-), der einfachste weg an die apid descriptions ranzukommen
		g_RemoteControl->current_PIDs = pids;
		g_RemoteControl->processAPIDnames();
		bool apidFound=false;
		for(unsigned int i= 0; i< pids.APIDs.size(); i++)
		{
			sprintf(tmp, "%u",  pids.APIDs[i].pid );
			extMessage +="            <audio pid=\"" + string(tmp) + "\" name=\"" + string(g_RemoteControl->current_PIDs.APIDs[i].desc)  + "\">\n";
			if(pids.APIDs[i].pid==apid)
				apidFound=true;
		}
		if(!apidFound)
		{
			// add spec apid to available
			extMessage +="            <audio pid=\"" + extAudioPID + "\" name=\"" + extAudioPID  + "\"/>\n";
		}
		extMessage +="        </audiopids>\n";
		extMessage +="    </record>\n";
		extMessage +="</neutrino>\n";

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
