#ifndef __screensetup__
#define __screensetup__

#include <stdio.h>
#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../driver/rcinput.h"
#include "../helpers/settings.h"

#include "menue.h"
#include "color.h"

#include <string>

using namespace std;

class CScreenSetup : public CMenuTarget
{
	string				name;
	SNeutrinoSettings	*settings;
	int					selected;
	FontsDef		*fonts;
	
		void paint(CFrameBuffer* frameBuffer);
		void paintBorderUL(CFrameBuffer* frameBuffer);
		void paintBorderLR(CFrameBuffer* frameBuffer);
	public:

		CScreenSetup(string Name, FontsDef *Fonts, SNeutrinoSettings* Settings);

		void hide(CFrameBuffer* frameBuffer);
		int exec(CFrameBuffer* frameBuffer, CRCInput *rcInput, CMenuTarget* parent, string actionKey );

};


#endif

