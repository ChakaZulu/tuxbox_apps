#ifndef __enigma_mainmenu_h
#define __enigma_mainmenu_h

#include <include/libsig_comp.h>
#include <core/gui/ewidget.h>

class gPixmap;
class eLabel;

class eMainMenu: public eWidget
{
	gPixmap *pixmaps[7][2];
	eLabel *label[7], *description;
	int active;
	void setActive(int i);
	void sel_tv();
	void sel_radio();
	void sel_vcr();
	void sel_setup();
	void sel_info();	
	void sel_quit();
	void sel_plugins();
protected:
	int eventHandler(const eWidgetEvent &event);
public:
	eMainMenu();
};

#endif
