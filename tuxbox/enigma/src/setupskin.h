#ifndef __apps_enigma_setupskin_h
#define __apps_enigma_setupskin_h

#include <core/gui/ewindow.h>

class eButton;
class eListbox;
class eListboxEntry;

class eSkinSetup: public eWindow
{
	eButton *baccept, *breject;
	eListbox *lskins;
	void loadSkins();
	void accept();
	void skinSelected(eListboxEntry *l);
	int keyDown(int rc);
public:
	eSkinSetup();
	~eSkinSetup();
};

#endif
