/*
	$Id: epgplus.cpp,v 1.52 2009/06/28 20:48:53 seife Exp $

	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Copyright (C) 2004 Martin Griep 'vivamiga'

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

#include <iostream>

#include <global.h>
#include <neutrino.h>

#include <gui/epgplus.h>
#include <gui/timerlist.h>

#include <sectionsdclient/sectionsdclient.h>

#include <gui/widget/icons.h>
#include <gui/widget/buttons.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/mountchooser.h>
#include <gui/widget/dirchooser.h>
#include "bouquetlist.h"

#include <zapit/client/zapittools.h> /* ZapitTools::UTF8_to_Latin1 */
#include <driver/rcinput.h>
#include <driver/screen_max.h>

#include <algorithm>
#include <sstream>

//#define DEBUG_

extern CBouquetList* bouquetList;

EpgPlus::Settings EpgPlus::settings(true);
EpgPlus::Fonts  EpgPlus::fonts;
EpgPlus::Colors EpgPlus::colors;
EpgPlus::Sizes  EpgPlus::sizes;

time_t EpgPlus::duration = 0;

int EpgPlus::horGap1Height = 0;
int EpgPlus::horGap2Height = 0;
int EpgPlus::verGap1Width = 0;
int EpgPlus::verGap2Width = 0;

int EpgPlus::horGap1Color = 0;
int EpgPlus::horGap2Color = 0;
int EpgPlus::verGap1Color = 0;
int EpgPlus::verGap2Color = 0;

int EpgPlus::sliderWidth        = 0;
int EpgPlus::channelsTableWidth = 0;

int EpgPlus::sliderBackColor = 0;
int EpgPlus::sliderKnobColor = 0;

static EpgPlus::FontSetting fontSettingDefaultTable[] =
{
	{ EpgPlus::EPGPlus_header_font                     , "EPGPlus.header_font"                           , LOCALE_EPGPLUS_HEADER_FONT                      , "md_khmurabi_10.ttf", "Bold"   , 20, true},
	{ EpgPlus::EPGPlus_timeline_fonttime               , "EPGPlus.timeline_fonttime"                     , LOCALE_EPGPLUS_TIMELINE_FONTTIME                , "micron.ttf", "Bold"   , 16, true},
	{ EpgPlus::EPGPlus_timeline_fontdate               , "EPGPlus.timeline_fontdate"                     , LOCALE_EPGPLUS_TIMELINE_FONTDATE                , "micron.ttf", "Bold"   , 14, true},
	{ EpgPlus::EPGPlus_channelentry_font               , "EPGPlus.channelentry_font"                     , LOCALE_EPGPLUS_CHANNELENTRY_FONT                , "micron.ttf", "Bold"   , 16, true},
	{ EpgPlus::EPGPlus_channelevententry_font          , "EPGPlus.channelevententry_font"                , LOCALE_EPGPLUS_CHANNELEVENTENTRY_FONT           , "micron.ttf", "Regular", 16, true},
	{ EpgPlus::EPGPlus_footer_fontbouquetchannelname   , "EPGPlus.footer_fontbouquetchannelname"         , LOCALE_EPGPLUS_FOOTER_FONTBOUQUETCHANNELNAME    , "micron.ttf", "Bold"   , 24, true},
	{ EpgPlus::EPGPlus_footer_fonteventdescription     , "EPGPlus.footer_fonteventdescription"           , LOCALE_EPGPLUS_FOOTER_FONTEVENTDESCRIPTION      , "micron.ttf", "Regular", 16, true},
	{ EpgPlus::EPGPlus_footer_fonteventshortdescription, "EPGPlus.footer_fonteventshortdescription"      , LOCALE_EPGPLUS_FOOTER_FONTEVENTSHORTDESCRIPTION , "micron.ttf", "Regular", 16, true},
	{ EpgPlus::EPGPlus_footer_fontbuttons              , "EPGPlus.footer_fontbuttons"                    , LOCALE_EPGPLUS_FOOTER_FONTBUTTONS               , "md_khmurabi_10.ttf", "Regular", 16, true},
};

static EpgPlus::ColorSetting colorSettingDefaultTable[] =
{
	{ EpgPlus::EPGPlus_header_color                           , "EPGPlus.header_color"                          , NONEXISTANT_LOCALE  , COL_MENUHEAD           , false},
	{ EpgPlus::EPGPlus_timeline_color1                        , "EPGPlus.timeline_color1"                       , NONEXISTANT_LOCALE  , COL_MENUCONTENT_PLUS_1 , false},
	{ EpgPlus::EPGPlus_timeline_color2                        , "EPGPlus.timeline_color2"                       , NONEXISTANT_LOCALE  , COL_MENUCONTENT_PLUS_2 , false},
	{ EpgPlus::EPGPlus_timeline_markcolor                     , "EPGPlus.timeline_markcolor"                    , NONEXISTANT_LOCALE  , COL_MENUCONTENTSELECTED, false},
	{ EpgPlus::EPGPlus_timeline_backmarkcolor                 , "EPGPlus.timeline_backmarkcolor"                , NONEXISTANT_LOCALE  , COL_MENUCONTENT        , false},
	{ EpgPlus::EPGPlus_timeline_gridcolor                     , "EPGPlus.timeline_gridcolor"                    , NONEXISTANT_LOCALE  , COL_MENUCONTENT_PLUS_5 , false},
	{ EpgPlus::EPGPlus_channelevententry_normalcolor1         , "EPGPlus.channelevententry_normalcolor1"        , NONEXISTANT_LOCALE  , COL_MENUCONTENT_PLUS_1 , false},
	{ EpgPlus::EPGPlus_channelevententry_normalcolor2         , "EPGPlus.channelevententry_normalcolor2"        , NONEXISTANT_LOCALE  , COL_MENUCONTENT_PLUS_2 , false},
	{ EpgPlus::EPGPlus_channelevententry_selectioncolor       , "EPGPlus.channelevententry_selectioncolor"      , NONEXISTANT_LOCALE  , COL_MENUCONTENTSELECTED, false},
	{ EpgPlus::EPGPlus_channelevententry_dummyeventcolor      , "EPGPlus.channelevententry_dummyeventcolor"     , NONEXISTANT_LOCALE  , COL_MENUCONTENT        , false},
	{ EpgPlus::EPGPlus_channelevententry_separationlinecolor  , "EPGPlus.channelevententry_separationlinecolor" , NONEXISTANT_LOCALE  , COL_MENUCONTENT_PLUS_5 , false},
	{ EpgPlus::EPGPlus_channelentry_normalcolor               , "EPGPlus.channelentry_normalcolor"              , NONEXISTANT_LOCALE  , COL_MENUCONTENT        , false},
	{ EpgPlus::EPGPlus_channelentry_selectioncolor            , "EPGPlus.channelentry_selectioncolor"           , NONEXISTANT_LOCALE  , COL_MENUCONTENTSELECTED, false},
	{ EpgPlus::EPGPlus_channelentry_separationlinecolor       , "EPGPlus.channelentry_separationlinecolor"      , NONEXISTANT_LOCALE  , COL_MENUCONTENT_PLUS_5 , false},
	{ EpgPlus::EPGPlus_footer_color                           , "EPGPlus.Footer_color"                          , NONEXISTANT_LOCALE  , COL_MENUHEAD           , false},
	{ EpgPlus::EPGPlus_slider_knobcolor                       , "EPGPlus.slider_knobcolor"                      , NONEXISTANT_LOCALE  , COL_MENUCONTENT_PLUS_3 , false},
	{ EpgPlus::EPGPlus_slider_backcolor                       , "EPGPlus.slider_backcolor"                      , NONEXISTANT_LOCALE  , COL_MENUCONTENT_PLUS_0 , false},
	{ EpgPlus::EPGPlus_horgap1_color                          , "EPGPlus.horgap1_color"                         , NONEXISTANT_LOCALE  , 0                      , false},
	{ EpgPlus::EPGPlus_horgap2_color                          , "EPGPlus.horgap2_color"                         , NONEXISTANT_LOCALE  , 0                      , false},
	{ EpgPlus::EPGPlus_vergap1_color                          , "EPGPlus.vergap1_color"                         , NONEXISTANT_LOCALE  , 0                      , false},
	{ EpgPlus::EPGPlus_vergap2_color                          , "EPGPlus.vergap2_color"                         , NONEXISTANT_LOCALE  , 0                      , false},
};

static EpgPlus::SizeSetting sizeSettingDefaultTable[] =
{
	{ EpgPlus::EPGPlus_channelentry_width               , "EPGPlus.channelentry_width"                    , LOCALE_EPGPLUS_CHANNELENTRY_WIDTH               , 100, true},
	{ EpgPlus::EPGPlus_channelentry_separationlineheight, "EPGPlus.channelentry_separationlineheight"     , LOCALE_EPGPLUS_CHANNELENTRY_SEPARATIONLINEHEIGHT, 2  , true},
	{ EpgPlus::EPGPlus_slider_width                     , "EPGPlus.slider_width"                          , LOCALE_EPGPLUS_SLIDER_WIDTH                     , 15 , true},
	{ EpgPlus::EPGPlus_horgap1_height                   , "EPGPlus.horgap1_height"                        , LOCALE_EPGPLUS_HORGAP1_HEIGHT                   , 4  , true},
	{ EpgPlus::EPGPlus_horgap2_height                   , "EPGPlus.horgap2_height"                        , LOCALE_EPGPLUS_HORGAP2_HEIGHT                   , 4  , true},
	{ EpgPlus::EPGPlus_vergap1_width                    , "EPGPlus.vergap1_width"                         , LOCALE_EPGPLUS_VERGAP1_WIDTH                    , 4  , true},
	{ EpgPlus::EPGPlus_vergap2_width                    , "EPGPlus.vergap2_width"                         , LOCALE_EPGPLUS_VERGAP2_WIDTH                    , 4  , true},
};

EpgPlus::Settings::~Settings()
{
	delete[] this->fontSettings;
}

EpgPlus::Settings::Settings
  ( bool doInit
  )
{
	this->fontSettings  = new FontSetting[sizeof(fontSettingDefaultTable)/sizeof(FontSetting)];
	this->colorSettings = new ColorSetting[sizeof(colorSettingDefaultTable)/sizeof(ColorSetting)];
	this->sizeSettings  = new SizeSetting[sizeof(sizeSettingDefaultTable)/sizeof(SizeSetting)];

	if (doInit)
	{
		for ( size_t i = 0
		    ; i < NumberOfFontSettings
			 ; ++i
			 )
		{
			this->fontSettings[i] = fontSettingDefaultTable[i];
		}

		for ( size_t i = 0
		    ; i < NumberOfColorSettings
			 ; ++i
			 )
		{
			this->colorSettings[i] = colorSettingDefaultTable[i];
		}

		for ( size_t i = 0
		    ; i < NumberOfSizeSettings
			 ; ++i
			 )
		{
			this->sizeSettings[i] = sizeSettingDefaultTable[i];
		}


		this->durationSetting = 2 * 60 * 60;

//		EpgPlus::loadSettings();
	}
}

Font* EpgPlus::Header::font      = NULL;
int   EpgPlus::Header::color = 0;

EpgPlus::Header::Header(CFrameBuffer* _frameBuffer, int _x, int _y, int _width)
{
	frameBuffer = _frameBuffer;
	x     = _x;
	y     = _y;
	width = _width;
}

EpgPlus::Header::~Header()
{
}

void EpgPlus::Header::init()
{
  font  = EpgPlus::fonts[EPGPlus_header_font];
	color = EpgPlus::colors[EPGPlus_header_color];
}

void EpgPlus::Header::paint()
{
	// clear the region
	this->frameBuffer->paintBoxRel
		( this->x
		, this->y
		, this->width
		, this->font->getHeight()
		, this->color
		, RADIUS_MID
		, CORNER_TOP
		);

	// display new text
  this->font->RenderString
		( this->x + 10
		, this->y + this->font->getHeight()
		, this->width - 20
		, g_Locale->getText(LOCALE_EPGPLUS_HEAD)
		, this->color
		, 0
		, true
		);

}

int EpgPlus::Header::getUsedHeight()
{
  return font->getHeight();
}

Font* EpgPlus::TimeLine::fontTime = NULL;
Font* EpgPlus::TimeLine::fontDate = NULL;
int   EpgPlus::TimeLine::color1        = 0;
int   EpgPlus::TimeLine::color2        = 0;
int   EpgPlus::TimeLine::markColor     = 0;
int   EpgPlus::TimeLine::backMarkColor = 0;
int   EpgPlus::TimeLine::gridColor     = 0;

EpgPlus::TimeLine::TimeLine(CFrameBuffer* _frameBuffer, int _x, int _y, int _width, int _startX, int _durationX)
{
	frameBuffer= _frameBuffer;
	x          = _x;
	y          = _y;
	width      = _width;
	startX     = _startX;
	durationX  = _durationX;
}

void EpgPlus::TimeLine::init()
{
  fontTime       = EpgPlus::fonts[EPGPlus_timeline_fonttime];
  fontDate       = EpgPlus::fonts[EPGPlus_timeline_fontdate];

	color1         = EpgPlus::colors[EPGPlus_timeline_color1];
	color2         = EpgPlus::colors[EPGPlus_timeline_color2];
	markColor      = EpgPlus::colors[EPGPlus_timeline_markcolor];
	backMarkColor  = EpgPlus::colors[EPGPlus_timeline_backmarkcolor];
	gridColor      = EpgPlus::colors[EPGPlus_timeline_gridcolor];
}

EpgPlus::TimeLine::~TimeLine()
{
}

void EpgPlus::TimeLine::paint(time_t startTime, int _duration)
{
#ifdef DEBUG_
	std::cout << "EpgPlus::TimeLine::paint" << std::endl;
#endif

	clearMark();
	int xPos = startX;

	currentDuration = _duration;
	int numberOfTicks = currentDuration / (60 * 60) * 2;
	int tickDist = durationX / numberOfTicks;
	time_t tickTime = startTime;
	bool toggleColor = false;

	// display date of begin
	frameBuffer->paintBoxRel(x, y, width, fontTime->getHeight(), color1);

	fontDate->RenderString(x + 4, y + fontDate->getHeight(), width,
			       EpgPlus::getTimeString(startTime, "%d-%b"), color1, 0, true); // UTF-8

	// paint ticks
	for (int i = 0; i < numberOfTicks; ++i, xPos += tickDist, tickTime += _duration / numberOfTicks)
	{
		int xWidth = tickDist;
		if (xPos + xWidth > x + width)
			xWidth = x + width - xPos;

		frameBuffer->paintBoxRel(xPos, y, xWidth, fontTime->getHeight(), toggleColor ? color1 : color2);
		std::string timeStr = EpgPlus::getTimeString(tickTime, "%H");
		int textWidth = fontTime->getRenderWidth(timeStr, true);

		fontTime->RenderString(xPos - textWidth - 4, y + fontTime->getHeight(), textWidth, timeStr,
				       toggleColor ? color2 : color1, 0, true); // UTF-8

		timeStr = EpgPlus::getTimeString(tickTime, "%M");
		textWidth = fontTime->getRenderWidth(timeStr, true);
		fontTime->RenderString(xPos + 4, y + fontTime->getHeight(), textWidth, timeStr,
				       toggleColor ? color1 : color2, 0, true); // UTF-8

		toggleColor = !toggleColor;
	}
}

void EpgPlus::TimeLine::paintGrid()
{
	#ifdef DEBUG_
		std::cout << "EpgPlus::TimeLine::paintGrid " << this->y << " " << std::endl;
	#endif

	int xPos = this->startX;
	int numberOfTicks = this->currentDuration/(60*60) * 2;
	int tickDist = (this->durationX)/numberOfTicks;
	// paint ticks
	for ( int i = 0
	    ; i < numberOfTicks
	    ;   ++i
	      , xPos += tickDist
	    )
	{
		// display a line for the tick
		this->frameBuffer->paintVLineRel
			( xPos
			, this->y
			, this->fontTime->getHeight()
			, this->gridColor
			);
	}
}

void EpgPlus::TimeLine::paintMark(time_t startTime, int _duration, int _x, int _width)
{
#ifdef DEBUG_
	std::cout << "EpgPlus::TimeLine::paintMark" << std::endl;
#endif

	// clear old mark
	clearMark();

	// paint new mark
	frameBuffer->paintBoxRel(_x, y + fontTime->getHeight(), _width, fontTime->getHeight(), markColor);
	// display start time before mark
	std::string timeStr = EpgPlus::getTimeString(startTime, "%H:%M");
	int textWidth = fontTime->getRenderWidth(timeStr, true);
	int textHeight = fontTime->getHeight();

	fontTime->RenderString(_x - textWidth, y + 2 * textHeight, textWidth, timeStr, backMarkColor, 0, true); // UTF-8

	// display end time after mark
	timeStr = EpgPlus::getTimeString(startTime + _duration, "%H:%M");
	textWidth = fontTime->getRenderWidth(timeStr, true);

	if (_x + _width + textWidth < x + width)
		fontTime->RenderString(_x + _width, y + 2 * textHeight, textWidth, timeStr, backMarkColor, 0, true); // UTF-8
	else if (textWidth < _width - 10)
		fontTime->RenderString(_x + _width - textWidth, y + 2 * textHeight, textWidth, timeStr, markColor, 0, true); // UTF-8
}

void EpgPlus::TimeLine::clearMark()
{
	// clear mark
	this->frameBuffer->paintBoxRel
		( this->x
    , this->y + this->fontTime->getHeight()
		, this->width
		, this->fontTime->getHeight()
		, this->backMarkColor
		);
}

int EpgPlus::TimeLine::getUsedHeight()
{
  return   std::max(fontDate->getHeight(), fontTime->getHeight())
         + fontTime->getHeight();
}

Font* EpgPlus::ChannelEventEntry::font = NULL;
int   EpgPlus::ChannelEventEntry::separationLineHeight = 0;
int   EpgPlus::ChannelEventEntry::separationLineColor  = 0;
int   EpgPlus::ChannelEventEntry::normalColor1         = 0;
int   EpgPlus::ChannelEventEntry::normalColor2         = 0;
int   EpgPlus::ChannelEventEntry::selectionColor       = 0;
int   EpgPlus::ChannelEventEntry::dummyEventColor      = 0;

EpgPlus::ChannelEventEntry::ChannelEventEntry(const CChannelEvent* _channelEvent, CFrameBuffer* _frameBuffer,
					      TimeLine* _timeLine, Footer* _footer, int _x, int _y, int _width)
{
	// copy neccessary?
	if (_channelEvent != NULL)
		channelEvent = *_channelEvent;

	frameBuffer = _frameBuffer;
	timeLine = _timeLine;
	footer = _footer;
	x      = _x;
	y      = _y;
	width  = _width;
}

void EpgPlus::ChannelEventEntry::init()
{
  font                 = EpgPlus::fonts[EPGPlus_channelevententry_font];

  normalColor1         = EpgPlus::colors[EPGPlus_channelevententry_normalcolor1];
	normalColor2         = EpgPlus::colors[EPGPlus_channelevententry_normalcolor2];
	selectionColor       = EpgPlus::colors[EPGPlus_channelevententry_selectioncolor];
	dummyEventColor      = EpgPlus::colors[EPGPlus_channelevententry_dummyeventcolor];
  separationLineColor  = EpgPlus::colors[EPGPlus_channelevententry_separationlinecolor];

  separationLineHeight = EpgPlus::sizes[EPGPlus_channelentry_separationlineheight];
}

EpgPlus::ChannelEventEntry::~ChannelEventEntry()
{
}

bool EpgPlus::ChannelEventEntry::isSelected
  ( time_t selectedTime
  ) const
{
	#ifdef DEBUG_
		std::cout << "isSelected " << EpgPlus::getTimeString(this->channelEvent.startTime, "%H:%M") << " " << this->channelEvent.duration << std::endl;
	#endif

	return (selectedTime >= this->channelEvent.startTime) && (selectedTime < this->channelEvent.startTime + time_t(this->channelEvent.duration));
}


void EpgPlus::ChannelEventEntry::paint(bool _isSelected, bool toggleColor)
{
#ifdef DEBUG_
	std::cout << "EpgPlus::ChannelEventEntry::paint " << this->x << " " << this->y << " " << this->width << " " << " " << this->channelEvent.description << std::endl;
#endif

	frameBuffer->paintBoxRel(x, y, width, font->getHeight(),
				 channelEvent.description.empty() ?
					dummyEventColor :
					(_isSelected ? selectionColor : (toggleColor ? normalColor1 : normalColor2)));

	font->RenderString(x + 2, y + font->getHeight(), width - 4 > 0 ? width - 4 : 0, channelEvent.description,
			   _isSelected ? selectionColor : (toggleColor ? normalColor1 : normalColor2), false);

	// paint the separation line
	if (separationLineHeight > 0)
		frameBuffer->paintBoxRel(x, y + font->getHeight(), width, separationLineHeight, separationLineColor);

	if (_isSelected)
	{
		if (channelEvent.description.empty())
		{// dummy channel event
			timeLine->clearMark();
		}
		else
		{
			timeLine->paintMark(channelEvent.startTime, channelEvent.duration, x, width);
		}
#ifdef DEBUG_
		std::cout << "paintEventDetails1" << std::endl;
#endif
		CShortEPGData shortEpgData;

		footer->paintEventDetails(channelEvent.description,
					  g_Sectionsd->getEPGidShort(channelEvent.eventID, &shortEpgData) ? shortEpgData.info1 : "");
#ifdef DEBUG_
		std::cout << "paintEventDetails2" << std::endl;
#endif
		timeLine->paintGrid();
	}
}

int EpgPlus::ChannelEventEntry::getUsedHeight()
{
  return  font->getHeight()
        + separationLineHeight;
}

Font* EpgPlus::ChannelEntry::font = NULL;
int   EpgPlus::ChannelEntry::separationLineHeight = 0;
int   EpgPlus::ChannelEntry::separationLineColor  = 0;
int   EpgPlus::ChannelEntry::selectionColor       = 0;
int   EpgPlus::ChannelEntry::normalColor          = 0;

EpgPlus::ChannelEntry::ChannelEntry(const CChannelList::CChannel* _channel, int _index,
				    CFrameBuffer* _frameBuffer, Footer* _footer, CBouquetList* _bouquetList,
				    int _x, int _y, int _width)
{
	channel = _channel;

	if (channel != NULL)
	{
		std::stringstream tmpName;
		tmpName << _index + 1 << " " << channel->getName();
		displayName  = ZapitTools::UTF8_to_Latin1(tmpName.str().c_str());
	}

	index = _index;
	frameBuffer = _frameBuffer;
	footer      = _footer;
	bouquetList = _bouquetList;
	x     = _x;
	y     = _y;
	width = _width;
}

void EpgPlus::ChannelEntry::init()
{
  font                 = EpgPlus::fonts[EPGPlus_channelentry_font];

	normalColor          = EpgPlus::colors[EPGPlus_channelentry_normalcolor];
	selectionColor       = EpgPlus::colors[EPGPlus_channelentry_selectioncolor];
  separationLineColor  = EpgPlus::colors[EPGPlus_channelentry_separationlinecolor];

  separationLineHeight = EpgPlus::sizes[EPGPlus_channelentry_separationlineheight];
}

EpgPlus::ChannelEntry::~ChannelEntry()
{
	for ( TCChannelEventEntries::iterator It = this->channelEventEntries.begin()
	    ; It != this->channelEventEntries.end()
	    ; ++It
	    )
		delete *It;
}

void EpgPlus::ChannelEntry::paint
  ( bool isSelected
  , time_t selectedTime
  )
{
	#ifdef DEBUG_
		std::cout << "EpgPlus::ChannelEntry::paint " << isSelected << " " << this->x << " " << this->y << " " << this->width << " " << " " << this->displayName << std::endl;
	#endif

	this->frameBuffer->paintBoxRel
		( this->x
		, this->y
		, this->width
		, this->font->getHeight()
		, isSelected?this->selectionColor:this->normalColor
		);

	this->font->RenderString
		( this->x + 2
		, this->y + this->font->getHeight()
		, this->width - 4
		, this->displayName
		, isSelected?this->selectionColor:this->normalColor
		, true
		);

  if (isSelected)
  {
	  for ( uint i = 0
        ; i < this->bouquetList->Bouquets.size()
        ; ++i
        )
	  {
      CBouquet* bouquet = this->bouquetList->Bouquets[i];
	    for ( int j = 0
          ; j < bouquet->channelList->getSize()
          ; ++j
          )
	    {

	      #ifdef DEBUG_
		      std::cout << "(*bouquet->channelList)[j]->number " << (*bouquet->channelList)[j]->number << " this->channel->number " << this->channel->number << std::endl;
	      #endif
        if ((*bouquet->channelList)[j]->number == this->channel->number)
        {
          this->footer->setBouquetChannelName
            ( bouquet->channelList->getName()
            , ZapitTools::UTF8_to_Latin1(this->channel->getName().c_str())
            );

          bouquet = NULL;

          break;
        }
      }
      if (bouquet == NULL)
        break;
	  }
  }

  // paint the separation line
  if (separationLineHeight > 0)
  {
	  this->frameBuffer->paintBoxRel
		  ( this->x
		  , this->y + this->font->getHeight()
		  , this->width
		  , this->separationLineHeight
		  , this->separationLineColor
		  );
  }

	bool toggleColor = false;
	for ( TCChannelEventEntries::iterator It = this->channelEventEntries.begin()
	    ; It != this->channelEventEntries.end()
	    ; ++It
	    )
	{
		(*It)->paint(isSelected && (*It)->isSelected(selectedTime), toggleColor);

		toggleColor = !toggleColor;
	}

}

int EpgPlus::ChannelEntry::getUsedHeight()
{
  return  font->getHeight()
        + separationLineHeight;
}

Font* EpgPlus::Footer::fontBouquetChannelName    = NULL;
Font* EpgPlus::Footer::fontEventDescription      = NULL;
Font* EpgPlus::Footer::fontEventShortDescription = NULL;
Font* EpgPlus::Footer::fontButtons = NULL;
int   EpgPlus::Footer::color   = 0;

EpgPlus::Footer::Footer(CFrameBuffer* _frameBuffer, int _x, int _y, int _width)
{
	frameBuffer = _frameBuffer;
	x     = _x;
	y     = _y;
	width = _width;
}

EpgPlus::Footer::~Footer()
{
}

void EpgPlus::Footer::init()
{
  fontBouquetChannelName    = EpgPlus::fonts[EPGPlus_footer_fontbouquetchannelname];
  fontEventDescription      = EpgPlus::fonts[EPGPlus_footer_fonteventdescription];
  fontEventShortDescription = EpgPlus::fonts[EPGPlus_footer_fonteventshortdescription];
  fontButtons               = EpgPlus::fonts[EPGPlus_footer_fontbuttons];

  color                     = EpgPlus::colors[EPGPlus_footer_color];
}


void EpgPlus::Footer::setBouquetChannelName
  ( const std::string& newBouquetName
  , const std::string& newChannelName
  )
{
  this->currentBouquetName = newBouquetName;
  this->currentChannelName = newChannelName;
}

int EpgPlus::Footer::getUsedHeight()
{
  return   fontBouquetChannelName->getHeight()
         + fontEventDescription->getHeight()
         + fontEventShortDescription->getHeight()
         + fontButtons->getHeight();
}

void EpgPlus::Footer::paintEventDetails
  ( const std::string& description
  , const std::string& shortDescription
  )
{
  int yPos = this->y;

  int height = this->fontBouquetChannelName->getHeight();

	// clear the region
	this->frameBuffer->paintBoxRel
		( this->x
		, yPos
		, this->width
		, height
		, this->color
		);

  yPos += height;

	// display new text
	this->fontBouquetChannelName->RenderString
		( this->x + 10
		, yPos
		, this->width - 20
    , this->currentBouquetName + " : " + this->currentChannelName
		, this->color
		, 0
		, false
		);

  height = this->fontEventDescription->getHeight();

	// clear the region
	this->frameBuffer->paintBoxRel
		( this->x
		, yPos
		, this->width
		, height
		, this->color
		);

  yPos += height;

	// display new text
  this->fontEventDescription->RenderString
		( this->x + 10
		, yPos
		, this->width - 20
    , description
		, this->color
		, 0
		, false
		);

  height = this->fontEventShortDescription->getHeight();

	// clear the region
	this->frameBuffer->paintBoxRel
		( this->x
		, yPos
		, this->width
		, height
		, this->color
		);

  yPos += height;

	// display new text
  this->fontEventShortDescription->RenderString
		( this->x + 10
		, yPos
		, this->width - 20
    , shortDescription
		, this->color
		, 0
		, false
		);
}

struct button_label buttonLabels[] =
{
	{ NEUTRINO_ICON_BUTTON_RED    , LOCALE_EPGPLUS_ACTIONS},
	{ NEUTRINO_ICON_BUTTON_GREEN  , LOCALE_EPGPLUS_PAGE_DOWN},
	{ NEUTRINO_ICON_BUTTON_YELLOW , LOCALE_EPGPLUS_PAGE_UP},
	{ NEUTRINO_ICON_BUTTON_BLUE   , LOCALE_EPGPLUS_OPTIONS},
	{ NEUTRINO_ICON_BUTTON_HELP_SMALL   , LOCALE_EPGPLUS_EVENT_INFO},
	{ NEUTRINO_ICON_BUTTON_DBOX	  , LOCALE_EPGPLUS_HIDE}
};

void EpgPlus::Footer::paintButtons(button_label* _buttonLabels, int numberOfButtons)
{
	int yPos = y + getUsedHeight() - fontButtons->getHeight();
	int buttonWidth =  40;

	frameBuffer->paintBoxRel(x, yPos, width, fontButtons->getHeight(), COL_INFOBAR_SHADOW_PLUS_1, RADIUS_MID, CORNER_BOTTOM);
	::paintButtons(frameBuffer, fontButtons, g_Locale, x + 5, yPos , buttonWidth, numberOfButtons, _buttonLabels, width - 25);
}

EpgPlus::EpgPlus()
{
	this->frameBuffer = CFrameBuffer::getInstance();

  this->currentViewMode = ViewMode_Scroll;
  this->currentSwapMode = SwapMode_ByPage;

	// this->usableScreenWidth  = 580;
	// this->usableScreenHeight = 480;
  this->usableScreenWidth  = w_max (g_settings.screen_EndX , 4);
  this->usableScreenHeight = h_max (g_settings.screen_EndY , 4);

  this->init();
}

EpgPlus::~EpgPlus()
{
  this->free();
}

void EpgPlus::createChannelEntries
  ( int selectedChannelEntryIndex
  )
{
	for ( TChannelEntries::iterator It = this->displayedChannelEntries.begin()
	    ; It != this->displayedChannelEntries.begin()
	    ; ++It
	    )
	    delete *It;

	this->displayedChannelEntries.clear();

	this->selectedChannelEntry = NULL;

	if (selectedChannelEntryIndex < this->channelList->getSize())
	{

    for (;;)
    {
      if (selectedChannelEntryIndex < this->channelListStartIndex)
      {
        this->channelListStartIndex -= this->maxNumberOfDisplayableEntries;
        if (this->channelListStartIndex < 0)
          this->channelListStartIndex = 0;
      }
      else if (selectedChannelEntryIndex >= this->channelListStartIndex + this->maxNumberOfDisplayableEntries)
      {
        this->channelListStartIndex += this->maxNumberOfDisplayableEntries;
      }
      else
        break;
    }

		#ifdef DEBUG_
			std::cout << "createChannelEntries" << std::endl;
			std::cout << " startTime " << getTimeString(this->startTime, "%H:%M") << " duration " << this->duration/60 << std::endl;
		#endif


		int yPosChannelEntry = this->channelsTableY;
		int yPosEventEntry   = this->eventsTableY;

		for ( int i = this->channelListStartIndex
		    ;    (i < this->channelListStartIndex + this->maxNumberOfDisplayableEntries)
		      && (i < this->channelList->getSize())
		    ;   ++i
		      , yPosChannelEntry += this->entryHeight
		      , yPosEventEntry   += this->entryHeight
		    )
		{
			#ifdef DEBUG_
				std::cout << " count "<< i << std::endl;
			#endif

			CChannelList::CChannel* channel = (*this->channelList)[i];

			ChannelEntry* channelEntry = new ChannelEntry
				( channel
				, i
				, this->frameBuffer
        , this->footer
        , this->bouquetList
				, this->channelsTableX + 2
				, yPosChannelEntry
				, this->channelsTableWidth
				);

			#ifdef DEBUG_
				std::cout << " channel name "<< channel->getName() << " "
					<< " channel_id "<< channel->channel_id << std::endl;
			#endif

			CChannelEventList channelEventList = g_Sectionsd->getEventsServiceKey(channel->channel_id);

			#ifdef DEBUG_
				std::cout << " channelEventList.size() "<< channelEventList.size() << std::endl;
			#endif

      int xPosEventEntry  = this->eventsTableX;
      int widthEventEntry = 0;
      time_t lastEndTime = this->startTime;

			CChannelEventList::const_iterator lastIt(channelEventList.end());
			for ( CChannelEventList::const_iterator It = channelEventList.begin()
			    ;    (It != channelEventList.end())
			      && (It->startTime < (this->startTime + this->duration))
			    ; ++It )
			{
				if ( (lastIt == channelEventList.end())
				   ||(lastIt->startTime != It->startTime)
				   )

				{
					#ifdef DEBUG_
						std::cout << " iterate: " << It->description << " startTime " << getTimeString(It->startTime, "%H:%M") << " duration " << (It->duration)/60 << std::endl;
					#endif

					int startTimeDiff = It->startTime - this->startTime;
					int endTimeDiff   = this->startTime + time_t(this->duration) - It->startTime - time_t(It->duration);


					if ( (startTimeDiff >= 0)
					   &&(endTimeDiff   >= 0)
					   )
					{ // channel event fits completely in the visible part of time line
						startTimeDiff = 0;
						endTimeDiff   = 0;
					}
					else
					if ( (startTimeDiff < 0)
					   &&(endTimeDiff   < 0)
					   )
					{ // channel event starts and ends outside visible part of the time line but covers complete visible part
					}
					else
					if ( (startTimeDiff < 0)
					   &&(endTimeDiff   < this->duration)
					   )
					{ // channel event starts before visible part of the time line but ends in the visible part
						endTimeDiff = 0;
					}
					else
					if ( (endTimeDiff   < 0)
					   &&(startTimeDiff < this->duration)
					   )
					{ // channel event ends after visible part of the time line but starts in the visible part
						startTimeDiff = 0;
					}
					else
					if (startTimeDiff > 0)
					{ // channel event starts and ends after visible part of the time line => break the loop
						break;
					}
					else
					{ // channel event starts and ends after visible part of the time line => ignore the channel event
						continue;
					}

					#ifdef DEBUG_
						std::cout << " push_back: " << std::endl;
					#endif

					if (lastEndTime < It->startTime)
					{// there is a gap between last end time and new start time => fill it with a new event entry

						CChannelEvent channelEvent;
						channelEvent.startTime = lastEndTime;
						channelEvent.duration  = It->startTime - channelEvent.startTime;

						ChannelEventEntry* channelEventEntry = new ChannelEventEntry
							( &channelEvent
							, this->frameBuffer
							, this->timeLine
							, this->footer
							, this->eventsTableX + ((channelEvent.startTime - this->startTime) * this->eventsTableWidth)/this->duration
							, yPosEventEntry
							, (channelEvent.duration * this->eventsTableWidth)/this->duration + 1
							);


						channelEntry->channelEventEntries.push_back(channelEventEntry);
          }

					// correct position
					xPosEventEntry  = this->eventsTableX + ((It->startTime - startTimeDiff - this->startTime) * this->eventsTableWidth)/this->duration;

					// correct width
					widthEventEntry = ((It->duration + startTimeDiff + endTimeDiff) * this->eventsTableWidth)/this->duration + 1;

					if (widthEventEntry < 0)
						widthEventEntry = 0;

					if (xPosEventEntry + widthEventEntry > this->eventsTableX + this->eventsTableWidth)
						widthEventEntry = this->eventsTableX + this->eventsTableWidth - xPosEventEntry;

					ChannelEventEntry* channelEventEntry = new ChannelEventEntry
						( &(*It)
						, this->frameBuffer
						, this->timeLine
						, this->footer
						, xPosEventEntry
						, yPosEventEntry
						, widthEventEntry
						);


					channelEntry->channelEventEntries.push_back(channelEventEntry);

					lastEndTime = It->startTime + It->duration;
				}

				lastIt = It;
			}

			if (lastEndTime < this->startTime + time_t(this->duration))
			{// there is a gap between last end time and end of the timeline => fill it with a new event entry

				CChannelEvent channelEvent;
				channelEvent.startTime = lastEndTime;
				channelEvent.duration  = this->startTime + this->duration - channelEvent.startTime;

 				ChannelEventEntry* channelEventEntry = new ChannelEventEntry
 					( &channelEvent
 					, this->frameBuffer
 					, this->timeLine
 					, this->footer
					, this->eventsTableX + ((channelEvent.startTime - this->startTime) * this->eventsTableWidth)/this->duration
 					, yPosEventEntry
					, (channelEvent.duration * this->eventsTableWidth)/this->duration + 1
 					);

 				channelEntry->channelEventEntries.push_back(channelEventEntry);
 			}

			this->displayedChannelEntries.push_back(channelEntry);
		}

	#ifdef DEBUG_
		std::cout << "leaving createChannelEntries1  8" << this->displayedChannelEntries.size() << " " << selectedChannelEntryIndex << " " << this->channelListStartIndex << std::endl;
	#endif
		this->selectedChannelEntry = this->displayedChannelEntries[selectedChannelEntryIndex - this->channelListStartIndex];

	#ifdef DEBUG_
		std::cout << "leaving createChannelEntries2" << std::endl;
	#endif
	}
	#ifdef DEBUG_
		std::cout << "leaving createChannelEntries3" << std::endl;
	#endif
}

void EpgPlus::init()
{
	for ( size_t i = 0
       ; i < NumberOfFontSettings
	    ; ++i
	    )
	{
		FontSetting* fontSetting = &this->settings.fontSettings[i];
		std::string FileName;
		FileName += std::string(FONTDIR);
		FileName += "/";
		FileName += fontSetting->name;

		std::string defaultStyle = g_fontRenderer->AddFont(FileName.c_str());
		std::string family       = g_fontRenderer->getFamily(FileName.c_str());
		Font* font               = g_fontRenderer->getFont(family.c_str(), fontSetting->name, fontSetting->size);

		if (font == NULL)
			font = g_fontRenderer->getFont(family.c_str(), defaultStyle.c_str(), fontSetting->size);

		this->fonts[fontSetting->settingID] = font;
	}

	for ( size_t i = 0
	    ; i < NumberOfColorSettings
	    ; ++i
	    )
	{
		ColorSetting* colorSetting = &this->settings.colorSettings[i];
		this->colors[colorSetting->settingID] = colorSetting->color;
	}

	for ( size_t i = 0
		  ; i < NumberOfSizeSettings
	    ; ++i
	    )
	{
		SizeSetting* sizeSetting = &this->settings.sizeSettings[i];
		this->sizes[sizeSetting->settingID] = sizeSetting->size;
	}

	Header::init();
	TimeLine::init();
	ChannelEntry::init();
	ChannelEventEntry::init();
	Footer::init();

	this->selectedChannelEntry = NULL;

	channelsTableWidth = this->sizes[EPGPlus_channelentry_width];
	sliderWidth        = this->sizes[EPGPlus_slider_width];

	horGap1Height = this->sizes[EPGPlus_horgap1_height];
	horGap2Height = this->sizes[EPGPlus_horgap2_height];
	verGap1Width  = this->sizes[EPGPlus_vergap1_width];
	verGap2Width  = this->sizes[EPGPlus_vergap2_width];

	sliderBackColor   = this->colors[EPGPlus_slider_backcolor];
	sliderKnobColor   = this->colors[EPGPlus_slider_knobcolor];

	horGap1Color = this->colors[EPGPlus_horgap1_color];
	horGap2Color = this->colors[EPGPlus_horgap2_color];
	verGap1Color = this->colors[EPGPlus_vergap1_color];
	verGap2Color = this->colors[EPGPlus_vergap2_color];

	int headerHeight   = Header::getUsedHeight();
	int timeLineHeight = TimeLine::getUsedHeight();
	this->entryHeight  = ChannelEntry::getUsedHeight();
	int footerHeight   = Footer::getUsedHeight();

	this->maxNumberOfDisplayableEntries = (this->usableScreenHeight - headerHeight - timeLineHeight - horGap1Height - horGap2Height - footerHeight)/this->entryHeight;

	this->usableScreenHeight = headerHeight + timeLineHeight + horGap1Height + this->maxNumberOfDisplayableEntries*this->entryHeight + horGap2Height + footerHeight; // recalc deltaY
	this->usableScreenX = (((g_settings.screen_EndX - g_settings.screen_StartX) - this->usableScreenWidth) / 2) + g_settings.screen_StartX;
	this->usableScreenY = (((g_settings.screen_EndY - g_settings.screen_StartY) - this->usableScreenHeight) / 2) + g_settings.screen_StartY;

	this->headerX     = this->usableScreenX;
	this->headerY     = this->usableScreenY;
	this->headerWidth = this->usableScreenWidth;

	this->timeLineX     = this->usableScreenX;
	this->timeLineY     = this->usableScreenY + headerHeight;
	this->timeLineWidth = this->usableScreenWidth;

	this->horGap1X     = this->usableScreenX;
	this->horGap1Y     = this->timeLineY + timeLineHeight;
	this->horGap1Width = this->usableScreenWidth;

	this->footerX     = usableScreenX;
	this->footerY     = this->usableScreenY + this->usableScreenHeight - footerHeight;
	this->footerWidth = this->usableScreenWidth;

	this->horGap2X     = this->usableScreenX;
	this->horGap2Y     = this->footerY - horGap2Height;
	this->horGap2Width = this->usableScreenWidth;

	this->channelsTableX	        = this->usableScreenX;
	this->channelsTableY	        = this->timeLineY + timeLineHeight + horGap1Height;
	this->channelsTableHeight     = this->maxNumberOfDisplayableEntries*entryHeight;

	this->verGap1X      = this->channelsTableX + channelsTableWidth;
	this->verGap1Y      = this->channelsTableY;
	this->verGap1Height = this->channelsTableHeight;

	this->eventsTableX      = this->channelsTableX + channelsTableWidth + verGap1Width;
	this->eventsTableY      = this->channelsTableY;
	this->eventsTableWidth  = this->usableScreenWidth - this->channelsTableWidth - this->sliderWidth - verGap1Width - verGap2Width;
	this->eventsTableHeight = this->channelsTableHeight;

	this->sliderX = this->usableScreenX + this->usableScreenWidth - this->sliderWidth;
	this->sliderY = this->eventsTableY;
	this->sliderHeight = this->channelsTableHeight;

	this->verGap2X      = this->sliderX - verGap2Width;
	this->verGap2Y      = this->channelsTableY;
	this->verGap2Height = this->channelsTableHeight;

	this->channelListStartIndex = 0;
	this->startTime = 0;
	this->duration = settings.durationSetting; // 2h

	this->refreshAll = false;
	this->currentViewMode = ViewMode_Scroll;
	this->currentSwapMode = SwapMode_ByPage;

	this->header = new Header
		( this->frameBuffer
		, this->headerX
		, this->headerY
		, this->headerWidth
		);

	this->timeLine = new TimeLine
		( this->frameBuffer
		, this->timeLineX
		, this->timeLineY
		, this->timeLineWidth
		, this->eventsTableX
		, this->eventsTableWidth
		);

	this->footer = new Footer
		( this->frameBuffer
		, this->footerX
		, this->footerY
		, this->footerWidth
		);
}

void EpgPlus::free()
{
	delete this->header;
	delete this->timeLine;
	delete this->footer;

  for ( Fonts::iterator It = this->fonts.begin()
      ; It != this->fonts.end()
      ; ++It
      )
  {
    delete It->second;
  }
}

int EpgPlus::exec(CChannelList* _channelList, int selectedChannelIndex, CBouquetList* _bouquetList)
{
	this->channelList = _channelList;
	this->channelListStartIndex = int(selectedChannelIndex/maxNumberOfDisplayableEntries)*maxNumberOfDisplayableEntries;
	this->bouquetList = _bouquetList;

	int res = menu_return::RETURN_REPAINT;

	do
	{
		this->refreshAll = false;
		this->refreshFooterButtons = false;
		this->is_visible = true;

		time_t currentTime = time(NULL);
		tm tmStartTime = *localtime(&currentTime);
		tmStartTime.tm_sec = 0;
		tmStartTime.tm_min = int(tmStartTime.tm_min/15) * 15;

		this->startTime = mktime(&tmStartTime);
		this->selectedTime = this->startTime;
		this->firstStartTime = this->startTime;

		if (this->selectedChannelEntry != NULL)
		{
			selectedChannelIndex = this->selectedChannelEntry->index;
		}
#ifdef DEBUG_
		std::cout << "exec " << selectedChannelIndex << " " << (*this->channelList)[selectedChannelIndex]->channel_id << " " << (*this->channelList)[selectedChannelIndex]->getName() << std::endl;
#endif
		neutrino_msg_t      msg;
		neutrino_msg_data_t data;

		this->createChannelEntries(selectedChannelIndex);
#ifdef DEBUG_
		std::cout << "paint" << std::endl;
#endif
		this->header->paint();
#ifdef DEBUG_
		std::cout << "paintButtons1" << std::endl;
#endif
		this->footer->paintButtons(buttonLabels, sizeof(buttonLabels)/sizeof(button_label));
#ifdef DEBUG_
		std::cout << "paintButtons2" << std::endl;
#endif
		this->paint();

		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);
		bool loop=true;
		while (loop)
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);
			neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

			if (msg <= CRCInput::RC_MaxRC)
				timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_CHANLIST]);

			if (msg == g_settings.key_channelList_pagedown || msg == CRCInput::RC_yellow)
			{
				if (this->channelList->getSize() > 0)
				{
					switch (this->currentSwapMode)
					{
						case SwapMode_ByPage:
						{
							int selectedChannelEntryIndex = this->selectedChannelEntry->index;
							selectedChannelEntryIndex    += this->maxNumberOfDisplayableEntries;

							if (selectedChannelEntryIndex > this->channelList->getSize() - 1)
								selectedChannelEntryIndex = 0;

							this->createChannelEntries(selectedChannelEntryIndex);
							this->paint();
							break;
						}

						case SwapMode_ByBouquet:
						{
							unsigned int currentBouquetNumber = bouquetList->getActiveBouquetNumber();
#ifdef DEBUG_
							std::cout << "ViewMode_Bouquets " << currentBouquetNumber << std::endl;
#endif
							++currentBouquetNumber;

							if (currentBouquetNumber == bouquetList->Bouquets.size())
								currentBouquetNumber = 0;

							CBouquet* bouquet = bouquetList->Bouquets[currentBouquetNumber];
#ifdef DEBUG_
							std::cout << "bouquet->unique_key " << bouquet->unique_key << " " << bouquet->channelList->getName() << std::endl;
#endif
							if (bouquet->channelList->getSize() > 0)
							{
								// select first channel of bouquet
#ifdef DEBUG_
								std::cout << "(*bouquet->channelList)[0]->number " << (*bouquet->channelList)[0]->number << std::endl;
#endif
								bouquetList->activateBouquet(currentBouquetNumber);
								this->channelListStartIndex = (*bouquet->channelList)[0]->number - 1;
								this->createChannelEntries(this->channelListStartIndex);
								this->paint();
							}
							break;
						}
					}
				}
			}
			else if (msg == g_settings.key_channelList_pageup || msg == CRCInput::RC_green)
			{
				if (this->channelList->getSize() > 0 )
				{
					switch (this->currentSwapMode)
					{
						case SwapMode_ByPage:
						{
							int selectedChannelEntryIndex = this->selectedChannelEntry->index;
							selectedChannelEntryIndex    -= this->maxNumberOfDisplayableEntries;

							if (selectedChannelEntryIndex < 0)
								selectedChannelEntryIndex = this->channelList->getSize() - 1;

							this->createChannelEntries(selectedChannelEntryIndex);
							this->paint();
							break;
						}

						case SwapMode_ByBouquet:
						{
							unsigned int currentBouquetNumber = bouquetList->getActiveBouquetNumber();
#ifdef DEBUG_
							std::cout << "ViewMode_Bouquets " << currentBouquetNumber << std::endl;
#endif
							--currentBouquetNumber;

							if (currentBouquetNumber == unsigned(-1))
								currentBouquetNumber = bouquetList->Bouquets.size() - 1;

							CBouquet* bouquet = bouquetList->Bouquets[currentBouquetNumber];
#ifdef DEBUG_
							std::cout << "bouquet->unique_key " << bouquet->unique_key << " " << bouquet->channelList->getName() << std::endl;
#endif
							if (bouquet->channelList->getSize() > 0)
							{
								// select first channel of bouquet
#ifdef DEBUG_
								std::cout << "(*bouquet->channelList)[0]->number " << (*bouquet->channelList)[0]->number << std::endl;
#endif
								bouquetList->activateBouquet(currentBouquetNumber);
								this->channelListStartIndex = (*bouquet->channelList)[0]->number - 1;
								this->createChannelEntries(this->channelListStartIndex);
								this->paint();
							}
							break;
						}
					}
				}
			}
			else if (msg_repeatok == g_settings.key_channelList_pageup ||
				 msg_repeatok == g_settings.key_channelList_pagedown ||
				 msg_repeatok == CRCInput::RC_green ||
				 msg_repeatok == CRCInput::RC_yellow)
			{
				/* ignore pageup/down if repeated.
				   on many machines, pageup/down is mapped to volume up/down keys and when we do not
				   ignore the repeate event here, it the repeat events will invoke volume setting
				   which is not wanted in this case.
				   Instead of ignoring, we could also allow repeated pgup/pgdown, but since page
				   flipping is rather slow, we probably don't want to do that. */
			}
			else if (msg == (neutrino_msg_t) CRCInput::RC_red)
			{
				fb_pixel_t savedScreen[this->usableScreenWidth * this->usableScreenHeight * sizeof(fb_pixel_t)];
				this->frameBuffer->SaveScreen(this->usableScreenX, this->usableScreenY,
							      this->usableScreenWidth, this->usableScreenHeight,
							      savedScreen);

				CMenuWidget menuWidgetActions(LOCALE_EPGPLUS_ACTIONS, "features.raw", 400);
				menuWidgetActions.addItem(new CMenuForwarder(LOCALE_EPGPLUS_RECORD     , true, NULL, new MenuTargetAddRecordTimer(this), NULL, CRCInput::RC_red   , NEUTRINO_ICON_BUTTON_RED   ), false);
				menuWidgetActions.addItem(new CMenuForwarder(LOCALE_EPGPLUS_REFRESH_EPG, true, NULL, new MenuTargetRefreshEpg    (this), NULL, CRCInput::RC_green , NEUTRINO_ICON_BUTTON_GREEN ), false);
				menuWidgetActions.addItem(new CMenuForwarder(LOCALE_EPGPLUS_REMIND     , true, NULL, new MenuTargetAddReminder   (this), NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW), false);

				menuWidgetActions.exec(NULL, "");

				this->frameBuffer->RestoreScreen(this->usableScreenX, this->usableScreenY,
								 this->usableScreenWidth, this->usableScreenHeight,
								 savedScreen);
			}
			else if (msg == (neutrino_msg_t) CRCInput::RC_blue)
			{
				fb_pixel_t savedScreen[this->usableScreenWidth * this->usableScreenHeight * sizeof(fb_pixel_t)];
				this->frameBuffer->SaveScreen(this->usableScreenX, this->usableScreenY,
							      this->usableScreenWidth, this->usableScreenHeight,
							      savedScreen);

				CMenuWidget menuWidgetOptions(LOCALE_EPGPLUS_OPTIONS, "features.raw", 500);
				menuWidgetOptions.addItem(new MenuOptionChooserSwitchSwapMode(this));
				menuWidgetOptions.addItem(new MenuOptionChooserSwitchViewMode(this));

				int result = menuWidgetOptions.exec(NULL, "");
				if (result == menu_return::RETURN_REPAINT)
				{
					this->frameBuffer->RestoreScreen(this->usableScreenX, this->usableScreenY,
									 this->usableScreenWidth, this->usableScreenHeight,
									 savedScreen);
				}
				else if (result == menu_return::RETURN_EXIT_ALL)
				{
					this->refreshAll = true;
				}
			}
			else if (CRCInput::isNumeric(msg))
			{ //numeric zap
				this->hide();
				this->channelList->numericZap(msg);

				int selectedChannelEntryIndex = this->channelList->getSelectedChannelIndex();
				if (selectedChannelEntryIndex < this->channelList->getSize())
				{
					this->hide();
					this->createChannelEntries(selectedChannelEntryIndex);
					this->header->paint();
					this->footer->paintButtons(buttonLabels, sizeof(buttonLabels)/sizeof(button_label));
					this->paint();
				}
			}
			else if (msg_repeatok == CRCInput::RC_up)
			{
				if (this->channelList->getSize() > 0) {
#ifdef DEBUG_
					std::cout << "RC_up" << std::endl;
#endif
					int selectedChannelEntryIndex     = this->selectedChannelEntry->index;
					int prevSelectedChannelEntryIndex = selectedChannelEntryIndex;

					--selectedChannelEntryIndex;
					if (selectedChannelEntryIndex < 0)
					{
#ifdef DEBUG_
						std::cout << "this->selectedChannelEntry->index < 0" << std::endl;
#endif
						selectedChannelEntryIndex = this->channelList->getSize() - 1;
					}

					int oldChannelListStartIndex = this->channelListStartIndex;
					this->channelListStartIndex = (selectedChannelEntryIndex / this->maxNumberOfDisplayableEntries) * this->maxNumberOfDisplayableEntries;

					if (oldChannelListStartIndex != this->channelListStartIndex)
					{
#ifdef DEBUG_
						std::cout << "oldChannelListStartIndex != this->channelListStartIndex" << std::endl;
#endif
						this->createChannelEntries(selectedChannelEntryIndex);
						this->paint();
						g_RCInput->clearRCMsg();
					}
					else
					{
						this->selectedChannelEntry = this->displayedChannelEntries[selectedChannelEntryIndex - this->channelListStartIndex];
						this->paintChannelEntry(prevSelectedChannelEntryIndex - this->channelListStartIndex);
						this->paintChannelEntry(selectedChannelEntryIndex     - this->channelListStartIndex);
					}
				}
			}
			else if (msg_repeatok == CRCInput::RC_down)
			{
				if (this->channelList->getSize() > 0 ) {
					int selectedChannelEntryIndex     = this->selectedChannelEntry->index;
					int prevSelectedChannelEntryIndex = this->selectedChannelEntry->index;

					selectedChannelEntryIndex = (selectedChannelEntryIndex + 1) % this->channelList->getSize();

					int oldChannelListStartIndex = this->channelListStartIndex;
					this->channelListStartIndex = (selectedChannelEntryIndex / this->maxNumberOfDisplayableEntries) * this->maxNumberOfDisplayableEntries;

					if (oldChannelListStartIndex != this->channelListStartIndex)
					{
						this->createChannelEntries(selectedChannelEntryIndex);
						this->paint();
						g_RCInput->clearRCMsg();
					}
					else
					{
						this->selectedChannelEntry = this->displayedChannelEntries[selectedChannelEntryIndex - this->channelListStartIndex];
						this->paintChannelEntry(prevSelectedChannelEntryIndex - this->channelListStartIndex);
						this->paintChannelEntry(this->selectedChannelEntry->index - this->channelListStartIndex);
					}
				}
			}
			else if (msg == CRCInput::RC_timeout || msg == g_settings.key_channelList_cancel)
			{
				loop=false;
			}

			else if (msg_repeatok == CRCInput::RC_left)
			{
				switch (this->currentViewMode)
				{
					case ViewMode_Stretch:
						if (this->duration - 30*60 > 30*60)
						{
							this->duration -= 30*60;
							this->hide();
							this->refreshAll = true;
						}
						break;
					case ViewMode_Scroll:
					{
#ifdef DEBUG_
						std::cout << "RC_left " << std::endl;
#endif
						TCChannelEventEntries::const_iterator It = this->getSelectedEvent();

						if ((It != this->selectedChannelEntry->channelEventEntries.begin()) &&
						    (It != this->selectedChannelEntry->channelEventEntries.end()))
						{
#ifdef DEBUG_
							std::cout << "--It" << std::endl;
#endif
							--It;
							this->selectedTime = (*It)->channelEvent.startTime + (*It)->channelEvent.duration/2;
							if (this->selectedTime < this->startTime)
								this->selectedTime = this->startTime;
#ifdef DEBUG_
							std::cout << "repaint channel entry" << std::endl;
#endif
							this->selectedChannelEntry->paint(true, this->selectedTime);
						}
						else
						{
							if (this->startTime != this->firstStartTime)
							{
#ifdef DEBUG_
								std::cout << "this->startTime != this->firstStartTime" << std::endl;
#endif
								if (this->startTime - this->duration > this->firstStartTime)
								{
#ifdef DEBUG_
									std::cout << "this->startTime - this->duration > this->firstStartTime" << std::endl;
#endif
									this->startTime -= this->duration;
								}
								else
								{
									this->startTime = this->firstStartTime;
								}

								this->selectedTime = this->startTime + this->duration - 1; // select last event
								this->createChannelEntries(this->selectedChannelEntry->index);

								this->paint();
								g_RCInput->clearRCMsg();
							}
						}
					}
					break;
				}
			}
			else if (msg_repeatok == CRCInput::RC_right)
			{
				switch (this->currentViewMode)
				{
					case ViewMode_Stretch:
						if (this->duration + 30*60 < 4*60*60)
						{
							this->duration += 60*60;
							this->hide();
							this->refreshAll = true;
						}
						break;
					case ViewMode_Scroll:
					{
#ifdef DEBUG_
						std::cout << "RC_right " << std::endl;
#endif
						TCChannelEventEntries::const_iterator It = this->getSelectedEvent();

						if ((It != this->selectedChannelEntry->channelEventEntries.end() - 1) &&
						    (It != this->selectedChannelEntry->channelEventEntries.end()))
						{
#ifdef DEBUG_
							std::cout << "++It" << std::endl;
#endif
							++It;

							this->selectedTime = (*It)->channelEvent.startTime + (*It)->channelEvent.duration/2;

							if (this->selectedTime > this->startTime + time_t(this->duration))
								this->selectedTime = this->startTime + this->duration;
#ifdef DEBUG_
							std::cout << "repaint channel entry" << std::endl;
#endif
							this->selectedChannelEntry->paint(true, this->selectedTime);
						}
						else
						{
#ifdef DEBUG_
							std::cout << "this->startTime += this->duration" << std::endl;
#endif
							this->startTime += this->duration;
							this->createChannelEntries(this->selectedChannelEntry->index);

							this->selectedTime = this->startTime;
							this->createChannelEntries(this->selectedChannelEntry->index);

							this->paint();
							g_RCInput->clearRCMsg();
						}
					}
					break;
				}
			}
			else if (msg==CRCInput::RC_ok)
			{
#ifdef DEBUG_
				std::cout << "zapTo " << this->selectedChannelEntry->index << std::endl;
#endif
				this->channelList->zapTo(this->selectedChannelEntry->index);
			}
			else if (msg==CRCInput::RC_help)
			{
				TCChannelEventEntries::const_iterator It = this->getSelectedEvent();

				if (It != this->selectedChannelEntry->channelEventEntries.end())
				{
					if ((*It)->channelEvent.eventID != 0)
					{
						this->hide();

						time_t _startTime = (*It)->channelEvent.startTime;
						res = g_EpgData->show(this->selectedChannelEntry->channel->channel_id,
								      (*It)->channelEvent.eventID, &_startTime);

						if (res == menu_return::RETURN_EXIT_ALL)
						{
							loop = false;
						}
						else
						{
							g_RCInput->getMsg(&msg, &data, 0);

							if (msg != CRCInput::RC_red && msg != CRCInput::RC_timeout)
							{
								// RC_red schlucken
								g_RCInput->postMsg(msg, data);
							}

							this->header->paint();
							this->footer->paintButtons(buttonLabels, sizeof(buttonLabels)/sizeof(button_label));
							this->paint();
						}
					}
				}
			}
			else if (msg==CRCInput::RC_setup)
			{
				while(loop)
				{
					if(msg == CRCInput::RC_setup)
					{
						if(is_visible)
						{
							std::string EPG_Plus;

							EPG_Plus = g_Locale->getText(LOCALE_EPGPLUS_SHOW);
							EPG_Plus.insert(0, " ");
						
							int epgplus_len = this->header->font->getRenderWidth(EPG_Plus, true); // UTF-8
							int theight     = this->header->font->getHeight();
							int dbox_icon_width = frameBuffer->getIconWidth(NEUTRINO_ICON_BUTTON_DBOX);

							is_visible = false;
							this->hide();

							frameBuffer->paintBoxRel(this->usableScreenX+ this->usableScreenWidth- epgplus_len- dbox_icon_width- 2- 2, this->usableScreenY, epgplus_len+ dbox_icon_width+ 2+ 2, theight+0, COL_MENUHEAD_PLUS_0, RADIUS_SMALL);
							frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_DBOX, this->usableScreenX+ this->usableScreenWidth- epgplus_len- dbox_icon_width- 2, this->usableScreenY);
							this->header->font->RenderString(this->usableScreenX+ this->usableScreenWidth- epgplus_len, this->usableScreenY+ theight+0, epgplus_len, EPG_Plus, this->header->color, 0, true); // UTF-8
						}
						else
						{
							is_visible = true;
							this->header->paint();
							this->footer->paintButtons(buttonLabels, sizeof(buttonLabels)/sizeof(button_label));
							this->paint();
							break;
						}
					}
					else if (msg == g_settings.key_channelList_cancel)
					{
						loop = false;
					}

					g_RCInput->getMsg(&msg, &data, 100);
				}
			}
			else
			{
				if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
				{
					loop = false;
					res = menu_return::RETURN_EXIT_ALL;
				}
			}

			if (this->refreshAll)
				loop = false;
			else if (this->refreshFooterButtons)
				this->footer->paintButtons(buttonLabels, sizeof(buttonLabels)/sizeof(button_label));
		}

		this->hide();

		for (TChannelEntries::iterator It = this->displayedChannelEntries.begin();
		     It != this->displayedChannelEntries.begin(); ++It)
			delete *It;
	}
	while (this->refreshAll);

	return res;
}

EpgPlus::TCChannelEventEntries::const_iterator EpgPlus::getSelectedEvent() const
{
	for ( TCChannelEventEntries::const_iterator It = this->selectedChannelEntry->channelEventEntries.begin()
	    ; It != this->selectedChannelEntry->channelEventEntries.end()
	    ; ++It
	    )
	{
		if ((*It)->isSelected(this->selectedTime))
		{
			return It;
		}

	}

	return this->selectedChannelEntry->channelEventEntries.end();
}

/*
void EpgPlus::loadSettings()
{
  CConfigFile configFile('\t');
  if (configFile.loadConfig(CONFIGDIR "/epgplus.conf"))
    EpgPlus::loadSettings(configFile);
}

void EpgPlus::saveSettings()
{
  CConfigFile configFile('\t');

  EpgPlus::saveSettings(configFile);

  if (configFile.getModifiedFlag())
    configFile.saveConfig(CONFIGDIR "/epgplus.conf");
}

void EpgPlus::loadSettings
  ( CConfigFile& configFile
  )
{
  for ( FontSettings::iterator It = settings.fontSettings.begin()
      ; It != settings.fontSettings.end()
      ; ++It
      )
  {
    if (It->second.isConfigurable)
    {
		It->second.name  = configFile.getString(std::string(setting2StringConverter[It->first]) + ".Name" , It->second.name );
      It->second.style = configFile.getString(std::string(setting2StringConverter[It->first]) + ".Style", It->second.style);
      It->second.size  = configFile.getInt32 (std::string(setting2StringConverter[It->first]) + ".Size" , It->second.size );
    }
  }

  for ( ColorSettings::iterator It = settings.colorSettings.begin()
      ; It != settings.colorSettings.end()
      ; ++It
      )
  {
    if (It->second.isConfigurable)
    {
      It->second.color  = configFile.getInt32(setting2StringConverter[It->first], It->second.color);
    }
  }

  for ( SizeSettings::iterator It = settings.sizeSettings.begin()
      ; It != settings.sizeSettings.end()
      ; ++It
      )
  {
    if (It->second.isConfigurable)
    {
      It->second.size  = configFile.getInt32(setting2StringConverter[It->first], It->second.size);
    }
  }

  settings.durationSetting = configFile.getInt32("EPGPlus.Duration", settings.durationSetting);
}

void EpgPlus::saveSettings
  ( CConfigFile& configFile
  )
{
  for ( FontSettings::const_iterator It = settings.fontSettings.begin()
      ; It != settings.fontSettings.end()
      ; ++It
      )
  {
    if (It->second.isConfigurable)
    {
		configFile.setString(std::string(setting2StringConverter[It->first]) + ".Name" , It->second.name );
      configFile.setString(std::string(setting2StringConverter[It->first]) + ".Style", It->second.style);
      configFile.setInt32 (std::string(setting2StringConverter[It->first]) + ".Size" , It->second.size );
    }
  }

  for ( ColorSettings::const_iterator It = settings.colorSettings.begin()
      ; It != settings.colorSettings.end()
      ; ++It
      )
  {
    if (It->second.isConfigurable)
    {
      configFile.setInt32(setting2StringConverter[It->first], It->second.color);
    }
  }

  for ( SizeSettings::const_iterator It = settings.sizeSettings.begin()
      ; It != settings.sizeSettings.end()
      ; ++It
      )
  {
    if (It->second.isConfigurable)
    {
      configFile.setInt32(setting2StringConverter[It->first], It->second.size);
    }
  }

  configFile.setInt32("EPGPlus.Duration", settings.durationSetting);
}
*/
void EpgPlus::hide()
{
	this->frameBuffer->paintBackgroundBoxRel
	( this->usableScreenX
	, this->usableScreenY
	, this->usableScreenWidth
	, this->usableScreenHeight
	);
}

void EpgPlus::paintChannelEntry
  ( int position
  )
{
	#ifdef DEBUG_
		std::cout << "paint channel entry " << position << std::endl;
	#endif

	ChannelEntry* channelEntry = this->displayedChannelEntries[position];

	bool currentChannelIsSelected = false;
	if (this->channelListStartIndex + position == this->selectedChannelEntry->index)
	{

		#ifdef DEBUG_
			std::cout << " currentChannelIsSelected = true" << std::endl;
		#endif

		currentChannelIsSelected = true;

	}

	channelEntry->paint
		( currentChannelIsSelected
		, this->selectedTime
		);

}

std::string EpgPlus::getTimeString
  ( const time_t& time
  , const std::string& format
  )
{
	char tmpstr[256];
	struct tm *tmStartTime = localtime(&time);


	strftime(tmpstr, sizeof(tmpstr), format.c_str(), tmStartTime );
	return tmpstr;
}

void EpgPlus::paint()
{
	// clear
	this->frameBuffer->paintBackgroundBoxRel
		( this->channelsTableX
		, this->channelsTableY
		, this->usableScreenWidth
		, this->channelsTableHeight
		);

  // paint the gaps
	this->frameBuffer->paintBoxRel
		( this->horGap1X
		, this->horGap1Y
		, this->horGap1Width
		, horGap1Height
		, horGap1Color
		);

	this->frameBuffer->paintBoxRel
		( this->horGap2X
		, this->horGap2Y
		, this->horGap2Width
		, horGap2Height
		, horGap2Color
		);

  this->frameBuffer->paintBoxRel
		( this->verGap1X
		, this->verGap1Y
		, verGap1Width
		, this->verGap1Height
		, verGap1Color
		);

  this->frameBuffer->paintBoxRel
		( this->verGap2X
		, this->verGap2Y
		, verGap2Width
		, this->verGap2Height
		, verGap2Color
		);

	// paint the time line
	timeLine->paint
		( this->startTime
		, this->duration
		);

	// paint the channel entries
	for ( int i = 0
	    ; i < (int)this->displayedChannelEntries.size()
	    ; ++i
	    )
	{
		this->paintChannelEntry(i);
	}

	// paint the time line grid
	this->timeLine->paintGrid();

	// paint slider
	this->frameBuffer->paintBoxRel
		( this->sliderX
		, this->sliderY
		, this->sliderWidth
		, this->sliderHeight
		, sliderBackColor
		);

	#ifdef DEBUG_
		std::cout << "paint5" << std::endl;
	#endif

	int tmp = ((this->channelList->getSize() - 1)/this->maxNumberOfDisplayableEntries) + 1;
	int sliderKnobPosition = this->selectedChannelEntry == NULL ? 0 : (this->selectedChannelEntry->index / this->maxNumberOfDisplayableEntries);

	this->frameBuffer->paintBoxRel
		( this->sliderX + 2
		, this->sliderY + sliderKnobPosition*(sliderHeight-4)/tmp
		, this->sliderWidth - 4
		, (sliderHeight - 4)/tmp
		, sliderKnobColor
		);
}






//
//  -- EPG+ Menue Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-05 rasc)
//

int CEPGplusHandler::exec(CMenuTarget* parent, const std::string &)
{
	int           res = menu_return::RETURN_EXIT_ALL;
	EpgPlus       *e;
	CChannelList  *channelList;


	if (parent) {
		parent->hide();
	}

	e = new EpgPlus;

	channelList = CNeutrinoApp::getInstance()->channelList;
	e->exec(channelList, channelList->getSelectedChannelIndex(), bouquetList);
	delete e;

	return res;
}

EpgPlus::MenuTargetAddReminder::MenuTargetAddReminder(EpgPlus* _epgPlus)
{
	epgPlus = _epgPlus;
}

int EpgPlus::MenuTargetAddReminder::exec(CMenuTarget*, const std::string&)
{
	#ifdef DEBUG_
		std::cout << "add reminder" << std::endl;
	#endif
	TCChannelEventEntries::const_iterator It = this->epgPlus->getSelectedEvent();

	if ( (It != this->epgPlus->selectedChannelEntry->channelEventEntries.end())
	   &&(!(*It)->channelEvent.description.empty())
	   )
	{
		CTimerdClient timerdclient;
		if (timerdclient.isTimerdAvailable())
		{
			timerdclient.addZaptoTimerEvent
				( this->epgPlus->selectedChannelEntry->channel->channel_id
				, (*It)->channelEvent.startTime
				, (*It)->channelEvent.startTime - ANNOUNCETIME
				, 0
				, (*It)->channelEvent.eventID
				, (*It)->channelEvent.startTime
				, 0
				);

			ShowLocalizedMessage(LOCALE_TIMER_EVENTTIMED_TITLE, LOCALE_TIMER_EVENTTIMED_MSG, CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw");
		}
		else
			printf("timerd not available\n");
	}
	return menu_return::RETURN_EXIT_ALL;
}

EpgPlus::MenuTargetAddRecordTimer::MenuTargetAddRecordTimer(EpgPlus* _epgPlus)
{
	epgPlus = _epgPlus;
}

int EpgPlus::MenuTargetAddRecordTimer::exec(CMenuTarget*, const std::string&)
{
	#ifdef DEBUG_
		std::cout << "add record timer 1" << std::endl;
	#endif
	TCChannelEventEntries::const_iterator It = this->epgPlus->getSelectedEvent();

	if ( (It != this->epgPlus->selectedChannelEntry->channelEventEntries.end())
	   &&(!(*It)->channelEvent.description.empty())
	)
	{
		#ifdef DEBUG_
			std::cout << "add record timer 2" << std::endl;
		#endif
		CTimerdClient timerdclient;
		if (timerdclient.isTimerdAvailable())
		{
#ifdef DEBUG_
			std::cout << "add record timer 3" << std::endl;
#endif
			std::string recDir = g_settings.recording_dir[0];
			if (g_settings.recording_choose_direct_rec_dir)
			{
				CRecDirChooser recDirs(LOCALE_TIMERLIST_RECORDING_DIR,NEUTRINO_ICON_SETTINGS,NULL,&recDir);
				epgPlus->hide();
				recDirs.exec(NULL,"");
				epgPlus->paint();
				recDir = recDirs.get_selected_dir();
			}
			if ((recDir != "") || (RECORDING_FILE != g_settings.recording_type))
			{
				if (timerdclient.addRecordTimerEvent
				    ( this->epgPlus->selectedChannelEntry->channel->channel_id
				      , (*It)->channelEvent.startTime
				      , (*It)->channelEvent.startTime + (*It)->channelEvent.duration
				      , (*It)->channelEvent.eventID
				      , (*It)->channelEvent.startTime
				      , (*It)->channelEvent.startTime - (ANNOUNCETIME + 120)
				      , TIMERD_APIDS_CONF
				      , true
				      , recDir
				      , false
				      ) == -1)
				{
					if(askUserOnTimerConflict((*It)->channelEvent.startTime - (ANNOUNCETIME + 120),
								  (*It)->channelEvent.startTime + (*It)->channelEvent.duration))
					{
						timerdclient.addRecordTimerEvent(this->epgPlus->selectedChannelEntry->channel->channel_id
										 , (*It)->channelEvent.startTime
										 , (*It)->channelEvent.startTime + (*It)->channelEvent.duration
										 , (*It)->channelEvent.eventID
										 , (*It)->channelEvent.startTime
										 , (*It)->channelEvent.startTime - (ANNOUNCETIME + 120)
										 , TIMERD_APIDS_CONF
										 , true
										 , recDir
										 , true);
						ShowLocalizedMessage(LOCALE_TIMER_EVENTRECORD_TITLE, LOCALE_TIMER_EVENTRECORD_MSG, CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw");
					}
				} else {
					ShowLocalizedMessage(LOCALE_TIMER_EVENTRECORD_TITLE, LOCALE_TIMER_EVENTRECORD_MSG, CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw");
				}
			}

		}
		else
			printf("timerd not available\n");
	}

	return menu_return::RETURN_EXIT_ALL;
}

EpgPlus::MenuTargetRefreshEpg::MenuTargetRefreshEpg(EpgPlus* _epgPlus)
{
	epgPlus = _epgPlus;
}


int EpgPlus::MenuTargetRefreshEpg::exec(CMenuTarget*, const std::string&)
{
#ifdef DEBUG_
	std::cout << "refresh mode" << std::endl;
#endif

	this->epgPlus->refreshAll = true;

	return menu_return::RETURN_EXIT_ALL;
}

struct CMenuOptionChooser::keyval menuOptionChooserSwitchSwapModes[] =
{
  {EpgPlus::SwapMode_ByPage   , LOCALE_EPGPLUS_BYPAGE_MODE},
  {EpgPlus::SwapMode_ByBouquet, LOCALE_EPGPLUS_BYBOUQUET_MODE}
};

EpgPlus::MenuOptionChooserSwitchSwapMode::MenuOptionChooserSwitchSwapMode(EpgPlus* _epgPlus)
	: CMenuOptionChooser(LOCALE_EPGPLUS_SWAP_MODE, (int*)(int)&_epgPlus->currentSwapMode,
			     menuOptionChooserSwitchSwapModes,
			     sizeof(menuOptionChooserSwitchSwapModes)/sizeof(CMenuOptionChooser::keyval),
			     true, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW)
{
	epgPlus = _epgPlus;
	oldSwapMode = _epgPlus->currentSwapMode;
	oldTimingMenuSettings = g_settings.timing[SNeutrinoSettings::TIMING_MENU];
}

EpgPlus::MenuOptionChooserSwitchSwapMode::~MenuOptionChooserSwitchSwapMode()
{
  g_settings.timing[SNeutrinoSettings::TIMING_MENU] = this->oldTimingMenuSettings;


  if (this->epgPlus->currentSwapMode != this->oldSwapMode)
  {
	  switch (this->epgPlus->currentSwapMode)
    {
      case SwapMode_ByPage:
        buttonLabels[1].locale = LOCALE_EPGPLUS_PAGE_DOWN;
        buttonLabels[2].locale = LOCALE_EPGPLUS_PAGE_UP;
        break;
      case SwapMode_ByBouquet:
        buttonLabels[1].locale = LOCALE_EPGPLUS_PREV_BOUQUET;
        buttonLabels[2].locale = LOCALE_EPGPLUS_NEXT_BOUQUET;
        break;
    }

    this->epgPlus->refreshAll = true;
  }
}

int EpgPlus::MenuOptionChooserSwitchSwapMode::exec
  ( CMenuTarget* parent
  )
{
  // change time out settings temporary
  g_settings.timing[SNeutrinoSettings::TIMING_MENU] = 1;

  CMenuOptionChooser::exec(parent);

  return menu_return::RETURN_REPAINT;
}

struct CMenuOptionChooser::keyval menuOptionChooserSwitchViewModes[] =
{
  {EpgPlus::ViewMode_Scroll , LOCALE_EPGPLUS_STRETCH_MODE },
  {EpgPlus::ViewMode_Stretch, LOCALE_EPGPLUS_SCROLL_MODE}
};

EpgPlus::MenuOptionChooserSwitchViewMode::MenuOptionChooserSwitchViewMode(EpgPlus* epgPlus)
	: CMenuOptionChooser(LOCALE_EPGPLUS_VIEW_MODE, (int*)(int)&epgPlus->currentViewMode,
			     menuOptionChooserSwitchViewModes,
			     sizeof(menuOptionChooserSwitchViewModes)/sizeof(CMenuOptionChooser::keyval),
			     true, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE)
{
	oldTimingMenuSettings = g_settings.timing[SNeutrinoSettings::TIMING_MENU];
}

EpgPlus::MenuOptionChooserSwitchViewMode::~MenuOptionChooserSwitchViewMode()
{
  g_settings.timing[SNeutrinoSettings::TIMING_MENU] = this->oldTimingMenuSettings;
}

int EpgPlus::MenuOptionChooserSwitchViewMode::exec
  ( CMenuTarget* parent
  )
{
  // change time out settings temporary
  g_settings.timing[SNeutrinoSettings::TIMING_MENU] = 1;

  CMenuOptionChooser::exec(parent);

  return menu_return::RETURN_REPAINT;
}
/*
EpgPlus::MenuTargetSettings::MenuTargetSettings
  ( EpgPlus* epgPlus
  )
{
	this->epgPlus = epgPlus;
}

int EpgPlus::MenuTargetSettings::exec
  ( CMenuTarget* parent
  , const std::string& actionKey
  )
{
  this->epgPlus->hide();

  MenuWidgetSettings(this->epgPlus).exec(parent, "");

  this->epgPlus->free();
  this->epgPlus->init();

	return menu_return::RETURN_EXIT_ALL;
}

EpgPlus::MenuWidgetSettings::MenuTargetSaveSettings::MenuTargetSaveSettings
  ( EpgPlus*  epgPlus
  )
{
  this->epgPlus  = epgPlus;
}

int EpgPlus::MenuWidgetSettings::MenuTargetSaveSettings::exec
	( CMenuTarget* parent
	, const std::string& actionKey
	)
{
  EpgPlus::saveSettings();
  return menu_return::RETURN_EXIT_ALL;
}

EpgPlus::MenuWidgetSettings::MenuTargetResetSettings::MenuTargetResetSettings
  ( EpgPlus*  epgPlus
  , Settings* settings
  )
{
  this->epgPlus  = epgPlus;
  this->settings = settings;
}

int EpgPlus::MenuWidgetSettings::MenuTargetResetSettings::exec
	( CMenuTarget* parent
	, const std::string& actionKey
	)
{
  *this->settings = EpgPlus::Settings(true);
  return menu_return::RETURN_NONE;
}

EpgPlus::MenuWidgetSettings::MenuTargetFontSettings::MenuTargetFontSettings
  ( EpgPlus*      epgPlus
  , FontSettings* fontSettings
  )
{
  this->epgPlus      = epgPlus;
  this->fontSettings = fontSettings;
}

int EpgPlus::MenuWidgetSettings::MenuTargetFontSettings::exec
	( CMenuTarget* parent
	, const std::string& actionKey
	)
{
  return EpgPlus::MenuWidgetFontSettings(this->epgPlus, this->fontSettings).exec(parent, actionKey);
}

EpgPlus::MenuWidgetSettings::MenuTargetSizeSettings::MenuTargetSizeSettings
  ( EpgPlus*      epgPlus
  , SizeSettings* sizeSettings
  )
{
  this->epgPlus      = epgPlus;
  this->sizeSettings = sizeSettings;
}

int EpgPlus::MenuWidgetSettings::MenuTargetSizeSettings::exec
	( CMenuTarget* parent
	, const std::string& actionKey
	)
{
  return EpgPlus::MenuWidgetSizeSettings(this->epgPlus, this->sizeSettings).exec(parent, actionKey);
}

EpgPlus::MenuWidgetSettings::MenuWidgetSettings
  ( EpgPlus* epgPlus
  )
  : CMenuWidget(LOCALE_EPGPLUS_SETTINGS, "features.raw", 400)
{
	this->epgPlus = epgPlus;
}

int EpgPlus::MenuWidgetSettings::exec(CMenuTarget* parent, const std::string & actionKey)
{
	Settings settings(this->epgPlus->settings);

	this->addItem(new CMenuForwarder(LOCALE_EPGPLUS_SAVE_SETTINGS , true, NULL, new MenuTargetSaveSettings (this->epgPlus                        ), NULL, CRCInput::RC_red   , NEUTRINO_ICON_BUTTON_RED   ), false);
	this->addItem(new CMenuForwarder(LOCALE_EPGPLUS_RESET_SETTINGS, true, NULL, new MenuTargetResetSettings(this->epgPlus, &settings             ), NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW), false);
	this->addItem(new CMenuForwarder(LOCALE_EPGPLUS_EDIT_FONTS    , true, NULL, new MenuTargetFontSettings (this->epgPlus, &settings.fontSettings), NULL, CRCInput::RC_1                                  ), false);
	this->addItem(new CMenuForwarder(LOCALE_EPGPLUS_EDIT_SIZES    , true, NULL, new MenuTargetSizeSettings (this->epgPlus, &settings.sizeSettings), NULL, CRCInput::RC_2                                  ), false);

	CMenuWidget::exec(parent, "");

	this->epgPlus->settings = settings;

	return menu_return::RETURN_EXIT;
}


EpgPlus::MenuWidgetFontSetting::MenuTargetSelectFontName::MenuTargetSelectFontName
  ( EpgPlus*     epgPlus
  , FontSetting* fontSetting
  )
{
  this->epgPlus     = epgPlus;
  this->fontSetting = fontSetting;
}

int EpgPlus::MenuWidgetFontSetting::MenuTargetSelectFontName::exec
	( CMenuTarget* parent
	, const std::string& actionKey
	)
{
	fb_pixel_t savedScreen[this->epgPlus->usableScreenWidth * this->epgPlus->usableScreenHeight * sizeof(fb_pixel_t)];
	this->epgPlus->frameBuffer->SaveScreen
		( this->epgPlus->usableScreenX
		, this->epgPlus->usableScreenY
		, this->epgPlus->usableScreenWidth
		, this->epgPlus->usableScreenHeight
		, savedScreen
		);


  CFileBrowser fileBrowser;
  CFileFilter fileFilter;
  fileFilter.addFilter("ttf");

  fileBrowser.Filter = &fileFilter;
  if (fileBrowser.exec(FONTDIR) == true)
  {
    this->fontSetting->name = fileBrowser.getSelectedFile()->getFileName();
  }

	this->epgPlus->frameBuffer->RestoreScreen
		( this->epgPlus->usableScreenX
		, this->epgPlus->usableScreenY
		, this->epgPlus->usableScreenWidth
		, this->epgPlus->usableScreenHeight
		, savedScreen
		);

  return menu_return::RETURN_NONE;
}

EpgPlus::MenuWidgetFontSetting::MenuTargetChangeFontSize::MenuTargetChangeFontSize
  ( EpgPlus*     epgPlus
  , FontSetting* fontSetting
  )
{
  this->epgPlus     = epgPlus;
  this->fontSetting = fontSetting;
}

int EpgPlus::MenuWidgetFontSetting::MenuTargetChangeFontSize::exec
	( CMenuTarget* parent
	, const std::string& actionKey
	)
{
  char value[5];
  {
    std::stringstream text;
    text << this->fontSetting->size;
    strcpy(value, text.str().c_str());
  }


  CStringInput input
    ( LOCALE_EPGPLUS_CHANGE_FONT_SIZE
    , value
    , 3
    , NONEXISTANT_LOCALE
    , NONEXISTANT_LOCALE
    , "0123456789 "
    );

  int result(input.exec(parent, actionKey));

  {
    std::stringstream text;
    text << value;
    text >> this->fontSetting->size;
  }

  return result;
}

struct CMenuOptionChooser::keyval menuOptionChooserFontStyles[] =
{
  {0, LOCALE_EPGPLUS_FONT_STYLE_REGULAR},
  {1, LOCALE_EPGPLUS_FONT_STYLE_BOLD},
  {2, LOCALE_EPGPLUS_FONT_STYLE_ITALIC}
};


EpgPlus::MenuWidgetFontSetting::MenuOptionChooserFontStyle::MenuOptionChooserFontStyle
  ( EpgPlus*     epgPlus
  , FontSetting* fontSetting
  )
  : CMenuOptionChooser
      ( LOCALE_EPGPLUS_CHANGE_FONT_STYLE
      , (int*)&this->indexFontStyle
      , menuOptionChooserFontStyles
      , sizeof(menuOptionChooserFontStyles)/sizeof(CMenuOptionChooser::keyval)
      , true
      , NULL
      , CRCInput::RC_yellow
      , NEUTRINO_ICON_BUTTON_GREEN
      )
{
  this->epgPlus     = epgPlus;
  this->fontSetting = fontSetting;

  if (this->fontSetting->style      == "Regular")
    this->indexFontStyle = 0;
  else if (this->fontSetting->style == "Bold")
    this->indexFontStyle = 1;
  else if (this->fontSetting->style == "Italic")
    this->indexFontStyle = 2;
  else
    this->indexFontStyle = 0;

}

int EpgPlus::MenuWidgetFontSetting::MenuOptionChooserFontStyle::exec
	( CMenuTarget* parent
	)
{
  int result = CMenuOptionChooser::exec(parent);

  switch (this->indexFontStyle)
  {
    case 0: this->fontSetting->style = "Regular";      break;
    case 1: this->fontSetting->style = "Bold";         break;
    case 2: this->fontSetting->style = "Italic";       break;
  };

  return result;
}

EpgPlus::MenuWidgetFontSetting::MenuWidgetFontSetting
  ( EpgPlus* epgPlus
  , FontSetting* fontSetting
  )
  : CMenuWidget(LOCALE_EPGPLUS_EDIT_FONTS, "features.raw", 400)
{
  this->epgPlus     = epgPlus;
  this->fontSetting = fontSetting;
}

int EpgPlus::MenuWidgetFontSetting::exec(CMenuTarget* parent, const std::string & actionKey)
{
	this->addItem(new CMenuForwarder(LOCALE_EPGPLUS_SELECT_FONT_NAME, true, NULL, new MenuTargetSelectFontName(this->epgPlus, this->fontSetting), NULL, CRCInput::RC_red   , NEUTRINO_ICON_BUTTON_RED   ), false);

	this->addItem(new MenuOptionChooserFontStyle(this->epgPlus, this->fontSetting), false);

	this->addItem(new CMenuForwarder(LOCALE_EPGPLUS_CHANGE_FONT_SIZE, true, NULL, new MenuTargetChangeFontSize(this->epgPlus, this->fontSetting), NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW), false);

	return CMenuWidget::exec(parent, "");
}

EpgPlus::MenuWidgetFontSettings::MenuWidgetFontSettings
  ( EpgPlus*      epgPlus
  , FontSettings* fontSettings
  )
  : CMenuWidget(LOCALE_EPGPLUS_EDIT_FONTS, "features.raw", 400)
{
  this->epgPlus      = epgPlus;
  this->fontSettings = fontSettings;
}

int EpgPlus::MenuWidgetFontSettings::exec
  ( CMenuTarget* parent
  , const std::string& actionKey
  )
{
	int digit = 1;
	for ( FontSettings::iterator It = this->fontSettings->begin()
		      ; It != this->fontSettings->end()
		      ;   ++It
		)
	{
		if (It->second.isConfigurable)
		{
			this->addItem(new CMenuForwarder(setting2LocaleConverter[It->first], true, NULL, new MenuTargetEditFont(this->epgPlus, &(It->second)), NULL, CRCInput::convertDigitToKey(digit)), false);
			digit++;
		}
	}

	return CMenuWidget::exec(parent, "");
}

EpgPlus::MenuWidgetFontSettings::MenuTargetEditFont::MenuTargetEditFont
  ( EpgPlus*     epgPlus
  , FontSetting* fontSetting
  )
{
  this->epgPlus     = epgPlus;
  this->fontSetting = fontSetting;
}

int EpgPlus::MenuWidgetFontSettings::MenuTargetEditFont::exec
	( CMenuTarget* parent
	, const std::string& actionKey
	)
{
  return MenuWidgetFontSetting(this->epgPlus, this->fontSetting).exec(parent, actionKey);
}

EpgPlus::MenuWidgetSizeSettings::MenuWidgetSizeSettings
  ( EpgPlus*      epgPlus
  , SizeSettings* sizeSettings
  )
  : CMenuWidget(LOCALE_EPGPLUS_EDIT_SIZES, "features.raw", 400)
{
  this->epgPlus      = epgPlus;
  this->sizeSettings = sizeSettings;
}

int EpgPlus::MenuWidgetSizeSettings::exec
  ( CMenuTarget* parent
  , const std::string& actionKey
  )
{
	int digit = 0;
	for (SizeSettings::iterator It = this->sizeSettings->begin()
		     ; It != this->sizeSettings->end()
		     ;   ++It
		)
	{
		if (It->second.isConfigurable)
		{
			this->addItem(new CMenuForwarder(setting2LocaleConverter[It->first], true, NULL, new MenuTargetEditFont(this->epgPlus, &(It->second)), NULL, CRCInput::convertDigitToKey(digit)), false);

			digit++;
		}
	}

	return CMenuWidget::exec(parent, "");
}

EpgPlus::MenuWidgetSizeSettings::MenuTargetEditSize::MenuTargetEditSize
  ( EpgPlus*     epgPlus
  , SizeSetting* sizeSetting
  )
{
  this->epgPlus     = epgPlus;
  this->sizeSetting = sizeSetting;
}

int EpgPlus::MenuWidgetSizeSettings::MenuTargetEditSize::exec
	( CMenuTarget* parent
	, const std::string& actionKey
	)
{
  char value[5];
  {
    std::stringstream text;
    text << this->sizeSetting->size;
    strcpy(value, text.str().c_str());
  }


  CStringInput input
    ( LOCALE_EPGPLUS_CHANGE_SIZE
    , value
    , 3
    , NONEXISTANT_LOCALE
    , NONEXISTANT_LOCALE
    , "0123456789 "
    );

  int result(input.exec(parent, actionKey));

  {
    std::stringstream text;
    text << value;
    text >> this->sizeSetting->size;
  }

  return result;
}

*/
