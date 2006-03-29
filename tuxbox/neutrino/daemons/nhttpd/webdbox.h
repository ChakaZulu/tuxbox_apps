/*      
        nhttpd  -  DBoxII-Project

        Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

        $Id: webdbox.h,v 1.44 2006/03/29 15:31:55 yjogol Exp $

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

#ifndef __nhttpd_webdbox_h__
#define __nhttpd_webdbox_h__

// c++
#include <map>
#include <string>

// tuxbox
#include <controldclient/controldclient.h>
#include <eventserver.h>
#include <sectionsdclient/sectionsdclient.h>
#include <timerdclient/timerdclient.h>
#include <zapit/client/zapitclient.h>

// nhttpd
#include "helper.h"
#include "request.h"
#include "lcdapi.h"
#include "yapi.h"

class CWebserver;
class CWebserverRequest;
class CControlAPI;
class CWebAPI;
class CBouqueteditAPI;
class CWebDbox;
class CLCDAPI;
class CyAPI;

#include "controlapi.h"
#include "bouqueteditapi.h"
#include "webapi.h"

//-------------------------------------------------------------------------


class CWebDbox
{
	CWebserver		*Parent;

	// Clientlibs
	CControldClient		*Controld;
	CSectionsdClient	*Sectionsd;
	CZapitClient		*Zapit;
	CTimerdClient		*Timerd;
	CLCDAPI                 *LcdAPI;

	CEventServer		*EventServer;

	// complete channellists
	CZapitClient::BouquetChannelList RadioChannelList,TVChannelList;
	// events of actual channel
	std::map<unsigned, CChannelEvent *> ChannelListEvents;
	// List of available tv bouquets
	std::map<int, CZapitClient::BouquetChannelList> TVBouquetsList;
	// List of available radio bouquets
	std::map<int, CZapitClient::BouquetChannelList> RadioBouquetsList;
	// List of bouquets
	CZapitClient::BouquetList BouquetList;

	//bool standby_mode;

	// some constants
	std::string Dbox_Hersteller[4];
	std::string videooutput_names[5];
	std::string videoformat_names[4];
	std::string audiotype_names[5];

	// get functions to collect data
	bool GetChannelEvents(void);
	bool GetStreamInfo(int bitinfo[10]);
	std::string GetServiceName(t_channel_id channel_id);
	CZapitClient::BouquetChannelList *GetBouquet(unsigned int BouquetNr, int Mode);
	CZapitClient::BouquetChannelList *GetChannelList(int Mode);

	// support functions
	void ZapTo          (const char * const target);
	void ZapToSubService(const char * const target);
	void ZapToChannelId (t_channel_id channel_id);
	t_channel_id ChannelNameToChannelId(std::string search_channel_name);

	void UpdateBouquet(unsigned int BouquetNr);
	void UpdateChannelList(void);
	void UpdateBouquets(void);

	void timerEventType2Str(CTimerd::CTimerEventTypes type, char *str, int len);
	void timerEventRepeat2Str(CTimerd::CTimerEventRepeat rep, char *str, int len);

public:
	CWebDbox(CWebserver *server);
	~CWebDbox(void);

	CChannelEventList	eList;

	CControlAPI		*ControlAPI;
	CWebAPI			*WebAPI;
	CBouqueteditAPI		*BouqueteditAPI;
	CyAPI                 	*yAPI;

	friend class CWebAPI;
	friend class CControlAPI;
	friend class CBouqueteditAPI;
	friend class CyAPI;
};

#endif /* __nhttpd_webdbox_h__ */
