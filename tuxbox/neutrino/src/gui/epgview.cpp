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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/epgview.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/icons.h>
#include <gui/widget/messagebox.h>

#include <global.h>
#include <neutrino.h>

#include <driver/encoding.h>
#include <driver/screen_max.h>

CEpgData::CEpgData()
{
	bigFonts = false;
	frameBuffer = CFrameBuffer::getInstance();
}

void CEpgData::start()
{
	// ox = 540;
	// oy = 320;
	ox = w_max (540, 0);
	oy = h_max (320, 0);
	sx = (((g_settings.screen_EndX-g_settings.screen_StartX) -ox) / 2) + g_settings.screen_StartX;



	topheight     = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->getHeight();
	topboxheight  = topheight + 6;
	botheight     = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight();
	botboxheight  = botheight + 6;
	medlineheight = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight();
	medlinecount  = (oy- botboxheight)/medlineheight;
	sb = medlinecount* medlineheight;

	oy = botboxheight+medlinecount*medlineheight; // recalculate
	sy = (((g_settings.screen_EndY-g_settings.screen_StartY)-(oy- topboxheight) ) / 2) + g_settings.screen_StartY;
	toph = topboxheight;

}

void CEpgData::addTextToArray(const std::string & text) // UTF-8
{
	//printf("line: >%s<\n", text.c_str() );
	if (text==" ")
	{
		emptyLineCount ++;
		if(emptyLineCount<2)
		{
			epgText.push_back(text);
		}
	}
	else
	{
		emptyLineCount = 0;
		epgText.push_back(text);
	}
}

void CEpgData::processTextToArray(std::string text) // UTF-8
{
	std::string	aktLine = "";
	std::string	aktWord = "";
	int	aktWidth = 0;
	text += ' ';
	char* text_= (char*) text.c_str();

	while(*text_!=0)
	{
		if ( (*text_==' ') || (*text_=='\n') || (*text_=='-') || (*text_=='.') )
		{
			//check the wordwidth - add to this line if size ok
			if(*text_=='\n')
			{	//enter-handler
				//printf("enter-");
				addTextToArray( aktLine );
				aktLine = "";
				aktWidth= 0;
			}
			else
			{
				aktWord += *text_;

				int aktWordWidth = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getRenderWidth(aktWord);
				if((aktWordWidth+aktWidth)<(ox- 20- 15))
				{//space ok, add
					aktWidth += aktWordWidth;
					aktLine += aktWord;
				}
				else
				{//new line needed
					addTextToArray( aktLine );
					aktLine = aktWord;
					aktWidth = aktWordWidth;
				}
				aktWord = "";
			}
		}
		else
		{
			aktWord += *text_;
		}
		text_++;
	}
	//add the rest
	addTextToArray( aktLine + aktWord );
}

void CEpgData::showText( int startPos, int ypos )
{
	// recalculate
	medlineheight = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getHeight();
	medlinecount=(oy- botboxheight)/medlineheight;

	int textCount = epgText.size();
	int y=ypos;

	frameBuffer->paintBoxRel(sx, y, ox- 15, sb, COL_MENUCONTENT_PLUS_0);

	for(int i=startPos; i<textCount && i<startPos+medlinecount; i++,y+=medlineheight)
	{
		if ( i< info1_lines )
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->RenderString(sx+10, y+medlineheight, ox- 15- 15, epgText[i], COL_MENUCONTENT, 0, true); // UTF-8
		else
			g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->RenderString(sx+10, y+medlineheight, ox- 15- 15, epgText[i], COL_MENUCONTENT, 0, true); // UTF-8
	}

	frameBuffer->paintBoxRel(sx+ ox- 15, ypos, 15, sb,  COL_MENUCONTENT_PLUS_1);

	int sbc= ((textCount- 1)/ medlinecount)+ 1;
	float sbh= (sb- 4)/ sbc;
	int sbs= (startPos+ 1)/ medlinecount;

	frameBuffer->paintBoxRel(sx+ ox- 13, ypos+ 2+ int(sbs* sbh) , 11, int(sbh),  COL_MENUCONTENT_PLUS_3);
}

#define GENRE_MOVIE_COUNT 9
const neutrino_locale_t genre_movie[GENRE_MOVIE_COUNT] =
{
	LOCALE_GENRE_MOVIE_0,
	LOCALE_GENRE_MOVIE_1,
	LOCALE_GENRE_MOVIE_2,
	LOCALE_GENRE_MOVIE_3,
	LOCALE_GENRE_MOVIE_4,
	LOCALE_GENRE_MOVIE_5,
	LOCALE_GENRE_MOVIE_6,
	LOCALE_GENRE_MOVIE_7,
	LOCALE_GENRE_MOVIE_8
};
#define GENRE_NEWS_COUNT 5
const neutrino_locale_t genre_news[GENRE_NEWS_COUNT] =
{
	LOCALE_GENRE_NEWS_0,
	LOCALE_GENRE_NEWS_1,
	LOCALE_GENRE_NEWS_2,
	LOCALE_GENRE_NEWS_3,
	LOCALE_GENRE_NEWS_4
};
#define GENRE_SHOW_COUNT 4
const neutrino_locale_t genre_show[GENRE_SHOW_COUNT] =
{
	LOCALE_GENRE_SHOW_0,
	LOCALE_GENRE_SHOW_1,
	LOCALE_GENRE_SHOW_2,
	LOCALE_GENRE_SHOW_3
};
#define GENRE_SPORTS_COUNT 12
const neutrino_locale_t genre_sports[GENRE_SPORTS_COUNT] =
{
	LOCALE_GENRE_SPORTS_0,
	LOCALE_GENRE_SPORTS_1,
	LOCALE_GENRE_SPORTS_2,
	LOCALE_GENRE_SPORTS_3,
	LOCALE_GENRE_SPORTS_4,
	LOCALE_GENRE_SPORTS_5,
	LOCALE_GENRE_SPORTS_6,
	LOCALE_GENRE_SPORTS_7,
	LOCALE_GENRE_SPORTS_8,
	LOCALE_GENRE_SPORTS_9,
	LOCALE_GENRE_SPORTS_10,
	LOCALE_GENRE_SPORTS_11
};
#define GENRE_CHILDRENS_PROGRAMMES_COUNT 6
const neutrino_locale_t genre_childrens_programmes[GENRE_CHILDRENS_PROGRAMMES_COUNT] =
{
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_0,
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_1,
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_2,
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_3,
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_4,
	LOCALE_GENRE_CHILDRENS_PROGRAMMES_5
};
#define GENRE_MUSIC_DANCE_COUNT 7
const neutrino_locale_t genre_music_dance[GENRE_MUSIC_DANCE_COUNT] =
{
	LOCALE_GENRE_MUSIC_DANCE_0,
	LOCALE_GENRE_MUSIC_DANCE_1,
	LOCALE_GENRE_MUSIC_DANCE_2,
	LOCALE_GENRE_MUSIC_DANCE_3,
	LOCALE_GENRE_MUSIC_DANCE_4,
	LOCALE_GENRE_MUSIC_DANCE_5,
	LOCALE_GENRE_MUSIC_DANCE_6
};
#define GENRE_ARTS_COUNT 12
const neutrino_locale_t genre_arts_dance[GENRE_ARTS_COUNT] =
{
	LOCALE_GENRE_ARTS_0,
	LOCALE_GENRE_ARTS_1,
	LOCALE_GENRE_ARTS_2,
	LOCALE_GENRE_ARTS_3,
	LOCALE_GENRE_ARTS_4,
	LOCALE_GENRE_ARTS_5,
	LOCALE_GENRE_ARTS_6,
	LOCALE_GENRE_ARTS_7,
	LOCALE_GENRE_ARTS_8,
	LOCALE_GENRE_ARTS_9,
	LOCALE_GENRE_ARTS_10,
	LOCALE_GENRE_ARTS_11
};
#define GENRE_SOCIAL_POLITICAL_COUNT 4
const neutrino_locale_t genre_social_political[GENRE_SOCIAL_POLITICAL_COUNT] =
{
	LOCALE_GENRE_SOCIAL_POLITICAL_0,
	LOCALE_GENRE_SOCIAL_POLITICAL_1,
	LOCALE_GENRE_SOCIAL_POLITICAL_2,
	LOCALE_GENRE_SOCIAL_POLITICAL_3
};
#define GENRE_DOCUS_MAGAZINES_COUNT 8
const neutrino_locale_t genre_docus_magazines[GENRE_DOCUS_MAGAZINES_COUNT] =
{
	LOCALE_GENRE_DOCUS_MAGAZINES_0,
	LOCALE_GENRE_DOCUS_MAGAZINES_1,
	LOCALE_GENRE_DOCUS_MAGAZINES_2,
	LOCALE_GENRE_DOCUS_MAGAZINES_3,
	LOCALE_GENRE_DOCUS_MAGAZINES_4,
	LOCALE_GENRE_DOCUS_MAGAZINES_5,
	LOCALE_GENRE_DOCUS_MAGAZINES_6,
	LOCALE_GENRE_DOCUS_MAGAZINES_7
};
#define GENRE_TRAVEL_HOBBIES_COUNT 8
const neutrino_locale_t genre_travel_hobbies[GENRE_TRAVEL_HOBBIES_COUNT] =
{
	LOCALE_GENRE_TRAVEL_HOBBIES_0,
	LOCALE_GENRE_TRAVEL_HOBBIES_1,
	LOCALE_GENRE_TRAVEL_HOBBIES_2,
	LOCALE_GENRE_TRAVEL_HOBBIES_3,
	LOCALE_GENRE_TRAVEL_HOBBIES_4,
	LOCALE_GENRE_TRAVEL_HOBBIES_5,
	LOCALE_GENRE_TRAVEL_HOBBIES_6,
	LOCALE_GENRE_TRAVEL_HOBBIES_7
};
const unsigned char genre_sub_classes[10] =
{
	GENRE_MOVIE_COUNT,
	GENRE_NEWS_COUNT,
	GENRE_SHOW_COUNT,
	GENRE_SPORTS_COUNT,
	GENRE_CHILDRENS_PROGRAMMES_COUNT,
	GENRE_MUSIC_DANCE_COUNT,
	GENRE_ARTS_COUNT,
	GENRE_SOCIAL_POLITICAL_COUNT,
	GENRE_DOCUS_MAGAZINES_COUNT,
	GENRE_TRAVEL_HOBBIES_COUNT
};
const neutrino_locale_t * genre_sub_classes_list[10] =
{
	genre_movie,
	genre_news,
	genre_show,
	genre_sports,
	genre_childrens_programmes,
	genre_music_dance,
	genre_arts_dance,
	genre_social_political,
	genre_docus_magazines,
	genre_travel_hobbies
};

const char * GetGenre(const unsigned char contentClassification) // UTF-8
{
	neutrino_locale_t res;

	unsigned char i = (contentClassification & 0x0F0);

	if ((i >= 0x010) && (i < 0x0B0))
	{
		i >>= 4;
		i--;
		res = genre_sub_classes_list[i][((contentClassification & 0x0F) < genre_sub_classes[i]) ? (contentClassification & 0x0F) : 0];
	}
	else
		res = LOCALE_GENRE_UNKNOWN;

	return g_Locale->getText(res);
}


int CEpgData::show(const t_channel_id channel_id, unsigned long long a_id, time_t* a_startzeit, bool doLoop )
{
	int res = menu_return::RETURN_REPAINT;
	static unsigned long long id;
	static time_t startzeit;
	 

	if(a_startzeit)
		startzeit=*a_startzeit;
	id=a_id;

	int height;
	height = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getHeight();
	if (doLoop)
	{
		frameBuffer->paintBoxRel(g_settings.screen_StartX, g_settings.screen_StartY, 50, height+5, COL_INFOBAR_PLUS_0);
		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(g_settings.screen_StartX+10, g_settings.screen_StartY+height, 40, "-@-", COL_INFOBAR);
	}

	GetEPGData(channel_id, id, &startzeit );
	if (doLoop)
	{
		evtlist = g_Sectionsd->getEventsServiceKey(channel_id);
		frameBuffer->paintBackgroundBoxRel(g_settings.screen_StartX, g_settings.screen_StartY, 50, height+5);
	}

	if (epgData.title.empty()) /* no epg info found */
	{
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_EPGVIEWER_NOTFOUND)); // UTF-8
		return res;
	}



	int pos;
	std::string text1 = epgData.title;
	std::string text2 = "";
	if (g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->getRenderWidth(text1) > 520)
	{
		do
	    	{
			pos = text1.find_last_of("[ .]+");
			if ( pos!=-1 )
				text1 = text1.substr( 0, pos );
		} while ( ( pos != -1 ) && (g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->getRenderWidth(text1) > 520));
		text2 = epgData.title.substr(text1.length()+ 1, uint(-1) );
	}

	int oldtoph= toph;

	if (text2!="")
		toph = 2* topboxheight;
	else
		toph = topboxheight;

	if ( (oldtoph> toph) && (!doLoop) )
	{
		frameBuffer->paintBackgroundBox (sx, sy- oldtoph- 1, sx+ ox, sy- toph);
	}

	if (!epgData.info1.empty())
		processTextToArray(Latin1_to_UTF8(epgData.info1));

	info1_lines = epgText.size();

	//scan epg-data - sort to list
	if ((epgData.info2.empty()) && (info1_lines == 0))
		processTextToArray(g_Locale->getText(LOCALE_EPGVIEWER_NODETAILED)); // UTF-8
	else
		processTextToArray(Latin1_to_UTF8(epgData.info2));

	if (epgData.fsk > 0)
	{
		char _tfsk[11];
		sprintf (_tfsk, "FSK: ab %d", epgData.fsk );
		processTextToArray( _tfsk ); // UTF-8
	}

	if (epgData.contentClassification.length()> 0)
		processTextToArray(GetGenre(epgData.contentClassification[0])); // UTF-8
//	processTextToArray( epgData.userClassification.c_str() );


	// -- display more screenings on the same channel
	// -- 2002-05-03 rasc
	processTextToArray("\n"); // UTF-8
	processTextToArray(std::string(g_Locale->getText(LOCALE_EPGVIEWER_MORE_SCREENINGS)) + ':'); // UTF-8
	FollowScreenings(channel_id, epgData.title);


	//show the epg
	frameBuffer->paintBoxRel(sx, sy- toph, ox, toph, COL_MENUHEAD_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->RenderString(sx+10, sy- toph+ topheight+ 3, ox-15, text1, COL_MENUHEAD);
	if (!(text2.empty()))
		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_TITLE]->RenderString(sx+10, sy- toph+ 2* topheight+ 3, ox-15, text2, COL_MENUHEAD);

	//show date-time....
	frameBuffer->paintBoxRel(sx, sy+oy-botboxheight, ox, botboxheight, COL_MENUHEAD_PLUS_0);
	std::string fromto;
	int widthl,widthr;
	fromto = epg_start;
	fromto += " - ";
	fromto += epg_end;

	widthl = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth(fromto);
	g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(sx+40,  sy+oy-3, widthl, fromto, COL_MENUHEAD);
	widthr = g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->getRenderWidth(epg_date);
	g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(sx+ox-40-widthr,  sy+oy-3, widthr, epg_date, COL_MENUHEAD);

	int showPos = 0;
	textCount = epgText.size();
	int textypos = sy;
	showText(showPos, textypos);

	// show Timer Event Buttons
	showTimerEventBar (true);

	//show progressbar
	if ( epg_done!= -1 )
	{
		int pbx = sx + 10 + widthl + 10 + ((ox-104-widthr-widthl-10-10-20)>>1);
		frameBuffer->paintBoxRel(pbx, sy+oy-height, 104, height-6, COL_MENUCONTENT_PLUS_6);
		frameBuffer->paintBoxRel(pbx+2, sy+oy-height+2, 100, height-10, COL_MENUCONTENT_PLUS_0);
		frameBuffer->paintBoxRel(pbx+2, sy+oy-height+2, epg_done, height-10, COL_MENUCONTENT_PLUS_3);
	}

	GetPrevNextEPGData( epgData.eventID, &epgData.epg_times.startzeit );
	if (prev_id != 0)
	{
		frameBuffer->paintBoxRel(sx+ 5, sy+ oy- botboxheight+ 4, botboxheight- 8, botboxheight- 8,  COL_MENUCONTENT_PLUS_3);
		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(sx+ 10, sy+ oy- 3, widthr, "<", COL_MENUCONTENT + 3);
	}

	if (next_id != 0)
	{
		frameBuffer->paintBoxRel(sx+ ox- botboxheight+ 8- 5, sy+ oy- botboxheight+ 4, botboxheight- 8, botboxheight- 8,  COL_MENUCONTENT_PLUS_3);
		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(sx+ ox- botboxheight+ 8, sy+ oy- 3, widthr, ">", COL_MENUCONTENT + 3);
	}

	if ( doLoop )
	{
		neutrino_msg_t      msg;
		neutrino_msg_data_t data;

		int scrollCount;

		bool loop = true;

		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_EPG]);

		while(loop)
		{
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

			scrollCount = medlinecount;

			switch ( msg )
			{
				case CRCInput::RC_left:
					if (prev_id != 0)
					{
						frameBuffer->paintBoxRel(sx+ 5, sy+ oy- botboxheight+ 4, botboxheight- 8, botboxheight- 8,  COL_MENUCONTENT_PLUS_1);
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(sx+ 10, sy+ oy- 3, widthr, "<", COL_MENUCONTENT + 1);

						show(channel_id, prev_id, &prev_zeit, false);
						showPos=0;
					}
					break;

				case CRCInput::RC_right:
					if (next_id != 0)
					{
						frameBuffer->paintBoxRel(sx+ ox- botboxheight+ 8- 5, sy+ oy- botboxheight+ 4, botboxheight- 8, botboxheight- 8,  COL_MENUCONTENT_PLUS_1);
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_DATE]->RenderString(sx+ ox- botboxheight+ 8, sy+ oy- 3, widthr, ">", COL_MENUCONTENT + 1);

						show(channel_id, next_id, &next_zeit, false);
						showPos=0;
					}
					break;

				case CRCInput::RC_down:
					if(showPos+scrollCount<textCount)
					{
						showPos += scrollCount;
						showText(showPos,textypos);
					}
					break;

				case CRCInput::RC_up:
					showPos -= scrollCount;
					if(showPos<0)
						showPos = 0;
					else
						showText(showPos,textypos);
					break;

				// 31.05.2002 dirch		record timer
				case CRCInput::RC_red:
					if (g_settings.recording_type != CNeutrinoApp::RECORDING_OFF)
					{
						CTimerdClient timerdclient;
						if(timerdclient.isTimerdAvailable())
						{
							timerdclient.addRecordTimerEvent(channel_id,
																		epgData.epg_times.startzeit,
																		epgData.epg_times.startzeit + epgData.epg_times.dauer,
																		epgData.eventID, epgData.epg_times.startzeit,
																		epgData.epg_times.startzeit - (ANNOUNCETIME + 120 ),
																		"", true );
							ShowMsgUTF(LOCALE_TIMER_EVENTRECORD_TITLE, g_Locale->getText(LOCALE_TIMER_EVENTRECORD_MSG), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
						}
						else
							printf("timerd not available\n");
					}
					break;

				// 31.05.2002 dirch		zapto timer
				case CRCInput::RC_yellow:
				{
					CTimerdClient timerdclient;
					if(timerdclient.isTimerdAvailable())
					{
						timerdclient.addZaptoTimerEvent(channel_id,
																  epgData.epg_times.startzeit,
																  epgData.epg_times.startzeit - ANNOUNCETIME, 0,
																  epgData.eventID, epgData.epg_times.startzeit,
																  "");
						ShowMsgUTF(LOCALE_TIMER_EVENTTIMED_TITLE, g_Locale->getText(LOCALE_TIMER_EVENTTIMED_MSG), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw"); // UTF-8
					}
					else
						printf("timerd not available\n");
					break;
				}

				case CRCInput::RC_help:
					bigFonts = bigFonts ? false : true;
					if(bigFonts)
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->setSize((int)(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getSize() * BIG_FONT_FAKTOR));
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->setSize((int)(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getSize() * BIG_FONT_FAKTOR));
					}else
					{
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->setSize((int)(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getSize() / BIG_FONT_FAKTOR));
						g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->setSize((int)(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getSize() / BIG_FONT_FAKTOR));
					}
					show(channel_id, id, &startzeit, false);
					showPos=0;
					break;

				case CRCInput::RC_ok:
				case CRCInput::RC_timeout:
					loop = false;
					break;

				default:
					// konfigurierbare Keys handlen...
					if (msg == (neutrino_msg_t)g_settings.key_channelList_cancel)
						loop = false;
					else
					{
						if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
						{
							loop = false;
							res = menu_return::RETURN_EXIT_ALL;
						}
					}
			}
		}
		hide();
	}
	return res;
}

void CEpgData::hide()
{
        // 2004-09-10 rasc  (bugfix, scale large font settings back to normal)
	if (bigFonts) {
		bigFonts = false;
		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->setSize((int)(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO1]->getSize() / BIG_FONT_FAKTOR));
		g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->setSize((int)(g_Font[SNeutrinoSettings::FONT_TYPE_EPG_INFO2]->getSize() / BIG_FONT_FAKTOR));
	}

	frameBuffer->paintBackgroundBox (sx, sy- toph, sx+ ox, sy+ oy);
        showTimerEventBar (false);
}

void CEpgData::GetEPGData(const t_channel_id channel_id, unsigned long long id, time_t* startzeit )
{
	epgText.clear();
	emptyLineCount = 0;

	bool res;

	if ( id!= 0 )
		res = g_Sectionsd->getEPGid( id, *startzeit, &epgData );
	else
		res = g_Sectionsd->getActualEPGServiceKey(channel_id, &epgData );

	if ( res )
	{
		struct tm *pStartZeit = localtime(&(epgData.epg_times).startzeit);
		char temp[11];
		strftime( temp, sizeof(temp), "%d.%m.%Y", pStartZeit);
		epg_date= temp;
		strftime( temp, sizeof(temp), "%H:%M", pStartZeit);
		epg_start= temp;

		long int uiEndTime((epgData.epg_times).startzeit+ (epgData.epg_times).dauer);
		struct tm *pEndeZeit = localtime((time_t*)&uiEndTime);
		strftime( temp, sizeof(temp), "%H:%M", pEndeZeit);
		epg_end= temp;

		epg_done= -1;
		if (( time(NULL)- (epgData.epg_times).startzeit )>= 0 )
		{
			unsigned nProcentagePassed=(unsigned)((float)(time(NULL)-(epgData.epg_times).startzeit)/(float)(epgData.epg_times).dauer*100.);
			if (nProcentagePassed<= 100)
				epg_done= nProcentagePassed;
		}
	}
}

void CEpgData::GetPrevNextEPGData( unsigned long long id, time_t* startzeit )
{
	prev_id= 0;
	next_id= 0;

	for ( unsigned int i= 0; i< evtlist.size(); i++ )
	{
		//printf("%d %llx/%llx - %x %x\n", i, evtlist[i].eventID, id, evtlist[i].startTime, *startzeit);
    	if ( ( evtlist[i].eventID == id ) && ( evtlist[i].startTime == *startzeit ) )
    	{
        	if ( i > 0 )
        	{
        		prev_id= evtlist[i- 1].eventID;
        		prev_zeit= evtlist[i- 1].startTime;
        	}
 			if ( i < ( evtlist.size()- 1 ) )
        	{
        		next_id= evtlist[i+ 1].eventID;
        		next_zeit= evtlist[i+ 1].startTime;
        	}
			break;
    	}
	}

}


//
// -- get following screenings of this program title
// -- yek! a better class design would be more helpfull
// -- BAD THING: Cross channel screenings will not be shown
// --            $$$TODO
// -- 2002-05-03 rasc
//

int CEpgData::FollowScreenings (const t_channel_id channel_id, const std::string & title)

{
  CChannelEventList::iterator e;
  time_t			curtime;
  struct  tm		*tmStartZeit;
  std::string		screening_dates,screening_nodual;
  int				count;
  char			tmpstr[256];


  	count = 0;
	screening_dates = screening_nodual = "";
	// alredy read: evtlist = g_Sectionsd->getEventsServiceKey( channel_id );
    	curtime = time(NULL);

	for ( e= evtlist.begin(); e != evtlist.end(); ++e )
	{
	    	if (e->startTime <= curtime) continue;
		if (! e->eventID) continue;
		if (e->description == title) {
			count++;
			tmStartZeit = localtime(&(e->startTime));

			screening_dates = "    ";

			screening_dates += g_Locale->getText(CLocaleManager::getWeekday(tmStartZeit));
			screening_dates += '.';

			strftime(tmpstr, sizeof(tmpstr), "  %d.", tmStartZeit );
			screening_dates += tmpstr;

			screening_dates += g_Locale->getText(CLocaleManager::getMonth(tmStartZeit));

			strftime(tmpstr, sizeof(tmpstr), ".  %H:%M ", tmStartZeit );
			screening_dates += tmpstr;
		if (screening_dates != screening_nodual){
			screening_nodual=screening_dates;
			processTextToArray(screening_dates ); // UTF-8
			}
		}
	}
	if (count == 0)
		processTextToArray("---\n"); // UTF-8
	return count;
}


//
// -- Just display or hide TimerEventbar
// -- 2002-05-13 rasc
//

void CEpgData::showTimerEventBar (bool show)

{
  int  x,y,w,h;
  int  cellwidth;		// 4 cells
  int  h_offset, pos;

  w = ox;
  h = 30;
  x = sx;
  y = sy + oy;
  h_offset = 5;
  cellwidth = w / 4;


    frameBuffer->paintBackgroundBoxRel(x,y,w,h);
    // hide only?
    if (! show) return;

    // frameBuffer->paintBoxRel(x,y,w,h, COL_INFOBAR_SHADOW_PLUS_1);
    frameBuffer->paintBoxRel(x,y,w,h, COL_MENUHEAD_PLUS_0);



    // Button: Timer Record & Channelswitch
	if (g_settings.recording_type != CNeutrinoApp::RECORDING_OFF)
	{
		pos = 0;
		frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, x+8+cellwidth*pos, y+h_offset );
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+29+cellwidth*pos, y+h-h_offset, w-30, g_Locale->getText(LOCALE_TIMERBAR_RECORDEVENT), COL_INFOBAR, 0, true); // UTF-8
	}
    // Button: Timer Channelswitch
    pos = 2;
    frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, x+8+cellwidth*pos, y+h_offset );
    g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(x+29+cellwidth*pos, y+h-h_offset, w-30, g_Locale->getText(LOCALE_TIMERBAR_CHANNELSWITCH), COL_INFOBAR, 0, true); // UTF-8
}





//
//  -- EPG Data Viewer Menu Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-06 rasc)
// 

int CEPGDataHandler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	int           res = menu_return::RETURN_EXIT_ALL;
	CChannelList  *channelList;
	CEpgData      *e;


	if (parent) {
		parent->hide();
	}

	e = new CEpgData;

	channelList = CNeutrinoApp::getInstance()->channelList;
	e->start();
	e->show( channelList->getActiveChannel_ChannelID() );
	delete e;

	return res;
}


