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
#include "helpers/locale.h"

extern  SNeutrinoSettings   g_settings;

extern  CLCDD           *g_lcdd;
extern  CControld       *g_Controld;
extern  CRemoteControl  *g_RemoteControl;

extern  FontsDef        *g_Fonts;
extern  CRCInput        *g_RCInput;
extern  CFrameBuffer    *g_FrameBuffer;

extern  CEpgData        *g_EpgData;
extern  CInfoViewer     *g_InfoViewer;
extern  CStreamInfo     *g_StreamInfo;

extern  CScreenSetup    *g_ScreenSetup;

extern CLocaleManager	*g_Locale;
