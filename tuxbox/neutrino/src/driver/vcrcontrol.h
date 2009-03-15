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
#include <list>

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
		} CVCRCommand;
	
	enum CVCRDevices
		{
			DEVICE_VCR,
			DEVICE_SERVER,
			DEVICE_FILE
		};

	class CDevice			// basisklasse f�r die devices
		{
		public:
			int sock_fd;
			int last_mode;
			virtual CVCRDevices getDeviceType(void) const = 0;
			CVCRStates  deviceState;
			virtual bool Stop() = 0;
			virtual bool Record(const t_channel_id channel_id = 0, int mode = 1, const event_id_t epgid = 0,
					    const std::string& epgTitle = "", unsigned char apids = 0, const time_t epg_time = 0) = 0;
			virtual bool Pause() = 0;
			virtual bool Resume() = 0;
			virtual bool IsAvailable() = 0;
			CDevice() { deviceState = CMD_VCR_STOP; };
			virtual ~CDevice(){};
			typedef struct {
				unsigned short apid;
				unsigned int index;
				bool ac3;
			} APIDDesc;
			typedef std::list<APIDDesc> APIDList;
			virtual void getAPIDs(const unsigned char apids, APIDList & apid_list);
		};

	class CVCRDevice : public CDevice		// VCR per IR
		{
		public:
			bool  SwitchToScart;
			
			virtual CVCRDevices getDeviceType(void) const
				{
					return DEVICE_VCR;
				};
			virtual bool Stop(); 
			virtual bool Record(const t_channel_id channel_id = 0, int mode = 1, const event_id_t epgid = 0,
					    const std::string& epgTitle = "", unsigned char apids = 0, const time_t epg_time = 0);
			virtual bool Pause();
			virtual bool Resume();
			virtual bool IsAvailable() { return true; };
			CVCRDevice(bool switchtoscart) { SwitchToScart = switchtoscart; };
			virtual ~CVCRDevice(){};
		};

	class CFileAndServerDevice : public CDevice
		{
		protected:
			void RestoreNeutrino(void);
			void CutBackNeutrino(const t_channel_id channel_id, const int mode);
			std::string getCommandString(const CVCRCommand command, const t_channel_id channel_id, const event_id_t epgid, const std::string& epgTitle, unsigned char apids);
			std::string getMovieInfoString(const t_channel_id channel_id, const event_id_t epgid, const time_t epg_time);

		public:
			bool	StopPlayBack;
			int	StopSectionsd;

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
			std::string  FilenameTemplate;
			bool         CreateTemplateDirectories;
			unsigned int SplitSize;
			bool         Use_O_Sync;
			bool         Use_Fdatasync;
			bool         StreamVTxtPid;
			bool         StreamSubtitlePid;
			unsigned int RingBuffers;
				
			virtual CVCRDevices getDeviceType(void) const
				{
					return DEVICE_FILE;
				};
				
			virtual bool Stop(); 
			virtual bool Record(const t_channel_id channel_id = 0, int mode = 1, const event_id_t epgid = 0,
					    const std::string& epgTitle = "", unsigned char apids = 0, const time_t epg_time = 0);
			
			CFileDevice(const bool stopplayback, const int stopsectionsd, const char * const directory, const unsigned int splitsize, const bool use_o_sync, const bool use_fdatasync, const bool stream_vtxt_pid, const bool stream_subtitle_pid, const unsigned int ringbuffers, bool createTemplateDirectories)
				
				: Directory(directory), FilenameTemplate(""), CreateTemplateDirectories(createTemplateDirectories),
				SplitSize(splitsize), Use_O_Sync(use_o_sync), Use_Fdatasync(use_fdatasync),
				StreamVTxtPid(stream_vtxt_pid),
				StreamSubtitlePid(stream_subtitle_pid), RingBuffers(ringbuffers)
			{
				StopPlayBack = stopplayback;
				StopSectionsd = stopsectionsd;
			};
			virtual ~CFileDevice()
			{
			};
		private:
			void appendEPGTitle(char *buf, unsigned int size, const event_id_t epgid, const std::string& epgTitle);
			void appendEPGInfo(char *buf, unsigned int size, const event_id_t epgid);
			void appendChannelName(char *buf, unsigned int size, const t_channel_id channel_id);
			bool createRecordingDir(const char *filename);

		};

	class CServerDevice : public CFileAndServerDevice // externer Streamingserver per tcp
		{
		private:
			bool serverConnect();
			void serverDisconnect();

			bool sendCommand(CVCRCommand command, const t_channel_id channel_id = 0, const event_id_t epgid = 0, const std::string& epgTitle="", unsigned char apids = 0);

		public:
			std::string  ServerAddress;
			unsigned int ServerPort;

			virtual CVCRDevices getDeviceType(void) const
				{
					return DEVICE_SERVER;
				};

			virtual bool Stop();
			virtual bool Record(const t_channel_id channel_id = 0, int mode = 1, const event_id_t epgid = 0,
					    const std::string& epgTitle = "", unsigned char apids = 0, const time_t epg_time = 0);

			CServerDevice(const bool stopplayback, const int stopsectionsd, const char * const serveraddress, const unsigned int serverport)
				{
					StopPlayBack       = stopplayback;
					StopSectionsd      = stopsectionsd;
					ServerAddress      = serveraddress;
					ServerPort         = serverport;
				};
			virtual ~CServerDevice(){};
		};

 public:
	CVCRControl();
	~CVCRControl();
	static CVCRControl * getInstance();

	CDevice * Device;
		
	void registerDevice(CDevice * const device);
	void unregisterDevice();

	inline bool isDeviceRegistered(void) const { return (Device != NULL); };

	inline CVCRStates getDeviceState(void) const { return Device->deviceState; };
	bool Stop(){return Device->Stop();};
	bool Record(const CTimerd::RecordingInfo * const eventinfo);
	bool Pause(){return Device->Pause();};
	bool Resume(){return Device->Resume();};
};


#endif
