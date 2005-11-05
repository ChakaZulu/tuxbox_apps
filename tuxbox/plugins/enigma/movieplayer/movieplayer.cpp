/*
 * $Id: movieplayer.cpp,v 1.2 2005/11/05 15:00:26 digi_casi Exp $
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

#define REL "Streaming Client GUI, Version 0.0.2 "
#define CONFFILE0 "/etc/movieplayer.xml"
#define CONFFILE1 "/var/tuxbox/config/movieplayer.xml"

player_value eSCplay::play_parms;

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

eSCGui::eSCGui(): MODE(DATA), menu(true)
{
	int fd = eSkin::getActive()->queryValue("fontsize", 20);
	
	bufferingBox = new eMessageBox(_("Please wait..."),_("Buffering..."), eMessageBox::iconInfo);

	cmove(ePoint(90, 110)); cresize(eSize(550, 350));

	addActionMap(&i_shortcutActions->map);
	addActionMap(&i_cursorActions->map);

	eConfig::getInstance()->getKey("/enigma/plugins/movieplayer/lastmode", MODE);

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

	eStatusBar *status = new eStatusBar(this);
	status->move(ePoint(10, clientrect.height() - 35));
	status->resize(eSize(clientrect.width() - 20, 30));
	status->loadDeco();

	timer = new eTimer(eApp);
	CONNECT(timer->timeout, eSCGui::playerStop);

	bool loading = false;
	if (access(CONFFILE1, R_OK) == 0)
		loading = loadXML(CONFFILE1);
	else
	if (access(CONFFILE0, R_OK) == 0)
		loading = loadXML(CONFFILE0);
		
	if (loading)
		loadList();
	else
	{
		hide();
		eMessageBox msg(_("Unable to open or parse configuration file movieplayer.xml"), _("Error"), eMessageBox::btOK);
		msg.show();
		msg.exec();
		msg.hide();
		show();
	}
}

eSCGui::~eSCGui()
{
	playList.clear();
	extList.clear();
	if (timer) 
		delete timer;

	eConfig::getInstance()->setKey("/enigma/plugins/movieplayer/lastmode", MODE);
}

void eSCGui::loadList()
{
	playList.clear();
	PLAYLIST a;

	eString tmp2, tmp3;
	switch(MODE)
	{
		case DATA:
			setText("VLC File Mode  Path: " + pathfull);
			break;
		case VCD:
			tmp2 = "VCD Mode";  
			setText("VLC" + tmp2 + "  Drive: " + cddrive); 
			tmp3 = "vcd:" + cddrive + "@1:1";
			break;
		case SVCD:
			tmp2 = "SVCD Mode"; 
			setText("VLC" + tmp2 + "  Drive: " + cddrive); 
			tmp3 = "vcd:" + cddrive + "@1:1";
			break;
		case DVD:
			tmp2 = "DVD Mode";  
			setText("VLC" + tmp2 + "  Drive: " + cddrive); 
			tmp3 = "dvdsimple:" + cddrive + "@1:1";
			break;
	}

	VLCsend::getInstance()->send("/admin/dboxfiles.html?dir=" + pathfull);
	eString response = VLCsend::getInstance()->send_parms.RESPONSE;

	std::replace(response.begin(), response.end(), '\\', '/');

	if (response && response != "-1")
	{
		if (MODE == DATA)
		{
			unsigned int start = 0;
			for (unsigned int pos = response.find('\n', 0); pos != std::string::npos; pos = response.find('\n', start))
			{
				eString entry = response.substr(start, pos - start);
				//eDebug("%s",entry.c_str());
				if (entry.find("DIR:") == 0)
				{
					if (entry.find("/..") != eString::npos)
					{
						if (pathfull == startdir || entry.mid(4, entry.length() - 7) == startdir)
							eDebug("[VLC] startdir = pathdir");
						else
						{
							a.Filename = _("[GO UP]");
							a.Fullname = entry.mid(4);
							a.Filetype = GOUP;
							playList.push_back(a);
						}
					}
					else
					{
						a.Filename = "[DIR] " + entry.mid(4);
						a.Fullname = entry.mid(4);
						a.Filetype = DIRS;
						playList.push_back(a);
					}
				}
				else
				{
					eString tmp = entry; 
					tmp.upper();
					for (ExtList::iterator p = extList.begin(); p != extList.end(); p++)
					{
						if (tmp.find((*p).EXT) != eString::npos)
						{
							a.Filename = entry;
							a.Fullname = entry;
							a.Filetype = FILES;
							a.Extitem = (*p).ITEM;
							playList.push_back(a);
							break;
						}
					}
				}
				start = pos + 1;
			}
		}
		else
		{
			char line[1024];
			strcpy(line, cddrive.c_str());
			if (line[strlen(line) - 1] == ':' && strlen(line) == 2) pathfull = cddrive + "/";

			for (ExtList::iterator p = extList.begin(); p != extList.end(); p++)
			{
				if ((*p).NAME == tmp2)
				{
					a.Filename = _("Play");
					a.Fullname = tmp3;
					a.Filetype = FILES;
					a.Extitem = (*p).ITEM;
					playList.push_back(a);
					break;
				}
			}
		}
		if (playList.size())
			viewList();
	}
}

void eSCGui::viewList()
{
	sort(playList.begin(), playList.end(), sortByType);

	list->beginAtomic();
	list->clearList();
	int current = 0;

	for (PlayList::iterator p = playList.begin(); p != playList.end(); p++)
	{
		switch ((*p).Filetype)
		{
			case GOUP:
				new eListBoxEntryText(list, (*p).Filename, (void *)current, 2);
				break;
			case DIRS:
			case FILES:
				new eListBoxEntryText(list, (*p).Filename, (void *)current);
				break;
		}
		current++;
		//eDebug("\n\nENTRY:%d\nFiletype:%d\nExtitem:%d\nFullname:%s",(*p).Filecounter, (*p).Filetype, (*p).Extitem, (*p).Fullname.c_str());
	}
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
		int val = (int)item->getKey();
		if (playList[val].Filetype == FILES)
			playerStart(val);
		else
		{
			pathfull = playList[val].Fullname;
			loadList();
		}
	}
}

bool eSCGui::loadXML(eString file)
{
	extList.clear();

	XMLTreeParser parser("ISO-8859-1");

	FILE *in = fopen(file.c_str(), "rt");
	if (!in) 
	{ 
		eDebug("[VLC] <unable to open %s>", file.c_str()); 
		return false; 
	}

	bool done = false;
	while (done == 0)
	{
		char buf[2048]; 
		unsigned int len = fread(buf, 1, sizeof(buf), in);
		done = len < sizeof(buf);
		if (!parser.Parse(buf, len, done))
		{
			eDebug("[VLC] <parse: %s at line %d>", parser.ErrorString(parser.GetErrorCode()), parser.GetCurrentLineNumber());
			fclose(in);
			return false;
		}
	}

	fclose(in);

	XMLTreeNode *root = parser.RootNode();
	if (!root)
		return false;

	int extcount = 0;
	for (XMLTreeNode *node = root->GetChild(); node; node = node->GetNext())
	{
		if (!strcmp(node->GetType(), "server"))
		{
			VLCsend::getInstance()->send_parms.IP = node->GetAttributeValue("ip");
			VLCsend::getInstance()->send_parms.IF_PORT = node->GetAttributeValue("webif-port");
			VLCsend::getInstance()->send_parms.STREAM_PORT = node->GetAttributeValue("stream-port");

			eString tmpuser = node->GetAttributeValue("user");
			eString tmppass = node->GetAttributeValue("pass");

			if (tmpuser && tmppass)
				VLCsend::getInstance()->send_parms.AUTH = tmpuser + ":" + tmppass;

		}
		else 
		if (!strcmp(node->GetType(), "config"))
		{
			startdir = node->GetAttributeValue("startdir");
			cddrive  = node->GetAttributeValue("cddrive");
			pathfull = startdir;
		}
		else 
		if (!strcmp(node->GetType(), "codec"))
		{
			str_mpeg1 = node->GetAttributeValue("mpeg1");
			str_mpeg2 = node->GetAttributeValue("mpeg2");
			str_audio = node->GetAttributeValue("audio");
		}
		else 
		if (!strcmp(node->GetType(), "setup"))
		{
			eString tmpname = node->GetAttributeValue("name");
			eString tmpext = node->GetAttributeValue("ext");
			eString tmpvrate = node->GetAttributeValue("Videorate");
			eString tmpvtrans = node->GetAttributeValue("Videotranscode");
			eString tmpvcodec = node->GetAttributeValue("Videocodec");
			eString tmpvsize = node->GetAttributeValue("Videosize");
			eString tmparate = node->GetAttributeValue("Audiorate");
			eString tmpatrans = node->GetAttributeValue("Audiotranscode");
			eString tmpac3 = node->GetAttributeValue("ac3");

			if (!tmpname || !tmpext || !tmpvrate || !tmpvtrans || !tmpvcodec || !tmpvsize || !tmparate  || !tmpatrans || !tmpac3)
			{
				eDebug("[VLC] <parse error in setup>");
				return false;
			}
			else
			{
				EXTLIST a;
				a.NAME = tmpname;
				a.EXT = "." + tmpext;
				a.VRATE = tmpvrate;
				a.VTRANS = (tmpvtrans == "1");
				a.VCODEC = tmpvcodec;
				a.VSIZE	 = tmpvsize;
				a.ARATE = tmparate;
				a.ITEM = extcount++;
				a.ATRANS = (tmpatrans == "1");
				a.AC3 = (tmpac3 == "1");
				extList.push_back(a);
			}
		}
	}

	/*eDebug("\nIP:%s",VLCsend::getInstance()->send_parms.IP.c_str());
	eDebug("WEBIF-PORT:%s",VLCsend::getInstance()->send_parms.IF_PORT.c_str());
	eDebug("Stream-PORT:%s\n",VLCsend::getInstance()->send_parms.STREAM_PORT.c_str());

	eDebug("STARTDIR:%s",startdir.c_str());
	eDebug("CDDRIVE:%s\n",cddrive.c_str());

	eDebug("MPEG1:%s",str_mpeg1.c_str());
	eDebug("MPEG2:%s",str_mpeg2.c_str());
	eDebug("Audio:%s\n",str_audio.c_str());

	for(ExtList::iterator p=extList.begin(); p!=extList.end() ;p++)
	{
		eDebug("name=%s ext=%s vrate=%s vcodec=%s vsize=%s arate=%s item=%d", (*p).NAME.c_str(), (*p).EXT.c_str(), (*p).VRATE.c_str(), (*p).VCODEC.c_str(),
		(*p).VSIZE.c_str(), (*p).ARATE.c_str(), (*p).ITEM);
	}*/

	return true;
}

void eSCGui::parseSout(int val)
{
	int curr = playList[val].Extitem;

	eSCplay::play_parms.ACT_AC3 = extList[curr].AC3;

	eString res_horiz, res_vert;
	
	if (extList[curr].VSIZE == "352x288")
	{
		res_horiz = "352";
		res_vert = "288";
	}
	else 
	if (extList[curr].VSIZE == "352x576")
	{
		res_horiz = "352";
		res_vert = "576";
	}
	else 
	if (extList[curr].VSIZE == "480x576")
	{
		res_horiz = "480";
		res_vert = "576";
	}
	else 
	if (extList[curr].VSIZE == "576x576")
	{
		res_horiz = "576";
		res_vert = "576";
	}
	else
	{
		res_horiz = "704";
		res_vert = "576";
	}

	eString conf = "#";
	if (extList[curr].VTRANS || extList[curr].ATRANS)
	{
		conf += "transcode{";
		if (extList[curr].VTRANS == true)
		{
			conf += "vcodec=";
			if (extList[curr].VCODEC == "mpeg2")
				conf += str_mpeg2;
			else
				conf += str_mpeg1;
			conf += ",vb=" + extList[curr].VRATE + ",width=" + res_horiz + ",height=" + res_vert;
		}
		if (extList[curr].ATRANS == true)
		{
			if (extList[curr].VTRANS == true)
				conf += ",";
			conf += "acodec=" + str_audio + ",ab=" + extList[curr].ARATE + ",channels=2";
		}
		conf += "}:";
	}
	conf += "duplicate{dst=std{access=http,mux=ts,url=:" + VLCsend::getInstance()->send_parms.STREAM_PORT + "/dboxstream}}";

	VLCsend::getInstance()->send("/?sout=" + conf);
	eDebug("[VLC] sout:%s", conf.c_str());
}

void eSCGui::showMenu()
{
	if (!menu)
	{
		hide();
		resize(eSize(550, 350));
		show();

		timer->stop();
		bufferingBox->hide();
		menu = true;
	}
}

void eSCGui::playerStop()
{
	if (eSCplay::play_parms.STAT == eSCplay::STOPPED)
	{
		if (MODE == DATA && !menu)
		{
			if (next_val >= (int) playList.size()) 
				next_val = 0;

			if (playList[next_val].Filetype == FILES)
			{
				playerStart(next_val);
				return;
			}
		}

		showMenu();
	}
	else
	{
		if (!eSCplay::play_parms.BUFFERFILLED)
			bufferingBox->show();
		else
			bufferingBox->hide();

		timer->start(1000,true);
	}
}

void eSCGui::playerStart(int val)
{
	if (menu)
	{
		hide();
		resize(eSize(0, 0));
		show();
		menu = false;
	}

	//start play
	eDebug("\n[VLC] play %d \"%s\"", val, playList[val].Fullname.c_str());
	VLCsend::getInstance()->send("/?control=add&mrl=" + playList[val].Fullname);
	next_val = val + 1;
	parseSout(val);
	eSCplay::playnow();
	timer->start(1000, true);
}

int eSCGui::eventHandler(const eWidgetEvent &e)
{
	bool new_jump = false;

	switch (e.type)
	{
	case eWidgetEvent::evtAction:
		if (e.action == &i_cursorActions->cancel)
		{
			showMenu();

			if (eSCplay::play_parms.STAT != eSCplay::STOPPED)
			{
				eSCplay::play_parms.STAT = eSCplay::STOPPED;
				showMenu();
			}
			else
				close(0);
		}
		else 
		if (e.action == &i_cursorActions->help && menu)
		{
			hide();
			eSCGuiInfo dlg;
			dlg.show();
			dlg.exec();
			dlg.hide();
			show();
		}
		else 
		if (e.action == &i_shortcutActions->red)
		{
			if (menu)
			{
				MODE=DATA;
				pathfull = startdir;
				loadList();
			}
			else
				eSCplay::play_parms.STAT = eSCplay::REW;
		}
		else 
		if (e.action == &i_shortcutActions->green)
		{
			if (menu)
			{
				MODE = VCD;
				loadList();
			}
			else
			{
				if (eSCplay::play_parms.STAT == eSCplay::PLAY)
					eSCplay::play_parms.STAT = eSCplay::RESYNC;
				else
					eSCplay::play_parms.STAT = eSCplay::PLAY;
			}
		}
		else 
		if (e.action == &i_shortcutActions->yellow)
		{
			if (menu)
			{
				MODE = SVCD;
				loadList();
			}
			else
				eSCplay::play_parms.STAT = eSCplay::STOPPED;
		}
		else 
		if (e.action == &i_shortcutActions->blue)
		{
			if (menu)
			{
				MODE = DVD;
				loadList();
			}
			else
				eSCplay::play_parms.STAT = eSCplay::FF;
		}
		else 
		if (e.action == &i_shortcutActions->number1) 
		{ 
			eSCplay::play_parms.JUMPMIN = 1; 
			new_jump = true; 
		}
		else 
		if (e.action == &i_shortcutActions->number2) 
		{ 
			eSCplay::play_parms.JUMPMIN = 2; 
			new_jump = true; 
		}
		else 
		if (e.action == &i_shortcutActions->number3) 
		{ 
			eSCplay::play_parms.JUMPMIN = 3; 
			new_jump = true; 
		}
		else 
		if (e.action == &i_shortcutActions->number4) 
		{ 
			eSCplay::play_parms.JUMPMIN = 4; 
			new_jump = true; 
		}
		else 
		if (e.action == &i_shortcutActions->number5) 
		{ 
			eSCplay::play_parms.JUMPMIN = 5; 
			new_jump = true; 
		}
		else 
		if (e.action == &i_shortcutActions->number6) 
		{ 
			eSCplay::play_parms.JUMPMIN = 6; 
			new_jump = true; 	
		}
		else 
		if (e.action == &i_shortcutActions->number7) 
		{ 
			eSCplay::play_parms.JUMPMIN = 7; 
			new_jump = true; 
		}
		else 
		if (e.action == &i_shortcutActions->number8) 
		{ 
			eSCplay::play_parms.JUMPMIN = 8; 
			new_jump = true; 
		}
		else 
		if (e.action == &i_shortcutActions->number9) 
		{ 
			eSCplay::play_parms.JUMPMIN = 9; 
			new_jump = true; 
		}
		else
			break;

		if (new_jump)
		{
			eMessageBox mb(eString().sprintf("Skip +/- %d minute(s)", eSCplay::play_parms.JUMPMIN).c_str(), _("Information"), eMessageBox::btOK|eMessageBox::iconInfo, eMessageBox::btOK, 3);
			mb.show();
			mb.exec();
			mb.hide();
		}
		return 1;
	default:
		break;
	}
	return eWindow::eventHandler(e);
}

static char * NAME[] = 
{
	"Keys (Menu is not visible)",
	"red:",
	_("Start skipping reverse"),
	"green:",
	_("Resync"),
	"yellow:",
	_("Select next entry"),
	"blue:",
	_("Start skipping forward"),
	"1 - 9:",
	"Set skip",
	" "
};

void eSCGuiInfo::info()
{
	cmove(ePoint(140, 100)); 
	cresize(eSize(440, 420)); 
	setText(_("Help window"));

	eListBox<eListBoxEntryText> *list = new eListBox<eListBoxEntryText>(this);
	list->move(ePoint(5, 5));
	list->resize(eSize(clientrect.width() - 10, clientrect.height() - 10));

	new eListBoxEntryText(list, REL);
	int i = 0;
	while (NAME[i] != " ")
		new eListBoxEntryText(list, NAME[i++]);
}

void eSCplay::playnow()
{
	play_parms.BUFFERFILLED = false;
	play_parms.AVPIDS_FOUND = false;
	play_parms.BUFFERTIME = 0;

	PlayStreamThread((void *) " ");
}

int eSCplay::VlcGetStreamTime()
{
	VLCsend::getInstance()->send("/admin/dboxfiles.html?stream_time=true");

	if (VLCsend::getInstance()->send_parms.RESPONSE)
		return atoi(VLCsend::getInstance()->send_parms.RESPONSE.c_str());
	else
		return -1;
}

int eSCplay::PlayStreamThread(void *mrl)
{
#if 0
	char buf[348 * 188];
	// use global pida and pidv
	play_parms.PIDA = 0, play_parms.PIDV = 0, play_parms.AC3 = -1;
	int dvr = -1;

	play_parms.STAT = SOFTRESET;

	int count=0;
	do
	{
		if ((dvr  = ::open (DVR, O_WRONLY | O_NONBLOCK)) < 0)
		{
			if (errno == EBUSY)
			{
				//eDebug("[VLC] pvr device busy try %d", count++);
				if (count < 40)
				{
					usleep(20000);
					continue;
				}
			}
			perror("[VLC] couldn't open /dev/pvr");
			play_parms.STAT = STOPPED;
		}
		break;
	}
	while(dvr < 0);

	ssize_t wr;
	size_t readsize, len = 0;
	bool driverready = false;

	while (play_parms.STAT > STOPPED)
	{
		readsize = ringbuffer_read_space (ringbuf);

		if (readsize > MAXREADSIZE)
			readsize = MAXREADSIZE;

		if (play_parms.BUFFERFILLED && readsize)
		{
			if (!driverready)
			{
				driverready = true;

				Decoder::parms.vpid=play_parms.PIDV;
				Decoder::parms.apid=play_parms.PIDA;
				Decoder::parms.audio_type= (play_parms.AC3 == 1 && play_parms.ACT_AC3) ? DECODE_AUDIO_AC3 : DECODE_AUDIO_MPEG;
				Decoder::Set();

				if ((play_parms.BUFFERTIME = VlcGetStreamTime()) < 0)
					play_parms.BUFFERTIME=0;
			}

			len = ringbuffer_read (ringbuf, buf, (readsize / 188) * 188);

			switch(play_parms.STAT)
			{
				case FF: case REW:
				{
					if (play_parms.STAT == FF)
						VLCsend::getInstance()->send(eString().sprintf("/?seek_value=%2B%dmin&control=seek",play_parms.JUMPMIN).c_str());
					else
						VLCsend::getInstance()->send(eString().sprintf("/?seek_value=%2D%dmin&control=seek",play_parms.JUMPMIN).c_str());
					cnt=0;
					play_parms.STAT = SKIP;
					if (::ioctl(dvr, 0)< 0)
					{
						//eDebug("PVR_FLUSH_BUFFER failed (%m)");
					}
					Decoder::flushBuffer();
					break;
				}
				case RESYNC:
				{
					//eDebug("[VLC] RESYNC: Buffering");
					Decoder::Pause();
					Decoder::Resume();
					play_parms.STAT = PLAY;
					break;
				}
				case PLAY:
				{
					int done = 0;
					while (len > 0)
					{
						wr = write (dvr, &buf[done], len);
						if (wr < 0)
						{
							if (errno != EAGAIN)
							{
								perror("[VLC] <write failed>");
								play_parms.STAT = STOPPED;
								break;
							}
							else
							{
								usleep(1000);
								continue;
							}
						}

						len -= wr;
						done += wr;
					}
					break;
				}
				case SOFTRESET:
				{
					play_parms.STAT = PLAY;
					break;
				}
				/*case PAUSE:
				{
					VLCsend::getInstance()->send("/?control=pause");
					Decoder::Pause();

					while (play_parms.STAT == PAUSE)
					{
						usleep(100000);
					}
					//eDebug("[VLC] PAUSE END");
					play_parms.BUFFERFILLED = false;
					VLCsend::getInstance()->send("/?control=pause");
					Decoder::Resume();
					//speed = 1;
					break;
				}
				case JF:
				case PREPARING:
				case STREAMERROR:
				case JB:*/
			}
		}
	}

	::close(dvr);

	Decoder::flushBuffer();
	if (::ioctl(dvr, 0)< 0)
	{
		//eDebug("PVR_FLUSH_BUFFER failed (%m)");
	}
	Decoder::Flush();

	pthread_join (rcst, NULL);
	//eDebug("[VLC] stopped succesfully\n");

	VLCsend::getInstance()->send("/?control=stop");
	VLCsend::getInstance()->send("/?control=empty");
	ringbuffer_free(ringbuf);
	pthread_exit (NULL);
#endif
	return 0;
}

size_t CurlDummyWrite (void *ptr, size_t size, size_t nmemb, void *data)
{
	std::string *pStr = (std::string *) data;
	*pStr += (char*) ptr;
	return size * nmemb;
}

CURLcode VLCsend::sendGetRequest (const eString& url, eString & response)
{
	CURL *curl;
	CURLcode httpres;

	eString tmpurl= "http://" + send_parms.IP + ":" + send_parms.IF_PORT + url;
	//eDebug("[VLC] GET: %s", tmpurl.c_str());

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, tmpurl.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlDummyWrite);
	curl_easy_setopt(curl, CURLOPT_FILE, (void *)&response);

	if(send_parms.AUTH)
		curl_easy_setopt (curl, CURLOPT_USERPWD, send_parms.AUTH.c_str());	
	curl_easy_setopt (curl, CURLOPT_FAILONERROR, true);
	httpres = curl_easy_perform (curl);
	//eDebug("[VLC] HTTP Result: %d", httpres);
	curl_easy_cleanup(curl);
	return httpres;
}

void VLCsend::send(eString val)
{
	send_parms.RESPONSE = "";
	if(send_parms.IP && send_parms.IF_PORT && val)
		sendGetRequest(val, send_parms.RESPONSE);
	else
		eDebug("[VLC] <send>");
}

VLCsend *VLCsend::getInstance()
{
	static VLCsend *m_VLCsend = NULL;
	if (m_VLCsend == NULL)
		m_VLCsend = new VLCsend();
	return m_VLCsend;
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
