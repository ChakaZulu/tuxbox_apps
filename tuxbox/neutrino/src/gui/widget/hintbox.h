#ifndef __hintbox__
#define __hintbox__

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "menue.h"

#include <string>
#include <vector>

using namespace std;

	class CHintBox
	{
		int						width;
		int						height;
		int						x;
		int						y;
		int						fheight;
		int						theight;
		string					caption;
		vector<string>			text;

		CMenuWidget*			parent;
		unsigned char*			pixbuf;


	public:
		CHintBox( CMenuWidget* Parent, string Caption, string Text, int Width = 500 );

		void paint( bool saveScreen = true );
		void hide();
	};

#endif
