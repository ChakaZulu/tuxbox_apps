/*
        Neutrino-GUI  -   DBoxII-Project

        Copyright (C) 2001 Steffen Hehn 'McClean'
        Copyright (C) 2004 Martin Griep 'vivamiga'

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


#ifndef __EPGPLUS_HPP__
#define __EPGPLUS_HPP__

#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <system/settings.h>

#include "color.h"
#include "channellist.h"
#include "infoviewer.h"

#include "widget/menue.h"

#include <string>

class EpgPlus
{
//// types, inner classes
private:
        class CFooter;

        class CHeader
        {
        //// construction / destruction
        public:
                CHeader
                ( CFrameBuffer* frameBuffer
                , int x
                , int y
                , int width
                , int height
                );

                ~CHeader();

        //// methods
        public:
                void paint();

        //// attributes
        public:
                CFrameBuffer* frameBuffer;

                int x;
                int y;
                int width;
                int height;

                int color;
        };


        class CTimeLine
        {
        //// construction / destruction
        public:
                CTimeLine
                ( CFrameBuffer* frameBuffer
                , int x
                , int y
                , int width
                , int height1
                , int height2
                , int startX
                , int durationX
                , int gridHeight
                );

                ~CTimeLine();

        //// methods
        public:
                void paint
                ( time_t startTime
                , int    duration
                );

                void paintMark
                ( time_t startTime
                , int    duration
                , int    x
                , int    width
                );

                void paintGrid();

                void clearMark();

        //// attributes
        public:
                CFrameBuffer* frameBuffer;

                int currentDuration;

                int x;
                int y;
                int width;
                int height1;
                int height2;
                int height3;
                int startX;
                int durationX;
                int gridHeight;

                int color1;
                int color2;
                int markColor;
                int backMarkColor;
                int gridColor;
        };

        class CChannelEventEntry
        {
        //// construction / destruction
        public:
                CChannelEventEntry
                ( const CChannelEvent* channelEvent
                , CFrameBuffer* frameBuffer
                , CTimeLine* timeLine
                , CFooter* footer
                , int x
                , int y
                , int width
                , int height
                );

                ~CChannelEventEntry();

        //// methods
        public:
                bool isSelected
                ( time_t selectedTime
                ) const;

                void paint
                ( bool isSelected
                , bool toggleColor
                );

        //// attributes
        public:
                CChannelEvent channelEvent;

                CFrameBuffer* frameBuffer;
                CTimeLine* timeLine;
                CFooter* footer;
                int x;
                int y;
                int width;
                int height;

                int normalColor1;
                int normalColor2;
                int selectionColor;
                int dummyEventColor;

         };

        typedef std::vector<CChannelEventEntry*> TChannelEventEntries;

        class CChannelEntry
        {
        //// construction / destruction
        public:
                CChannelEntry
                ( const CChannelList::CChannel* channel
                , int index
                , CFrameBuffer* frameBuffer
                , int x
                , int y
                , int width
                , int height
                );

                ~CChannelEntry();

        //// methods
        public:
                void paint
                ( bool   isSelected
                , time_t selectedTime
                );

        //// attributes
        public:
                const CChannelList::CChannel* channel;
                std::string displayName;
                int index;

                CFrameBuffer* frameBuffer;

                int x;
                int y;
                int width;
                int height;

                int normalColor;
                int selectionColor;

                TChannelEventEntries      channelEventEntries;
        };

        typedef std::vector<CChannelEntry*> TChannelEntries;

        class CFooter
        {
        //// construction / destruction
        public:
                CFooter
                ( CFrameBuffer* frameBuffer
                , int x
                , int y
                , int width
                , int height1
                , int height2
                , int height3
                );

                ~CFooter();

        //// methods
        public:
                void paintEventDetails
                ( const std::string& description
                , const std::string& shortDescription
                );

                void paintButtons
                ( bool isStretchMode
                );

        //// attributes
        public:
                CFrameBuffer* frameBuffer;

                int x;
                int y;
                int width;
                int height1;
                int height2;
                int height3;

                int color;
        };

//// construction / destruction
public:
        EpgPlus();
        ~EpgPlus();

//// methods
public:

        int exec ( CChannelList* channelList , int selectedChannelIndex); // UTF-8


private:
        static std::string getTimeString
        ( const time_t& time
        , const std::string& format
        );

        TChannelEventEntries::const_iterator getSelectedEvent() const;


        void createChannelEntries
                ( int selectedChannelEntryIndex
                );


        void paint();

        void paintChannelEntry
        ( int position
        );

        void hide();

//// properties
private:
        CFrameBuffer*   frameBuffer;


        TChannelEntries displayedChannelEntries;

        CHeader*        header;
        CTimeLine*      timeLine;

        CChannelList*   channelList;

        CFooter*        footer;

        CChannelEntry*  selectedChannelEntry;
        time_t          selectedTime;

        int             channelListStartIndex;
        int             maxNumberOfDisplayableEntries; // maximal number of displayable entrys

        time_t          startTime;
        time_t          firstStartTime;
        time_t          duration;

        int             entryHeight;

        bool                isStretchMode;

        int             headerX;
        int             headerY;
        int             headerWidth;
        int             headerHeight;

        int             usableScreenWidth;
        int             usableScreenHeight;
        int             usableScreenX;
        int             usableScreenY;

        int             timeLineX;
        int             timeLineY;
        int             timeLineWidth;
        int             timeLineHeight1;
        int             timeLineHeight2;

        int             channelsTableX;
        int             channelsTableY;
        int             channelsTableWidth;
        int             channelsTableHeight;

        int             eventsTableX;
        int             eventsTableY;
        int             eventsTableWidth;
        int             eventsTableHeight;

        int             sliderX;
        int             sliderY;
        int             sliderWidth;
        int             sliderHeight;

        int             footerX;
        int             footerY;
        int             footerWidth;
        int             footerHeight1;
        int             footerHeight2;
        int             footerHeight3;


        int             horGap1;
        int             verGap1;
        int             verGap2;
};



class CEPGplusHandler : public CMenuTarget
{
	public:
		int  exec( CMenuTarget* parent,  const std::string &actionkey);

};

#endif
