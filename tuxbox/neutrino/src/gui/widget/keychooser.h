#ifndef __keychooser__
#define __keychooser__

#include <stdio.h>
#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../driver/rcinput.h"

#include "menue.h"
#include "color.h"

#include <string>

using namespace std;

class CKeyChooserItem;
class CKeyChooserItemNoKey;
class CKeyChooser : public CMenuWidget
{
	int*					key;
	CKeyChooserItem			*keyChooser;
	CKeyChooserItemNoKey	*keyDeleter;

	public:
		CKeyChooser(int *Key, string title, FontsDef *fonts, string Icon="" );
		~CKeyChooser();

		void paint(CFrameBuffer* frameBuffer);
};

class CKeyChooserItem : public CMenuTarget
{
	int x;
	int y;
	int width;
	int height;
	int hheight,mheight; // head/menu font height
	
	string	name;
	int		*key;
	FontsDef	*fonts;
	
		void paint(CFrameBuffer* frameBuffer);

	public:

		CKeyChooserItem(string Name, FontsDef *fonts, int *Key);

		void hide(CFrameBuffer* frameBuffer);
		int exec(CFrameBuffer* frameBuffer, CRCInput *rcInput, CMenuTarget* parent, string actionKey );

};

class CKeyChooserItemNoKey : public CMenuTarget
{
	int		*key;

	public:

		CKeyChooserItemNoKey(int *Key){key=Key;};

		int exec(CFrameBuffer* frameBuffer, CRCInput *rcInput, CMenuTarget* parent, string actionKey )
			{*key=CRCInput::RC_nokey;return CMenuTarget::RETURN_REPAINT;}

};


#endif
