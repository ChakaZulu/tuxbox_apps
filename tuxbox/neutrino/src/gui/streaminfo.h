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

		void paint(CFrameBuffer* frameBuffer, FontsDef *fonts);

	public:

		CStreamInfo();

		void hide(CFrameBuffer* frameBuffer);
		int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput *rcInput, CMenuTarget* parent, string actionKey );

};



#endif
