#ifndef __stringinput__
#define __stringinput__

#include <stdio.h>
#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../driver/rcinput.h"

#include "menue.h"
#include "color.h"

#include <string>

using namespace std;

class CStringInput : public CMenuTarget
{
	int x;
	int y;
	int width;
	int height;

	string	name;
	char*	value;
	int		size;
	int		selected;

		void paint(CFrameBuffer* frameBuffer, FontsDef *fonts);
		void paintChar(CFrameBuffer* frameBuffer, FontsDef *fonts, int pos);

	public:

		CStringInput(string Name, char* Value, int Size);

		void hide(CFrameBuffer* frameBuffer);
		int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput *rcInput, CMenuTarget* parent, string actionKey );

};


#endif
