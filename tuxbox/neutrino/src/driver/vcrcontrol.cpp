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
#include "vcrcontrol.h"

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
int CVCRControl::registerDevice(CVCRDevices deviceType, CDeviceInfo *deviceInfo)
{
	static int i = 0;
	if (deviceType == DEVICE_SERVER)
	{
		CServerDevice * device =  new CServerDevice(i++);		
		CServerDeviceInfo *serverinfo = (CServerDeviceInfo *) deviceInfo;
		device->Name = deviceInfo->Name;
		device->ServerAddress = serverinfo->ServerAddress;
		device->ServerPort = serverinfo->ServerPort;
		Devices[device->deviceID] = (CDevice*) device;
		printf("CVCRControl registered new serverdevice: %u %s\n",device->deviceID,device->Name.c_str());
	}
	else if (deviceType == DEVICE_VCR)
	{
		CVCRDevice * device = new CVCRDevice(i++);
		device->Name = deviceInfo->Name;
		Devices[device->deviceID] = (CDevice*) device;
		printf("CVCRControl registered new vcrdevice: %u %s\n",device->deviceID,device->Name.c_str());
	}
}



//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Stop()
{
	printf("Stop\n");
	if(sendCommand(CMD_VCR_STOP))
	{
		if(!g_Zapit->isPlayBackActive())
			g_Zapit->startPlayBack();
		g_Sectionsd->setPauseScanning(false);
		g_Zapit->setRecordMode( false );
		return true;
	}
	return false;
}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::Record(unsigned onidsid, unsigned long long epgid)	
{
	printf("Record\n");
	if(onidsid != 0)		// wenn ein channel angegeben ist
		if(g_Zapit->getCurrentServiceID() != onidsid)	// und momentan noch nicht getuned ist
			g_Zapit->zapTo_serviceID(onidsid);		// dann umschalten

	if(StopPlayBack && g_Zapit->isPlayBackActive())	// wenn playback gestoppt werden soll und noch läuft
		g_Zapit->stopPlayBack();					// dann playback stoppen

	if(StopSectionsd)								// wenn sectionsd gestoppt werden soll
		g_Sectionsd->setPauseScanning(true);		// sectionsd stoppen

	g_Zapit->setRecordMode( true );					// recordmode einschalten

	return sendCommand(CMD_VCR_RECORD,onidsid,epgid);
}


//-------------------------------------------------------------------------
void CVCRControl::CServerDevice::serverDisconnect()
{
	close(sock_fd);
}

//-------------------------------------------------------------------------
bool CVCRControl::CServerDevice::sendCommand(CVCRCommand command,unsigned onidsid,unsigned long long epgid)
{
	if(serverConnect())
	{
		externalCommand extcommand;

		extcommand.version	= 1;
		extcommand.command	= command;
		extcommand.onidsid	= onidsid;
		extcommand.epgID	= epgid;

		write(sock_fd, &extcommand, sizeof(extcommand));
		
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
