/*
 * $Id: movieplayer.cpp,v 1.12 2005/12/26 11:51:22 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
 *          based on vlc plugin by mechatron
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#include <plugin.h>
#include "movieplayer.h"

#define REL "Movieplayer Plugin, Version 0.6.1"

extern "C" int plugin_exec(PluginParam *par);
extern eString getWebifVersion();

bool sortByName(const PLAYLIST& a, const PLAYLIST& b)
{
	return a.Filename < b.Filename;
}

bool sortByType(const PLAYLIST& a, const PLAYLIST& b)
{
	if (a.Filetype == b.Filetype)
		return sortByName(a, b);
	else
		return a.Filetype < b.Filetype;
}

eSCGui::eSCGui(): menu(true)
{
	int fd = eSkin::getActive()->queryValue("fontsize", 20);
	
	bufferingBox = new eMessageBox(_("Buffering video stream... Please wait."),_("Information"), eMessageBox::iconInfo);

	cmove(ePoint(90, 110)); 
	cresize(eSize(550, 350));

	addActionMap(&i_shortcutActions->map);
	addActionMap(&i_cursorActions->map);

	list = new eListBox<eListBoxEntryText>(this);
	list->move(ePoint(10, 10));
	list->resize(eSize(clientrect.width() - 20, 260));
	CONNECT(list->selected, eSCGui::listSelected);
	CONNECT(list->selchanged, eSCGui::listSelChanged);

	eLabel *l_root = new eLabel(this);
	l_root->move(ePoint(10, clientrect.height() - 80));
	l_root->resize(eSize(130, fd + 10));
	l_root->setProperty("align", "center"); 
	l_root->setProperty("vcenter", "");
	l_root->setProperty("backgroundColor", "std_dred");
	l_root->setText("File");

	eLabel *l_vcd = new eLabel(this);
	l_vcd->move(ePoint(140, clientrect.height() - 80));
	l_vcd->resize(eSize(130, fd + 10));
	l_vcd->setProperty("align", "center"); 
	l_vcd->setProperty("vcenter", "");
	l_vcd->setProperty("backgroundColor", "std_dgreen");
	l_vcd->setText("VCD");

	eLabel *l_svcd = new eLabel(this);
	l_svcd->move(ePoint(270, clientrect.height() - 80)); 
	l_svcd->resize(eSize(130, fd + 10));
	l_svcd->setProperty("align", "center"); 
	l_svcd->setProperty("vcenter", "");
	l_svcd->setProperty("backgroundColor", "std_dyellow"); 
	l_svcd->setText("SVCD");

	eLabel *l_dvd = new eLabel(this);
	l_dvd->move(ePoint(400, clientrect.height() - 80));
	l_dvd->resize(eSize(130, fd + 10));
	l_dvd->setProperty("align", "center"); 
	l_dvd->setProperty("vcenter", "");
	l_dvd->setProperty("backgroundColor", "std_dblue");
	l_dvd->setText("DVD");

	status = new eStatusBar(this);
	status->move(ePoint(10, clientrect.height() - 35));
	status->resize(eSize(clientrect.width() - 20, 30));
	status->loadDeco();

	timer = new eTimer(eApp);
	CONNECT(timer->timeout, eSCGui::timerHandler);

	eMoviePlayer::getInstance()->mpconfig.load();
	server = eMoviePlayer::getInstance()->mpconfig.getServerConfig();
	
	send_parms.IP = server.serverIP;
	send_parms.IF_PORT = server.webifPort;
	
	startdir = server.startDir;
	cddrive = server.CDDrive;	

	if (server.vlcUser && server.vlcPass)
		send_parms.AUTH = server.vlcUser + ":" + server.vlcPass;
	
	int mode = DATA;
	eConfig::getInstance()->getKey("/enigma/plugins/movieplayer/lastmode", mode);
	loadList(mode, startdir);
}

eSCGui::~eSCGui()
{
	playList.clear();
	delete timer;
}

void eSCGui::loadList(int mode, eString pathfull)
{
	playList.clear();
	PLAYLIST a;

	eString tmp2, tmp3;
	switch (mode)
	{
		case DATA:
			tmp2 = "File";
			setText("VLC " + tmp2 + " - Path: " + pathfull);
			tmp3 = "";
			break;
		case VCD:
			tmp2 = "VCD";  
			setText("VLC " + tmp2 + " - Drive: " + cddrive); 
			tmp3 = "vcd:" + cddrive + "@1:1";
			break;
		case SVCD:
			tmp2 = "SVCD"; 
			setText("VLC " + tmp2 + " - Drive: " + cddrive); 
			tmp3 = "vcd:" + cddrive + "@1:1";
			break;
		case DVD:
			tmp2 = "DVD";  
			setText("VLC " + tmp2 + " - Drive: " + cddrive); 
			tmp3 = "dvdsimple:" + cddrive + "@1:1";
			break;
	}

	if (mode == DATA)
	{
		eString response;
		sendGetRequest("/admin/dboxfiles.html?dir=" + pathfull.strReplace(" ", "%20"), response);

		std::replace(response.begin(), response.end(), '\\', '/');

		if (response && response != "-1")
		{
			unsigned int start = 0;
			for (unsigned int pos = response.find('\n', 0); pos != std::string::npos; pos = response.find('\n', start))
			{
				eString entry = response.substr(start, pos - start);
				eDebug("[MOVIEPLAYER] file: %s",entry.c_str());
				if (entry.find("DIR:") == 0)
				{
					if (entry.right(3) == "/..")
					{
						if (pathfull == startdir || entry.mid(4, entry.length() - 7) == startdir)
							eDebug("[VLC] startdir = pathdir");
						else
						{
							a.Filename = _("[GO UP]");
							a.Fullname = entry.mid(4);
							a.Fullname = a.Fullname.left(a.Fullname.length() - 3);
							a.Fullname = a.Fullname.left(a.Fullname.find_last_of("/") + 1);
							a.Filetype = GOUP;
							eDebug("[MOVIEPLAYER] goup: %s",a.Fullname.c_str());
							playList.push_back(a);
						}
					}
					else
					{
						a.Filename = "[DIR] " + entry.mid(4);
						a.Fullname = entry.mid(4);
						a.Filetype = DIRS;
						eDebug("[MOVIEPLAYER] dir: %s",a.Fullname.c_str());
						playList.push_back(a);
					}
				}
				else
				{
					a.Filename = entry;
					a.Fullname = entry;
					a.Filetype = FILES;
					eDebug("[MOVIEPLAYER] file: %s",a.Fullname.c_str());
					playList.push_back(a);
				}
				start = pos + 1;
			}
		}
		else
		{
			eMessageBox msg(eString("Movieplayer could not communicate to VLC.\nPlease make sure that the IP address of the PC is configured correctly in the streaming settings and that VLC is started."), _("Error"), eMessageBox::btOK);
			msg.show();
			msg.exec();
			msg.hide();
		}
	}
	else
	{
		if (cddrive[cddrive.length() - 1] == ':' && cddrive.length() == 2) 
			pathfull = cddrive + "/";

		a.Filename = _("Play");
		a.Fullname = tmp3;
		a.Filetype = FILES;
		playList.push_back(a);
	}
	
	viewList();

	eConfig::getInstance()->setKey("/enigma/plugins/movieplayer/lastmode", mode);
}

void eSCGui::viewList()
{
	sort(playList.begin(), playList.end(), sortByType);

	list->beginAtomic();
	list->clearList();
	int current = 0;

	for (PlayList::iterator p = playList.begin(); p != playList.end(); p++)
		new eListBoxEntryText(list, (*p).Filename, (void *)current++);
		
	list->endAtomic();
	setStatus(0);
}

void eSCGui::setStatus(int val)
{
	if (playList.size())
	{
		switch (playList[val].Filetype)
		{
			case GOUP:
				list->setHelpText(_("Go up one directory level"));
				break;
			case DIRS:
				list->setHelpText(_("Enter directory"));
				break;
			case FILES:
				list->setHelpText(_("Press OK to play"));
				break;
		}
	}
}

void eSCGui::listSelChanged(eListBoxEntryText *item)
{
	if (item)
		setStatus((int)item->getKey());
}

void eSCGui::listSelected(eListBoxEntryText *item)
{
	if (item)
	{
		val = (int)item->getKey();
		if (playList[val].Filetype == FILES)
			playerStart(val);
		else
			loadList(DATA, playList[val].Fullname);
	}
}

void eSCGui::showMenu()
{
	if (!menu)
	{
		timer->stop();
		bufferingBox->hide();
		
		hide();
		cmove(ePoint(90, 110)); 
		status->loadDeco();
		show();
		
		menu = true;
	}
}

void eSCGui::timerHandler()
{
	eDebug("[MOVIEPLAYER PLUGIN] timerHandler: status = %d", eMoviePlayer::getInstance()->status.STAT);
	
	switch (eMoviePlayer::getInstance()->status.STAT)
	{
		case eMoviePlayer::STREAMERROR:
		{
			eMessageBox msg(_("A streaming error occurred.\nPlease make sure that VLC is started on the PC and that it can play the file you selected."), _("Error"), eMessageBox::btOK|eMessageBox::iconError); 
			msg.show(); 
			msg.exec(); 
			msg.hide();
		}
		case eMoviePlayer::STOPPED:
		{
			if (playList.size() > 1)
			{
				if (++val >= playList.size())
					val = 0;
				playerStart(val);
			}
			else
				showMenu();
			break;
		}
		default:
		{
			if (!eMoviePlayer::getInstance()->status.BUFFERFILLED)
				bufferingBox->show();
			else
				bufferingBox->hide();

			timer->start(1000, true);
			break;
		}
	}
}

void eSCGui::playerStart(int val)
{
	if (menu)
	{
		hide();
		cmove(ePoint(90, 800)); 
		menu = false;
	}

	eDebug("\n[VLC] play %d \"%s\"", val, playList[val].Fullname.c_str());
	if (eMoviePlayer::getInstance()->status.STAT != eMoviePlayer::STOPPED)
		eMoviePlayer::getInstance()->control("terminate", "");
	
	eMoviePlayer::getInstance()->control("start2", playList[val].Fullname.c_str());
	timer->start(2000, true);
}

int eSCGui::eventHandler(const eWidgetEvent &e)
{
	int jump = 0;

	switch (e.type)
	{
	case eWidgetEvent::evtAction:
		if (e.action == &i_cursorActions->cancel)
		{
			if (eMoviePlayer::getInstance()->status.STAT != eMoviePlayer::STOPPED)
				eMoviePlayer::getInstance()->control("stop", "");
				
			if (!menu)
				showMenu();
			else
				close(0);
		}
		else
		if (e.action == &i_cursorActions->help)
		{
			if (menu)
				hide();
			eSCGuiInfo info;
			info.show();
			info.exec();
			info.hide();
			if (menu)
				show();
		}
		else 
		if (e.action == &i_shortcutActions->red)
		{
			if (menu)
				loadList(DATA, startdir);
			else
				eMoviePlayer::getInstance()->control("rewind", "");
		}
		else 
		if (e.action == &i_shortcutActions->green)
		{
			if (menu)
				loadList(VCD, "");
			else
			{
				if (eMoviePlayer::getInstance()->status.STAT == eMoviePlayer::PLAY)
					eMoviePlayer::getInstance()->control("resync", "");
				else
					eMoviePlayer::getInstance()->control("play", "");
			}
		}
		else 
		if (e.action == &i_shortcutActions->yellow)
		{
			if (menu)
				loadList(SVCD, "");
			else
				eMoviePlayer::getInstance()->control("pause", "");
		}
		else 
		if (e.action == &i_shortcutActions->blue)
		{
			if (menu)
				loadList(DVD, "");
			else
				eMoviePlayer::getInstance()->control("forward", "");
		}
		else 
		if (e.action == &i_shortcutActions->number1) 
			jump = 1; 
		else 
		if (e.action == &i_shortcutActions->number2) 
			jump = 2; 
		else 
		if (e.action == &i_shortcutActions->number3) 
			jump = 3; 
		else 
		if (e.action == &i_shortcutActions->number4) 
			jump = 4; 
		else 
		if (e.action == &i_shortcutActions->number5) 
			jump = 5; 
		else 
		if (e.action == &i_shortcutActions->number6) 
			jump = 6; 	
		else 
		if (e.action == &i_shortcutActions->number7) 
			jump = 7; 
		else 
		if (e.action == &i_shortcutActions->number8) 
			jump = 8; 
		else 
		if (e.action == &i_shortcutActions->number9) 
			jump = 9; 
		else
			break;

		if (jump > 0)
		{
			eMessageBox mb(eString().sprintf("Skip %d minute(s)", jump), _("Information"), eMessageBox::btOK|eMessageBox::iconInfo, eMessageBox::btOK, 3);
			mb.show();
			mb.exec();
			mb.hide();
			eMoviePlayer::getInstance()->control("jump", eString().sprintf("%d", jump).c_str());
		}
		return 1;
	default:
		break;
	}
	return eWindow::eventHandler(e);
}

static char *NAME[] = 
{
	"-----------------------------",
	"Keys (if menu is not visible)",
	"-----------------------------",
	"red: Start skipping reverse",
	"green: Play/Resync",
	"yellow: Pause",
	"blue: Start skipping forward",
	"1-9: Skip 1-9 minutes",
	" "
};

eSCGuiInfo::eSCGuiInfo()
{
	cmove(ePoint(140, 100)); 
	cresize(eSize(440, 400)); 
	setText(_("Help"));

	list = new eListBox<eListBoxEntryText>(this);
	list->move(ePoint(5, 5));
	list->resize(eSize(clientrect.width() - 10, clientrect.height() - 10));

	new eListBoxEntryText(list, REL);
	int i = 0;
	while (eString(NAME[i]) != " ")
		new eListBoxEntryText(list, NAME[i++]);
}

size_t CurlDummyWrite (void *ptr, size_t size, size_t nmemb, void *data)
{
	std::string *pStr = (std::string *)data;
	*pStr += (char *)ptr;
	return size * nmemb;
}

CURLcode eSCGui::sendGetRequest (const eString& url, eString& response)
{
	CURL *curl;
	CURLcode httpres;

	eString tmpurl= "http://" + send_parms.IP + ":" + send_parms.IF_PORT + url;
	response = "";

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, tmpurl.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlDummyWrite);
	curl_easy_setopt(curl, CURLOPT_FILE, (void *)&response);

	if(send_parms.AUTH)
		curl_easy_setopt (curl, CURLOPT_USERPWD, send_parms.AUTH.c_str());	
	curl_easy_setopt (curl, CURLOPT_FAILONERROR, true);
	curl_easy_setopt (curl, CURLOPT_TIMEOUT, 5);
	httpres = curl_easy_perform (curl);
	//eDebug("[VLC] HTTP Result: %d", httpres);
	curl_easy_cleanup(curl);
	return httpres;
}

int plugin_exec(PluginParam *par)
{
	eSCGui dlg;
	eString webifVersion = getWebifVersion();
	if (webifVersion.find("Expert") != eString::npos)
	{
		dlg.show();
		dlg.exec();
		dlg.hide();
	}
	else
	{
		eMessageBox msg(eString("This plugin requires the EXPERT version of the web interface to be installed.\nInstalled web interface version: " + webifVersion), _("Error"), eMessageBox::btOK);
		msg.show();
		msg.exec();
		msg.hide();
	}
	return 0;
}
