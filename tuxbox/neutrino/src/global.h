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


//#define USEACTIONLOG

#include "neutrino.h"
#include "controldclient.h"
#include "lcddclient.h"
#include "zapitclient.h"
#include "sectionsdclient.h"
#include "timerdclient.h"
#include "daemonc/remotecontrol.h"

#include "driver/fontrenderer.h"
#include "driver/rcinput.h"
#include "driver/framebuffer.h"

#include "helpers/epgdata.h"
#include "helpers/infoviewer.h"
#include "helpers/streaminfo.h"

#include "widget/screensetup.h"
#include "widget/bouquetlist.h"
#include "helpers/settings.h"
#include "helpers/scan.h"
#include "widget/eventlist.hpp"
#include "helpers/locale.h"
#include "helpers/update.h"
#include "widget/gamelist.h"


#ifndef NEUTRINO_CPP
#define NEUTRINO_CPP extern
#endif

NEUTRINO_CPP int		NeutrinoMode;

NEUTRINO_CPP  CNeutrinoApp        *neutrino;
NEUTRINO_CPP  SNeutrinoSettings   g_settings;

NEUTRINO_CPP  CLcddClient		*g_lcdd;
NEUTRINO_CPP  CControldClient	*g_Controld;
NEUTRINO_CPP  CZapitClient		*g_Zapit;
NEUTRINO_CPP  CSectionsdClient	*g_Sectionsd;
NEUTRINO_CPP  CTimerdClient		*g_Timerd;
NEUTRINO_CPP  CRemoteControl	*g_RemoteControl;

NEUTRINO_CPP  fontRenderClass	*g_fontRenderer;
NEUTRINO_CPP  FontsDef			*g_Fonts;
NEUTRINO_CPP  CRCInput			*g_RCInput;

NEUTRINO_CPP  CEpgData			*g_EpgData;
NEUTRINO_CPP  CInfoViewer		*g_InfoViewer;
NEUTRINO_CPP  EventList			*g_EventList;
NEUTRINO_CPP  CStreamInfo		*g_StreamInfo;
NEUTRINO_CPP  CScanTs			*g_ScanTS;
NEUTRINO_CPP  CFlashUpdate		*g_Update;

NEUTRINO_CPP  CScreenSetup		*g_ScreenSetup;

NEUTRINO_CPP CLocaleManager		*g_Locale;

NEUTRINO_CPP CBouquetList		*bouquetList;
NEUTRINO_CPP CPlugins   		*g_PluginList;



#ifdef USEACTIONLOG
	#include "helpers/actionlog.h"
	NEUTRINO_CPP CActionLog	  *g_ActionLog;
#endif

