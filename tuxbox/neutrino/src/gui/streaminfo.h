#ifndef __streaminfo__
#define __streaminfo__

#include <stdio.h>
#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../driver/rcinput.h"

#include "../widget/menue.h"
#include "../widget/color.h"

#include <string>

using namespace std;


class CStreamInfo : public CMenuTarget
{
		int x;
		int y;
		int width;
		int height;
		FontsDef *fonts;

		void paint(CFrameBuffer* frameBuffer);

	public:

		CStreamInfo(FontsDef *Fonts);

		void hide(CFrameBuffer* frameBuffer);
		int exec(CFrameBuffer* frameBuffer, CRCInput *rcInput, CMenuTarget* parent, string actionKey );

};



#endif
