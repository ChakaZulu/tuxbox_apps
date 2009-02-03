#ifndef __wizard_scan_h
#define __wizard_scan_h

#include <lib/gui/listbox.h>
#include <lib/gui/ewindow.h>

class eDiseqcChoice;

class eWizardSelectDiseqc: public eWindow
{
	eListBox<eDiseqcChoice> *diseqclist;
	eLabel *description;
	void selected(eDiseqcChoice *choice);
	void selchanged(eDiseqcChoice *choice);
	void init_eWizardSelectDiseqc();
public:
	eWizardSelectDiseqc();
	static int run();
};

#endif
