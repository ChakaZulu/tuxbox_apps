/*
  Neutrino-GUI  -   DBoxII-Project

  Copyright (C) 2003,2004 gagga
  Homepage: http://www.giggo.de/dbox

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

#ifndef __movieplayergui__
#define __movieplayergui__

#include <config.h>
#include <configfile.h>
#include "driver/framebuffer.h"
#include "gui/filebrowser.h"
#include "gui/bookmarkmanager.h"
#include "gui/widget/menue.h"

#ifdef ENABLE_MOVIEBROWSER
#include "gui/moviebrowser.h"
#include "gui/movieinfo.h"
#endif /* ENABLE_MOVIEBROWSER */

extern "C" {
               #include <driver/ringbuffer.h>
}
#include <stdio.h>

#include <string>
#include <vector>

class CMoviePlayerGui : public CMenuTarget
{
 public:
    enum state
        {
            STOPPED     =  0,
            PREPARING   =  1,
            STREAMERROR =  2,
            PLAY        =  3,
            PAUSE       =  4,
            FF          =  5,
            REW         =  6,
            RESYNC      =  7,
            JPOS        =  8, // jump to absolute position
            JF          =  9,
            JB          = 10,
            SKIP        = 11,
            AUDIOSELECT = 12,
            ITEMSELECT  = 13,
            SOFTRESET   = 99
        };

 private:
        static int     lastParental;

    pthread_t      rct;
    CFrameBuffer * frameBuffer;
    int            m_LastMode;
    const char     *filename;

    std::string Path_local;
    std::string Path_vlc;
    std::string Path_vlc_settings;

    CFileBrowser * filebrowser;
#ifdef ENABLE_MOVIEBROWSER
    CMovieBrowser* moviebrowser;
#endif /* ENABLE_MOVIEBROWSER */

    CBookmarkManager * bookmarkmanager;

    void PlayStream(int streamtype);
    void PlayFile(int parental=0);
    void ParentalEntrance(void);

    CFileFilter tsfilefilter;
    CFileFilter vlcfilefilter;
    void showHelpTS(void);
    void showHelpVLC(void);
    void showFileInfoVLC(void);
    void showMovieViewer(void);

 public:
    CMoviePlayerGui();
    ~CMoviePlayerGui();
    int exec(CMenuTarget* parent, const std::string & actionKey);
#ifdef ENABLE_MOVIEBROWSER
    void fileInfoStale(){if(moviebrowser != NULL)moviebrowser->fileInfoStale();};
#endif
    std::string getMoviePlayerVersion(void);
};


class CAPIDSelectExec : public CMenuTarget
{
    public:
        int exec(CMenuTarget* parent, const std::string & actionKey);
};

#endif
