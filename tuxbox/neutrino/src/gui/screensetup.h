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

		void paint(CFrameBuffer* frameBuffer, FontsDef *fonts);
		void paintBorderUL(CFrameBuffer* frameBuffer, FontsDef *fonts);
		void paintBorderLR(CFrameBuffer* frameBuffer, FontsDef *fonts);
	public:

		CScreenSetup(string Name, SNeutrinoSettings* Settings);

		void hide(CFrameBuffer* frameBuffer);
		int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput *rcInput, CMenuTarget* parent, string actionKey );

};


#endif
