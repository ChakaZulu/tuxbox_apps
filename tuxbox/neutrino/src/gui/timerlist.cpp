/*
	$Id: timerlist.cpp,v 1.100 2009/03/28 14:48:57 seife Exp $

	Neutrino-GUI  -   DBoxII-Project

	Timerliste by Zwen

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

#include <gui/timerlist.h>
#include <gui/pluginlist.h>
#include <gui/plugins.h>

#include <daemonc/remotecontrol.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/color.h>
#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/channellist.h>

#include <gui/widget/buttons.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/mountchooser.h>
#include <gui/widget/dirchooser.h>

#include <system/settings.h>
#include <system/fsmounter.h>

#include <global.h>
#include <neutrino.h>

#include <zapit/client/zapitclient.h>
#include <zapit/client/zapittools.h>

#include <string.h>
#include <algorithm>

#define info_height 60


class CTimerListNewNotifier : public CChangeObserver
{
private:
	CMenuItem* m1;
	CMenuItem* m2;
	CMenuItem* m3;
	CMenuItem* m4;
	CMenuItem* m5;
	CMenuItem* m6;
	char* display;
	int* iType;
	time_t* stopTime;
public:
	CTimerListNewNotifier( int* Type, time_t* time,CMenuItem* a1, CMenuItem* a2,
			       CMenuItem* a3, CMenuItem* a4, CMenuItem* a5, CMenuItem* a6,char* d)
	{
		m1 = a1;
		m2 = a2;
		m3 = a3;
		m4 = a4;
		m5 = a5;
		m6 = a6;
		display=d;
		iType=Type;
		stopTime=time;
	}
	bool changeNotify(const neutrino_locale_t, void *)
	{
		CTimerd::CTimerEventTypes type = (CTimerd::CTimerEventTypes) *iType;
		if(type == CTimerd::TIMER_RECORD)
		{
			*stopTime=(time(NULL)/60)*60;
			struct tm *tmTime2 = localtime(stopTime);
			sprintf( display, "%02d.%02d.%04d %02d:%02d", tmTime2->tm_mday, tmTime2->tm_mon+1,
						tmTime2->tm_year+1900,
						tmTime2->tm_hour, tmTime2->tm_min);
			m1->setActive(true);
			m6->setActive((g_settings.recording_type == RECORDING_FILE));
		}
		else
		{
			*stopTime=0;
			strcpy(display,"                ");
			m1->setActive (false);
			m6->setActive(false);
		}
		if(type == CTimerd::TIMER_RECORD ||
			type == CTimerd::TIMER_ZAPTO ||
			type == CTimerd::TIMER_NEXTPROGRAM)
		{
			m2->setActive(true);
		}
		else
		{
			m2->setActive(false);
		}
		if(type == CTimerd::TIMER_STANDBY)
			m3->setActive(true);
		else
			m3->setActive(false);
		if(type == CTimerd::TIMER_REMIND)
			m4->setActive(true);
		else
			m4->setActive(false);
		if(type == CTimerd::TIMER_EXEC_PLUGIN)
			m5->setActive(true);
		else
			m5->setActive(false);
		return true;
	}
};

class CTimerListRepeatNotifier : public CChangeObserver
{
private:
	CMenuForwarder* m1;
	CMenuForwarder* m2;

	int* iRepeat;
public:
	CTimerListRepeatNotifier( int* repeat, CMenuForwarder* a1, CMenuForwarder *a2)
	{
		m1 = a1;
		m2 = a2;
		iRepeat=repeat;
	}

	bool changeNotify(const neutrino_locale_t, void *)
	{
		if(*iRepeat >= (int)CTimerd::TIMERREPEAT_WEEKDAYS)
			m1->setActive (true);
		else
			m1->setActive (false);
		if (*iRepeat != (int)CTimerd::TIMERREPEAT_ONCE)
			m2->setActive(true);
		else
			m2->setActive(false);
		return true;
	}
};

class CTimerListApidNotifier : public CChangeObserver
{
private:
	int* o_dflt;
	int* o_std;
	int* o_alt;
	int* o_ac3;
	CMenuItem* m_dflt;
	CMenuItem* m_std;
	CMenuItem* m_alt;
	CMenuItem* m_ac3;
public:
	CTimerListApidNotifier( int* o1, int* o2, int* o3, int* o4)
	{
		o_dflt=o1;
		o_std=o2;
		o_alt=o3;
		o_ac3=o4;
	}

	void setItems(CMenuItem* m1, CMenuItem* m2, CMenuItem* m3, CMenuItem* m4)
	{
		m_dflt=m1;
		m_std=m2;
		m_alt=m3;
		m_ac3=m4;
	}

	bool changeNotify(const neutrino_locale_t OptionName, void *)
	{
		if(OptionName == LOCALE_TIMERLIST_APIDS_DFLT)
		{
			if(*o_dflt==0)
			{
				m_std->setActive(true);
				m_alt->setActive(true);
				m_ac3->setActive(true);
			}
			else
			{
				m_std->setActive(false);
				m_alt->setActive(false);
				m_ac3->setActive(false);
				*o_std=0;
				*o_alt=0;
				*o_ac3=0;
			}
		}
		else
		{
			if(*o_std || *o_alt || *o_ac3)
					 *o_dflt=0;
		}
		return true;
	}
};


CTimerList::CTimerList()
{
	frameBuffer = CFrameBuffer::getInstance();
	visible = false;
	selected = 0;
	// Max
	width = 720;
	if(g_settings.screen_EndX-g_settings.screen_StartX < width)
		width=g_settings.screen_EndX-g_settings.screen_StartX-10;
	buttonHeight = 25;
	theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;
	liststart = 0;
	Timer = new CTimerdClient();
	skipEventID=0;
}

CTimerList::~CTimerList()
{
	timerlist.clear();
	delete Timer;
}

int CTimerList::exec(CMenuTarget* parent, const std::string & actionKey)
{
	const char * key = actionKey.c_str();
	if (strcmp(key, "modifytimer") == 0)
	{
		timerlist[selected].announceTime = timerlist[selected].alarmTime -60;
		if(timerlist[selected].eventRepeat >= CTimerd::TIMERREPEAT_WEEKDAYS)
			Timer->getWeekdaysFromStr((int *)&timerlist[selected].eventRepeat, m_weekdaysStr);
		if(timerlist[selected].eventType == CTimerd::TIMER_RECORD)
		{
			timerlist[selected].announceTime -= 120; // 2 more mins for rec timer
			if (timer_apids_dflt)
				timerlist[selected].apids = TIMERD_APIDS_CONF;
			else
				timerlist[selected].apids = (timer_apids_std * TIMERD_APIDS_STD) | (timer_apids_ac3 * TIMERD_APIDS_AC3) |
					(timer_apids_alt * TIMERD_APIDS_ALT);
			Timer->modifyTimerAPid(timerlist[selected].eventID,timerlist[selected].apids);
			Timer->modifyRecordTimerEvent(timerlist[selected].eventID, timerlist[selected].announceTime,
						      timerlist[selected].alarmTime,
						      timerlist[selected].stopTime, timerlist[selected].eventRepeat,
						      timerlist[selected].repeatCount,timerlist[selected].recordingDir);
		} else
		{
			Timer->modifyTimerEvent(timerlist[selected].eventID, timerlist[selected].announceTime,
						timerlist[selected].alarmTime,
						timerlist[selected].stopTime, timerlist[selected].eventRepeat,
						timerlist[selected].repeatCount);
		}
		return menu_return::RETURN_EXIT;
	}
	else if (strcmp(key, "newtimer") == 0)
	{
		timerNew.announceTime=timerNew.alarmTime-60;
		CTimerd::EventInfo eventinfo;
		CTimerd::RecordingInfo recinfo;
		eventinfo.epgID=0;
		eventinfo.epg_starttime=0;
		eventinfo.channel_id=timerNew.channel_id;
		eventinfo.apids = TIMERD_APIDS_CONF;
		eventinfo.recordingSafety = false;
		timerNew.standby_on = (timerNew_standby_on == 1);
		void *data=NULL;
		if(timerNew.eventType == CTimerd::TIMER_STANDBY)
			data=&(timerNew.standby_on);
		else if(timerNew.eventType==CTimerd::TIMER_NEXTPROGRAM ||
			timerNew.eventType==CTimerd::TIMER_ZAPTO ||
			timerNew.eventType==CTimerd::TIMER_RECORD)
		{
			if (strcmp(timerNew_channel_name, "---")==0)
				return menu_return::RETURN_REPAINT;
			if (timerNew.eventType==CTimerd::TIMER_RECORD)
			{
				recinfo.epgID=0;
				recinfo.epg_starttime=0;
				recinfo.channel_id=timerNew.channel_id;
				recinfo.apids=TIMERD_APIDS_CONF;
				recinfo.recordingSafety = false;

				timerNew.announceTime-= 120; // 2 more mins for rec timer
				strncpy(recinfo.recordingDir,timerNew.recordingDir,sizeof(recinfo.recordingDir));
				data = &recinfo;
			} else
				data= &eventinfo;
		}
		else if(timerNew.eventType==CTimerd::TIMER_REMIND)
			data= timerNew.message;
		else if (timerNew.eventType==CTimerd::TIMER_EXEC_PLUGIN)
		{
			if (strcmp(timerNew.pluginName, "---") == 0)
				return menu_return::RETURN_REPAINT;
			data= timerNew.pluginName;
		}
		if(timerNew.eventRepeat >= CTimerd::TIMERREPEAT_WEEKDAYS)
			Timer->getWeekdaysFromStr((int *)&timerNew.eventRepeat, m_weekdaysStr);

		if (Timer->addTimerEvent(timerNew.eventType,data,timerNew.announceTime,timerNew.alarmTime,
					 timerNew.stopTime,timerNew.eventRepeat,timerNew.repeatCount,false) == -1)
		{
			bool forceAdd = askUserOnTimerConflict(timerNew.announceTime,timerNew.stopTime);

			if (forceAdd)
			{
				Timer->addTimerEvent(timerNew.eventType,data,timerNew.announceTime,timerNew.alarmTime,
						     timerNew.stopTime, timerNew.eventRepeat,timerNew.repeatCount,true);
			}
		}
		return menu_return::RETURN_EXIT;
	}
	else if (strncmp(key, "SC:", 3) == 0)
	{
		int delta;
		sscanf(&(key[3]),
		       SCANF_CHANNEL_ID_TYPE
		       "%n",
		       &timerNew.channel_id,
		       &delta);
#if 0
		strncpy(timerNew_channel_name, &(key[3 + delta + 1]), 30);
#else
		strncpy(timerNew_channel_name, ZapitTools::UTF8_to_Latin1(&(key[3 + delta + 1])).c_str(), 30);
#endif
		g_RCInput->postMsg(CRCInput::RC_timeout, 0); // leave underlying menu also
		g_RCInput->postMsg(CRCInput::RC_timeout, 0); // leave underlying menu also
		return menu_return::RETURN_EXIT;
	}

	if(parent)
	{
		parent->hide();
	}

	int ret = show();

	return ret;
/*
	if( ret > -1)
	{
		return menu_return::RETURN_REPAINT;
	}
	else if( ret == -1)
	{
		// -1 bedeutet nur REPAINT
		return menu_return::RETURN_REPAINT;
	}
	else
	{
		// -2 bedeutet EXIT_ALL
		return menu_return::RETURN_EXIT_ALL;
	}*/
}

void CTimerList::updateEvents(void)
{
	timerlist.clear();
	Timer->getTimerList (timerlist);
	//Remove last deleted event from List
	CTimerd::TimerList::iterator timer = timerlist.begin();
	for(; timer != timerlist.end();timer++)
	{
		if(timer->eventID==skipEventID)
		{
			timerlist.erase(timer);
			break;
		}
	}
	sort(timerlist.begin(), timerlist.end());

	height = (g_settings.screen_EndY-g_settings.screen_StartY)-(info_height+50);
	listmaxshow = (height-theight-0)/(fheight*2);
	height = theight+0+listmaxshow*fheight*2;	// recalc height
	if(timerlist.size() < listmaxshow)
	{
		listmaxshow=timerlist.size();
		height = theight+0+listmaxshow*fheight*2;	// recalc height
	}
	if(selected==timerlist.size() && !(timerlist.empty()))
	{
		selected=timerlist.size()-1;
		liststart = (selected/listmaxshow)*listmaxshow;
	}
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-( height+ info_height) ) / 2) + g_settings.screen_StartY;
}


int CTimerList::show()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

	bool loop=true;
	bool update=true;
	while(loop)
	{
		if(update)
		{
			hide();
			updateEvents();
			update=false;
//			if (timerlist.empty())
//			{
				//evtl. anzeige dass keine kanalliste....
				/* ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_TIMERLIST_EMPTY));
				 return -1;*/
//			}
			paint();
		}
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

		if( ( msg == CRCInput::RC_timeout ) ||
			 ( msg == CRCInput::RC_home) )
		{ //Exit after timeout or cancel key
			loop=false;
		}
		else if (msg_repeatok == CRCInput::RC_up && !timerlist.empty())
		{
			int prevselected=selected;
			if(selected==0)
			{
				selected = timerlist.size()-1;
			}
			else
				selected--;
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if (msg_repeatok == CRCInput::RC_down && !timerlist.empty())
		{
			int prevselected=selected;
			selected = (selected+1)%timerlist.size();
			paintItem(prevselected - liststart);
			unsigned int oldliststart = liststart;
			liststart = (selected/listmaxshow)*listmaxshow;
			if(oldliststart!=liststart)
			{
				paint();
			}
			else
			{
				paintItem(selected - liststart);
			}
		}
		else if ((msg == CRCInput::RC_ok) && !(timerlist.empty()))
		{
			if (modifyTimer()==menu_return::RETURN_EXIT_ALL)
			{
				res=menu_return::RETURN_EXIT_ALL;
				loop=false;
			}
			else
				update=true;
		}
		else if((msg == CRCInput::RC_red) && !(timerlist.empty()))
		{
			Timer->removeTimerEvent(timerlist[selected].eventID);
			skipEventID=timerlist[selected].eventID;
			update=true;
		}
		else if(msg==CRCInput::RC_green)
		{
			if (newTimer()==menu_return::RETURN_EXIT_ALL)
			{
				res=menu_return::RETURN_EXIT_ALL;
				loop=false;
			}
			else
				update=true;
		}
		else if(msg==CRCInput::RC_yellow)
		{
			update=true;
		}
		else if((msg==CRCInput::RC_blue)||
				  (CRCInput::isNumeric(msg)) )
		{
			//pushback key if...
			g_RCInput->postMsg( msg, data );
			loop=false;
		}
		else if(msg==CRCInput::RC_setup)
		{
			res=menu_return::RETURN_EXIT_ALL;
			loop=false;
		}
		else if( msg == CRCInput::RC_help )
		{
			CTimerd::responseGetTimer* timer=&timerlist[selected];
			if(timer!=NULL)
			{
				if(timer->eventType == CTimerd::TIMER_RECORD || timer->eventType == CTimerd::TIMER_ZAPTO)
				{
					hide();
					res = g_EpgData->show(timer->channel_id, timer->epgID, &timer->epg_starttime);
					if(res==menu_return::RETURN_EXIT_ALL)
						loop=false;
					else
						paint();
				}
			}
			// help key
		}
		else
		{
			if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
				res = menu_return::RETURN_EXIT_ALL;
			}
		}
	}
	hide();

	return(res);
}

void CTimerList::hide()
{
	if(visible)
	{
		frameBuffer->paintBackgroundBoxRel(x, y, width, height+ info_height+ 5);
		visible = false;
	}
}

void CTimerList::paintItem(int pos)
{
	int ypos = y+ theight+0 + pos*fheight*2;

	uint8_t    color;
	fb_pixel_t bgcolor;

	if (pos & 1)
	{
		color   = COL_MENUCONTENTDARK;
		bgcolor = COL_MENUCONTENTDARK_PLUS_0;
	}
	else
	{
		color   = COL_MENUCONTENT;
		bgcolor = COL_MENUCONTENT_PLUS_0;
	}

	if (liststart + pos == selected)
	{
		color   = COL_MENUCONTENTSELECTED;
		bgcolor = COL_MENUCONTENTSELECTED_PLUS_0;
	}

	int real_width=width;
	if(timerlist.size()>listmaxshow)
	{
		real_width-=15; //scrollbar
	}

	frameBuffer->paintBoxRel(x,ypos, real_width, 2*fheight, bgcolor);
	if(liststart+pos<timerlist.size())
	{
		CTimerd::responseGetTimer & timer = timerlist[liststart+pos];
		char zAlarmTime[25] = {0};
		struct tm *alarmTime = localtime(&(timer.alarmTime));
		strftime(zAlarmTime,20,"%d.%m. %H:%M",alarmTime);
		char zStopTime[25] = {0};
		struct tm *stopTime = localtime(&(timer.stopTime));
		strftime(zStopTime,20,"%d.%m. %H:%M",stopTime);
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10,ypos+fheight, 150, zAlarmTime, color, fheight, true); // UTF-8
		if(timer.stopTime != 0)
		{
			g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+10,ypos+2*fheight, 150, zStopTime, color, fheight, true); // UTF-8
		}
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+160,ypos+fheight, (real_width-160)/2-5, convertTimerRepeat2String(timer.eventRepeat), color, fheight, true); // UTF-8

		if (timer.eventRepeat != CTimerd::TIMERREPEAT_ONCE)
		{
			char srepeatcount[25] = {0};
			if (timer.repeatCount == 0)
// Unicode 8734 (hex: 221E) not available in all fonts
// 			sprintf(srepeatcount,"∞");
				sprintf(srepeatcount,"00");
			else
				sprintf(srepeatcount,"%ux",timer.repeatCount);
			g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+160+(real_width-300)/2,ypos+fheight, (real_width-160)/2-5, srepeatcount, color, fheight, true); // UTF-8
		}
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+160+(real_width-160)/2,ypos+fheight, (real_width-160)/2-5, convertTimerType2String(timer.eventType), color, fheight, true); // UTF-8
		std::string zAddData("");
		switch(timer.eventType)
		{
			case CTimerd::TIMER_NEXTPROGRAM :
			case CTimerd::TIMER_ZAPTO :
			case CTimerd::TIMER_RECORD :
				{
					zAddData = convertChannelId2String(timer.channel_id); // UTF-8
					if(timer.apids != TIMERD_APIDS_CONF)
					{
						std::string sep = "";
						zAddData += " (";
						if(timer.apids & TIMERD_APIDS_STD)
						{
							zAddData += "STD";
							sep = "/";
						}
						if(timer.apids & TIMERD_APIDS_ALT)
						{
							zAddData += sep;
							zAddData += "ALT";
							sep = "/";
						}
						if(timer.apids & TIMERD_APIDS_AC3)
						{
							zAddData += sep;
							zAddData += "AC3";
							sep = "/";
						}
						zAddData += ')';
					}
					if(timer.epgID!=0)
					{
						CEPGData epgdata;
						if (g_Sectionsd->getEPGid(timer.epgID, timer.epg_starttime, &epgdata))
						{
#warning fixme sectionsd should deliver data in UTF-8 format
							zAddData += " : ";
							zAddData += Latin1_to_UTF8(epgdata.title);
						}
						else if(strlen(timer.epgTitle)!=0)
						{
							zAddData += " : ";
							zAddData += Latin1_to_UTF8(timer.epgTitle);
						}
					}
					else if(strlen(timer.epgTitle)!=0)
					{
						zAddData += " : ";
						zAddData += Latin1_to_UTF8(timer.epgTitle);
					}
				}
				break;
			case CTimerd::TIMER_STANDBY:
				{
					zAddData = g_Locale->getText(timer.standby_on ? LOCALE_TIMERLIST_STANDBY_ON : LOCALE_TIMERLIST_STANDBY_OFF);
					break;
				}
			case CTimerd::TIMER_REMIND :
				{
					zAddData = timer.message; // must be UTF-8 encoded !
				}
				break;
			case CTimerd::TIMER_EXEC_PLUGIN :
			{
				zAddData = timer.pluginName;
			}
			break;
			default:{}
		}
		g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+160,ypos+2*fheight, real_width-165, zAddData, color, fheight, true); // UTF-8
		// LCD Display
		if(liststart+pos==selected)
		{
			std::string line1 = convertTimerType2String(timer.eventType); // UTF-8
			std::string line2 = zAlarmTime;
			switch(timer.eventType)
			{
				case CTimerd::TIMER_RECORD :
					line2+= " -";
					line2+= zStopTime+6;
				case CTimerd::TIMER_NEXTPROGRAM :
				case CTimerd::TIMER_ZAPTO :
					{
						line1 += ' ';
						line1 += convertChannelId2String(timer.channel_id); // UTF-8
					}
					break;
				case CTimerd::TIMER_STANDBY :
					{
						if(timer.standby_on)
							line1+=" ON";
						else
							line1+=" OFF";
					}
					break;
			default:;
			}
			CLCD::getInstance()->showMenuText(0, line1.c_str(), -1, true); // UTF-8
			CLCD::getInstance()->showMenuText(1, line2.c_str(), -1, true); // UTF-8
		}
	}
}

void CTimerList::paintHead()
{
	int c_rad_mid = RADIUS_MID;
	frameBuffer->paintBoxRel(x, y, width, theight, COL_MENUHEAD_PLUS_0, c_rad_mid, CORNER_TOP);
	frameBuffer->paintIcon("timer.raw", x + 5, y + 6);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x + 35, y + theight + 2, width - 45, g_Locale->getText(LOCALE_TIMERLIST_NAME), COL_MENUHEAD, 0, true); // UTF-8

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, x + width - 30, y + 6 );
}

const struct button_label TimerListButtons[3] =
{
	{ NEUTRINO_ICON_BUTTON_RED   , LOCALE_TIMERLIST_DELETE },
	{ NEUTRINO_ICON_BUTTON_GREEN , LOCALE_TIMERLIST_NEW    },
	{ NEUTRINO_ICON_BUTTON_YELLOW, LOCALE_TIMERLIST_RELOAD }
};

void CTimerList::paintFoot()
{
	int c_rad_mid = RADIUS_MID;
	int ButtonWidth = (width - 20) / 4;
	frameBuffer->paintBoxRel(x, y + height, width, buttonHeight, COL_INFOBAR_SHADOW_PLUS_1, c_rad_mid, CORNER_BOTTOM);

	if (timerlist.empty())
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + ButtonWidth + 10, y + height + 1, ButtonWidth, 2, &(TimerListButtons[1]));
	else
	{
		::paintButtons(frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, x + 10, y + height + 1, ButtonWidth, 3, TimerListButtons);

		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_OKAY_SMALL, x + width - ButtonWidth + 10, y + height + 3);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x + width - ButtonWidth + 38, y + height + 24 - 1, ButtonWidth - 28, g_Locale->getText(LOCALE_TIMERLIST_MODIFY), COL_INFOBAR_SHADOW_PLUS_1, 0, true); // UTF-8
	}
}

void CTimerList::paint()
{
	unsigned int page_nr = (listmaxshow == 0) ? 0 : (selected / listmaxshow);
	liststart = page_nr * listmaxshow;

	CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, g_Locale->getText(LOCALE_TIMERLIST_NAME));

	paintHead();
	for(unsigned int count=0;count<listmaxshow;count++)
	{
		paintItem(count);
	}

	if(timerlist.size()>listmaxshow)
	{
		int ypos = y+ theight;
		int sb = 2*fheight* listmaxshow;
		frameBuffer->paintBoxRel(x+ width- 15,ypos, 15, sb, COL_MENUCONTENT_PLUS_1);

		int sbc= ((timerlist.size()- 1)/ listmaxshow)+ 1;

		frameBuffer->paintBoxRel(x+ width-13, ypos+2+page_nr*(sb-4)/sbc , 11, (sb-4)/sbc, COL_MENUCONTENT_PLUS_3);
	}

	paintFoot();
	visible = true;
}

const char * CTimerList::convertTimerType2String(const CTimerd::CTimerEventTypes type) // UTF-8
{
	switch(type)
	{
		case CTimerd::TIMER_SHUTDOWN    : return g_Locale->getText(LOCALE_TIMERLIST_TYPE_SHUTDOWN   );
		case CTimerd::TIMER_NEXTPROGRAM : return g_Locale->getText(LOCALE_TIMERLIST_TYPE_NEXTPROGRAM);
		case CTimerd::TIMER_ZAPTO       : return g_Locale->getText(LOCALE_TIMERLIST_TYPE_ZAPTO      );
		case CTimerd::TIMER_STANDBY     : return g_Locale->getText(LOCALE_TIMERLIST_TYPE_STANDBY    );
		case CTimerd::TIMER_RECORD      : return g_Locale->getText(LOCALE_TIMERLIST_TYPE_RECORD     );
		case CTimerd::TIMER_REMIND      : return g_Locale->getText(LOCALE_TIMERLIST_TYPE_REMIND     );
		case CTimerd::TIMER_SLEEPTIMER  : return g_Locale->getText(LOCALE_TIMERLIST_TYPE_SLEEPTIMER );
		case CTimerd::TIMER_EXEC_PLUGIN : return g_Locale->getText(LOCALE_TIMERLIST_TYPE_EXECPLUGIN );
		default                         : return g_Locale->getText(LOCALE_TIMERLIST_TYPE_UNKNOWN    );
	}
}

std::string CTimerList::convertTimerRepeat2String(const CTimerd::CTimerEventRepeat rep) // UTF-8
{
	switch(rep)
	{
		case CTimerd::TIMERREPEAT_ONCE               : return g_Locale->getText(LOCALE_TIMERLIST_REPEAT_ONCE              );
		case CTimerd::TIMERREPEAT_DAILY              : return g_Locale->getText(LOCALE_TIMERLIST_REPEAT_DAILY             );
		case CTimerd::TIMERREPEAT_WEEKLY             : return g_Locale->getText(LOCALE_TIMERLIST_REPEAT_WEEKLY            );
		case CTimerd::TIMERREPEAT_BIWEEKLY           : return g_Locale->getText(LOCALE_TIMERLIST_REPEAT_BIWEEKLY          );
		case CTimerd::TIMERREPEAT_FOURWEEKLY         : return g_Locale->getText(LOCALE_TIMERLIST_REPEAT_FOURWEEKLY        );
		case CTimerd::TIMERREPEAT_MONTHLY            : return g_Locale->getText(LOCALE_TIMERLIST_REPEAT_MONTHLY           );
		case CTimerd::TIMERREPEAT_BYEVENTDESCRIPTION : return g_Locale->getText(LOCALE_TIMERLIST_REPEAT_BYEVENTDESCRIPTION);
		default:
			if(rep >=CTimerd::TIMERREPEAT_WEEKDAYS)
			{
				int weekdays = (((int)rep) >> 9);
				std::string weekdayStr="";
				if(weekdays & 1)
					weekdayStr+= g_Locale->getText(LOCALE_TIMERLIST_REPEAT_MONDAY);
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= g_Locale->getText(LOCALE_TIMERLIST_REPEAT_TUESDAY);
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= g_Locale->getText(LOCALE_TIMERLIST_REPEAT_WEDNESDAY);
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= g_Locale->getText(LOCALE_TIMERLIST_REPEAT_THURSDAY);
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= g_Locale->getText(LOCALE_TIMERLIST_REPEAT_FRIDAY);
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= g_Locale->getText(LOCALE_TIMERLIST_REPEAT_SATURDAY);
				weekdays >>= 1;
				if(weekdays & 1)
					weekdayStr+= g_Locale->getText(LOCALE_TIMERLIST_REPEAT_SUNDAY);
				return weekdayStr;
			}
			else
				return g_Locale->getText(LOCALE_TIMERLIST_REPEAT_UNKNOWN);
	}
}

std::string CTimerList::convertChannelId2String(const t_channel_id id) // UTF-8
{
	CZapitClient Zapit;
	std::string name = Zapit.getChannelName(id); // UTF-8
	if (name.empty())
		name = g_Locale->getText(LOCALE_TIMERLIST_PROGRAM_UNKNOWN);

	return name;
}

#define TIMERLIST_REPEAT_OPTION_COUNT 7
const CMenuOptionChooser::keyval TIMERLIST_REPEAT_OPTIONS[TIMERLIST_REPEAT_OPTION_COUNT] =
{
	{ CTimerd::TIMERREPEAT_ONCE       , LOCALE_TIMERLIST_REPEAT_ONCE       },
	{ CTimerd::TIMERREPEAT_DAILY      , LOCALE_TIMERLIST_REPEAT_DAILY      },
	{ CTimerd::TIMERREPEAT_WEEKLY     , LOCALE_TIMERLIST_REPEAT_WEEKLY     },
	{ CTimerd::TIMERREPEAT_BIWEEKLY   , LOCALE_TIMERLIST_REPEAT_BIWEEKLY   },
	{ CTimerd::TIMERREPEAT_FOURWEEKLY , LOCALE_TIMERLIST_REPEAT_FOURWEEKLY },
	{ CTimerd::TIMERREPEAT_MONTHLY    , LOCALE_TIMERLIST_REPEAT_MONTHLY    },
	{ CTimerd::TIMERREPEAT_WEEKDAYS   , LOCALE_TIMERLIST_REPEAT_WEEKDAYS   }
};

#define TIMERLIST_STANDBY_OPTION_COUNT 2
const CMenuOptionChooser::keyval TIMERLIST_STANDBY_OPTIONS[TIMERLIST_STANDBY_OPTION_COUNT] =
{
	{ 0 , LOCALE_TIMERLIST_STANDBY_OFF },
	{ 1 , LOCALE_TIMERLIST_STANDBY_ON  }
};

#if 1
#define TIMERLIST_TYPE_OPTION_COUNT 7
#else
#define TIMERLIST_TYPE_OPTION_COUNT 8
#endif
const CMenuOptionChooser::keyval TIMERLIST_TYPE_OPTIONS[TIMERLIST_TYPE_OPTION_COUNT] =
{
	{ CTimerd::TIMER_SHUTDOWN   , LOCALE_TIMERLIST_TYPE_SHUTDOWN    },
#if 0
	{ CTimerd::TIMER_NEXTPROGRAM, LOCALE_TIMERLIST_TYPE_NEXTPROGRAM },
#endif
	{ CTimerd::TIMER_ZAPTO      , LOCALE_TIMERLIST_TYPE_ZAPTO       },
	{ CTimerd::TIMER_STANDBY    , LOCALE_TIMERLIST_TYPE_STANDBY     },
	{ CTimerd::TIMER_RECORD     , LOCALE_TIMERLIST_TYPE_RECORD      },
	{ CTimerd::TIMER_SLEEPTIMER , LOCALE_TIMERLIST_TYPE_SLEEPTIMER  },
	{ CTimerd::TIMER_REMIND     , LOCALE_TIMERLIST_TYPE_REMIND      },
	{ CTimerd::TIMER_EXEC_PLUGIN, LOCALE_TIMERLIST_TYPE_EXECPLUGIN  }
};

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};

int CTimerList::modifyTimer()
{
	CTimerd::responseGetTimer* timer=&timerlist[selected];
	CMenuWidget timerSettings(LOCALE_TIMERLIST_MENUMODIFY, NEUTRINO_ICON_SETTINGS);
	timerSettings.addItem(GenericMenuSeparator);
	timerSettings.addItem(GenericMenuBack);
	timerSettings.addItem(GenericMenuSeparatorLine);
	timerSettings.addItem(new CMenuForwarder(LOCALE_TIMERLIST_SAVE, true, NULL, this, "modifytimer", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	timerSettings.addItem(GenericMenuSeparatorLine);

	char type[80];
	strcpy(type, ZapitTools::UTF8_to_Latin1(convertTimerType2String(timer->eventType)).c_str()); // UTF8, UTF8 -> Latin1
	CMenuForwarder *m0 = new CMenuForwarder(LOCALE_TIMERLIST_TYPE, false, type);
	timerSettings.addItem( m0);

	CDateInput timerSettings_alarmTime(LOCALE_TIMERLIST_ALARMTIME, &timer->alarmTime , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CMenuForwarder *m1 = new CMenuForwarder(LOCALE_TIMERLIST_ALARMTIME, true, timerSettings_alarmTime.getValue (), &timerSettings_alarmTime );
	timerSettings.addItem( m1);

	CDateInput timerSettings_stopTime(LOCALE_TIMERLIST_STOPTIME, &timer->stopTime , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	if(timer->stopTime != 0)
	{
		CMenuForwarder *m2 = new CMenuForwarder(LOCALE_TIMERLIST_STOPTIME, true, timerSettings_stopTime.getValue (), &timerSettings_stopTime );
		timerSettings.addItem( m2);
	}

	Timer->setWeekdaysToStr(timer->eventRepeat, m_weekdaysStr);
	timer->eventRepeat = (CTimerd::CTimerEventRepeat)(((int)timer->eventRepeat) & 0x1FF);
	CStringInput timerSettings_weekdays(LOCALE_TIMERLIST_WEEKDAYS, m_weekdaysStr, 7, LOCALE_TIMERLIST_WEEKDAYS_HINT_1, LOCALE_TIMERLIST_WEEKDAYS_HINT_2, "-X");
	CMenuForwarder *m4 = new CMenuForwarder(LOCALE_TIMERLIST_WEEKDAYS, ((int)timer->eventRepeat) >= (int)CTimerd::TIMERREPEAT_WEEKDAYS, m_weekdaysStr, &timerSettings_weekdays );
	CIntInput timerSettings_repeatCount(LOCALE_TIMERLIST_REPEATCOUNT, (long&)timer->repeatCount,3, LOCALE_TIMERLIST_REPEATCOUNT_HELP1, LOCALE_TIMERLIST_REPEATCOUNT_HELP2);

	CMenuForwarder *m5 = new CMenuForwarder(LOCALE_TIMERLIST_REPEATCOUNT, timer->eventRepeat != (int)CTimerd::TIMERREPEAT_ONCE ,timerSettings_repeatCount.getValue() , &timerSettings_repeatCount);

	CTimerListRepeatNotifier notifier((int *)&timer->eventRepeat,m4,m5);
	CMenuOptionChooser* m3 = new CMenuOptionChooser(LOCALE_TIMERLIST_REPEAT, (int *)&timer->eventRepeat, TIMERLIST_REPEAT_OPTIONS, TIMERLIST_REPEAT_OPTION_COUNT, true, &notifier);

	CRecDirChooser recDirs(LOCALE_TIMERLIST_RECORDING_DIR,NEUTRINO_ICON_SETTINGS,NULL,timer->recordingDir);
	bool recDirEnabled = (timer->eventType == CTimerd::TIMER_RECORD) && (g_settings.recording_type == RECORDING_FILE);
	CMenuForwarder* m6 = new CMenuForwarder(LOCALE_TIMERLIST_RECORDING_DIR,recDirEnabled,recDirEnabled ? timer->recordingDir : "", &recDirs);

	timerSettings.addItem(GenericMenuSeparatorLine);
	timerSettings.addItem(m3);
	timerSettings.addItem(m4);
	timerSettings.addItem(m5);
	timerSettings.addItem(GenericMenuSeparatorLine);
	timerSettings.addItem(m6);

	CMenuWidget timerSettings_apids(LOCALE_TIMERLIST_APIDS, NEUTRINO_ICON_SETTINGS);
	CTimerListApidNotifier apid_notifier(&timer_apids_dflt, &timer_apids_std, &timer_apids_ac3, &timer_apids_alt);
	timer_apids_dflt = (timer->apids == 0) ? 1 : 0 ;
	timer_apids_std = (timer->apids & TIMERD_APIDS_STD) ? 1 : 0 ;
	timer_apids_ac3 = (timer->apids & TIMERD_APIDS_AC3) ? 1 : 0 ;
	timer_apids_alt = (timer->apids & TIMERD_APIDS_ALT) ? 1 : 0 ;
	timerSettings_apids.addItem(GenericMenuSeparator);
	timerSettings_apids.addItem(GenericMenuBack);
	timerSettings_apids.addItem(GenericMenuSeparatorLine);
	CMenuOptionChooser* ma1 = new CMenuOptionChooser(LOCALE_TIMERLIST_APIDS_DFLT, &timer_apids_dflt, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, &apid_notifier);
	timerSettings_apids.addItem(ma1);
	CMenuOptionChooser* ma2 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_STD, &timer_apids_std, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, &apid_notifier);
	timerSettings_apids.addItem(ma2);
	CMenuOptionChooser* ma3 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_ALT, &timer_apids_alt, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, &apid_notifier);
	timerSettings_apids.addItem(ma3);
	CMenuOptionChooser* ma4 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_AC3, &timer_apids_ac3, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, &apid_notifier);
	timerSettings_apids.addItem(ma4);
	apid_notifier.setItems(ma1,ma2,ma3,ma4);
	if(timer->eventType ==  CTimerd::TIMER_RECORD)
	{  
		timerSettings.addItem( new CMenuForwarder(LOCALE_TIMERLIST_APIDS, true, NULL, &timerSettings_apids ));
	}

	return timerSettings.exec(this,"");
}

int CTimerList::newTimer()
{
	std::vector<CMenuWidget *> toDelete;
	// Defaults
	timerNew.eventType = CTimerd::TIMER_RECORD ;
	timerNew.eventRepeat = CTimerd::TIMERREPEAT_ONCE ;
	timerNew.repeatCount = 0;
	timerNew.alarmTime = (time(NULL)/60)*60;
	timerNew.stopTime = (time(NULL)/60)*60;
	timerNew.channel_id = 0;
	strcpy(timerNew.message, "");
	timerNew_standby_on =false;
	strncpy(timerNew.recordingDir,g_settings.recording_dir[0].c_str(),sizeof(timerNew.recordingDir));


	CMenuWidget timerSettings(LOCALE_TIMERLIST_MENUNEW, NEUTRINO_ICON_SETTINGS);
	timerSettings.addItem(GenericMenuSeparator);
	timerSettings.addItem(GenericMenuBack);
	timerSettings.addItem(GenericMenuSeparatorLine);
	timerSettings.addItem(new CMenuForwarder(LOCALE_TIMERLIST_SAVE, true, NULL, this, "newtimer", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	timerSettings.addItem(GenericMenuSeparatorLine);

	CDateInput timerSettings_alarmTime(LOCALE_TIMERLIST_ALARMTIME, &(timerNew.alarmTime) , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CMenuForwarder *m1 = new CMenuForwarder(LOCALE_TIMERLIST_ALARMTIME, true, timerSettings_alarmTime.getValue (), &timerSettings_alarmTime );

	CDateInput timerSettings_stopTime(LOCALE_TIMERLIST_STOPTIME, &(timerNew.stopTime) , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CMenuForwarder *m2 = new CMenuForwarder(LOCALE_TIMERLIST_STOPTIME, true, timerSettings_stopTime.getValue (), &timerSettings_stopTime );

	CStringInput timerSettings_weekdays(LOCALE_TIMERLIST_WEEKDAYS, m_weekdaysStr, 7, LOCALE_TIMERLIST_WEEKDAYS_HINT_1, LOCALE_TIMERLIST_WEEKDAYS_HINT_2, "-X");
	CMenuForwarder *m4 = new CMenuForwarder(LOCALE_TIMERLIST_WEEKDAYS, false,  m_weekdaysStr, &timerSettings_weekdays);

	CIntInput timerSettings_repeatCount(LOCALE_TIMERLIST_REPEATCOUNT, (long&)timerNew.repeatCount,3, LOCALE_TIMERLIST_REPEATCOUNT_HELP1, LOCALE_TIMERLIST_REPEATCOUNT_HELP2);
	CMenuForwarder *m5 = new CMenuForwarder(LOCALE_TIMERLIST_REPEATCOUNT, false,timerSettings_repeatCount.getValue() , &timerSettings_repeatCount);

	CTimerListRepeatNotifier notifier((int *)&timerNew.eventRepeat,m4,m5);
	strcpy(m_weekdaysStr,"-------");
	CMenuOptionChooser* m3 = new CMenuOptionChooser(LOCALE_TIMERLIST_REPEAT, (int *)&timerNew.eventRepeat, TIMERLIST_REPEAT_OPTIONS, TIMERLIST_REPEAT_OPTION_COUNT, true, &notifier);

	CZapitClient zapit;
	CZapitClient::BouquetList bouquetlist;
	zapit.getBouquets(bouquetlist, false, true, CZapitClient::MODE_ALL); // UTF-8
	CZapitClient::BouquetList::iterator bouquet = bouquetlist.begin();
	CMenuWidget mctv(LOCALE_TIMERLIST_BOUQUETSELECT, NEUTRINO_ICON_SETTINGS);
	CMenuWidget mcradio(LOCALE_TIMERLIST_BOUQUETSELECT, NEUTRINO_ICON_SETTINGS);
	for(; bouquet != bouquetlist.end();bouquet++)
	{
		CMenuWidget* mwtv = new CMenuWidget(LOCALE_TIMERLIST_CHANNELSELECT, NEUTRINO_ICON_SETTINGS);
		toDelete.push_back(mwtv);
		CMenuWidget* mwradio = new CMenuWidget(LOCALE_TIMERLIST_CHANNELSELECT, NEUTRINO_ICON_SETTINGS);
		toDelete.push_back(mwradio);
		CZapitClient::BouquetChannelList subchannellist;
		zapit.getBouquetChannels(bouquet->bouquet_nr,subchannellist,CZapitClient::MODE_TV, true); // UTF-8
		CZapitClient::BouquetChannelList::iterator channel = subchannellist.begin();
		for(; channel != subchannellist.end();channel++)
		{
			char cChannelId[3+16+1+1];
			sprintf(cChannelId,
				"SC:"
				PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
				",",
				channel->channel_id);
			mwtv->addItem(new CMenuForwarderNonLocalized(channel->name, true, NULL, this, (std::string(cChannelId) + channel->name).c_str()));
		}
		if (!subchannellist.empty())
			mctv.addItem(new CMenuForwarderNonLocalized(bouquet->name, true, NULL, mwtv));
		subchannellist.clear();
		zapit.getBouquetChannels(bouquet->bouquet_nr,subchannellist,CZapitClient::MODE_RADIO, true); // UTF-8
		channel = subchannellist.begin();
		for(; channel != subchannellist.end();channel++)
		{
			char cChannelId[3+16+1+1];
			sprintf(cChannelId,
				"SC:"
				PRINTF_CHANNEL_ID_TYPE_NO_LEADING_ZEROS
				",",
				channel->channel_id);
			mwradio->addItem(new CMenuForwarderNonLocalized(channel->name, true, NULL, this, (std::string(cChannelId) + channel->name).c_str()));
		}
		if (!subchannellist.empty())
			mcradio.addItem(new CMenuForwarderNonLocalized(bouquet->name, true, NULL, mwradio));
	}
	CMenuWidget mm(LOCALE_TIMERLIST_MODESELECT, NEUTRINO_ICON_SETTINGS);
	mm.addItem(new CMenuForwarder(LOCALE_TIMERLIST_MODETV, true, NULL, &mctv));
	mm.addItem(new CMenuForwarder(LOCALE_TIMERLIST_MODERADIO, true, NULL, &mcradio));
	strcpy(timerNew_channel_name,"---");
	CMenuForwarder* m6 = new CMenuForwarder(LOCALE_TIMERLIST_CHANNEL, true, timerNew_channel_name, &mm);

	CRecDirChooser recDirs(LOCALE_TIMERLIST_RECORDING_DIR,NEUTRINO_ICON_SETTINGS,NULL,timerNew.recordingDir);
	CMenuForwarder* m7 = new CMenuForwarder(LOCALE_TIMERLIST_RECORDING_DIR,true,timerNew.recordingDir, &recDirs);

	CMenuOptionChooser* m8 = new CMenuOptionChooser(LOCALE_TIMERLIST_STANDBY, &timerNew_standby_on, TIMERLIST_STANDBY_OPTIONS, TIMERLIST_STANDBY_OPTION_COUNT, false);

	CStringInputSMS timerSettings_msg(LOCALE_TIMERLIST_MESSAGE, timerNew.message, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789-.,:!?/ ");
	CMenuForwarder *m9 = new CMenuForwarder(LOCALE_TIMERLIST_MESSAGE, false, timerNew.message, &timerSettings_msg );

	strcpy(timerNew.pluginName,"---");
	CPluginChooser plugin_chooser(LOCALE_TIMERLIST_PLUGIN, CPlugins::P_TYPE_SCRIPT | CPlugins::P_TYPE_TOOL, timerNew.pluginName);
	CMenuForwarder *m10 = new CMenuForwarder(LOCALE_TIMERLIST_PLUGIN, false, timerNew.pluginName, &plugin_chooser);


	CTimerListNewNotifier notifier2((int *)&timerNew.eventType,
					&timerNew.stopTime,m2,m6,m8,m9,m10,m7,
					timerSettings_stopTime.getValue());
	CMenuOptionChooser* m0 = new CMenuOptionChooser(LOCALE_TIMERLIST_TYPE, (int *)&timerNew.eventType, TIMERLIST_TYPE_OPTIONS, TIMERLIST_TYPE_OPTION_COUNT, true, &notifier2);

	timerSettings.addItem( m0);
	timerSettings.addItem( m1);
	timerSettings.addItem( m2);
	timerSettings.addItem(GenericMenuSeparatorLine);
	timerSettings.addItem( m3);
	timerSettings.addItem( m4);
	timerSettings.addItem( m5);
	timerSettings.addItem(GenericMenuSeparatorLine);
	timerSettings.addItem( m6);
	timerSettings.addItem( m7);
	timerSettings.addItem( m8);
	timerSettings.addItem( m9);
	timerSettings.addItem( m10);

	int ret=timerSettings.exec(this,"");

	// delete dynamic created objects
	for(unsigned int count=0;count<toDelete.size();count++)
	{
		delete toDelete[count];
	}
	toDelete.clear();

	return ret;
}

bool askUserOnTimerConflict(time_t announceTime, time_t stopTime)
{
	CTimerdClient Timer;
	int result = true;
	CTimerd::TimerList overlappingTimers = Timer.getOverlappingTimers(announceTime,stopTime);

	std::string timerbuf = g_Locale->getText(LOCALE_TIMERLIST_OVERLAPPING_TIMER1); 
	timerbuf += "\n";
	char rec[25] = {0};
	struct tm *recTime = localtime(&announceTime);
	strftime(rec,20,"%d.%m. %H:%M",recTime);
	timerbuf += rec;
	recTime = localtime(&stopTime);
	if(stopTime > announceTime)
	{
		strftime(rec,20,"- %d.%m. %H:%M",recTime);
		timerbuf += rec;
	}
	
	timerbuf += "\n \n";
	timerbuf += g_Locale->getText(LOCALE_TIMERLIST_OVERLAPPING_TIMER2); 
	timerbuf += "\n";
	for (CTimerd::TimerList::iterator it = overlappingTimers.begin();
	     it != overlappingTimers.end();it++)
	{
		char at[25] = {0};
		struct tm *annTime = localtime(&(it->alarmTime));
		strftime(at,20,"%d.%m. %H:%M",annTime);
		timerbuf += at;
		timerbuf += " - ";

		char st[25] = {0};
		struct tm *sTime = localtime(&(it->stopTime));
		strftime(st,20,"%d.%m. %H:%M",sTime);
		timerbuf += st;
		timerbuf += " -> ";
		timerbuf += CTimerList::convertTimerType2String(it->eventType);
		timerbuf += "\n   ";

		timerbuf += CTimerList::convertChannelId2String(it->channel_id); // UTF-8
		if(it->epgID != 0)
		{
			CEPGData epgdata;
			timerbuf += ": ";
			if (g_Sectionsd->getEPGid(it->epgID, it->epg_starttime, &epgdata))
			{
#warning fixme sectionsd should deliver data in UTF-8 format
				timerbuf += Latin1_to_UTF8(epgdata.title);
			}
			else if(strlen(it->epgTitle)!=0)
			{
				timerbuf += Latin1_to_UTF8(it->epgTitle);
			}
		}
		timerbuf += "\n";
	}
	timerbuf += " \n";
	// todo: localize message
	timerbuf += g_Locale->getText(LOCALE_TIMERLIST_OVERLAPPING_TIMER3); 
	timerbuf += " \n";
	result = ShowMsgUTF(LOCALE_MESSAGEBOX_INFO,timerbuf,CMessageBox::mbrCancel,CMessageBox::mbNo|CMessageBox::mbYes|CMessageBox::mbCancel);
	if(result == CMessageBox::mbrYes)
	{
		printf("delete all conflict timers\n");
		for (CTimerd::TimerList::iterator it = overlappingTimers.begin();
		     it != overlappingTimers.end();it++)
		{
			Timer.removeTimerEvent(it->eventID);
		}
	}
	return ( result == CMessageBox::mbrYes || result == CMessageBox::mbrNo);
}




