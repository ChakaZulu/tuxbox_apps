#ifndef __MENU__
#define __MENU__

#include <stdio.h>
#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../driver/rcinput.h"
#include "color.h"

#include <string>
#include <vector>

using namespace std;




class CChangeObserver
{
	public:
		virtual ~CChangeObserver(){};
		virtual void changeNotify(string OptionName){};
};

class CMenuTarget
{
	public:
		enum
		{
			RETURN_REPAINT = 1,
			RETURN_EXIT = 2,
			RETURN_EXIT_ALL = 4
		};
	
	CMenuTarget(){};
	virtual ~CMenuTarget(){};
	virtual void hide(CFrameBuffer* frameBuffer){};
	virtual int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput* rcInput, CMenuTarget* parent, string actionKey){return 0;};
};


class CMenuItem
{
	protected:
		int x, y, dx;
	public:

		enum
		{
			RETURN_REPAINT = 1,
			RETURN_EXIT = 2,
			RETURN_EXIT_ALL = 4
		};

		CMenuItem(){};
		virtual ~CMenuItem(){};

		virtual void init(int X, int Y, int DX){x=X;y=Y;dx=DX;}
		virtual int paint(CFrameBuffer*	frameBuffer, FontsDef* fonts, bool selected=false){return -1;};
		virtual int getHeight(){return -1;};
		virtual bool isSelectable(){return false;};

		virtual int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput* rcInput, CMenuTarget* parent){return 0;};
};

class CMenuSeparator : public CMenuItem
{
	int		height;
	int		type;
	string	text;

	public:
		enum
		{
			EMPTY = 0,
			LINE = 1,
			STRING = 2,
			ALIGN_CENTER = 4,
			ALIGN_LEFT = 8,
			ALIGN_RIGHT = 16
		};

		
		CMenuSeparator(int Height=5, int Type=0, string Text="");

		int paint(CFrameBuffer*	frameBuffer, FontsDef* fonts, bool selected=false);
		int getHeight(){return height;};
};

class CMenuForwarder : public CMenuItem
{
	int				height;
	string			text;
	char*			option;
	bool			active;
	CMenuTarget*	jumpTarget;
	string			actionKey;

	public:
		
		CMenuForwarder(string Text, bool Active=true, char *Option=NULL, CMenuTarget* Target=NULL, string ActionKey="");
		int paint(CFrameBuffer* frameBuffer, FontsDef *fonts, bool selected=false);
		int getHeight(){return height;};
		int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput* rcInput, CMenuTarget* parent);
		bool isSelectable(){return active;};
};

class CMenuOptionChooser : public CMenuItem
{
	struct keyval
	{
		int key;
		string value;
	};

	vector<keyval*>		options;
	int					height;
	string				optionName;
	bool				active;
	int*				optionValue;
	CChangeObserver*	observ;

	public:
		CMenuOptionChooser(){};
		CMenuOptionChooser(string OptionName, int* OptionValue, bool Active = false, CChangeObserver* Observ = NULL);
		~CMenuOptionChooser();

		void addOption(int key, string value);
		int paint(CFrameBuffer* frameBuffer, FontsDef *fonts, bool selected);
		int getHeight(){return height;};
		bool isSelectable(){return active;};

		int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput* rcInput, CMenuTarget* parent);
};

class CMenuWidget : public CMenuTarget
{
	protected:
		vector<CMenuItem*>	items;
		string				name;
		string				iconfile;

		int					width;
		int					height;
		int					x;
		int					y;
		int					selected;

	public:
		CMenuWidget(){name="";iconfile="";selected = -1;};
		CMenuWidget(string Name, string Icon="");
		~CMenuWidget();

		virtual void addItem(CMenuItem* menuItem, bool defaultselected=false);
		virtual void paint(CFrameBuffer* frameBuffer, FontsDef *fonts);
		virtual void hide(CFrameBuffer* frameBuffer);
		virtual int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput* rcInput, CMenuTarget* parent, string actionKey);

		void setName(string Name){name=Name;};
		void setIcon(string Icon){iconfile=Icon;};
};


#endif
