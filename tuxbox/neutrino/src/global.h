// 
// $Id: global.h,v 1.9 2001/10/30 22:18:09 McClean Exp $
//
// $Log: global.h,v $
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

NEUTRINO_CPP  CScreenSetup    *g_ScreenSetup;

NEUTRINO_CPP CLocaleManager	*g_Locale;



