/*
	Neutrino-GUI  -   DBoxII-Project

	Timerliste by Zwen
	
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

#include <gui/timeosd.h>
#include <driver/fontrenderer.h>
#include <global.h>
#include <system/settings.h>

#define TIMEOSD_FONT SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME

CTimeOSD::CTimeOSD()
{
	frameBuffer = CFrameBuffer::getInstance();
	visible=false;
	GetDimensions();
}

CTimeOSD::~CTimeOSD()
{
	hide();
}

void CTimeOSD::show(time_t time_show)
{
	GetDimensions();
	visible = true;
	m_time_dis  = time(NULL);
	m_time_show = time_show;
	update();
}

void CTimeOSD::GetDimensions()
{
	m_xstart = g_settings.screen_StartX + 10;
	m_xend = g_settings.screen_EndX - 10;
	m_height = g_Font[TIMEOSD_FONT]->getHeight();
	m_y = g_settings.screen_StartY + 10;
	m_width = g_Font[TIMEOSD_FONT]->getRenderWidth("00:00:00");
}

void CTimeOSD::update()
{
	if(visible)
	{
		char cDisplayTime[8+1];
		time_t tDisplayTime = m_time_show + (time(NULL) - m_time_dis);
		strftime(cDisplayTime, 9, "%T", gmtime(&tDisplayTime));
		frameBuffer->paintBackgroundBoxRel(m_xend - m_width -20, m_y - 10 , m_width + 40, m_height + 20);
		frameBuffer->paintBoxRel(m_xend - m_width - 10, m_y , m_width + 10 , m_height, COL_MENUCONTENT);
		g_Font[SNeutrinoSettings::TIMEOSD_FONT]->RenderString(m_xend - m_width - 5,m_y + m_height,
																								  m_width+5,
																								  cDisplayTime, 
																								  COL_MENUCONTENT);
	}
}

void CTimeOSD::hide()
{
	frameBuffer->paintBackgroundBoxRel(m_xend - m_width -20, m_y - 10 , m_width + 40, m_height + 20);
	visible=false;
}
