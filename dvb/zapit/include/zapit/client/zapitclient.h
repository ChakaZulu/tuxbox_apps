/*
  Client-Interface für zapit  -   DBoxII-Project

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

#ifndef __zapitclient__
#define __zapitclient__


#include <string>
#include <vector>

/* zapit */
#include "zapittypes.h"
#include "basicclient.h"


#define ZAPIT_UDS_NAME "/tmp/zapit.sock"


class CZapitClient:public CBasicClient
{
		inline bool zapit_connect();
		inline void zapit_close();

	public:
		static const char ACTVERSION = 3;

		enum commands
		{
			CMD_ZAPTO		 = 1,
			CMD_ZAPTO_CHANNELNR,
			CMD_STOP_VIDEO,						// not supported yet
			CMD_SET_MODE,
			CMD_GET_LAST_CHANNEL,
			CMD_GET_APID_VPID,					// not supported yet
			CMD_GET_VTXT_PID,					// not supported yet
			CMD_GET_NVOD_CHANNELS,				// not supported yet
			CMD_REINIT_CHANNELS,
			CMD_GET_CHANNELS,
			CMD_GET_BOUQUETS,
			CMD_GET_BOUQUET_CHANNELS,
			CMD_RESTORE_BOUQUETS,
			CMD_GET_CA_INFO,					// not supported yet
			CMD_GET_CURRENT_SERVICEID,
			CMD_GET_CURRENT_SERVICEINFO,

			CMD_SCANSTART,
			CMD_SCANREADY,
			CMD_SCANGETSATLIST,
			CMD_SCANSETSCANSATLIST,
			CMD_SCANSETDISEQCTYPE,
			CMD_SCANSETDISEQCREPEAT,
			CMD_SCANSETBOUQUETMODE,

			CMD_BQ_ADD_BOUQUET,
			CMD_BQ_MOVE_BOUQUET,
			CMD_BQ_MOVE_CHANNEL,
			CMD_BQ_DELETE_BOUQUET,
			CMD_BQ_RENAME_BOUQUET,
			CMD_BQ_EXISTS_BOUQUET,					// Check if BouquetName existiert
			CMD_BQ_SET_LOCKSTATE,
			CMD_BQ_SET_HIDDENSTATE,
			CMD_BQ_ADD_CHANNEL_TO_BOUQUET,
			CMD_BQ_EXISTS_CHANNEL_IN_BOUQUET,			// Check if Channel already in BQ
			CMD_BQ_REMOVE_CHANNEL_FROM_BOUQUET,
			CMD_BQ_RENUM_CHANNELLIST,
			CMD_BQ_SAVE_BOUQUETS,

			CMD_SET_RECORD_MODE,
			CMD_GET_RECORD_MODE,
			CMD_SB_START_PLAYBACK,
			CMD_SB_STOP_PLAYBACK,
			CMD_SB_GET_PLAYBACK_ACTIVE,
			CMD_SET_DISPLAY_FORMAT,
			CMD_SET_AUDIO_MODE,
			CMD_READY,
			CMD_GETPIDS,
			CMD_SETSUBSERVICES,
			CMD_ZAPTO_SERVICEID,
			CMD_ZAPTO_SUBSERVICEID,
			CMD_ZAPTO_SERVICEID_NOWAIT,
			CMD_ZAPTO_SUBSERVICEID_NOWAIT,
			CMD_SET_AUDIOCHAN,
			CMD_REGISTEREVENTS,
			CMD_UNREGISTEREVENTS,
			CMD_MUTE,
			CMD_SET_VOLUME,
			CMD_COMMIT_BOUQUET_CHANGE
		};

		enum events
		{
			EVT_ZAP_FAILED,
			EVT_ZAP_COMPLETE,
			EVT_ZAP_COMPLETE_IS_NVOD,
			EVT_ZAP_SUB_COMPLETE,
			EVT_ZAP_SUB_FAILED,
			EVT_SCAN_COMPLETE,
			EVT_SCAN_NUM_TRANSPONDERS,
			EVT_SCAN_SATELLITE,
			EVT_SCAN_NUM_CHANNELS,
			EVT_SCAN_PROVIDER,
			EVT_RECORDMODE_ACTIVATED,
			EVT_RECORDMODE_DEACTIVATED,
			EVT_BOUQUETS_CHANGED
		};

		enum zapStatus
		{
			ZAP_OK = 0x01,
			ZAP_IS_NVOD = 0x02,
			ZAP_INVALID_PARAM = 0x04
		};

		enum bouquetMode
		{
			BM_CREATEBOUQUETS,
			BM_DELETEBOUQUETS,
			BM_DONTTOUCHBOUQUETS,
			BM_UPDATEBOUQUETS	 // not yet supported
		};


		//command structures
		struct commandHead
		{
			unsigned char  version;
			unsigned char  cmd;
		};

		struct commandBoolean
		{
			bool truefalse;
		};

		struct commandInt
		{
			int val;
		};

		struct commandVolume
		{
			unsigned int left;
			unsigned int right;
		};

		struct commandSetRecordMode
		{
			bool activate;
		};

		struct commandZapto
		{
			unsigned int bouquet;
			unsigned int channel;
		};

		struct commandZaptoChannelNr
		{
			unsigned int channel;
		};

		struct commandZaptoServiceID
		{
			t_channel_id channel_id;
		};

		struct commandSetAudioChannel
		{
			unsigned int channel;
		};

		struct commandGetBouquets
		{
			bool emptyBouquetsToo;
		};

		typedef enum channelsMode_
		{
			MODE_CURRENT,
			MODE_TV,
			MODE_RADIO
		} channelsMode;

		struct commandSetMode
		{
			channelsMode mode;
		};

		struct commandGetBouquetChannels
		{
			unsigned int bouquet;
			channelsMode mode;
		};

		typedef enum channelsOrder_
		{
			SORT_ALPHA,
			SORT_BOUQUET
		} channelsOrder;

		struct commandGetChannels
		{
			channelsMode  mode;
			channelsOrder order;
		};

		struct commandAddBouquet
		{
			char name[30];
		};

		struct commandExistsBouquet
		{
			char name[30];
		};

		struct commandExistsChannelInBouquet
		{
			unsigned int bouquet;
			t_channel_id channel_id;
		};


		struct commandAddChannelToBouquet
		{
			unsigned int bouquet;
			t_channel_id channel_id;
		};

		struct commandRemoveChannelFromBouquet
		{
			unsigned int bouquet;
			t_channel_id channel_id;
		};

		struct commandDeleteBouquet
		{
			unsigned int bouquet;
		};

		struct commandRenameBouquet
		{
			unsigned int bouquet;
			char name[30];
		};

		struct commandMoveBouquet
		{
			unsigned int bouquet;
			unsigned int newPos;
		};

		struct commandStartScan
		{
			unsigned int satelliteMask;
		};

		struct commandBouquetState
		{
			unsigned int bouquet;
			bool	 state;
		};

		struct commandMoveChannel
		{
			unsigned int bouquet;
			unsigned int oldPos;
			unsigned int newPos;
			channelsMode  mode;
		};



		struct responseGeneralTrueFalse		// 2002-04-02 rasc
		{
			bool status;
		};

		struct responseGeneralInteger		// 2002-04-03 rasc
		{
			int number;
		};

		struct responseGetRecordModeState
		{
			bool activated;
		};

		struct responseGetPlaybackState
		{
			bool activated;
		};

		struct responseGetCurrentServiceID
		{
			t_channel_id channel_id;
		};

		struct responseZapComplete
		{
			unsigned int zapStatus;
		};

		struct responseGetBouquets
		{
			unsigned int bouquet_nr;
			char	 name[30];
			bool	 locked;
			bool	 hidden;
		};

		typedef std::vector<responseGetBouquets> BouquetList;

		struct responseChannels
		{
			unsigned int nr;
			t_channel_id channel_id;
			char	 name[30];
		};

		struct responseGetBouquetChannels : public responseChannels
		{};

		typedef std::vector<responseGetBouquetChannels> BouquetChannelList;

		struct responseIsScanReady
		{
			bool scanReady;
			unsigned int satellite;
			unsigned int transponder;
			unsigned int services;
		};

		struct responseCmd
		{
			unsigned char cmd;
		};

		struct responseGetAPIDs
		{
			uint    pid;
		char    desc[25];
		int     is_ac3;
		int     component_tag;
		};

		typedef std::vector<responseGetAPIDs> APIDList;

		struct responseGetOtherPIDs
		{
		uint		vpid;
		uint		ecmpid;
		uint		vtxtpid;
			uint		pcrpid;
			uint		selected_apid;
		};

		class CCurrentServiceInfo
		{
			public:
				t_original_network_id onid;
				t_service_id           sid;
				t_transport_stream_id tsid;
				unsigned short	vdid;
				unsigned short	apid;
				unsigned short	pcrpid;
				unsigned short	vtxtpid;
				unsigned int	tsfrequency;
				unsigned char	polarisation;
				unsigned char	diseqc;
		};

		struct responseGetPIDs
		{
			responseGetOtherPIDs	PIDs;
			APIDList		APIDs;
		};

		struct commandAddSubServices
		{
			t_original_network_id original_network_id;
			t_service_id          service_id;
			t_transport_stream_id transport_stream_id;
		};

		struct responseGetLastChannel
		{
			unsigned int	channelNumber;
			char		mode;
		};

		struct responseGetSatelliteList
		{
			char satName[30];
		};
		typedef std::vector<responseGetSatelliteList> SatelliteList;

		struct commandSetScanSatelliteList
		{
			char satName[30];
			int  diseqc;
		};
		typedef std::vector<commandSetScanSatelliteList> ScanSatelliteList;

		typedef std::vector<commandAddSubServices> subServiceList;

	protected:
		void send(const commands command, char* data, const unsigned int size);

	public:

		/****************************************/
		/*					*/
		/* general functions for zapping	*/
		/*					*/
		/****************************************/

		/* zaps to channel of specifeid bouquet */
		void zapTo( unsigned int bouquet, unsigned int channel );

		/* zaps to channel  */
		void zapTo( unsigned int channel );

		/* zaps to channel, returns the "zap-status" */
		unsigned int zapTo_serviceID(const t_channel_id channel_id);

		/* zaps to subservice, returns the "zap-status" */
		unsigned int zapTo_subServiceID(const t_channel_id channel_id);

		/* zaps to channel, does NOT wait for completion (uses event) */
		void zapTo_serviceID_NOWAIT(const t_channel_id channel_id);

		/* zaps to subservice, does NOT wait for completion (uses event) */
		void zapTo_subServiceID_NOWAIT(const t_channel_id channel_id);

		/* return the current (tuned) ServiceID */
		t_channel_id getCurrentServiceID();

		/* get last channel-information */
		void getLastChannel(unsigned int &channumber, char &mode);

		/* audiochan set */
		void setAudioChannel( unsigned channel );

		/* gets all bouquets */
		void getBouquets( BouquetList& bouquets, bool emptyBouquetsToo = false);

		/* gets all channels that are in specified bouquet */
		void getBouquetChannels( unsigned int bouquet, BouquetChannelList& channels, channelsMode mode = MODE_CURRENT);

		/* gets all channels */
		void getChannels( BouquetChannelList& channels, channelsMode mode = MODE_CURRENT, channelsOrder order = SORT_BOUQUET);

		/* restore bouquets so as if they where just loaded*/
		void restoreBouquets();

		/* reloads channels and services*/
		void reinitChannels();

		/* get current APID-List */
		void getPIDS( responseGetPIDs& pids );

		/* get info about the current serivice */
		CZapitClient::CCurrentServiceInfo getCurrentServiceInfo();

		/* transfer SubService-List to zapit */
		void setSubServices( subServiceList& subServices );

		/* set Mode */
		void setMode( channelsMode mode );

		/* set RecordMode*/
		void setRecordMode( bool activate );

		/* get RecordMode*/
		bool isRecordModeActive();

		/* mute audio */
		void muteAudio( bool mute );

		/* set audio volume */
		void setVolume( unsigned int left, unsigned int right );


		/****************************************/
		/*					*/
		/* Scanning stuff			*/
		/*					*/
		/****************************************/
		/* start TS-Scan */
		bool startScan();

		/* query if ts-scan is ready - response gives status */
		bool isScanReady(unsigned int &satellite, unsigned int &transponder, unsigned int &services );

		/* query possible satellits*/
		void getScanSatelliteList( SatelliteList& satelliteList );

		/* tell zapit which satellites to scan*/
		void setScanSatelliteList( ScanSatelliteList& satelliteList );

		/* set diseqcType*/
		void setDiseqcType( diseqc_t diseqc);

		/* set diseqcRepeat*/
		void setDiseqcRepeat( uint32_t repeat);

		/* set diseqcRepeat*/
		void setScanBouquetMode( bouquetMode mode);

		/****************************************/
		/*					*/
		/* Bouquet editing functions		*/
		/*					*/
		/****************************************/

		/* adds bouquet at the end of the bouquetlist*/
		void addBouquet(std::string name);

		/* moves a bouquet from one position to another, bouquet list begins at position=1*/
		void moveBouquet( unsigned int bouquet, unsigned int newPos);

		/* moves a channel of a bouquet from one position to another, channel lists begin at position=1*/
		void moveChannel( unsigned int bouquet, unsigned int oldPos, unsigned int newPos, channelsMode mode = MODE_CURRENT);

		/* deletes a bouquet with all its channels*/
		void deleteBouquet( unsigned int bouquet);

		/* assigns new name to bouquet*/
		void renameBouquet( unsigned int bouquet, std::string newName);

		// -- check if Bouquet-Name exists (2002-04-02 rasc)
		// -- Return bq_id or 0
		unsigned int  existsBouquet(std::string name);


		// -- check if Channel already in Bouquet (2002-04-05 rasc)
		// -- Return true/false
		bool  existsChannelInBouquet(const unsigned int bouquet, const t_channel_id channel_id);


		/* adds a channel at the end of then channel list to specified bouquet */
		/* same channels can be in more than one bouquet */
		/* bouquets can contain both tv and radio channels */
		void addChannelToBouquet(const unsigned int bouquet, const t_channel_id channel_id);

		/* removes a channel from specified bouquet */
		void removeChannelFromBouquet(const unsigned int bouquet, const t_channel_id channel_id);

		/* set a bouquet's lock-state*/
		void setBouquetLock( unsigned int bouquet, bool lock);

		/* set a bouquet's hidden-state*/
		void setBouquetHidden( unsigned int bouquet, bool hidden);

		/* renums the channellist, means gives the channels new numbers */
		/* based on the bouquet order and their order within bouquets */
		/* necessarily after bouquet editing operations*/
		void renumChannellist();

		/* saves current bouquet configuration to bouquets.xml*/
		void saveBouquets();

		/* commit bouquet change */
		void commitBouquetChange();

		/****************************************/
		/*					*/
		/* blah functions			*/
		/*					*/
		/****************************************/

		void startPlayBack();
		void stopPlayBack();
		bool isPlayBackActive();
		void setDisplayFormat(int mode);
		void setAudioMode(int mode);

		/****************************************/
		/*					*/
		/* Event functions			*/
		/*					*/
		/****************************************/

		/*
			ein beliebiges Event anmelden
		*/
		void registerEvent(unsigned int eventID, unsigned int clientID, std::string udsName);

		/*
			ein beliebiges Event abmelden
		*/
		void unRegisterEvent(unsigned int eventID, unsigned int clientID);

};




#endif
