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
#ifndef __vcrcontrol__
#define __vcrcontrol__

#include <string>
//#include <map>

#include <sectionsdclient/sectionsdclient.h>
#include <timerdclient/timerdclient.h>

#include <neutrinoMessages.h>


class CVCRControl
{
	public:
		typedef enum CVCRStates 
		{
			CMD_VCR_UNKNOWN =	0,
			CMD_VCR_RECORD	=	1,
			CMD_VCR_STOP	=	2,
			CMD_VCR_PAUSE	=	3,
			CMD_VCR_RESUME	=	4,
			CMD_VCR_AVAILABLE =	5
		}CVCRCommand;

		enum CVCRDevices
		{
			DEVICE_VCR,
			DEVICE_SERVER,
			DEVICE_FILE
		};


		class CDeviceInfo
		{
			public:
				CDeviceInfo(){Name="";};
				std::string   Name;
		};

		class CVCRDeviceInfo : public CDeviceInfo
		{
			public:
				CVCRDeviceInfo() : CDeviceInfo(){};
				bool  SwitchToScart;
		};
		
		class CFileAndServerDeviceInfo : public CDeviceInfo		
		{
			public:
				bool StopPlayBack;
				bool StopSectionsd;
				CFileAndServerDeviceInfo() : CDeviceInfo() { StopPlayBack = false; StopSectionsd = true; };
		};

		class CFileDeviceInfo : public CFileAndServerDeviceInfo		
		{
			public:
				std::string	Directory;
				unsigned int	SplitSize;
				bool            StreamAllAudioPids;
				CFileDeviceInfo() : CFileAndServerDeviceInfo() { Directory = ""; SplitSize = 0; StreamAllAudioPids = true; };
		};

		class CServerDeviceInfo : public CFileAndServerDeviceInfo
		{
			public:
				std::string	ServerAddress;
				unsigned int    ServerPort;
				CServerDeviceInfo() : CFileAndServerDeviceInfo() { ServerAddress = ""; ServerPort = 0; };
		};

	private:

		class CDevice			// basisklasse für die devices
		{
			public:
				int sock_fd;
				int last_mode;
				std::string Name;
				virtual CVCRDevices getDeviceType(void) const = 0;
				CVCRStates  deviceState;
				virtual bool Stop() = 0;
				virtual bool Record(const t_channel_id channel_id = 0, int mode=1, const event_id_t epgid = 0, const std::string & apids = ""){return false;};
				virtual bool Pause() = 0;
				virtual bool Resume() = 0;
				virtual bool IsAvailable() = 0;
				CDevice(){deviceState = CMD_VCR_STOP;};
				virtual ~CDevice(){};
		};

		class CVCRDevice : public CDevice		// VCR per IR
		{
			public:
				bool SwitchToScart;
				virtual CVCRDevices getDeviceType(void) const
					{
						return DEVICE_VCR;
					};
				virtual bool Stop(); 
				virtual bool Record(const t_channel_id channel_id = 0, int mode=1, const event_id_t epgid = 0, const std::string & apids = "");	
				virtual bool Pause();
				virtual bool Resume();
				virtual bool IsAvailable(){return true;};
				CVCRDevice() {};
				virtual ~CVCRDevice(){};
		};

		class CFileAndServerDevice : public CDevice
			{
			protected:
				void RestoreNeutrino(void);
				void CutBackNeutrino(const t_channel_id channel_id, const int mode);
				std::string getCommandString(const CVCRCommand command, const t_channel_id channel_id, const event_id_t epgid, const std::string & apids) const;

			public:
				bool	StopPlayBack;
				bool	StopSectionsd;

				virtual bool Pause()
					{
						return false;
					};
				virtual bool Resume()
					{
						return false;
					};

				virtual bool IsAvailable()
					{
						return true;
					};
			};

		class CFileDevice : public CFileAndServerDevice
			{
			public:
				std::string  Directory;
				unsigned int SplitSize;
				bool         StreamAllAudioPids;
				
				virtual CVCRDevices getDeviceType(void) const
					{
						return DEVICE_FILE;
					};
				
				virtual bool Stop(); 
				virtual bool Record(const t_channel_id channel_id = 0, int mode=1, const event_id_t epgid = 0, const std::string & apids = "");	
				
				CFileDevice()
					{
					};
				virtual ~CFileDevice()
					{
					};
			};

		class CServerDevice : public CFileAndServerDevice // externer Streamingserver per tcp
		{
			private:
				bool serverConnect();
				void serverDisconnect();

				bool sendCommand(CVCRCommand command, const t_channel_id channel_id = 0, const event_id_t epgid = 0, const std::string & apids = "");

			public:
				std::string  ServerAddress;
				unsigned int ServerPort;

				virtual CVCRDevices getDeviceType(void) const
					{
						return DEVICE_SERVER;
					};

				virtual bool Stop();
				virtual bool Record(const t_channel_id channel_id = 0, int mode=1, const event_id_t epgid = 0, const std::string & apids = "");

				CServerDevice() {};			
				virtual ~CServerDevice(){};
		};

//		typedef std::map<int, CDevice*> CDeviceMap;
	

	public:
		CVCRControl();
		~CVCRControl();
		static CVCRControl* getInstance();

//		CDeviceMap Devices;
		CDevice *Device;
		
		bool registerDevice(CVCRDevices deviceType, CDeviceInfo *deviceInfo);
		void unregisterDevice();

//		int registeredDevices(){return Devices.size();};
		bool isDeviceRegistered(){return Device != NULL;};
		void setDeviceOptions(CDeviceInfo *deviceInfo);

		int getDeviceState(){return Device->deviceState;};
		bool Stop(){return Device->Stop();};
		bool Record(const CTimerd::RecordingInfo * const eventinfo);
		bool Pause(){return Device->Pause();};
		bool Resume(){return Device->Resume();};
};


#endif
