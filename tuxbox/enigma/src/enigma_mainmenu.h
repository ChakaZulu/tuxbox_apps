#ifndef __enigma_mainmenu_h
#define __enigma_mainmenu_h

#include <core/gui/elbwindow.h>
#include <include/libsig_comp.h>

class eListBoxEntry;

class eMainMenu: public Object
{
	eLBWindow* window;
private:
	void sel_close(eListboxEntry *);
	void sel_vcr(eListboxEntry *);
	void sel_setup(eListboxEntry *);
	void sel_info(eListboxEntry *);	
	void sel_quit(eListboxEntry *);
	void sel_plugins(eListboxEntry *);
public:
	eMainMenu();
	~eMainMenu();
	void setLCD(eWidget *a, eWidget *b);
	int exec();
};

#endif
