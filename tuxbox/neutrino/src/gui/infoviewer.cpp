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

#include "infoviewer.h"
#include "../global.h"

#define COL_INFOBAR_BUTTONS				COL_INFOBAR_SHADOW+ 1
#define COL_INFOBAR_BUTTONS_GRAY		COL_INFOBAR_SHADOW+ 1

#define ICON_LARGE 30
#define ICON_SMALL 20
#define ICON_OFFSET (2*ICON_LARGE+ ICON_SMALL+ 5)
#define BOTTOM_BAR_OFFSET 0
#define SHADOW_OFFSET 6


int time_left_width;
int time_dot_width;
int time_height;
char old_timestr[10];

CInfoViewer::CInfoViewer()
{
	BoxStartX= BoxStartY= BoxEndX= BoxEndY=0;
	is_visible		= false;
	showButtonBar	= false;
	gotTime 		= g_Sectionsd->getIsTimeSet();
	CurrentChannel = "";
}

void CInfoViewer::start()
{
	InfoHeightY = g_Fonts->infobar_number->getHeight()*9/8 +
                      2*g_Fonts->infobar_info->getHeight() +
                      25;
	InfoHeightY_Info = g_Fonts->infobar_small->getHeight()+ 5;

	ChanWidth = g_Fonts->infobar_number->getRenderWidth("0000") + 10;
	ChanHeight = g_Fonts->infobar_number->getHeight()*9/8;

	aspectRatio = g_Controld->getAspectRatio();

	time_height = g_Fonts->infobar_channame->getHeight()+5;
	time_left_width = g_Fonts->infobar_channame->getRenderWidth("99");
	time_dot_width = g_Fonts->infobar_channame->getRenderWidth(":");
}

void CInfoViewer::paintTime( bool show_dot, bool firstPaint )
{
	if ( gotTime )
	{
	    int ChanNameY = BoxStartY + (ChanHeight>>1)   + 5; //oberkante schatten?

		char timestr[10];
		struct timeb tm;

		ftime(&tm);
		strftime((char*) &timestr, 20, "%H:%M", localtime(&tm.time) );

		if ( ( !firstPaint ) && ( strcmp( timestr, old_timestr ) == 0 ) )
		{
			if ( show_dot )
        		g_FrameBuffer->paintBoxRel(BoxEndX- time_left_width- time_dot_width- 10, ChanNameY, time_dot_width, time_height/2, COL_INFOBAR);
        	else
        		g_Fonts->infobar_channame->RenderString(BoxEndX-time_left_width- time_dot_width- 10, ChanNameY+ time_height, time_dot_width, ":", COL_INFOBAR);
        	strcpy( old_timestr, timestr );
        }
        else
        {
        	strcpy( old_timestr, timestr );


    		if ( !firstPaint )
    			g_FrameBuffer->paintBoxRel(BoxEndX- time_left_width*2- time_dot_width- 10, ChanNameY, time_left_width*2+ time_dot_width, time_height, COL_INFOBAR);

			g_Fonts->infobar_channame->RenderString(BoxEndX-time_left_width*2- time_dot_width-10, ChanNameY+ time_height, time_left_width*2+ time_dot_width, timestr, COL_INFOBAR);
            if ( show_dot )
        		g_FrameBuffer->paintBoxRel(BoxEndX- time_left_width- time_dot_width- 10, ChanNameY, time_dot_width, time_height/2, COL_INFOBAR);
		}
	}
}

void CInfoViewer::showTitle( int ChanNum, string Channel, unsigned int onid_sid, bool calledFromNumZap )
{
neutrino->showProfiling("infoviewer showtitle");

        CurrentChannel = Channel;
        current_onid_sid = onid_sid;
        showButtonBar = !calledFromNumZap;
        is_visible = true;

        BoxStartX = g_settings.screen_StartX+ 20;
        BoxEndX   = g_settings.screen_EndX- 20;
        BoxEndY   = g_settings.screen_EndY- 20;

        int BoxEndInfoY = showButtonBar?(BoxEndY- InfoHeightY_Info):(BoxEndY);
		BoxStartY = BoxEndInfoY- InfoHeightY;

 		if ( !gotTime )
 			gotTime = g_Sectionsd->getIsTimeSet();

		// kill linke seite
        g_FrameBuffer->paintBackgroundBox(BoxStartX, BoxStartY+ ChanHeight, BoxStartX + (ChanWidth/3), BoxStartY+ ChanHeight+ InfoHeightY_Info+ 10);
        // kill progressbar
        g_FrameBuffer->paintBackgroundBox(BoxEndX- 120, BoxStartY, BoxEndX, BoxStartY+ ChanHeight);

        //number box
        g_FrameBuffer->paintBoxRel(BoxStartX+10, BoxStartY+10, ChanWidth, ChanHeight, COL_INFOBAR_SHADOW);
        g_FrameBuffer->paintBoxRel(BoxStartX,    BoxStartY,    ChanWidth, ChanHeight, COL_INFOBAR);

        //channel number
        char strChanNum[10];
        sprintf( (char*) strChanNum, "%d", ChanNum);
        int ChanNumXPos = BoxStartX + ((ChanWidth - g_Fonts->infobar_number->getRenderWidth(strChanNum))>>1);
        g_Fonts->infobar_number->RenderString(ChanNumXPos, BoxStartY+ChanHeight, ChanWidth, strChanNum, COL_INFOBAR);

        //infobox
        int ChanNameX = BoxStartX + ChanWidth + 10;
        int ChanNameY = BoxStartY + (ChanHeight>>1)   + 5; //oberkante schatten?

       	g_FrameBuffer->paintBox(ChanNameX, ChanNameY, BoxEndX, BoxEndInfoY, COL_INFOBAR);

        int height=time_height;
        int timewidth= time_left_width* 2+ time_dot_width;

		paintTime( false, true );

		// ... with channel name
        g_Fonts->infobar_channame->RenderString(ChanNameX+ 10, ChanNameY+height, BoxEndX- (ChanNameX+ 20)- timewidth- 15, Channel.c_str(), COL_INFOBAR);

        ChanInfoX = BoxStartX + (ChanWidth / 3);
        int ChanInfoY = BoxStartY + ChanHeight+ 10;
        ButtonWidth = (BoxEndX- ChanInfoX- ICON_OFFSET)>> 2;

        g_FrameBuffer->paintBox(ChanInfoX, ChanInfoY, ChanNameX, BoxEndInfoY, COL_INFOBAR);

neutrino->showProfiling("before if ( showButtonBar )");

        if ( showButtonBar )
        {
         	sec_timer_id = g_RCInput->addTimer(1000000, false);

        	if ( BOTTOM_BAR_OFFSET> 0 )
	        	g_FrameBuffer->paintBackgroundBox(ChanInfoX, BoxEndInfoY, BoxEndX, BoxEndInfoY+ BOTTOM_BAR_OFFSET);

       		g_FrameBuffer->paintBox(ChanInfoX, BoxEndInfoY+ BOTTOM_BAR_OFFSET, BoxEndX, BoxEndY, COL_INFOBAR_BUTTONS);
		}

neutrino->showProfiling("before getEPG();");

		getEPG();

neutrino->showProfiling("after getEPG();");

		if ( !( info_CurrentNext.flags & ( sectionsd::epgflags::has_later | sectionsd::epgflags::has_current |  sectionsd::epgflags::not_broadcast ) ) )
		{
			// nicht gefunden / noch nicht geladen
			g_Fonts->infobar_info->RenderString(ChanNameX+ 10, ChanInfoY+ 2* g_Fonts->infobar_info->getHeight()+ 5, BoxEndX- (ChanNameX+ 20), g_Locale->getText(gotTime?(showButtonBar?"infoviewer.epgwait":"infoviewer.epgnotload"):"infoviewer.waittime").c_str(), COL_INFOBAR);
		}
		else
			show_Data();

neutrino->showProfiling("after showData();");

        if ( showButtonBar )
        {
			// blau
			g_FrameBuffer->paintIcon("blau.raw", BoxEndX- ICON_OFFSET- ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
			g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.streaminfo").c_str(), COL_INFOBAR_BUTTONS);

			showButton_Audio();
			showButton_SubServices();
			showIcon_16_9();
			showIcon_VTXT();

neutrino->showProfiling("g_Sectionsd->setServiceChanged");
			if ( ( ( info_CurrentNext.flags & sectionsd::epgflags::has_next ) &&
				   ( info_CurrentNext.flags & ( sectionsd::epgflags::has_current | sectionsd::epgflags::has_no_current ) ) ) ||
				 ( info_CurrentNext.flags & sectionsd::epgflags::not_broadcast ) )
			{
				// alles was nötig ist, ist da!
				g_Sectionsd->setServiceChanged( onid_sid, false );
			}
			else
			{
				// EVENT anfordern!
				g_Sectionsd->setServiceChanged( onid_sid, true );
			}
        }

neutrino->showProfiling("vor Schatten;");
		// Schatten
        g_FrameBuffer->paintBox(BoxEndX, ChanNameY+ SHADOW_OFFSET, BoxEndX+ SHADOW_OFFSET, BoxEndY, COL_INFOBAR_SHADOW);
        g_FrameBuffer->paintBox(ChanInfoX+ SHADOW_OFFSET, BoxEndY, BoxEndX+ SHADOW_OFFSET, BoxEndY+ SHADOW_OFFSET, COL_INFOBAR_SHADOW);

        uint msg; uint data;


        if ( !calledFromNumZap )
        {
            bool show_dot= true;

        	neutrino->showProfiling("start infoviewer loop");
       		bool hideIt = true;
			unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_infobar >> 1 );

			int res = messages_return::none;

			while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
			{
				g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
                //printf(" g_RCInput->getMsgAbsoluteTimeout %x\n", msg);

				if ( msg == CRCInput::RC_help )
				{
					g_RCInput->postMsg( NeutrinoMessages::SHOW_EPG, 0 );
					res = messages_return::cancel_info;
				}
				else if ( ( msg == CRCInput::RC_timeout ) ||
				          ( msg == CRCInput::RC_ok ) ||
				          ( msg == CRCInput::RC_home ) )
				{
					res = messages_return::cancel_info;
				}
				else if ( ( msg == g_settings.key_quickzap_up ) ||
               	 	 	  ( msg == g_settings.key_quickzap_down ) )
				{
					hideIt = false;
					g_RCInput->postMsg( msg, data );
					res = messages_return::cancel_info;
				}
				else if ( msg == NeutrinoMessages::EVT_TIMESET )
				{
					// Handle anyway!
					neutrino->handleMsg( msg, data );
        			g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );
					res = messages_return::cancel_all;
				}
				else if ( ( msg == NeutrinoMessages::EVT_TIMER ) && ( data = sec_timer_id ) )
				{
        			paintTime( show_dot, false );
 					show_dot = !show_dot;
				}
				else
				{
            		res = neutrino->handleMsg( msg, data );

            		if ( res == messages_return::unhandled )
            		{
            			// raus hier und im Hauptfenster behandeln...
            			g_RCInput->postMsg(  msg, data );
						res = messages_return::cancel_info;
					}
				}
				neutrino->showProfiling("end infoviewer loop");
			}


            if ( hideIt )
				killTitle();

            g_RCInput->killTimer(sec_timer_id);
        }

	neutrino->showProfiling("end infoviewer");
}

void CInfoViewer::showIcon_16_9()
{
	g_FrameBuffer->paintIcon( ( aspectRatio == 3 )?"16_9.raw":"16_9_gray.raw", BoxEndX- 2* ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
}

void CInfoViewer::showIcon_VTXT()
{
	if ( g_RemoteControl->current_PIDs.PIDs.vtxtpid != 0 )
		g_FrameBuffer->paintIcon("vtxt.raw", BoxEndX- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
	else
		g_FrameBuffer->paintIcon("vtxt_gray.raw", BoxEndX- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
}

int CInfoViewer::handleMsg(uint msg, uint data)
{
    if ( msg == NeutrinoMessages::EVT_CURRENTNEXT_EPG )
	{
		if ( data == current_onid_sid )
		{
            getEPG();

			if ( is_visible )
				show_Data( true );
		}
	    return messages_return::handled;
	}
    else if ( msg == NeutrinoMessages::EVT_ZAP_GOTAPIDS )
	{
		if ( data == current_onid_sid )
		{
			if ( is_visible && showButtonBar )
				showButton_Audio();
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_GOTPIDS )
	{
		if ( data == current_onid_sid )
		{
			if ( is_visible && showButtonBar )
				showIcon_VTXT();
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_GOT_SUBSERVICES )
	{
		if ( data == current_onid_sid )
		{
			if ( is_visible && showButtonBar )
				showButton_SubServices();
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_SUB_COMPLETE )
	{
		//if ( data == current_onid_sid )
		{
			if ( is_visible && showButtonBar &&  ( !g_RemoteControl->are_subchannels ) )
				show_Data( true );
		}
	    return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_ZAP_FAILED )
	{
		if ( data == current_onid_sid )
		{
			// show failure..!
			printf("zap failed!\n");
			#ifdef USEACTIONLOG
				g_ActionLog->println("channel unavailable");
			#endif
		}
	    return messages_return::handled;
	}
    else if ( msg == NeutrinoMessages::EVT_MODECHANGED )
	{
        aspectRatio = data;
        if ( is_visible && showButtonBar )
			showIcon_16_9();

        return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_TIMESET )
	{
		gotTime = true;
	}

	return messages_return::unhandled;
}


void CInfoViewer::showButton_SubServices()
{
	if ( g_RemoteControl->subChannels.size()> 0 )
	{
		// gelbe Taste für NVODs / Subservices
		g_FrameBuffer->paintIcon("gelb.raw", BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );

		if ( g_RemoteControl->are_subchannels )
			// SubServices
			g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.subservice").c_str(), COL_INFOBAR_BUTTONS);
		else
			// NVOD
			g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 2* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.selecttime").c_str(), COL_INFOBAR_BUTTONS);
	}
}

void CInfoViewer::getEPG()
{
	g_Sectionsd->getCurrentNextServiceKey( current_onid_sid, info_CurrentNext );

	if ( ( info_CurrentNext.flags & sectionsd::epgflags::has_current ) &&
		 ( showButtonBar ) )
	{
		sectionsd::CurrentNextInfo*	_info = new sectionsd::CurrentNextInfo;
		*_info = info_CurrentNext;
		g_RCInput->postMsg( NeutrinoMessages::EVT_CURRENTEPG, (unsigned) _info, false );
	}


}


void CInfoViewer::show_Data( bool calledFromEvent = false )
{
	char runningStart[10];
	char runningDuration[10];
	char runningRest[20];
	char runningPercent;

	char nextStart[10];
	char nextDuration[10];

	int is_nvod= false;

	if ( is_visible )
	{

       	if ( ( g_RemoteControl->current_onid_sid == current_onid_sid ) &&
       		 ( g_RemoteControl->subChannels.size()> 0 ) && ( !g_RemoteControl->are_subchannels ) )
       	{
			is_nvod = true;
 			info_CurrentNext.current_zeit.startzeit = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].startzeit;
 			info_CurrentNext.current_zeit.dauer = g_RemoteControl->subChannels[g_RemoteControl->selected_subchannel].dauer;
        }
        else
        {
        	if ( ( info_CurrentNext.flags & sectionsd::epgflags::has_current) &&
        		 ( info_CurrentNext.flags & sectionsd::epgflags::has_next) &&
        		 ( showButtonBar ) )
        	{
        		if ( info_CurrentNext.next_zeit.startzeit < ( info_CurrentNext.current_zeit.startzeit+ info_CurrentNext.current_zeit.dauer ) )
        		{
        			is_nvod = true;
        		}
        	}
        }

		time_t jetzt=time(NULL);

		if ( info_CurrentNext.flags & sectionsd::epgflags::has_current)
		{
			unsigned dauer = info_CurrentNext.current_zeit.dauer / 60;
			int rest = ( (info_CurrentNext.current_zeit.startzeit + info_CurrentNext.current_zeit.dauer) - jetzt ) / 60;

			int seit = ( jetzt - info_CurrentNext.current_zeit.startzeit ) / 60;
			if ( seit< 0 )
			{
				runningPercent= 0;
				sprintf( (char*)&runningRest, "in %d min", -seit);
			}
			else
			{
				runningPercent=(unsigned)((float)(jetzt-info_CurrentNext.current_zeit.startzeit)/(float)info_CurrentNext.current_zeit.dauer*100.);
				sprintf( (char*)&runningRest, "%d / %d min", seit, rest);
			}

			struct tm *pStartZeit = localtime(&info_CurrentNext.current_zeit.startzeit);
        	sprintf( (char*)&runningStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min );


		}

		if ( info_CurrentNext.flags & sectionsd::epgflags::has_next)
		{
			unsigned dauer = info_CurrentNext.next_zeit.dauer/ 60;
			sprintf( (char*)&nextDuration, "%d min", dauer);
			struct tm *pStartZeit = localtime(&info_CurrentNext.next_zeit.startzeit);
			sprintf( (char*)&nextStart, "%02d:%02d", pStartZeit->tm_hour, pStartZeit->tm_min);
		}

		int height = g_Fonts->infobar_channame->getHeight()/3;
		int ChanInfoY = BoxStartY + ChanHeight+ 15; //+10

		//percent
		if ( showButtonBar )
		{
			int posy = BoxStartY+12;
			int height2= 20;

			if ( info_CurrentNext.flags & sectionsd::epgflags::has_current)
			{
				g_FrameBuffer->paintBoxRel(BoxEndX-114, posy,   2+100+2, height2, COL_INFOBAR_SHADOW); //border
				g_FrameBuffer->paintBoxRel(BoxEndX-112, posy+2, runningPercent+2, height2-4, COL_INFOBAR+7);//fill(active)
				g_FrameBuffer->paintBoxRel(BoxEndX-112+runningPercent, posy+2, 100-runningPercent, height2-4, COL_INFOBAR+3);//fill passive
			}
			if ( info_CurrentNext.flags & sectionsd::epgflags::has_anything )
			{
				g_FrameBuffer->paintIcon("rot.raw", BoxEndX- ICON_OFFSET- 4* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
				g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 4* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.eventlist").c_str(), COL_INFOBAR_BUTTONS);
			}
		}

		height = g_Fonts->infobar_info->getHeight();
		int xStart= BoxStartX + ChanWidth;

		g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

		if ( ( info_CurrentNext.flags & sectionsd::epgflags::not_broadcast ) ||
			 ( ( calledFromEvent ) && !( info_CurrentNext.flags & ( sectionsd::epgflags::has_next | sectionsd::epgflags::has_current ) ) ) )
		{
			// kein EPG verfügbar
			ChanInfoY += height;
			g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height, COL_INFOBAR);
			g_Fonts->infobar_info->RenderString(BoxStartX + ChanWidth + 20,  ChanInfoY+height, BoxEndX- (BoxStartX + ChanWidth + 20), g_Locale->getText(gotTime?"infoviewer.noepg":"infoviewer.waittime").c_str(), COL_INFOBAR);
		}
		else
		{
			// irgendein EPG gefunden
			int duration1Width   = g_Fonts->infobar_info->getRenderWidth(runningRest);
			int duration1TextPos = BoxEndX- duration1Width- 10;

			int duration2Width   = g_Fonts->infobar_info->getRenderWidth(nextDuration);
			int duration2TextPos = BoxEndX- duration2Width- 10;

			if ( ( info_CurrentNext.flags & sectionsd::epgflags::has_next ) && ( !( info_CurrentNext.flags & sectionsd::epgflags::has_current )) )
			{
				// spätere Events da, aber kein aktuelles...
				g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, BoxEndX- xStart, g_Locale->getText("infoviewer.nocurrent").c_str(), COL_INFOBAR);

				ChanInfoY += height;

				//info next
				g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

				g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
				g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, duration2TextPos- xStart- 5, info_CurrentNext.next_name, COL_INFOBAR);
				g_Fonts->infobar_info->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
			}
			else
			{
				g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, runningStart, COL_INFOBAR);
				g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, duration1TextPos- xStart- 5, info_CurrentNext.current_name, COL_INFOBAR);
				g_Fonts->infobar_info->RenderString(duration1TextPos,            ChanInfoY+height, duration1Width, runningRest, COL_INFOBAR);

				ChanInfoY += height;

				//info next
				g_FrameBuffer->paintBox(ChanInfoX+ 10, ChanInfoY, BoxEndX, ChanInfoY+ height , COL_INFOBAR);

				if ( ( !is_nvod ) && ( info_CurrentNext.flags & sectionsd::epgflags::has_next ) )
				{
					g_Fonts->infobar_info->RenderString(ChanInfoX+10,                ChanInfoY+height, 100, nextStart, COL_INFOBAR);
					g_Fonts->infobar_info->RenderString(xStart,  ChanInfoY+height, duration2TextPos- xStart- 5, info_CurrentNext.next_name, COL_INFOBAR);
					g_Fonts->infobar_info->RenderString(duration2TextPos,            ChanInfoY+height, duration2Width, nextDuration, COL_INFOBAR);
				}
			}
		}
	}
}

void CInfoViewer::showButton_Audio()
{
/*        string  to_compare= getActiveChannelID();

        if ( strcmp(g_RemoteControl->audio_chans.name, to_compare.c_str() )== 0 )
        {
                if ( ( g_RemoteControl->ecmpid == invalid_ecmpid_found ) ||
                     ( ( g_RemoteControl->audio_chans.count_apids == 0 ) && ( g_RemoteControl->vpid == 0 ) ) )
                {
                        int height = g_Fonts->infobar_info->getHeight();
                        int ChanInfoY = BoxStartY + ChanHeight+ 15+ 2* height;
                        int xStart= BoxStartX + ChanWidth + 20;

                        //int ChanNameX = BoxStartX + ChanWidth + 10;
                        int ChanNameY = BoxStartY + ChanHeight + 10;


                        string  disp_text;
                        if ( ( g_RemoteControl->ecmpid == invalid_ecmpid_found ) )
						{
                                disp_text= g_Locale->getText("infoviewer.cantdecode");
								#ifdef USEACTIONLOG
									g_ActionLog->println("cannot decode");
								#endif
						}
                        else
						{
                                disp_text= g_Locale->getText("infoviewer.notavailable");
								#ifdef USEACTIONLOG
									g_ActionLog->println("not available");
								#endif
						}

                        g_FrameBuffer->paintBox(ChanInfoX, ChanNameY, BoxEndX, ChanInfoY, COL_INFOBAR);
                        g_Fonts->infobar_info->RenderString(xStart, ChanInfoY, BoxEndX- xStart, disp_text.c_str(), COL_INFOBAR);
                        KillShowEPG = true;
                };
*/
	// grün, wenn mehrere APIDs
	int count = g_RemoteControl->current_PIDs.APIDs.size();
	if ( count > 1 )
	{
		g_FrameBuffer->paintIcon("gruen.raw", BoxEndX- ICON_OFFSET- 3* ButtonWidth+ 8, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
		g_Fonts->infobar_small->RenderString(BoxEndX- ICON_OFFSET- 3* ButtonWidth+ 29, BoxEndY - 2, ButtonWidth- 30, g_Locale->getText("infoviewer.languages").c_str(), COL_INFOBAR_BUTTONS);
	};

	if ( ( g_RemoteControl->selected_apid < count ) &&
	     ( g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->selected_apid].is_ac3 ) )
		g_FrameBuffer->paintIcon("dd.raw", BoxEndX- ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
	else if ( g_RemoteControl->has_ac3 )
		g_FrameBuffer->paintIcon("dd_avail.raw", BoxEndX- ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
	else
		g_FrameBuffer->paintIcon("dd_gray.raw", BoxEndX- ICON_LARGE- ICON_SMALL, BoxEndY- ((InfoHeightY_Info+ 16)>>1) );
}

void CInfoViewer::killTitle()
{
	if (is_visible )
	{
		is_visible = false;
		g_FrameBuffer->paintBackgroundBox(BoxStartX, BoxStartY, BoxEndX+ SHADOW_OFFSET, BoxEndY+ SHADOW_OFFSET );
	}
}



