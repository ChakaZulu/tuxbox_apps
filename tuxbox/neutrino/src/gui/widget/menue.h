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


#ifndef __MENU__
#define __MENU__

#include <driver/framebuffer.h>
#include <driver/rcinput.h>

#include <string>
#include <vector>

struct menu_return
{
	enum
		{
			RETURN_NONE	= 0,
			RETURN_REPAINT 	= 1,
			RETURN_EXIT 	= 2,
			RETURN_EXIT_ALL = 4
		};
};

class CChangeObserver
{
	public:
		virtual ~CChangeObserver(){}
		virtual bool changeNotify(std::string OptionName, void *Data)
		{
			return false;
		}
};

class COnPaintNotifier
{
	public:
		virtual ~COnPaintNotifier(){}
		virtual bool onPaintNotify(std::string MenuName)
		{
			return false;
		}
};

class CMenuTarget
{
	public:

		CMenuTarget(){}
		virtual ~CMenuTarget(){}
		virtual void hide(){}
		virtual int exec(CMenuTarget* parent, std::string actionKey)
		{
			return 0;
		}
};


class CMenuItem
{
	protected:
		CFrameBuffer	*frameBuffer;
		int x, y, dx, offx;
		bool		active;
	public:
		int		directKey;
		std::string	iconName;

		CMenuItem()
		{
			directKey = -1;
			iconName = "";
		}
		virtual ~CMenuItem(){}

		virtual void init(int X, int Y, int DX, int OFFX)
		{
			x=X;
			y=Y;
			dx=DX;
			offx=OFFX;
		}
		virtual int paint(bool selected=false)
		{
			return -1;
		}

		virtual int getHeight(void) const = 0;

		virtual bool isSelectable(void) const
		{
			return false;
		}

		virtual int exec(CMenuTarget* parent)
		{
			return 0;
		}
		virtual void setActive( bool Active)
		{
			active = Active;
			paint();
		};
};

class CMenuSeparator : public CMenuItem
{
		int		height;
		int		type;
		std::string	text;

	public:
		enum
		{
		    EMPTY =	0,
		    LINE =	1,
		    STRING =	2,
		    ALIGN_CENTER = 4,
		    ALIGN_LEFT =   8,
		    ALIGN_RIGHT = 16
		};


		CMenuSeparator(const int Type = 0, const char * const Text = NULL); // UTF-8

		int paint(bool selected=false);
		int getHeight(void) const
		{
			return height;
		}
};

class CMenuForwarder : public CMenuItem
{
		int		height;
		std::string         text;
		const char *        option;
		const std::string * option_string;
		CMenuTarget*	jumpTarget;
		std::string         actionKey;
		bool		localizing;
	public:

		// Text must be UTF-8 encoded:
		CMenuForwarder(const char * const Text, const bool Active=true, const char * const Option=NULL, CMenuTarget* Target=NULL, std::string ActionKey="", bool Localizing= true, uint DirectKey= CRCInput::RC_nokey, std::string IconName= "");
		CMenuForwarder(const char * const Text, const bool Active, const std::string &Option, CMenuTarget* Target=NULL, std::string ActionKey="", bool Localizing= true, uint DirectKey= CRCInput::RC_nokey, std::string IconName= "");
		int paint(bool selected=false);
		int getHeight(void) const
		{
			return height;
		}
		int exec(CMenuTarget* parent);
		bool isSelectable(void) const
		{
			return active;
		}
};

class CMenuOptionChooser : public CMenuItem
{
		struct keyval
		{
			int key;
			std::string value;
		};

		std::vector<keyval *> options;
		int                   height;
		std::string           optionName;
		int *                 optionValue;
		CChangeObserver *     observ;
		bool                  localizing;

	public:
		CMenuOptionChooser(const char * const OptionName, int * const OptionValue, const bool Active = false, CChangeObserver * const Observ = NULL, const bool Localizing = true, const uint DirectKey = CRCInput::RC_nokey, const std::string IconName= ""); // UTF-8
		~CMenuOptionChooser();


		void addOption(const int key, const char * const value_utf8_encoded); // UTF-8
		void removeAllOptions();
		void setOptionValue(const int newvalue);
		int getOptionValue(void) const;

		int paint(bool selected);
		int getHeight(void) const
		{
			return height;
		}
		bool isSelectable(void) const
		{
			return active;
		}

		int exec(CMenuTarget* parent);
};

class CMenuOptionStringChooser : public CMenuItem
{
		std::vector<std::string> options;
		int                      height;
		std::string              optionName;
		char *                   optionValue;
		CChangeObserver *        observ;
		bool                     localizing;

	public:
		// OptionName: UTF-8 encoded
		CMenuOptionStringChooser(const char * const OptionName, char* OptionValue, bool Active = false, CChangeObserver* Observ = NULL, bool Localizing= true);
		~CMenuOptionStringChooser();

		void addOption(std::string value);
		int paint(bool selected);
		int getHeight(void) const
		{
			return height;
		}
		bool isSelectable(void) const
		{
			return active;
		}

		int exec(CMenuTarget* parent);
};


class CMenuWidget : public CMenuTarget
{
	protected:
		CFrameBuffer		*frameBuffer;
		COnPaintNotifier*	onPaintNotifier;
		std::vector<CMenuItem*>	items;
		std::vector<unsigned int> page_start;
		std::vector<unsigned int> page_end;
		std::string			name;
		std::string			iconfile;
		bool			localizing;

		int			width;
		int			height;
		int         wanted_height;
		int			x;
		int			y;
		int			selected;
		int 			iconOffset;
		unsigned int         item_start_y;
		unsigned int         current_page;
		unsigned int         total_pages;
		virtual void paintItems();

	public:
		CMenuWidget()
		{
			name="";
			iconfile="";
			selected=-1;
			onPaintNotifier=NULL;
			iconOffset= 0;
		};
		// Name must be UTF-8 encoded:
		CMenuWidget(const char * const Name, const std::string Icon = "", const int mwidth = 400, const int mheight = 576, const bool Localizing = true);
		~CMenuWidget();

		virtual void addItem(CMenuItem* menuItem, bool defaultselected=false);
		virtual void paint();
		virtual void hide();
		virtual int exec(CMenuTarget* parent, std::string actionKey);

		void setOnPaintNotifier( COnPaintNotifier* );
		void setName(std::string Name)
		{
			name=Name;
		}
		void setIcon(std::string Icon)
		{
			iconfile=Icon;
		}
};

class CPINProtection
{
	protected:
		char* validPIN;
		bool check();
		virtual CMenuTarget* getParent() = NULL;
	public:
		CPINProtection( char* validpin){ validPIN = validpin;};
};

class CZapProtection : public CPINProtection
{
	protected:
		virtual CMenuTarget* getParent() { return( NULL);};
	public:
		int	fsk;

		CZapProtection( char* validpin, int	FSK ) : CPINProtection(validpin){ fsk= FSK; };
		bool check();
};

class CLockedMenuForwarder : public CMenuForwarder, public CPINProtection
{
	CMenuTarget* Parent;
	bool AlwaysAsk;

	protected:
		virtual CMenuTarget* getParent(){ return Parent;};
	public:
		CLockedMenuForwarder(const char * const Text, char* validPIN, bool alwaysAsk=false, const bool Active=true, char *Option=NULL,
		                     CMenuTarget* Target=NULL, std::string ActionKey="", bool Localizing= true,
		                     uint DirectKey= CRCInput::RC_nokey, std::string IconName= "")

		                     : CMenuForwarder(Text, Active, Option, Target, ActionKey, Localizing,
		                     DirectKey, IconName) ,
		                       CPINProtection( validPIN){AlwaysAsk = alwaysAsk;};

		virtual int exec(CMenuTarget* parent);
};


#endif
