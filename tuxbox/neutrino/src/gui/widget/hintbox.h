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
		string					text;

		CMenuWidget*			parent;

	public:
		CHintBox( CMenuWidget* Parent, string Caption, string Text);

		void paint();
		void hide( bool showParent=true);
	};

#endif
