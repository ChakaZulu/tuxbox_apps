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
		CKeyChooser(int *Key, string title, string Icon="" );
		~CKeyChooser();

		void paint(CFrameBuffer* frameBuffer, FontsDef *fonts);
};

class CKeyChooserItem : public CMenuTarget
{
	int x;
	int y;
	int width;
	int height;

	string	name;
	int		*key;

		void paint(CFrameBuffer* frameBuffer, FontsDef *fonts);

	public:

		CKeyChooserItem(string Name, int *Key);

		void hide(CFrameBuffer* frameBuffer);
		int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput *rcInput, CMenuTarget* parent, string actionKey );

};

class CKeyChooserItemNoKey : public CMenuTarget
{
	int		*key;

	public:

		CKeyChooserItemNoKey(int *Key){key=Key;};

		int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput *rcInput, CMenuTarget* parent, string actionKey )
			{*key=CRCInput::RC_nokey;return CMenuTarget::RETURN_REPAINT;}

};


#endif
