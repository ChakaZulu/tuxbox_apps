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


#include "remotecontrol.h"
#include "../global.h"


CRemoteControl::CRemoteControl()
{
	current_onid_sid = 0;
	current_sub_onid_sid = 0;
	zap_completion_timeout = 0;
	current_EPGid= 0;
	memset(&current_PIDs.PIDs, 0, sizeof(current_PIDs.PIDs) );
    selected_apid = 0;
	has_ac3 = false;
	selected_subchannel = -1;
	needs_nvods = false;
}

int CRemoteControl::handleMsg(uint msg, uint data)
{
	if ( zap_completion_timeout != 0 )
	{
    	if ( ( msg == NeutrinoMessages::EVT_ZAP_COMPLETE ) ||
    		 ( msg == NeutrinoMessages:: EVT_ZAP_FAILED ) ||
    		 ( msg == NeutrinoMessages:: EVT_ZAP_ISNVOD ) )
		{
			if ( data != current_onid_sid )
			{
				g_Zapit->zapTo_serviceID_NOWAIT( current_onid_sid );
				zap_completion_timeout = getcurrenttime() + 2 * (long long) 1000000;

				return messages_return::handled;
			}
			else
				zap_completion_timeout = 0;
		}
	}

    if ( msg == NeutrinoMessages::EVT_CURRENTEPG )
	{
		sectionsd::CurrentNextInfo* info_CN = (sectionsd::CurrentNextInfo*) data;

		if ( ( info_CN->current_uniqueKey >> 16) == current_onid_sid )
		{
			//CURRENT-EPG für den aktuellen Kanal bekommen!;

			current_EPGid= info_CN->current_uniqueKey;
			if ( has_unresolved_ctags )
				processAPIDnames();

			if ( info_CN->flags & sectionsd::epgflags::current_has_linkagedescriptors )
				getSubChannels();

			if ( needs_nvods )
				getNVODs();

		}
	    return messages_return::handled;
	}
	else if ( ( msg == NeutrinoMessages::EVT_ZAP_COMPLETE ) || ( msg == NeutrinoMessages:: EVT_ZAP_SUB_COMPLETE ) )
	{
		if ( data == (( msg == NeutrinoMessages::EVT_ZAP_COMPLETE )?current_onid_sid:current_sub_onid_sid) )
		{
			g_Zapit->getPIDS( current_PIDs );
			g_RCInput->postMsg( NeutrinoMessages::EVT_ZAP_GOTPIDS, current_onid_sid, false );

			processAPIDnames();
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_ISNVOD )
	{
		if ( data == current_onid_sid )
		{
			if ( current_EPGid == 0)
			{
				// epg noch nicht da -> Bedarf für NVOD-Zeiten anmelden!
				needs_nvods = true;
			}
			else
			{
				getNVODs();
			}
		}
	    return messages_return::handled;
	}
	else
		return messages_return::unhandled;
}

void CRemoteControl::getSubChannels()
{
	if ( subChannels.size() == 0 )
	{
		sectionsd::LinkageDescriptorList	linkedServices;
		if ( g_Sectionsd->getLinkageDescriptorsUniqueKey( current_EPGid, linkedServices ) )
		{
			if ( linkedServices.size()> 1 )
			{
            	are_subchannels = true;
				for (int i=0; i< linkedServices.size(); i++)
				{
					subChannels.insert( subChannels.end(),
										 CSubService( linkedServices[i].originalNetworkId<<16 | linkedServices[i].serviceId,
													  linkedServices[i].transportStreamId,
													  linkedServices[i].name) );
				}

				copySubChannelsToZapit();
    	        g_RCInput->postMsg( NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES, current_onid_sid, false );
			}
		}
	}
}

void CRemoteControl::getNVODs()
{
	if ( subChannels.size() == 0 )
	{
		sectionsd::NVODTimesList	NVODs;
		if ( g_Sectionsd->getNVODTimesServiceKey( current_onid_sid, NVODs ) )
		{
            needs_nvods = false;

			are_subchannels = false;
			for (int i=0; i< NVODs.size(); i++)
			{
				if ( NVODs[i].zeit.dauer> 0 )
				{
					CSubService newService ( NVODs[i].onid_sid, NVODs[i].tsid,
											 NVODs[i].zeit.startzeit, NVODs[i].zeit.dauer);

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
            g_RCInput->postMsg( NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES, current_onid_sid, false );
			setSubChannel( subChannels.size()- 1 );
		}
	}
}

void CRemoteControl::processAPIDnames()
{
	has_unresolved_ctags= false;
	has_ac3 = false;

	for(int count=0; count< current_PIDs.APIDs.size(); count++)
	{
		if ( current_PIDs.APIDs[count].component_tag != -1 )
			has_unresolved_ctags= true;

		if ( strlen( current_PIDs.APIDs[count].desc ) == 3 )
		{
			// unaufgeloeste Sprache...
			strcpy( current_PIDs.APIDs[count].desc, getISO639Description( current_PIDs.APIDs[count].desc ) );
		}

		if ( current_PIDs.APIDs[count].is_ac3 )
		{
			strcat( current_PIDs.APIDs[count].desc, " (AC3)");
			has_ac3 = true;
		}
	}

	if ( has_unresolved_ctags )
	{
		if ( current_EPGid != 0 )
		{
			sectionsd::ComponentTagList tags;
			if ( g_Sectionsd->getComponentTagsUniqueKey( current_EPGid, tags ) )
			{
				has_unresolved_ctags = false;
				has_ac3 = false;

				for (int i=0; i< tags.size(); i++)
				{
					for (int j=0; j< current_PIDs.APIDs.size(); j++)
					{
						if ( current_PIDs.APIDs[j].component_tag == tags[i].componentTag )
						{
							strncpy( current_PIDs.APIDs[j].desc, tags[i].component.c_str(), 25 );
							if ( current_PIDs.APIDs[j].is_ac3 )
								strncat( current_PIDs.APIDs[j].desc, " (AC3)", 25 );
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
						if ( e->component_tag != -1 )
						{
							current_PIDs.APIDs.erase( e );
							continue;
						}
						else
							has_ac3 = true;
					}
					e++;
				}

				if ( g_settings.audio_DolbyDigital == 1)
				{
					for (int j=0; j< current_PIDs.APIDs.size(); j++)
						if ( current_PIDs.APIDs[j].is_ac3 )
						{
							setAPID( j );
							break;
						}
				}
			}
		}
	}


	g_RCInput->postMsg( NeutrinoMessages::EVT_ZAP_GOTAPIDS, current_onid_sid, false );
}


void CRemoteControl::copySubChannelsToZapit()
{
	CZapitClient::subServiceList 		zapitList;
	CZapitClient::commandAddSubServices	zapitSubChannel;

	for(CSubServiceListSorted::iterator e=subChannels.begin(); e!=subChannels.end(); ++e)
	{
		zapitSubChannel.onidsid = e->onid_sid;
		zapitSubChannel.tsid = e->tsid;
		zapitList.insert ( zapitList.end(), zapitSubChannel );
	}
	g_Zapit->setSubServices( zapitList );
}


void CRemoteControl::setAPID( int APID )
{
	if ((selected_apid == APID ) || (APID < 0) || (APID >= current_PIDs.APIDs.size()) )
		return;

	selected_apid = APID;
	g_Zapit->setAudioChannel( APID );
	#ifdef USEACTIONLOG
		char buf[1000];
		sprintf((char*) buf, "select audio: \"%s\"", current_PIDs.APIDs[APID].desc );
		g_ActionLog->println(buf);
	#endif
}

string CRemoteControl::setSubChannel(unsigned numSub, bool force_zap )
{
	if ((numSub < 0) || (numSub >= subChannels.size()))
		return "";

	if ((selected_subchannel == numSub ) && (!force_zap))
		return "";

	selected_subchannel = numSub;
	current_sub_onid_sid = subChannels[numSub].onid_sid;

	g_Zapit->zapTo_subServiceID_NOWAIT( current_sub_onid_sid );

	string perspectiveName = subChannels[numSub].subservice_name;

	#ifdef USEACTIONLOG
		char buf[1000];
		if(perspectiveName!="")
		{
			sprintf((char*) buf, "perspective change: \"%s\"", perspectiveName.c_str() );
			g_ActionLog->println(buf);
		}
		else
		{
			struct  tm *tmZeit;
			tmZeit= localtime( &subChannels[numSub].startzeit );
			sprintf((char*) buf, "select nvod: (%02d:%02d)", tmZeit->tm_hour, tmZeit->tm_min );
			g_ActionLog->println(buf);
		}
	#endif

	return perspectiveName;
}

string CRemoteControl::subChannelUp()
{
	return setSubChannel( (selected_subchannel + 1) % subChannels.size());
}

string CRemoteControl::subChannelDown()
{
	if (selected_subchannel == 0 )
	{
		return setSubChannel(subChannels.size() - 1);
	}
	else
	{
		return setSubChannel(selected_subchannel - 1);
	}
}

void CRemoteControl::zapTo_onid_sid( unsigned int onid_sid, string channame)
{
	current_onid_sid = onid_sid;

    current_sub_onid_sid = 0;
	current_EPGid = 0;

	memset(&current_PIDs.PIDs, 0, sizeof(current_PIDs.PIDs) );

	current_PIDs.APIDs.clear();
    selected_apid = 0;
	has_ac3 = false;

	subChannels.clear();
	selected_subchannel = -1;
	needs_nvods = false;

	#ifdef USEACTIONLOG
		char buf[1000];
		sprintf((char*) buf, "zapto: %08x \"%s\"", onid_sid, channame.c_str() );
		g_ActionLog->println(buf);
	#endif

	long long now = getcurrenttime();
	if ( zap_completion_timeout < now )
	{
		g_Zapit->zapTo_serviceID_NOWAIT( onid_sid );
		zap_completion_timeout = now + 2 * (long long) 1000000;
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


