/*
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
#include <sectionsdclient/sectionsdclient.h>

#include <gui/widget/icons.h>
#include <gui/widget/buttons.h>
#include <gui/widget/messagebox.h>
#include <gui/channellist.h>

#include <zapit/client/zapitclient.h> /* CZapitClient::Utf8_to_Latin1 */
#include <driver/rcinput.h>
#include <driver/screen_max.h>

#include <algorithm>
#include <sstream>

//#define DEBUG_

EpgPlus::CHeader::CHeader ( CFrameBuffer* frameBuffer , int x , int y , int width , int height)
{
	this->frameBuffer = frameBuffer;
	this->x       = x;
	this->y       = y;
	this->width   = width;
	this->height  = height;

	this->textColor = COL_MENUHEAD;
	this->backColor = COL_MENUHEAD_PLUS_0;
}

EpgPlus::CHeader::~CHeader()
{
}

void EpgPlus::CHeader::paint()
{
	// clear the region
	this->frameBuffer->paintBoxRel
		( this->x
		, this->y
		, this->width
		, this->height
		, this->backColor
		);

	// display new text
	g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE]->RenderString
		( this->x + 10
		, this->y + this->height
		, this->width
		, g_Locale->getText("EPGPlus.head")
		, this->textColor
		, 0
		, true
		); // UTF-8

}

EpgPlus::CTimeLine::CTimeLine
( CFrameBuffer* frameBuffer
, int x
, int y
, int width
, int height1
, int height2
, int startX
, int durationX
, int gridHeight
)
{
	this->frameBuffer = frameBuffer;
	this->x         = x;
	this->y         = y;
	this->width     = width;
	this->height1   = height1;
	this->height2   = height2;
	this->startX    = startX;
	this->durationX = durationX;
	this->gridHeight = gridHeight;

	this->backColor1     = COL_MENUCONTENT_PLUS_1;
	this->backColor2     = COL_MENUCONTENT_PLUS_2;
	this->dateColor      = COL_MENUHEAD;
	this->markColor      = COL_MENUCONTENTSELECTED;
	this->backMarkColor  = COL_MENUCONTENT;
	this->textColor      = COL_MENUCONTENT_PLUS_1;
	this->gridColor      = COL_MENUCONTENT_PLUS_5;
}

EpgPlus::CTimeLine::~CTimeLine()
{
}

void EpgPlus::CTimeLine::paint ( time_t startTime , int    duration)
{
	#ifdef DEBUG_
		std::cout << "EpgPlus::CTimeLine::paint" << std::endl;
	#endif

	this->clearMark();

	int xPos = this->startX;

	this->currentDuration = duration;
	int numberOfTicks = this->currentDuration/(60*60) * 2;
	int tickDist = (this->durationX)/numberOfTicks;
	time_t tickTime = startTime;
	bool toggleColor = false;

	// display date of begin
	this->frameBuffer->paintBoxRel
		( this->x
		, this->y
		, this->width
		, this->height1
		, toggleColor?this->backColor2:this->backColor1
		);

	g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString
		( this->x + 4
		, this->y + this->height1
		, this->width
		, EpgPlus::getTimeString(startTime, "%d-%b")
		, dateColor
		, 0
		, true
		); // UTF-8

	// paint ticks
	for ( int i = 0
	    ; i < numberOfTicks
	    ;   ++i
	      , xPos += tickDist
	      , tickTime += duration/numberOfTicks
	    )
	{
		int xWidth = tickDist;
		if (xPos + xWidth > this->x + width)
			xWidth = this->x + width - xPos;


		this->frameBuffer->paintBoxRel
			( xPos
			, this->y
			, xWidth
			, this->height1
			, toggleColor?this->backColor1:this->backColor2
			);

		std::string timeStr = EpgPlus::getTimeString(tickTime, "%H");


		int textWidth = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getRenderWidth(timeStr, true);

		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString
			( xPos - textWidth - 4
			, this->y + this->height1
			, textWidth
			, timeStr
			, this->textColor
			, 0
			, true
			); // UTF-8

		timeStr = EpgPlus::getTimeString(tickTime, "%M");
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString
			( xPos + 4
			, this->y + this->height1
			, textWidth
			, timeStr
			, this->textColor
			, 0
			, true
			); // UTF-8



		toggleColor = !toggleColor;
	}
}

void EpgPlus::CTimeLine::paintGrid()
{
	#ifdef DEBUG_
		std::cout << "EpgPlus::CTimeLine::paintGrid " << this->y << " " << this->gridHeight << std::endl;
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
			, this->gridHeight
			, this->gridColor
			);
	}
}

void EpgPlus::CTimeLine::paintMark ( time_t startTime , int    duration , int    x , int    width)
{
	#ifdef DEBUG_
		std::cout << "EpgPlus::CTimeLine::paintMark" << std::endl;
	#endif

	// clear old mark
	this->clearMark();

	// paint new mark
	this->frameBuffer->paintBoxRel
		( x
		, this->y + this->height1
		, width
		, this->height2
		, this->markColor
		);


	// display start time before mark
	std::string timeStr = EpgPlus::getTimeString(startTime, "%H:%M");
	int textWidth = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getRenderWidth(timeStr, true);

	g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString
		( x - textWidth
		, this->y + this->height1 + this->height2
		, textWidth
		, timeStr
		, this->textColor
		, 0
		, true
		); // UTF-8

	// display end time after mark
	timeStr = EpgPlus::getTimeString(startTime + duration, "%H:%M");
	textWidth = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getRenderWidth(timeStr, true);

	if (x + width + textWidth < this->x + this->width)
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString
			( x + width
			, this->y + this->height1 + this->height2
			, textWidth
			, timeStr
			, this->textColor
			, 0
			, true
			); // UTF-8
	}
	else
	if (textWidth < width - 10)
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString
			( x + width - textWidth
			, this->y + this->height1 + this->height2
			, textWidth
			, timeStr
			, this->textColor
			, 0
			, true
			); // UTF-8


	}
}

void EpgPlus::CTimeLine::clearMark()
{
	// clear mark
	this->frameBuffer->paintBoxRel
		( this->x
		, this->y + this->height1
		, this->width
		, this->height2
		, this->backMarkColor
		);
}

EpgPlus::CChannelEventEntry::CChannelEventEntry ( const CChannelEvent* channelEvent,
		CFrameBuffer* frameBuffer , CTimeLine* timeLine , CFooter* footer ,
		int x , int y , int width , int height)
{
	// copy neccessary?
	if (channelEvent != NULL)
		this->channelEvent = *channelEvent;

	this->frameBuffer = frameBuffer;
	this->timeLine = timeLine;
	this->footer = footer;
	this->x      = x;
	this->y      = y;
	this->width  = width;
	this->height = height;

	this->normalColor1    = COL_MENUCONTENT_PLUS_1;
	this->normalColor2    = COL_MENUCONTENT_PLUS_2;
	this->selectionColor  = COL_MENUCONTENTSELECTED;
	this->dummyEventColor = COL_MENUCONTENT;
}

EpgPlus::CChannelEventEntry::~CChannelEventEntry()
{
}

bool EpgPlus::CChannelEventEntry::isSelected ( time_t selectedTime) const
{
	#ifdef DEBUG_
		std::cout << "isSelected " << EpgPlus::getTimeString(this->channelEvent.startTime, "%H:%M") << " " << this->channelEvent.duration << std::endl;
	#endif

	return (selectedTime >= this->channelEvent.startTime) && (selectedTime < this->channelEvent.startTime + time_t(this->channelEvent.duration));
}


void EpgPlus::CChannelEventEntry::paint ( bool isSelected , bool toggleColor)
{
	#ifdef DEBUG_
		std::cout << "EpgPlus::CChannelEventEntry::paint " << this->x << " " << this->y << " " << this->width << " " << this->height << " " << this->channelEvent.description << std::endl;
	#endif


	this->frameBuffer->paintBoxRel
		( this->x
		, this->y
		, this->width
		, this->height
		, this->channelEvent.description.empty()?this->dummyEventColor:(isSelected?this->selectionColor:(toggleColor?this->normalColor1:this->normalColor2))
		);


	g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString
		( this->x + 2
		, this->y + this->height
		, this->width - 4 > 0?this->width - 4:0
		, this->channelEvent.description
		, isSelected?this->selectionColor:(toggleColor?this->normalColor1:this->normalColor2)
		, false
		);

	if (isSelected)
	{
		if (this->channelEvent.description.empty())
		{// dummy channel event
			this->timeLine->clearMark();
		}
		else
		{
			this->timeLine->paintMark
				( this->channelEvent.startTime
				, this->channelEvent.duration
				, this->x
				, this->width
				);
		}

		#ifdef DEBUG_
			std::cout << "paintEventDetails1" << std::endl;
		#endif
		CShortEPGData shortEpgData;

		this->footer->paintEventDetails
			( this->channelEvent.description
			, g_Sectionsd->getEPGidShort(this->channelEvent.eventID, &shortEpgData)?shortEpgData.info1:""
			);
		#ifdef DEBUG_
			std::cout << "paintEventDetails2" << std::endl;
		#endif

		this->timeLine->paintGrid();
	}
}

EpgPlus::CChannelEntry::CChannelEntry ( const CChannelList::CChannel* channel , int index ,
		CFrameBuffer* frameBuffer , int x , int y , int width , int height)
{
	this->channel = channel;
	std::stringstream displayName;
	displayName
		<< index + 1
		<< " "
		<< channel->getName();

	this->displayName  = CZapitClient::Utf8_to_Latin1(displayName.str());
	this->index = index;

	this->frameBuffer = frameBuffer;
	this->x      = x;
	this->y      = y;
	this->width  = width;
	this->height = height;

	this->normalColor    = COL_MENUCONTENT;
	this->selectionColor = COL_MENUCONTENTSELECTED;
}

EpgPlus::CChannelEntry::~CChannelEntry()
{
	for ( TChannelEventEntries::iterator It = this->channelEventEntries.begin()
	    ; It != this->channelEventEntries.end()
	    ; ++It
	    )
		delete *It;
}

void EpgPlus::CChannelEntry::paint
( bool isSelected
, time_t selectedTime
)
{
	#ifdef DEBUG_
		std::cout << "EpgPlus::CChannelEntry::paint " << isSelected << " " << this->x << " " << this->y << " " << this->width << " " << this->height << " " << this->displayName << std::endl;
	#endif

	this->frameBuffer->paintBoxRel
		( this->x
		, this->y
		, this->width
		, this->height
		, isSelected?this->selectionColor:this->normalColor
		);

	g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString
		( this->x + 2
		, this->y + this->height
		, this->width - 4
		, this->displayName
		, isSelected?this->selectionColor:this->normalColor
		, true
		);

	bool toggleColor = false;
	for ( TChannelEventEntries::iterator It = this->channelEventEntries.begin()
	    ; It != this->channelEventEntries.end()
	    ; ++It
	    )
	{
		(*It)->paint(isSelected && (*It)->isSelected(selectedTime), toggleColor);

		toggleColor = !toggleColor;
	}
}

EpgPlus::CFooter::CFooter ( CFrameBuffer* frameBuffer , int x , int y , int width , int height1 , int height2 , int height3)
{
	this->frameBuffer = frameBuffer;
	this->x       = x;
	this->y       = y;
	this->width   = width;
	this->height1 = height1;
	this->height2 = height2;
	this->height3 = height3;

	this->textColor = COL_MENUHEAD;
	this->backColor = COL_MENUHEAD_PLUS_0;


}

EpgPlus::CFooter::~CFooter()
{
}

void EpgPlus::CFooter::paintEventDetails ( const std::string& description , const std::string& shortDescription)
{
	// clear the region
	this->frameBuffer->paintBoxRel
		( this->x
		, this->y
		, this->width
		, this->height1
		, this->backColor
		);

	// display new text
	g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->RenderString
		( this->x + 10
		, this->y + this->height1
		, this->width - 20
		, description
		, this->textColor
		, 0
		, false
		);

	// clear the region
	this->frameBuffer->paintBoxRel
		( this->x
		, this->y + this->height1
		, this->width
		, this->height2
		, this->backColor
		);

	// display new text
	g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->RenderString
		( this->x + 10
		, this->y + this->height1 + this->height2
		, this->width
		, shortDescription
		, this->textColor
		, 0
		, false
		);

}

struct button_label channelListButtons[] =
{
	{ NEUTRINO_ICON_BUTTON_RED    , "EPGPlus.record"},
	{ NEUTRINO_ICON_BUTTON_GREEN  , "EPGPlus.refresh_epg"},
	{ NEUTRINO_ICON_BUTTON_YELLOW , "EPGPlus.remind"},
	{ NEUTRINO_ICON_BUTTON_BLUE   , ""},
};

void EpgPlus::CFooter::paintButtons ( bool isStretchMode)
{
	int buttonWidth = (this->width - 20) / 4;

	int buttonHeight = 7 + std::min(16, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight());
	#ifdef DEBUG_
		std::cout << "EpgPlus::CFooter::paintButtons1" << std::endl;
	#endif

	this->frameBuffer->paintBoxRel(this->x, this->y + this->height1 + this->height2, this->width, this->height3, COL_MENUHEAD_PLUS_0);
	#ifdef DEBUG_
		std::cout << "EpgPlus::CFooter::paintButtons2" << std::endl;
	#endif

	channelListButtons[3].locale = isStretchMode?"EPGPlus.stretch_mode":"EPGPlus.scroll_mode";
	::paintButtons(this->frameBuffer, g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL], g_Locale, this->x + 10, this->y + this->height1 + this->height2 + (this->height3 - buttonHeight) + 3, buttonWidth, sizeof(channelListButtons)/sizeof(button_label), channelListButtons);
	#ifdef DEBUG_
		std::cout << "EpgPlus::CFooter::paintButtons3" << std::endl;
	#endif

	this->frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_HELP, this->x + this->width - 30, this->y + this->height1 + this->height2 );

}

EpgPlus::EpgPlus()
{
	this->frameBuffer = CFrameBuffer::getInstance();
	this->selectedChannelEntry = NULL;

	this->isStretchMode = false;

	// this->usableScreenWidth  = 580;
	// this->usableScreenHeight = 480;
	this->usableScreenWidth  = w_max (g_settings.screen_EndX , 4);
	this->usableScreenHeight = h_max (g_settings.screen_EndY , 4);

	this->headerHeight  = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE]->getHeight();

	this->entryHeight = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getHeight();

	this->horGap1 = 4;
	this->verGap1 = 4;
	this->verGap2 = 0;


	this->timeLineHeight1 = this->entryHeight;
	this->timeLineHeight2 = this->entryHeight;

	this->footerHeight1 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE]->getHeight();
	this->footerHeight2 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getHeight();
	this->footerHeight3 = g_Font[SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL]->getHeight();
	this->maxNumberOfDisplayableEntries = (this->usableScreenHeight - this->headerHeight - this->timeLineHeight1 - this->timeLineHeight2 - this->horGap1 - this->footerHeight1 - this->footerHeight2 - this->footerHeight3)/this->entryHeight;

	this->usableScreenHeight = this->headerHeight + this->timeLineHeight1 + this->timeLineHeight2 + this->horGap1 + this->maxNumberOfDisplayableEntries*this->entryHeight + this->footerHeight1 + this->footerHeight2 + this->footerHeight3; // recalc deltaY
	this->usableScreenX = (((g_settings.screen_EndX - g_settings.screen_StartX) - this->usableScreenWidth) / 2) + g_settings.screen_StartX;
	this->usableScreenY = (((g_settings.screen_EndY - g_settings.screen_StartY) - this->usableScreenHeight) / 2) + g_settings.screen_StartY;

	this->footerX     = usableScreenX;
	this->footerY     = this->usableScreenY + this->usableScreenHeight - this->footerHeight1 - this->footerHeight2 - this->footerHeight3;
	this->footerWidth = this->usableScreenWidth;

	this->headerX     = this->usableScreenX;
	this->headerY     = this->usableScreenY;
	this->headerWidth = this->usableScreenWidth;

	this->timeLineX     = this->usableScreenX;
	this->timeLineY     = this->usableScreenY + this->headerHeight;
	this->timeLineWidth = this->usableScreenWidth;

	this->channelsTableX	        = this->usableScreenX;
	this->channelsTableY	        = this->timeLineY + this->timeLineHeight1 + this->timeLineHeight2 + this->horGap1;
	this->channelsTableWidth        = 100;
	this->channelsTableHeight       = this->maxNumberOfDisplayableEntries*this->entryHeight;

	this->sliderWidth  = 15;
	this->eventsTableX      = this->channelsTableX + channelsTableWidth + this->verGap1;
	this->eventsTableY      = this->channelsTableY;
	this->eventsTableWidth  = this->usableScreenWidth - this->channelsTableWidth - this->sliderWidth - this->verGap1 - this->verGap2;
	this->eventsTableHeight = this->channelsTableHeight;

	this->sliderX = this->usableScreenX + this->usableScreenWidth - this->sliderWidth;
	this->sliderY = this->eventsTableY;
	this->sliderHeight = this->channelsTableHeight;

	this->channelListStartIndex = 0;
	this->startTime = 0;
	this->duration = 60 * 60 * 2; // 2h


	this->header = new CHeader
		( this->frameBuffer
		, this->headerX
		, this->headerY
		, this->headerWidth
		, this->headerHeight
		);

	this->timeLine = new CTimeLine
		( this->frameBuffer
		, this->timeLineX
		, this->timeLineY
		, this->timeLineWidth
		, this->timeLineHeight1
		, this->timeLineHeight2
		, this->eventsTableX
		, this->eventsTableWidth
		, this->timeLineHeight1 //+ this->timeLineHeight2 + this->horGap1 + this->eventsTableHeight
		);

	this->footer = new CFooter
		( this->frameBuffer
		, this->footerX
		, this->footerY
		, this->footerWidth
		, this->footerHeight1
		, this->footerHeight2
		, this->footerHeight3
		);

}


EpgPlus::~EpgPlus()
{
	delete this->header;
	delete this->timeLine;
	delete this->footer;
}


void EpgPlus::createChannelEntries ( int selectedChannelEntryIndex)
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

		this->channelListStartIndex = int(selectedChannelEntryIndex/this->maxNumberOfDisplayableEntries)*this->maxNumberOfDisplayableEntries;

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

			CChannelEntry* channelEntry = new CChannelEntry
				( channel
				, i
				, this->frameBuffer
				, this->channelsTableX + 2
				, yPosChannelEntry
				, this->channelsTableWidth
				, this->entryHeight
				);

			#ifdef DEBUG_
				std::cout << " channel name "<< channel->getName() << " "
					<< " channel_id "<< channel->channel_id << std::endl;
			#endif

			CChannelEventList channelEventList = g_Sectionsd->getEventsServiceKey(channel->channel_id);

			#ifdef DEBUG_
				std::cout << " channelEventList.size() "<< channelEventList.size() << std::endl;
			#endif

			int xPosEventEntry  = 0;
			int widthEventEntry = this->eventsTableX;

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

					// correct position
					xPosEventEntry  = this->eventsTableX + ((It->startTime - startTimeDiff - this->startTime) * this->eventsTableWidth)/this->duration;

					// correct width
					widthEventEntry = ((It->duration + startTimeDiff + endTimeDiff) * this->eventsTableWidth)/this->duration + 1;

					if (widthEventEntry < 0)
						widthEventEntry = 0;

					if (xPosEventEntry + widthEventEntry > this->eventsTableX + this->eventsTableWidth)
						widthEventEntry = this->eventsTableX + this->eventsTableWidth - xPosEventEntry;

					CChannelEventEntry* channelEventEntry = new CChannelEventEntry
						( &(*It)
						, this->frameBuffer
						, this->timeLine
						, this->footer
						, xPosEventEntry
						, yPosEventEntry
						, widthEventEntry
						, this->entryHeight
						);


					channelEntry->channelEventEntries.push_back(channelEventEntry);

				}

				lastIt = It;
			}

			if ( (channelEntry->channelEventEntries.empty()) // no channel event found
			   ||(xPosEventEntry + widthEventEntry < this->eventsTableX + eventsTableWidth) // there is remainding time between last channel event and end of the timeline
			   )
			{
				xPosEventEntry += widthEventEntry;
				widthEventEntry = this->eventsTableX + this->eventsTableWidth - xPosEventEntry;



				#ifdef DEBUG_
					std::cout << "add dummy channel entry " << channelEntry->channelEventEntries.size() << " " << xPosEventEntry << " " << widthEventEntry << " "<< channelEntry->displayName << xPosEventEntry << " " << widthEventEntry << std::endl;
				#endif

				TChannelEventEntries::const_iterator It = channelEntry->channelEventEntries.end() - 1;

				CChannelEvent channelEvent;
				channelEvent.startTime = channelEntry->channelEventEntries.empty()?this->startTime:(*It)->channelEvent.startTime + (*It)->channelEvent.duration;
				channelEvent.duration  = channelEntry->channelEventEntries.empty()?this->duration:(this->duration + this->startTime)-((*It)->channelEvent.startTime + (*It)->channelEvent.duration);
				// add a dummy channel entry
				CChannelEventEntry* channelEventEntry = new CChannelEventEntry
					( &channelEvent
					, this->frameBuffer
					, this->timeLine
					, this->footer
					, xPosEventEntry
					, yPosEventEntry
					, widthEventEntry
					, this->entryHeight
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

int EpgPlus::exec ( CChannelList* channelList , int selectedChannelIndex) // UTF-8
{
	this->channelList = channelList;
	this->channelListStartIndex = int(selectedChannelIndex/maxNumberOfDisplayableEntries)*maxNumberOfDisplayableEntries;

	bool refreshAll = false;
	int res = menu_return::RETURN_REPAINT;

	do
	{
		refreshAll = false;
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

		this->footer->paintButtons(this->isStretchMode);
		#ifdef DEBUG_
			std::cout << "paintButtons2" << std::endl;
		#endif

		this->paint();

		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_chanlist );
		bool loop=true;
		while (loop)
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

			if ( msg <= CRCInput::RC_MaxRC )
				timeoutEnd = CRCInput::calcTimeoutEnd( g_settings.timing_chanlist );

			if (msg == (neutrino_msg_t)g_settings.key_channelList_pageup)
			{
				int selectedChannelEntryIndex = this->selectedChannelEntry->index;
				selectedChannelEntryIndex    += this->maxNumberOfDisplayableEntries;

				if (selectedChannelEntryIndex > this->channelList->getSize() - 1)
					selectedChannelEntryIndex = 0;

				this->createChannelEntries(selectedChannelEntryIndex);

				this->paint();
			}
			else if (msg == (neutrino_msg_t)g_settings.key_channelList_pagedown)
			{
				int selectedChannelEntryIndex = this->selectedChannelEntry->index;
				selectedChannelEntryIndex    -= this->maxNumberOfDisplayableEntries;

				if (selectedChannelEntryIndex < 0)
					selectedChannelEntryIndex = this->channelList->getSize() - 1;

				this->createChannelEntries(selectedChannelEntryIndex);

				this->paint();
			}
			else if (msg == (neutrino_msg_t) CRCInput::RC_red)
			{
				#ifdef DEBUG_
					std::cout << "add record timer 1" << std::endl;
				#endif
				TChannelEventEntries::const_iterator It = this->getSelectedEvent();

				if ( (It != this->selectedChannelEntry->channelEventEntries.end())
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

						timerdclient.addRecordTimerEvent
							( this->selectedChannelEntry->channel->channel_id
							, (*It)->channelEvent.startTime
							, (*It)->channelEvent.startTime + (*It)->channelEvent.duration
							, (*It)->channelEvent.eventID
							, (*It)->channelEvent.startTime
							, (*It)->channelEvent.startTime - (ANNOUNCETIME + 120)
							, ""
							, true
							);
						ShowMsgUTF
							( "timer.eventrecord.title"
							, g_Locale->getText("timer.eventrecord.msg")
							, CMessageBox::mbrBack
							, CMessageBox::mbBack
							, "info.raw"
							); // UTF-8
					}
					else
						printf("timerd not available\n");
				}
			}
			else if ( msg == (neutrino_msg_t) CRCInput::RC_yellow)
			{
				#ifdef DEBUG_
					std::cout << "add reminder" << std::endl;
				#endif
				TChannelEventEntries::const_iterator It = this->getSelectedEvent();

				if ( (It != this->selectedChannelEntry->channelEventEntries.end())
				&&(!(*It)->channelEvent.description.empty())
				)
				{
					CTimerdClient timerdclient;
					if (timerdclient.isTimerdAvailable())
					{
						timerdclient.addZaptoTimerEvent
							( this->selectedChannelEntry->channel->channel_id
							, (*It)->channelEvent.startTime
							, (*It)->channelEvent.startTime - ANNOUNCETIME
							, 0
							, (*It)->channelEvent.eventID
							, (*It)->channelEvent.startTime
							, ""
							);

						ShowMsgUTF
							( "timer.eventtimed.title"
							, g_Locale->getText("timer.eventtimed.msg")
							, CMessageBox::mbrBack
							, CMessageBox::mbBack
							, "info.raw"
							); // UTF-8
					}
					else
						printf("timerd not available\n");
				}
			}
			else if (CRCInput::isNumeric(msg))
			{ //numeric zap
				this->hide();
				this->channelList->numericZap( msg );

				int selectedChannelEntryIndex = this->channelList->getSelectedChannelIndex();
				if (selectedChannelEntryIndex < this->channelList->getSize())
				{
					this->hide();
					this->createChannelEntries(selectedChannelEntryIndex);

					this->header->paint();
					this->footer->paintButtons(this->isStretchMode);
					this->paint();
				}

			}
			else if ( msg == CRCInput::RC_up )
			{
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
				}
				else
				{
					this->selectedChannelEntry = this->displayedChannelEntries[selectedChannelEntryIndex - this->channelListStartIndex];

					this->paintChannelEntry(prevSelectedChannelEntryIndex - this->channelListStartIndex);
					this->paintChannelEntry(selectedChannelEntryIndex     - this->channelListStartIndex);
				}
			}
			else if ( msg == CRCInput::RC_down )
			{
				int selectedChannelEntryIndex     = this->selectedChannelEntry->index;
				int prevSelectedChannelEntryIndex = this->selectedChannelEntry->index;

				selectedChannelEntryIndex = (selectedChannelEntryIndex + 1) % this->channelList->getSize();


				int oldChannelListStartIndex = this->channelListStartIndex;
				this->channelListStartIndex = (selectedChannelEntryIndex / this->maxNumberOfDisplayableEntries) * this->maxNumberOfDisplayableEntries;

				if (oldChannelListStartIndex != this->channelListStartIndex)
				{
					this->createChannelEntries(selectedChannelEntryIndex);

					this->paint();
				}
				else
				{
					this->selectedChannelEntry = this->displayedChannelEntries[selectedChannelEntryIndex - this->channelListStartIndex];

					this->paintChannelEntry(prevSelectedChannelEntryIndex - this->channelListStartIndex);
					this->paintChannelEntry(this->selectedChannelEntry->index - this->channelListStartIndex);
				}

			}
			else if ((msg == CRCInput::RC_timeout                             ) ||
				(msg == (neutrino_msg_t)g_settings.key_channelList_cancel))
			{
				loop=false;
			}

			else if ( msg==CRCInput::RC_left )
			{
				if (this->isStretchMode)
				{
					if (this->duration - 30*60 > 30*60)
					{
						this->duration -= 30*60;
						this->hide();
						refreshAll = true;
						loop = false;
					}
				}
				else
				{
					#ifdef DEBUG_
						std::cout << "RC_left " << std::endl;
					#endif
					TChannelEventEntries::const_iterator It = this->getSelectedEvent();

					if ( (It != this->selectedChannelEntry->channelEventEntries.begin())
					&&(It != this->selectedChannelEntry->channelEventEntries.end())
					)
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
						}

					}
				}
			}
			else if ( msg==CRCInput::RC_right )
			{
				if (this->isStretchMode)
				{
					if (this->duration + 30*60 < 4*60*60)
					{
						this->duration += 60*60;
						this->hide();
						refreshAll = true;
						loop = false;
					}
				}
				else
				{
					#ifdef DEBUG_
						std::cout << "RC_right " << std::endl;
					#endif
					TChannelEventEntries::const_iterator It = this->getSelectedEvent();

					if ( (It != this->selectedChannelEntry->channelEventEntries.end() - 1)
					&&(It != this->selectedChannelEntry->channelEventEntries.end())
					)
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
					}
				}
			}
			else if ( msg==CRCInput::RC_green )
			{
				refreshAll = true;
				loop = false;
			}
			else if ( msg==CRCInput::RC_blue )
			{
				this->isStretchMode = !this->isStretchMode;
				this->footer->paintButtons(this->isStretchMode);
			}
			else if ( msg==CRCInput::RC_ok )
			{
				#ifdef DEBUG_
					std::cout << "zapTo " << this->selectedChannelEntry->index << std::endl;
				#endif
				this->channelList->zapTo(this->selectedChannelEntry->index);
			}
			else if (msg==CRCInput::RC_help )
			{
				TChannelEventEntries::const_iterator It = this->getSelectedEvent();

				if (It != this->selectedChannelEntry->channelEventEntries.end())
				{

					if ( (*It)->channelEvent.eventID != 0 )
					{
						this->hide();

						time_t startTime = (*It)->channelEvent.startTime;
						res = g_EpgData->show
							( this->selectedChannelEntry->channel->channel_id
							, (*It)->channelEvent.eventID
							, &startTime
							);

						if ( res == menu_return::RETURN_EXIT_ALL )
						{
							loop = false;
						}
						else
						{
							g_RCInput->getMsg( &msg, &data, 0 );

							if ( ( msg != CRCInput::RC_red ) &&
							( msg != CRCInput::RC_timeout ) )
							{
								// RC_red schlucken
								g_RCInput->postMsg( msg, data );
							}

							this->header->paint();
							this->footer->paintButtons(this->isStretchMode);
							this->paint();
						}
					}
				}
			}
			else
			{
				if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) &
				messages_return::cancel_all )
				{
					loop = false;
					res = menu_return::RETURN_EXIT_ALL;
				}
			}
		}

		this->hide();

		for ( TChannelEntries::iterator It = this->displayedChannelEntries.begin()
		; It != this->displayedChannelEntries.begin()
		; ++It
		)
		delete *It;
	}
	while (refreshAll);


	return res;
}

EpgPlus::TChannelEventEntries::const_iterator EpgPlus::getSelectedEvent() const
{
	for ( TChannelEventEntries::const_iterator It = this->selectedChannelEntry->channelEventEntries.begin()
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

void EpgPlus::hide()
{
	this->frameBuffer->paintBackgroundBoxRel
	( this->usableScreenX
	, this->usableScreenY
	, this->usableScreenWidth
	, this->usableScreenHeight
	);
} 

void EpgPlus::paintChannelEntry ( int position)
{
	#ifdef DEBUG_
		std::cout << "paint channel entry " << position << std::endl;
	#endif

	CChannelEntry* channelEntry = this->displayedChannelEntries[position];

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

std::string EpgPlus::getTimeString ( const time_t& time , const std::string& format)
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
		, COL_MENUCONTENT_PLUS_0
		);

	#ifdef DEBUG_
		std::cout << "paint5" << std::endl;
	#endif

	int tmp = ((this->channelList->getSize() - 1)/this->maxNumberOfDisplayableEntries) + 1;
	float sliderKnobHeight = (sliderHeight - 4)/tmp;
	int sliderKnobPosition = this->selectedChannelEntry == NULL ? 0 : (this->selectedChannelEntry->index / this->maxNumberOfDisplayableEntries);

	this->frameBuffer->paintBoxRel
		( this->sliderX + 2
		, this->sliderY + int(sliderKnobPosition*sliderKnobHeight)
		, 11
		, int(sliderKnobHeight)
		, COL_MENUCONTENT_PLUS_3
		);
}






//
//  -- EPG+ Menue Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-05 rasc)
// 

int CEPGplusHandler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	int           res = menu_return::RETURN_EXIT_ALL;
	EpgPlus       *e;
	CChannelList  *channelList;


	if (parent) {
		parent->hide();
	}

	e = new EpgPlus;

	channelList = CNeutrinoApp::getInstance()->channelList;
	e->exec(channelList, channelList->getSelectedChannelIndex());
	delete e;

	return res;
}


