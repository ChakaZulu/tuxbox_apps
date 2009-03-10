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

#include <daemonc/remotecontrol.h>

#include <global.h>
#include <neutrino.h>

#include <driver/encoding.h>


CSubService::CSubService(const t_original_network_id anoriginal_network_id, const t_service_id aservice_id, const t_transport_stream_id atransport_stream_id, const std::string &asubservice_name)
{
	service.original_network_id = anoriginal_network_id;
	service.service_id          = aservice_id;
	service.transport_stream_id = atransport_stream_id;
	startzeit                   = 0;
	dauer                       = 0;
	subservice_name             = asubservice_name;
}

CSubService::CSubService(const t_original_network_id anoriginal_network_id, const t_service_id aservice_id, const t_transport_stream_id atransport_stream_id, const time_t astartzeit, const unsigned adauer)
{
	service.original_network_id = anoriginal_network_id;
	service.service_id          = aservice_id;
	service.transport_stream_id = atransport_stream_id;
	startzeit                   = astartzeit;
	dauer                       = adauer;
	subservice_name             = "";
}

t_channel_id CSubService::getChannelID(void) const
{
	return CREATE_CHANNEL_ID_FROM_SERVICE_ORIGINALNETWORK_TRANSPORTSTREAM_ID(service.service_id, service.original_network_id, service.transport_stream_id);
}


CRemoteControl::CRemoteControl()
{
	current_channel_id = 	0;
	current_sub_channel_id = 0;
	current_channel_name = 	"";

	zap_completion_timeout = 0;

	current_EPGid =	0;
	next_EPGid = 	0;
	memset(&current_PIDs.PIDs, 0, sizeof(current_PIDs.PIDs) );
	has_ac3 = 	false;
	selected_subchannel = -1;
	needs_nvods = 	false;
	director_mode = 0;
//	current_programm_timer = 0;
	is_video_started = true;
}

int CRemoteControl::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	if ( zap_completion_timeout != 0 )
	{
		// warte auf Meldung vom ZAPIT
    		if ((msg == NeutrinoMessages::EVT_ZAP_COMPLETE) ||
		    (msg == NeutrinoMessages::EVT_ZAP_FAILED  ) ||
		    (msg == NeutrinoMessages::EVT_ZAP_ISNVOD  ))
		{
			if ((*(t_channel_id *)data) != current_channel_id)
			{
 				g_Zapit->zapTo_serviceID_NOWAIT(current_channel_id );
				g_Sectionsd->setServiceChanged(current_channel_id, false);

				zap_completion_timeout = getcurrenttime() + 2 * (long long) 1000000;

				return messages_return::handled;
			}
			else
				zap_completion_timeout = 0;
		}
	}
	else
	{
		if ((msg == NeutrinoMessages::EVT_ZAP_COMPLETE) ||
		    (msg == NeutrinoMessages::EVT_ZAP_FAILED  ) ||
		    (msg == NeutrinoMessages::EVT_ZAP_ISNVOD  ))
		{
			// warte auf keine Meldung vom ZAPIT -> jemand anderer hat das zappen ausgel�st...
			if ((*(t_channel_id *)data) != current_channel_id)
			{
				current_channel_id = *(t_channel_id *)data;
				current_channel_name =g_Zapit->getChannelName(current_channel_id);
				CLCD::getInstance()->showServicename(current_channel_name); // UTF-8
				is_video_started= true;

				current_EPGid = 0;
				next_EPGid = 0;

				memset(&current_PIDs.PIDs, 0, sizeof(current_PIDs.PIDs) );

				current_PIDs.APIDs.clear();
				has_ac3 = false;

				subChannels.clear();
				selected_subchannel = -1;
				director_mode = 0;
				needs_nvods = (msg == NeutrinoMessages:: EVT_ZAP_ISNVOD);

				g_Sectionsd->setServiceChanged( current_channel_id, true );
				CNeutrinoApp::getInstance()->channelList->adjustToChannelID(current_channel_id);
				if ( g_InfoViewer->is_visible )
					g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR , 0 );
			}
		}
		else
			if ((msg == NeutrinoMessages::EVT_ZAP_SUB_COMPLETE) ||
			    (msg == NeutrinoMessages::EVT_ZAP_SUB_FAILED))
			{
				if ((*(t_channel_id *)data) != current_sub_channel_id)
				{
					current_sub_channel_id = *(t_channel_id *)data;

					for(unsigned int i = 0; i < subChannels.size(); i++)
					if (subChannels[i].getChannelID() == (*(t_channel_id *)data))
					{
						selected_subchannel = i;
						break;
					}
				}
			}
	}

    if ( msg == NeutrinoMessages::EVT_CURRENTEPG )
	{
		CSectionsdClient::CurrentNextInfo* info_CN = (CSectionsdClient::CurrentNextInfo*) data;

//		printf("Current/Next channelID: old(%llx) -> new(%llx)\n", current_channel_id, info_CN->current_uniqueKey >> 16);
		if ( ((info_CN->current_uniqueKey >> 16) == current_channel_id ) || ((info_CN->current_uniqueKey >> 16) == current_sub_channel_id ) )
		{
			//CURRENT-EPG f�r den aktuellen Kanal bekommen!;
//			printf("Current/Next EPGID: old(%llx) -> new(%llx)\n", current_EPGid, info_CN->current_uniqueKey);
			CLCD::getInstance()->setEPGTitle(info_CN->current_name);
			if ( info_CN->current_uniqueKey != current_EPGid )
			{
			    if ( current_EPGid != 0 )
			    {
			    	// ist nur ein neues Programm, kein neuer Kanal

			    	// PIDs neu holen
			    	g_Zapit->getPIDS( current_PIDs );

			    	// APID Bearbeitung neu anstossen
			    	has_unresolved_ctags = true;

					// infobar indicate on epg change 
					g_InfoViewer->showEpgInfo();                            
                }

				current_EPGid= info_CN->current_uniqueKey;

				if ( has_unresolved_ctags )
					processAPIDnames();

				if ( info_CN->flags & CSectionsdClient::epgflags::current_has_linkagedescriptors ){
					subChannels.clear();
					getSubChannels();
				}

				if ( needs_nvods )
					getNVODs();

#if 0
				if ( current_programm_timer != 0 )
					g_RCInput->killTimer( current_programm_timer );

				time_t end_program= info_CN->current_zeit.startzeit+ info_CN->current_zeit.dauer;
				current_programm_timer = g_RCInput->addTimer( &end_program );
#endif

				// is_video_started is only false if channel is locked
				if (((!is_video_started) && (info_CN->current_fsk == 0)) || ((!is_video_started) && (g_settings.parentallock_prompt == PARENTALLOCK_PROMPT_CHANGETOLOCKED)))
					g_RCInput->postMsg( NeutrinoMessages::EVT_PROGRAMLOCKSTATUS, 0x100, false );
				else
					g_RCInput->postMsg( NeutrinoMessages::EVT_PROGRAMLOCKSTATUS, info_CN->current_fsk, false );
			}
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_NEXTEPG )
	{
		CSectionsdClient::CurrentNextInfo* info_CN = (CSectionsdClient::CurrentNextInfo*) data;

		if ( ( info_CN->next_uniqueKey >> 16) == current_channel_id )
		{
			// next-EPG f�r den aktuellen Kanal bekommen, current ist leider net da?!;
			if ( info_CN->next_uniqueKey != next_EPGid )
			{
			    next_EPGid= info_CN->next_uniqueKey;

				// timer setzen

#if 0
	        	if ( current_programm_timer != 0 )
					g_RCInput->killTimer( current_programm_timer );

				time_t end_program= info_CN->next_zeit.startzeit;
				current_programm_timer = g_RCInput->addTimer( &end_program );
#endif
			}
		}
		if ( !is_video_started )
			g_RCInput->postMsg( NeutrinoMessages::EVT_PROGRAMLOCKSTATUS, 0x100, false );

	    return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_NOEPG_YET)
	{
		if ((*(t_channel_id *)data) == current_channel_id)
		{
			if ( !is_video_started )
				g_RCInput->postMsg( NeutrinoMessages::EVT_PROGRAMLOCKSTATUS, 0x100, false );
		}
		return messages_return::handled;
	}
	else if ((msg == NeutrinoMessages::EVT_ZAP_COMPLETE    ) ||
		 (msg == NeutrinoMessages::EVT_ZAP_SUB_COMPLETE))
	{
		if ((*(t_channel_id *)data) == ((msg == NeutrinoMessages::EVT_ZAP_COMPLETE) ? current_channel_id : current_sub_channel_id))
		{
			CLCD::getInstance()->showServicename(current_channel_name); // UTF-8
			g_Zapit->getPIDS( current_PIDs );
			g_Sectionsd->setPrivatePid( current_PIDs.PIDs.privatepid );

			char *p = new char[sizeof(t_channel_id)];
			memcpy(p, &current_channel_id, sizeof(t_channel_id));
			g_RCInput->postMsg(NeutrinoMessages::EVT_ZAP_GOTPIDS, (const neutrino_msg_data_t)p, false); // data is pointer to allocated memory

			processAPIDnames();
		}
	    return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_ZAP_ISNVOD)
	{
		if ((*(t_channel_id *)data) == current_channel_id)
		{
			needs_nvods = true;
			CLCD::getInstance()->showServicename(std::string("[") + current_channel_name + ']'); // UTF-8
			if ( current_EPGid != 0)
			{
				getNVODs();
				if (subChannels.empty())
					g_Sectionsd->setServiceChanged( current_channel_id, true );
			}
			else
				// EVENT anfordern!
				g_Sectionsd->setServiceChanged( current_channel_id, true );

		}
		return messages_return::handled;
	}
#if 0
	else if ( ( msg == NeutrinoMessages::EVT_TIMER ) && ( data == current_programm_timer ) )
	{
		//printf("new program !\n");

		t_channel_id * p = new t_channel_id;
		*p = current_channel_id;
		g_RCInput->postMsg(NeutrinoMessages::EVT_NEXTPROGRAM, (const neutrino_msg_data_t)p, false); // data is pointer to allocated memory

 		return messages_return::handled;
	}
#endif
	else
		return messages_return::unhandled;
}

void CRemoteControl::getSubChannels()
{
// check for updates of the subchannels
//	if ( subChannels.size() == 0 )
	{
		CSectionsdClient::LinkageDescriptorList	linkedServices;
		if ( g_Sectionsd->getLinkageDescriptorsUniqueKey( current_EPGid, linkedServices ) )
		{
			if ( linkedServices.size()> 1 )
			{
				t_channel_id searchkey;
				are_subchannels = true;
				if (current_sub_channel_id>0) 	searchkey = current_sub_channel_id;
				else				searchkey = current_channel_id;

				for (unsigned int i=0; i< linkedServices.size(); i++)
				{
					subChannels.push_back(CSubService(
								      linkedServices[i].originalNetworkId,
								      linkedServices[i].serviceId,
								      linkedServices[i].transportStreamId,
								      linkedServices[i].name));
					if (subChannels[i].getChannelID() == searchkey )
						selected_subchannel = i;
				}
				copySubChannelsToZapit();

				char *p = new char[sizeof(t_channel_id)];
				memcpy(p, &current_channel_id, sizeof(t_channel_id));
				g_RCInput->postMsg(NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES, (const neutrino_msg_data_t)p, false); // data is pointer to allocated memory
			}
		}
	}
}

void CRemoteControl::getNVODs()
{
	if ( subChannels.size() == 0 )
	{
		CSectionsdClient::NVODTimesList	NVODs;
		if ( g_Sectionsd->getNVODTimesServiceKey( current_channel_id, NVODs ) )
		{
			are_subchannels = false;
			for (unsigned int i=0; i< NVODs.size(); i++)
			{
				if ( NVODs[i].zeit.dauer> 0 )
				{
					CSubService newService(
						NVODs[i].original_network_id,
						NVODs[i].service_id,
						NVODs[i].transport_stream_id,
						NVODs[i].zeit.startzeit, 
						NVODs[i].zeit.dauer);

					CSubServiceListSorted::iterator e= subChannels.begin();
					for(; e!=subChannels.end(); ++e)
					{
						if ( e->startzeit > newService.startzeit )
							break;
					}
					subChannels.insert( e, newService );
				}

			}

			copySubChannelsToZapit();

			char *p = new char[sizeof(t_channel_id)];
			memcpy(p, &current_channel_id, sizeof(t_channel_id));
			g_RCInput->postMsg(NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES, (const neutrino_msg_data_t)p, false); // data is pointer to allocated memory

			if ( selected_subchannel == -1 )
			{
				// beim ersten Holen letzten NVOD-Kanal setzen!
				setSubChannel( subChannels.size()- 1 );
			}
			else
			{
				// sollte nur passieren, wenn die aktuelle Sendung vorbei ist?!
				selected_subchannel = -1;
			}
		}
	}
}

void CRemoteControl::processAPIDnames()
{
	has_unresolved_ctags= false;
	has_ac3 = false;

	for(unsigned int count=0; count< current_PIDs.APIDs.size(); count++)
	{
		if ( current_PIDs.APIDs[count].component_tag != 0xFF )
		{
			has_unresolved_ctags= true;
		}
		if ( strlen( current_PIDs.APIDs[count].desc ) == 3 )
		{
			// unaufgeloeste Sprache...
			// getISO639Description returns its argument if nothing is found, so...
			const char *p = getISO639Description(current_PIDs.APIDs[count].desc);
			if (p != current_PIDs.APIDs[count].desc) // ...check, because arguments of strcpy must not overlap
				strcpy(current_PIDs.APIDs[count].desc, p);
		}

		if ( current_PIDs.APIDs[count].is_ac3 )
		{
			strncat(current_PIDs.APIDs[count].desc, " (AC3)", 25);
			has_ac3 = true;
		}
	}

	if ( has_unresolved_ctags )
	{
		if ( current_EPGid != 0 )
		{
			CSectionsdClient::ComponentTagList tags;
			if ( g_Sectionsd->getComponentTagsUniqueKey( current_EPGid, tags ) )
			{
				has_unresolved_ctags = false;
				has_ac3 = false;

				for (unsigned int i=0; i< tags.size(); i++)
				{
					for (unsigned int j=0; j< current_PIDs.APIDs.size(); j++)
					{
						if ( current_PIDs.APIDs[j].component_tag == tags[i].componentTag )
						{
							// workaround for buggy ZDF ctags / or buggy sectionsd/drivers , who knows...
							if(!tags[i].component.empty())
							{
								strncpy(current_PIDs.APIDs[j].desc, (Latin1_to_UTF8(tags[i].component)).c_str(), 25);
								if (current_PIDs.APIDs[j].is_ac3)
									strncat(current_PIDs.APIDs[j].desc, " (AC3)", 25);
							}
							current_PIDs.APIDs[j].component_tag = -1;
							break;
						}
					}
				}

				CZapitClient::APIDList::iterator e = current_PIDs.APIDs.begin();
				while ( e != current_PIDs.APIDs.end() )
				{
					if ( e->is_ac3 )
					{
							has_ac3 = true;
					}
					e++;
				}

				if ( g_settings.audio_DolbyDigital == 1)
				{
					for (unsigned int j=0; j< current_PIDs.APIDs.size(); j++)
						if ( current_PIDs.APIDs[j].is_ac3 )
						{
							setAPID( j );
							break;
						}
				}

				if ( current_PIDs.PIDs.selected_apid >= current_PIDs.APIDs.size() )
				{
                	setAPID( 0 );
				}
			}
		}
	}

	char *p = new char[sizeof(t_channel_id)];
	memcpy(p, &current_channel_id, sizeof(t_channel_id));
	g_RCInput->postMsg(NeutrinoMessages::EVT_ZAP_GOTAPIDS, (const neutrino_msg_data_t)p, false); // data is pointer to allocated memory
}


void CRemoteControl::copySubChannelsToZapit(void)
{
	CZapitClient::subServiceList zapitList;

	for (CSubServiceListSorted::const_iterator e = subChannels.begin(); e != subChannels.end(); e++)
		zapitList.push_back(e->getAsZapitSubService());

	g_Zapit->setSubServices(zapitList);
}


void CRemoteControl::setAPID( uint APID )
{
	if ((current_PIDs.PIDs.selected_apid == APID ) ||
	    (APID >= current_PIDs.APIDs.size()))
		return;

	current_PIDs.PIDs.selected_apid = APID;
	g_Zapit->setAudioChannel( APID );
}

static const std::string empty_string;

const std::string & CRemoteControl::setSubChannel(const int numSub, const bool force_zap)
{
	if ((numSub < 0) || (numSub >= (int)subChannels.size()))
		return empty_string;

	if ((selected_subchannel == numSub ) && (!force_zap))
		return empty_string;

	selected_subchannel = numSub;
	current_sub_channel_id = subChannels[numSub].getChannelID();
	g_Zapit->zapTo_subServiceID_NOWAIT( current_sub_channel_id );

	return subChannels[numSub].subservice_name;
}

const std::string & CRemoteControl::subChannelUp(void)
{
	// if there are any NVOD/subchannels switch these else switch audio channel (if any)
	if (subChannels.size() > 0 || !g_settings.audiochannel_up_down_enable)
	{
		return setSubChannel((subChannels.size() == 0) ? -1 : (int)((selected_subchannel + 1) % subChannels.size()));
	}
	else
	{
		if (current_PIDs.APIDs.size() > 0)
		{
			setAPID((current_PIDs.PIDs.selected_apid + 1) % current_PIDs.APIDs.size());
		}
		return (empty_string);
	}
}

const std::string & CRemoteControl::subChannelDown(void)
{
	// if there are any NVOD/subchannels switch these else switch audio channel (if any)
	if (subChannels.size() > 0 || !g_settings.audiochannel_up_down_enable)
	{
		return setSubChannel((selected_subchannel <= 0) ? (subChannels.size() - 1) : (selected_subchannel - 1));
	}
	else
	{
		if (current_PIDs.APIDs.size() > 0)
		{
			if (current_PIDs.PIDs.selected_apid <= 0)
				setAPID(current_PIDs.APIDs.size() - 1);
			else
				setAPID((current_PIDs.PIDs.selected_apid - 1));
		}
		return (empty_string);
	}
}

void CRemoteControl::zapTo_ChannelID(const t_channel_id channel_id, const std::string & channame, const bool start_video) // UTF-8
{
	current_channel_id = channel_id;
	current_channel_name = channame;

	if (start_video)
		startvideo();
	else
		stopvideo();

	current_sub_channel_id = 0;
	current_EPGid = 0;
	next_EPGid = 0;

	memset(&current_PIDs.PIDs, 0, sizeof(current_PIDs.PIDs) );

	current_PIDs.APIDs.clear();
	has_ac3 = false;

	subChannels.clear();
	selected_subchannel = -1;
	needs_nvods = false;
	director_mode = 0;

	unsigned long long now = getcurrenttime();
	if ( zap_completion_timeout < now )
	{
		g_Zapit->zapTo_serviceID_NOWAIT(channel_id);
		g_Sectionsd->setServiceChanged( current_channel_id, false );

		zap_completion_timeout = now + 2 * (long long) 1000000;
#if 0
		if ( current_programm_timer != 0 )
		{
			g_RCInput->killTimer( current_programm_timer );
			current_programm_timer = 0;
		}
#endif
	}
}


void CRemoteControl::startvideo()
{
	if ( !is_video_started )
	{
		is_video_started= true;
		g_Zapit->startPlayBack();
	}
}

void CRemoteControl::stopvideo()
{
	if ( is_video_started )
	{
		is_video_started= false;
		g_Zapit->stopPlayBack();
	}
}

void CRemoteControl::radioMode()
{
	g_Zapit->setMode( CZapitClient::MODE_RADIO );
}

void CRemoteControl::tvMode()
{
	g_Zapit->setMode( CZapitClient::MODE_TV );
}
