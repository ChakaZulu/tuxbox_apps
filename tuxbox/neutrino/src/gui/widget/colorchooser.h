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

	CChangeObserver* observer;

		void paint();
		void setColor();
		void paintSlider(int x, int y, unsigned char *spos, string text, string iconname, bool selected);

	public:

		CColorChooser(string Name, unsigned char *R, unsigned char *G, unsigned char *B, unsigned char* Alpha, CChangeObserver* Observer = NULL);

		void hide();
		int exec( CMenuTarget* parent, string actionKey );

};


#endif

