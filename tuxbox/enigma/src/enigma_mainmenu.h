#ifndef __enigma_mainmenu_h
#define __enigma_mainmenu_h

#include <core/gui/listbox.h>
#include <include/libsig_comp.h>

class eMainMenu: public Object
{
	eListBoxWindow<eListBoxEntryMenu> window;
private:
	void sel_close();
	void sel_vcr();
	void sel_setup();
	void sel_info();	
	void sel_quit();
	void sel_plugins();
public:
	eMainMenu();
	void setLCD(eWidget *a, eWidget *b);
	int exec();
};

#endif
