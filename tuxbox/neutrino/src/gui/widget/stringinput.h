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
	int hheight, mheight, iheight; // head/menu font height
	
	string	name;
    string  hint_1, hint_2;
    char*   validchars;
	char*	value;
	int		size;
	int		selected;
    CChangeObserver*   observ;

    void paint();
    void paintChar(int pos);

	public:

		CStringInput(string Name, char* Value, int Size, string Hint_1 = "", string Hint_2 = "", char* Valid_Chars= "0123456789. ", CChangeObserver* Observ = NULL);

		void hide();
		int exec( CMenuTarget* parent, string actionKey );

};


#endif


