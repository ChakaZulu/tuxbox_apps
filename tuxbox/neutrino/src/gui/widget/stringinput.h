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
	int hheight, mheight; // head/menu font height
	
	string	name;
	char*	value;
	int		size;
	int		selected;

    void paint();
    void paintChar(int pos);

	public:

		CStringInput(string Name, char* Value, int Size);

		void hide();
		int exec( CMenuTarget* parent, string actionKey );

};


#endif


