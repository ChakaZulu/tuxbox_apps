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

	unsigned char *r;
	unsigned char *g;
	unsigned char *b;
	unsigned char *alpha;

	string	name;

	CChangeObserver* observer;

		void paint(CFrameBuffer* frameBuffer, FontsDef *fonts);
		void setColor(CFrameBuffer* frameBuffer, FontsDef *fonts);
		void paintSlider(CFrameBuffer* frameBuffer, FontsDef *fonts, int x, int y, unsigned char *spos, string text, bool selected);

	public:

		CColorChooser(string Name, unsigned char *R, unsigned char *G, unsigned char *B, unsigned char* Alpha, CChangeObserver* Observer = NULL);

		void hide(CFrameBuffer* frameBuffer);
		int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput *rcInput, CMenuTarget* parent, string actionKey );

};



#endif
