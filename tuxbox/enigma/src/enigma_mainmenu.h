#ifndef __enigma_mainmenu_h
#define __enigma_mainmenu_h

#include "elbwindow.h"

#include <sigc++/signal_system.h>
#ifdef SIGC_CXX_NAMESPACES
using namespace SigC;
#endif

class eListBoxEntry;

class eMainMenu: public /*Q*/Object
{
//	Q_OBJECT;
	eLBWindow* window;
private:// slots:
	void sel_close(eListboxEntry *);
	void sel_vcr(eListboxEntry *);
	void sel_scan(eListboxEntry *);
	void sel_setup(eListboxEntry *);
	void sel_streaminfo(eListboxEntry *);
	void sel_quit(eListboxEntry *);
	void sel_bnversion(eListboxEntry *);
	void sel_plugins(eListboxEntry *);
	void sel_about(eListboxEntry *);
public:
	eMainMenu();
	~eMainMenu();
	void setLCD(eWidget *a, eWidget *b);
	int exec();
};

#endif
