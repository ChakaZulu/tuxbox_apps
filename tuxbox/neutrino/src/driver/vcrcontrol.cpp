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

#include "global.h"
#include "gui/widget/messagebox.h"

#include "vcrcontrol.h"
#include "sectionsdclient.h"

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
	Devices.clear();
}

//-------------------------------------------------------------------------
CVCRControl::~CVCRControl()
{
	if(Devices.size() > 0)
	{
		for(CDeviceMap::iterator pos = Devices.begin();pos != Devices.end();pos++)
		{
			delete pos->second;
			Devices.erase(pos->first);
		}
	}

}
//-------------------------------------------------------------------------

void CVCRControl::setDeviceOptions(int deviceID, CDeviceInfo *deviceInfo)
{
	if(Devices[deviceID]->deviceType == DEVICE_SERVER)
	{
		CServerDevice * device = (CServerDevice *) Devices[deviceID];		
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
//-------------------------------------------------------------------------

int CVCRControl::registerDevice(CVCRDevices deviceType, CDeviceInfo *deviceInfo)
{
	static int i = 0;
	if (deviceType == DEVICE_SERVER)
	{
		CServerDevice * device =  new CServerDevice(i++);		
//		CServerDeviceInfo *serverinfo = (CServerDeviceInfo *) deviceInfo;
		Devices[device->deviceID] = (CDevice*) device;
		setDeviceOptions(device->deviceID,deviceInfo);
/*
		device->Name = deviceInfo->Name;
		device->ServerAddress = serverinfo->ServerAddress;
		device->ServerPort = serverinfo->ServerPort;
		device->StopPlayBack = serverinfo->StopPlayBack;
		device->StopSectionsd = serverinfo->StopSectionsd;
*/
		printf("CVCRControl registered new serverdevice: %u %s\n",device->deviceID,device->Name.c_str());
		return device->deviceID;
	}
	else if (deviceType == DEVICE_VCR)
	{
		CVCRDevice * device = new CVCRDevice(i++);
		device->Name = deviceInfo->Name;
		Devices[device->deviceID] = (CDevice*) device;
		printf("CVCRControl registered new vcrdevice: %u %s\n",device->deviceID,device->Name.c_str());
		return device->deviceID;
	}
	else
		return -1;
}



//-------------------------------------------------------------------------
void CVCRControl::CVCRDevice::IRDeviceDisconnect()
{
	close(sock_fd);
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::sendCommand(std::string command,unsigned onidsid,unsigned long long epgid)
{
	if(IRDeviceConnect())
	{
		std::stringstream ostr;
		ostr << "SEND_ONCE " << Name << " " << command << std::endl << std::ends;
		write(sock_fd, ostr.str().c_str(), ostr.str().length());
		IRDeviceDisconnect();
		return true;
	}
	else
		return false;
}
//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::IRDeviceConnect()
{
	struct sockaddr_un addr;

	addr.sun_family=AF_UNIX;
	strcpy(addr.sun_path, "/dev/lircd");
	sock_fd = socket(AF_UNIX,SOCK_STREAM,0);
	if(!sock_fd)
	{
		printf("could not open lircd-socket\n");
		return false;
	};

	if(!connect(sock_fd,(struct sockaddr *)&addr,sizeof(addr))==-1)
	{
		printf("could not connect to lircd-socket\n");
		return false;
	};
	return true;

}
//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Stop()
{
	return true;
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Record(unsigned onidsid, unsigned long long epgid)	
{
	return true;
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Pause()
{
	return true;
}

//-------------------------------------------------------------------------
bool CVCRControl::CVCRDevice::Resume()
{
	return true;
}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Stop()
{
	printf("Stop\n"); 
	if(!g_Zapit->isPlayBackActive())
		g_Zapit->startPlayBack();
	g_Sectionsd->setPauseScanning(false);
	g_Zapit->setRecordMode( false );
	if(sendCommand(CMD_VCR_STOP))
		return true;
	else
		return false;
}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Record(unsigned onidsid, unsigned long long epgid)	
{
	printf("Record onidsid: %x epg: %llx\n",onidsid,epgid);
	if(onidsid != 0)		// wenn ein channel angegeben ist
		if(g_Zapit->getCurrentServiceID() != onidsid)	// und momentan noch nicht getuned ist
			g_Zapit->zapTo_serviceID(onidsid);		// dann umschalten

	if(StopPlayBack && g_Zapit->isPlayBackActive())	// wenn playback gestoppt werden soll und noch läuft
		g_Zapit->stopPlayBack();					// dann playback stoppen

	if(StopSectionsd)								// wenn sectionsd gestoppt werden soll
		g_Sectionsd->setPauseScanning(true);		// sectionsd stoppen

	g_Zapit->setRecordMode( true );					// recordmode einschalten

	if(!sendCommand(CMD_VCR_RECORD,onidsid,epgid))
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
bool CVCRControl::CServerDevice::sendCommand(CVCRCommand command,unsigned onidsid,unsigned long long epgid)
{
	printf("Send command: %d onidsid: %x epgid: %llx\n",command, onidsid, epgid);
	if(serverConnect())
	{
		char tmp[40];
		string extCommand="unknown";
		string extOnidsid="error";
		string extEpgid="error";
		string extVideoPID="error";
		string extAudioPID="error";
		string extEPGTitle="not available";
		sprintf(tmp,"%u", onidsid);
		extOnidsid = tmp;
		sprintf(tmp,"%llu", epgid);
		extEpgid = tmp;
		sprintf(tmp,"%u", g_RemoteControl->current_PIDs.PIDs.vpid );
		extVideoPID = tmp;
		sprintf(tmp,"%u", g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
		extAudioPID = tmp;

		CSectionsdClient sections;
		sectionsd::responseGetCurrentNextInfoChannelID current_next;
		if(sections.getCurrentNextServiceKey(onidsid, current_next))
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
		extMessage +="        <channelname>" + g_RemoteControl->current_channel_name + "</channelname>\n";
		extMessage +="        <epgtitle>" + extEPGTitle + "</epgtitle>\n";
		extMessage +="        <onidsid>" + extOnidsid + "</onidsid>\n";
		extMessage +="        <epgid>" + extEpgid + "</epgid>\n";
		extMessage +="        <videopid>" + extVideoPID + "</videopid>\n";
		extMessage +="        <audiopids selected=\"" + extAudioPID + "\">\n";
		for (unsigned int i= 0; i< g_RemoteControl->current_PIDs.APIDs.size(); i++)
		{
			sprintf(tmp, "%u",  g_RemoteControl->current_PIDs.APIDs[i].pid );
			extMessage +="            <audio pid=\"" + string(tmp) + "\" name=\"" + string(g_RemoteControl->current_PIDs.APIDs[i].desc)  + "\">\n";
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
