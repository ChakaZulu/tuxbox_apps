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

//
// $Id: global.h,v 1.12 2001/12/01 23:12:55 Simplex Exp $
//
// $Log: global.h,v $
// Revision 1.12  2001/12/01 23:12:55  Simplex
// global instance of watchdog
//
// Revision 1.11  2001/11/23 16:58:41  McClean
// update-functions
//
// Revision 1.10  2001/11/15 11:42:41  McClean
// gpl-headers added
//
// Revision 1.9  2001/10/30 22:18:09  McClean
// add ts-scan mask
//
// Revision 1.8  2001/10/09 21:48:37  McClean
// ucode-check
//
// Revision 1.7  2001/09/20 19:21:37  fnbrd
// Channellist mit IDs.
//
// Revision 1.6  2001/09/18 20:20:26  field
// Eventlist in den Infov. verschoben (gelber Knopf), Infov.-Anzeige auf Knoepfe
// vorbereitet
//
// Revision 1.5  2001/09/17 18:37:40  fnbrd
// inserted id and log.
//
//
#include "neutrino.h"
#include "daemonc/lcdd.h"
#include "daemonc/controld.h"
#include "daemonc/remotecontrol.h"

#include "driver/fontrenderer.h"
#include "driver/rcinput.h"
#include "driver/framebuffer.h"

#include "helpers/epgdata.h"
#include "helpers/infoviewer.h"
#include "helpers/streaminfo.h"

#include "widget/screensetup.h"
#include "helpers/settings.h"
#include "helpers/scan.h"
#include "widget/eventlist.hpp"
#include "helpers/locale.h"
#include "helpers/update.h"
#include "helpers/streamwatchdog.h"

#ifndef NEUTRINO_CPP
#define NEUTRINO_CPP extern
#endif

NEUTRINO_CPP bool zapit;

NEUTRINO_CPP  CNeutrinoApp        *neutrino;
NEUTRINO_CPP  SNeutrinoSettings   g_settings;

NEUTRINO_CPP  CLCDD           *g_lcdd;
NEUTRINO_CPP  CControld       *g_Controld;
NEUTRINO_CPP  CRemoteControl  *g_RemoteControl;

NEUTRINO_CPP  fontRenderClass *g_fontRenderer;
NEUTRINO_CPP  FontsDef        *g_Fonts;
NEUTRINO_CPP  CRCInput        *g_RCInput;
NEUTRINO_CPP  CFrameBuffer    *g_FrameBuffer;

NEUTRINO_CPP  CEpgData        *g_EpgData;
NEUTRINO_CPP  CInfoViewer     *g_InfoViewer;
NEUTRINO_CPP  EventList       *g_EventList;
NEUTRINO_CPP  CStreamInfo     *g_StreamInfo;
NEUTRINO_CPP  CUCodeCheck     *g_UcodeCheck;
NEUTRINO_CPP  CScanTs         *g_ScanTS;
NEUTRINO_CPP  CFlashUpdate    *g_Update;

NEUTRINO_CPP  CScreenSetup    *g_ScreenSetup;

NEUTRINO_CPP CLocaleManager   *g_Locale;

NEUTRINO_CPP CStreamWatchDog  *g_WatchDog;


