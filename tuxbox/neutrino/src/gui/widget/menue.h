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
		virtual bool changeNotify(const std::string & OptionName, void *Data)
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
		virtual int exec(CMenuTarget* parent, const std::string & actionKey) = 0;
};


class CMenuItem
{
	protected:
		int x, y, dx, offx;
		bool           active;
	public:
		neutrino_msg_t directKey;
		std::string    iconName;

		CMenuItem()
		{
			directKey = CRCInput::RC_nokey;
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
		virtual int paint (bool selected = false) = 0;

		virtual int getHeight(void) const = 0;

		virtual bool isSelectable(void) const
		{
			return false;
		}

		virtual int exec(CMenuTarget* parent)
		{
			return 0;
		}
		virtual void setActive(const bool Active)
		{
			active = Active;
			if (x != -1)
				paint();
		};
};

class CMenuSeparator : public CMenuItem
{
		int		type;

	public:
		std::string	text;

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
		int getHeight(void) const;
};

class CMenuForwarder : public CMenuItem
{
	const char *        option;
	const std::string * option_string;
	CMenuTarget *       jumpTarget;
	std::string         actionKey;
	bool                localizing;

 protected:
	std::string          text;

	virtual const char * getOption(void);
	
 public:

		// Text must be UTF-8 encoded:
		CMenuForwarder(const char * const Text, const bool Active=true, const char * const Option=NULL, CMenuTarget* Target=NULL, const char * const ActionKey = NULL, const bool Localizing = true, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const char * const IconName = NULL);
		CMenuForwarder(const char * const Text, const bool Active, const std::string &Option, CMenuTarget* Target=NULL, const char * const ActionKey = NULL, const bool Localizing = true, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const char * const IconName = NULL);
		int paint(bool selected=false);
		int getHeight(void) const;
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
		CMenuOptionChooser(const char * const OptionName, int * const OptionValue, const bool Active = false, CChangeObserver * const Observ = NULL, const bool Localizing = true, const neutrino_msg_t DirectKey = CRCInput::RC_nokey, const std::string & IconName= ""); // UTF-8
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

		void addOption(const char * value);
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
		std::vector<CMenuItem*>	items;
		std::vector<unsigned int> page_start;
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
			iconOffset= 0;
		};
		// Name must be UTF-8 encoded:
		CMenuWidget(const char * const Name, const std::string & Icon = "", const int mwidth = 400, const int mheight = 576, const bool Localizing = true);
		~CMenuWidget();

		virtual void addItem(CMenuItem* menuItem, const bool defaultselected = false);
		virtual void paint();
		virtual void hide();
		virtual int exec(CMenuTarget* parent, const std::string & actionKey);
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
		                     CMenuTarget* Target=NULL, const char * const ActionKey = NULL, const bool Localizing = true,
		                     neutrino_msg_t DirectKey = CRCInput::RC_nokey, const char * const IconName = NULL)

		                     : CMenuForwarder(Text, Active, Option, Target, ActionKey, Localizing,
		                     DirectKey, IconName) ,
		                       CPINProtection( validPIN){AlwaysAsk = alwaysAsk;};

		virtual int exec(CMenuTarget* parent);
};

extern CMenuSeparator * const GenericMenuSeparator;
extern CMenuSeparator * const GenericMenuSeparatorLine;
extern CMenuForwarder * const GenericMenuBack;



#endif
