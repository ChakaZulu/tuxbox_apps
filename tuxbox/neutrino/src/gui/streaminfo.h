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
		int hheight,mheight; // head/menu font height
		
		void paint();

	public:

		CStreamInfo();

		void hide();
		int exec( CMenuTarget* parent, string actionKey );

};



#endif

