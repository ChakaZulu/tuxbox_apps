#ifndef __colorchooser__
#define __colorchooser__

#include <stdio.h>
#include "../driver/framebuffer.h"
#include "../driver/fontrenderer.h"
#include "../driver/rcinput.h"

#include "menue.h"
#include "color.h"

#include <string>

using namespace std;

class CColorChooser : public CMenuTarget
{
	int x;
	int y;
	int width;
	int height;
	int hheight,mheight; // head/menu font height
	
	unsigned char *r;
	unsigned char *g;
	unsigned char *b;
	unsigned char *alpha;

	string	name;
	FontsDef *fonts;

	CChangeObserver* observer;

		void paint(CFrameBuffer* frameBuffer);
		void setColor(CFrameBuffer* frameBuffer);
		void paintSlider(CFrameBuffer* frameBuffer, int x, int y, unsigned char *spos, string text, bool selected);

	public:

		CColorChooser(string Name, FontsDef *Fonts, unsigned char *R, unsigned char *G, unsigned char *B, unsigned char* Alpha, CChangeObserver* Observer = NULL);

		void hide(CFrameBuffer* frameBuffer);
		int exec(CFrameBuffer* frameBuffer, CRCInput *rcInput, CMenuTarget* parent, string actionKey );

};

#endif
