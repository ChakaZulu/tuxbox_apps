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
	int					selected;
	
		void paint();
		void paintBorderUL();
		void paintBorderLR();
	public:

		void hide();
		int exec( CMenuTarget* parent, string actionKey );

};


#endif

