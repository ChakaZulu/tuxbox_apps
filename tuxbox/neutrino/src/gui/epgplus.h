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
#include "filebrowser.h"

#include "widget/menue.h"

#include <string>

class CBouquetList;
class ConfigFile;
struct button_label;

class EpgPlus
{
//// types, inner classes
public:
  enum FontSettingID
  {
    EPGPlus_header_font = 0,                         
    EPGPlus_timeline_fonttime,                   
    EPGPlus_timeline_fontdate,                   
    EPGPlus_channelentry_font,                   
    EPGPlus_channelevententry_font,              
    EPGPlus_footer_fontbouquetchannelname,       
    EPGPlus_footer_fonteventdescription,         
    EPGPlus_footer_fonteventshortdescription,    
    EPGPlus_footer_fontbuttons,                  
	 NumberOfFontSettings
  };

  enum ColorSettingID
  {
    EPGPlus_header_color = 0,                      
    EPGPlus_timeline_color1,                   
    EPGPlus_timeline_color2,                   
    EPGPlus_timeline_markcolor,                    
    EPGPlus_timeline_backmarkcolor,                
    EPGPlus_timeline_gridcolor,                    
    EPGPlus_channelevententry_normalcolor1,        
    EPGPlus_channelevententry_normalcolor2,        
    EPGPlus_channelevententry_selectioncolor,      
    EPGPlus_channelevententry_dummyeventcolor,     
    EPGPlus_channelevententry_separationlinecolor, 
    EPGPlus_channelentry_normalcolor,              
    EPGPlus_channelentry_selectioncolor,           
    EPGPlus_channelentry_separationlinecolor,      
    EPGPlus_footer_color,                      
    EPGPlus_slider_knobcolor,                      
    EPGPlus_slider_backcolor,                      
    EPGPlus_horgap1_color,                         
    EPGPlus_horgap2_color,                         
    EPGPlus_vergap1_color,                         
    EPGPlus_vergap2_color,
	 NumberOfColorSettings
  };

  enum SizeSettingID
  {                                                 
    EPGPlus_channelentry_width = 0,                    
    EPGPlus_channelentry_separationlineheight,     
    EPGPlus_slider_width,                          
    EPGPlus_horgap1_height,                        
    EPGPlus_horgap2_height,                        
    EPGPlus_vergap1_width,                         
    EPGPlus_vergap2_width,                         
	 NumberOfSizeSettings
  };

	struct FontSetting
	{
		FontSettingID     settingID;
		char*             settingName;
		neutrino_locale_t locale;            
		char*             name;
		char*             style;
		int               size;
		bool              isConfigurable;
	};

	struct ColorSetting
	{
		ColorSettingID    settingID;
		char*             settingName;
		neutrino_locale_t locale;            
		int               color;           
		bool              isConfigurable;
	};

	struct SizeSetting
	{
		SizeSettingID     settingID;
		char*             settingName;
		neutrino_locale_t locale;            
		int               size;           
		bool              isConfigurable;
	};


  enum TViewMode
  {
    ViewMode_Stretch,
    ViewMode_Scroll,
  };

  enum TSwapMode  
  {
    SwapMode_ByPage,
    SwapMode_ByBouquet,
  };

  class Footer;

  class Header
  {
  //// construction / destruction
  public:
    Header
      ( CFrameBuffer* frameBuffer
      , int x
      , int y
      , int width
      );

    ~Header();

  //// methods
  public:
    static void init();

    void paint();

    static int getUsedHeight();

  //// attributes
  public:
    CFrameBuffer* frameBuffer;

    int x;
    int y;
    int width;
    
    static Font* font;

    static int color;
  };


  class TimeLine
  {
  //// construction / destruction
  public:
    TimeLine
      ( CFrameBuffer* frameBuffer
      , int x
      , int y
      , int width
      , int startX
      , int durationX
      );
    
    ~TimeLine();

  //// methods
  public:
    static void init();

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

    static int getUsedHeight();

  //// attributes
  public:
    CFrameBuffer* frameBuffer;

    int currentDuration;

    int x;
    int y;
    int width;
  
    static Font* fontTime;
    static Font* fontDate;

    int startX;
    int durationX;

    static int color1;
    static int color2;
    static int markColor;
    static int backMarkColor;
    static int gridColor;
  };

  class ChannelEventEntry
  {
  //// construction / destruction
  public:
    ChannelEventEntry
      ( const CChannelEvent* channelEvent
      , CFrameBuffer* frameBuffer
      , TimeLine* timeLine
      , Footer* footer
      , int x
      , int y
      , int width
      );

  ~ChannelEventEntry();

  //// methods
  public:
    static void init();

    bool isSelected
      ( time_t selectedTime
      ) const;

    void paint
      ( bool isSelected
      , bool toggleColor
      );

    static int getUsedHeight();

  //// attributes
  public:
    CChannelEvent channelEvent;

    CFrameBuffer* frameBuffer;
    TimeLine* timeLine;
    Footer* footer;

    int x;
    int y;
    int width;
    static int separationLineHeight;

    static Font* font;

    static int normalColor1;
    static int normalColor2;
    static int selectionColor;
    static int dummyEventColor;
    static int separationLineColor;

  };

  typedef std::vector<ChannelEventEntry*> TCChannelEventEntries;

  class ChannelEntry
  {
  //// construction / destruction
  public:
    ChannelEntry
      ( const CChannelList::CChannel* channel
      , int index
      , CFrameBuffer* frameBuffer
      , Footer* footer
      , CBouquetList* bouquetList
      , int x
      , int y
      , int width
      );

    ~ChannelEntry();

  //// methods
  public:
    static void init();

    void paint
      ( bool   isSelected
      , time_t selectedTime
      );

    static int getUsedHeight();

  //// attributes
  public:
    const CChannelList::CChannel* channel;
    std::string displayName;
    int index;

    CFrameBuffer* frameBuffer;
    Footer* footer;
    CBouquetList* bouquetList;

    int x;
    int y;
    int width;
    static int separationLineHeight;

    static Font* font;

    static int normalColor;
    static int selectionColor;
    static int separationLineColor;

    TCChannelEventEntries      channelEventEntries;
  };

  typedef std::vector<ChannelEntry*> TChannelEntries;

  class Footer
  {
  //// construction / destruction
  public:
    Footer
      ( CFrameBuffer* frameBuffer
      , int x
      , int y
      , int width
      );

    ~Footer();

  //// methods
  public:
    static void init();

    void setBouquetChannelName
      ( const std::string& newBouquetName
      , const std::string& newChannelName
      );

    void paintEventDetails
      ( const std::string& description
      , const std::string& shortDescription
      );

    void paintButtons
      ( button_label* buttonLabels
      , int numberOfButtons
      );

    static int getUsedHeight();
  
  //// attributes
  public:
    CFrameBuffer* frameBuffer;

    int x;
    int y;
    int width;

    static Font*  fontBouquetChannelName;
    static Font*  fontEventDescription;     
    static Font*  fontEventShortDescription;
    static Font*  fontButtons;

    static int color;

    std::string currentBouquetName;
    std::string currentChannelName;
  };

	
  class MenuTargetAddReminder : public CMenuTarget
	{
		public:
			MenuTargetAddReminder
			( EpgPlus* epgPlus
			);

		public:
			int exec
			( CMenuTarget* parent
			, const std::string& actionKey
			);


		private:
			EpgPlus* epgPlus;

	};

	class MenuTargetAddRecordTimer : public CMenuTarget
	{
		public:
			MenuTargetAddRecordTimer
			( EpgPlus* epgPlus
			);

		public:
			int exec
			( CMenuTarget* parent
			, const std::string& actionKey
			);


		private:
			EpgPlus* epgPlus;

	};

	class MenuTargetRefreshEpg : public CMenuTarget
	{
		public:
			MenuTargetRefreshEpg
			( EpgPlus* epgPlus
			);

		public:
			int exec
			( CMenuTarget* parent
			, const std::string& actionKey
			);


		private:
			EpgPlus* epgPlus;

	};

  class MenuOptionChooserSwitchSwapMode : public CMenuOptionChooser
	{
		public:
			MenuOptionChooserSwitchSwapMode
			( EpgPlus* epgPlus
			);

			virtual ~MenuOptionChooserSwitchSwapMode();

		public:
      int exec
			( CMenuTarget* parent
			);

		private:
      int oldTimingMenuSettings;
      TSwapMode oldSwapMode;
			EpgPlus* epgPlus;
  };

  class MenuOptionChooserSwitchViewMode : public CMenuOptionChooser
	{
		public:
			MenuOptionChooserSwitchViewMode
			( EpgPlus* epgPlus
			);

			virtual ~MenuOptionChooserSwitchViewMode();

		public:
      int exec
			( CMenuTarget* parent
			);

		private:
      int oldTimingMenuSettings;
  };

	class MenuTargetSettings : public CMenuTarget
	{
		public:
			MenuTargetSettings
			( EpgPlus* epgPlus
			);

		public:
			int exec
			( CMenuTarget* parent
			, const std::string& actionKey
			);

		private:
			EpgPlus* epgPlus;
	};

	typedef time_t DurationSetting;

	struct Settings
	{
		Settings
		( bool doInit = true
		);

		virtual ~Settings();

		FontSetting*    fontSettings;
		ColorSetting*   colorSettings;
		SizeSetting*    sizeSettings;
		DurationSetting durationSetting;
	};
/*  
  class MenuWidgetFontSetting : public CMenuWidget
  {
	  class MenuTargetSelectFontName : public CMenuTarget
	  {
		  public:
			  MenuTargetSelectFontName
          ( EpgPlus*     epgPlus
          , FontSetting* fontSetting
          );

		  public:
			  int exec
			    ( CMenuTarget* parent
			    , const std::string& actionKey
			    );

		  private:
			  EpgPlus* epgPlus;
        FontSetting* fontSetting;
    };

	  class MenuTargetChangeFontSize : public CMenuTarget
	  {
		  public:
			  MenuTargetChangeFontSize
          ( EpgPlus*     epgPlus
          , FontSetting* fontSetting
          );

		  public:
			  int exec
			    ( CMenuTarget* parent
			    , const std::string& actionKey
			    );

		  private:
			  EpgPlus* epgPlus;
        FontSetting* fontSetting;
    };

	  class MenuOptionChooserFontStyle : public CMenuOptionChooser
	  {
		  public:
			  MenuOptionChooserFontStyle
          ( EpgPlus*     epgPlus
          , FontSetting* fontSetting
          );

		  public:
			  int exec
			    ( CMenuTarget* parent
			    );

		  private:
			  EpgPlus* epgPlus;
        FontSetting* fontSetting;
        int indexFontStyle;
    };

    public:
			MenuWidgetFontSetting
			  ( EpgPlus* epgPlus
        , FontSetting* fontSetting
			  );

		public:
			int exec
			  ( CMenuTarget* parent
			  , const std::string& actionKey
			  );

		private:
			EpgPlus* epgPlus;
      FontSetting* fontSetting;
  };  


  
  class MenuWidgetFontSettings : public CMenuWidget
  {
	  class MenuTargetEditFont : public CMenuTarget
	  {
		  public:
			  MenuTargetEditFont
          ( EpgPlus*     epgPlus
          , FontSetting* fontSetting
          );

		  public:
			  int exec
			    ( CMenuTarget* parent
			    , const std::string& actionKey
			    );

		  private:
			  EpgPlus*     epgPlus;
        FontSetting* fontSetting;
    };

    public:
			MenuWidgetFontSettings
			  ( EpgPlus*      epgPlus
        , FontSettings* fontSettings
			  );

		public:
			int exec
        ( CMenuTarget* parent
        , const std::string& actionKey
			  );

		private:
			EpgPlus*      epgPlus;
      FontSettings* fontSettings;
  };  
  
  class MenuWidgetSizeSettings : public CMenuWidget
  {
	  class MenuTargetEditSize : public CMenuTarget
	  {
		  public:
			  MenuTargetEditSize
          ( EpgPlus*     epgPlus
          , SizeSetting* sizeSetting
          );

		  public:
			  int exec
			    ( CMenuTarget* parent
			    , const std::string& actionKey
			    );

		  private:
			  EpgPlus*     epgPlus;
        SizeSetting* sizeSetting;
    };

    public:
			MenuWidgetSizeSettings
			  ( EpgPlus*      epgPlus
        , SizeSettings* sizeSettings
			  );

		public:
			int exec
        ( CMenuTarget* parent
        , const std::string& actionKey
			  );

		private:
			EpgPlus*      epgPlus;
      SizeSettings* sizeSettings;
  };  
  
  class MenuWidgetSettings : public CMenuWidget
  {
	  class MenuTargetSaveSettings : public CMenuTarget
	  {
		  public:
			  MenuTargetSaveSettings
			  ( EpgPlus*  epgPlus
			  );

		  public:
			  int exec
			  ( CMenuTarget* parent
			  , const std::string& actionKey
			  );

		  private:
			  EpgPlus* epgPlus;
        Settings* settings;
	  };

	  class MenuTargetResetSettings : public CMenuTarget
	  {
		  public:
			  MenuTargetResetSettings
			  ( EpgPlus*  epgPlus
        , Settings* settings
			  );

		  public:
			  int exec
			  ( CMenuTarget* parent
			  , const std::string& actionKey
			  );

		  private:
			  EpgPlus* epgPlus;
        Settings* settings;
	  };

	  class MenuTargetFontSettings : public CMenuTarget
	  {
		  public:
			  MenuTargetFontSettings
			    ( EpgPlus*      epgPlus
          , FontSettings* fontSettings
			    );

		  public:
			  int exec
			    ( CMenuTarget* parent
			    , const std::string& actionKey
			    );

		  private:
				EpgPlus*			epgPlus;
        FontSettings* fontSettings;
	  };

	  class MenuTargetSizeSettings : public CMenuTarget
	  {
		  public:
			  MenuTargetSizeSettings
			    ( EpgPlus*      epgPlus
          , SizeSettings* SizeSettings
			    );

		  public:
			  int exec
			    ( CMenuTarget* parent
			    , const std::string& actionKey
			    );

		  private:
				EpgPlus*			epgPlus;
        SizeSettings* sizeSettings;
	  };


    public:
			MenuWidgetSettings
			  ( EpgPlus* epgPlus
			  );

		public:
			int exec
        ( CMenuTarget* parent
        , const std::string& actionKey
        );

		private:
			EpgPlus* epgPlus;
  };  
*/
  typedef std::map<int, Font*> Fonts;
  typedef std::map<int, int>   Colors;
  typedef std::map<int, int>   Sizes;

  friend class EpgPlus::MenuOptionChooserSwitchSwapMode;
  friend class EpgPlus::MenuOptionChooserSwitchViewMode;
/*  friend class EpgPlus::MenuTargetSettings;
  friend class EpgPlus::MenuWidgetSettings;
*/
  friend class EpgPlus::ChannelEntry;
  friend class EpgPlus::ChannelEventEntry;

//// construction / destruction
public:
  EpgPlus();
  ~EpgPlus();

//// methods
public:
  void init();
  void free();

  int exec 
    ( CChannelList* channelList 
    , int selectedChannelIndex
    , CBouquetList* bouquetList
    ); 

/*  static void loadSettings();

  static void saveSettings();

  static void loadSettings
    ( CConfigFile& configFile
    );

  static void saveSettings
    ( CConfigFile& configFile
    );
*/
private:
  static std::string getTimeString
    ( const time_t& time
    , const std::string& format
    );

  TCChannelEventEntries::const_iterator getSelectedEvent() const;


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

  Header*         header;
  TimeLine*       timeLine;

  CChannelList*   channelList;
  CBouquetList*   bouquetList;

  Footer*         footer;

  ChannelEntry*   selectedChannelEntry;
  time_t          selectedTime;

  int             channelListStartIndex;
  int             maxNumberOfDisplayableEntries; // maximal number of displayable entrys

  time_t          startTime;
  time_t          firstStartTime;
  static time_t   duration;

  int             entryHeight;

  TViewMode       currentViewMode;
  TSwapMode       currentSwapMode;

  int             headerX;
  int             headerY;
  int             headerWidth;

  int             usableScreenWidth;
  int             usableScreenHeight;
  int             usableScreenX;
  int             usableScreenY;

  int             timeLineX;
  int             timeLineY;
  int             timeLineWidth;

  int             channelsTableX;
  int             channelsTableY;
  static int      channelsTableWidth;
  int             channelsTableHeight;

  int             eventsTableX;
  int             eventsTableY;
  int             eventsTableWidth;
  int             eventsTableHeight;

  int             sliderX;
  int             sliderY;
  static int      sliderWidth;
  int             sliderHeight;
  static int      sliderBackColor;
  static int      sliderKnobColor;

  int             footerX;
  int             footerY;
  int             footerWidth;

  int             horGap1X;
  int             horGap1Y;
  int             horGap1Width;
  int             horGap2X;
  int             horGap2Y;
  int             horGap2Width;
  int             verGap1X;
  int             verGap1Y;
  int             verGap1Height;
  int             verGap2X;
  int             verGap2Y;
  int             verGap2Height;

  static int      horGap1Height;
  static int      horGap2Height;
  static int      verGap1Width;
  static int      verGap2Width;

  static int      horGap1Color;
  static int      horGap2Color;
  static int      verGap1Color;
  static int      verGap2Color;

  bool            refreshAll;
  bool            refreshFooterButtons;


  static Settings settings;
  static Fonts    fonts;
  static Colors   colors;
  static Sizes    sizes;
};



class CEPGplusHandler : public CMenuTarget
{
	public:
		int exec( CMenuTarget* parent,  const std::string &actionKey);

};

#endif
