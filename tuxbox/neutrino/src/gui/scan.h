#ifndef __scants__
#define __scants__

#include <stdio.h>
#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../driver/rcinput.h"

#include "../widget/menue.h"
#include "../widget/color.h"
#include "../../../libucodes/libucodes.h"

#include <string>

using namespace std;


class CScanTs : public CMenuTarget
{
		int x;
		int y;
		int width;
		int height;
		int hheight,mheight; // head/menu font height
		
		void paint();
		bool scanReady(int *ts, int *services);
		void startScan();

	public:

		CScanTs();

		void hide();
		int exec( CMenuTarget* parent, string actionKey );

};



#endif

