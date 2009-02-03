#ifndef __enigma_scan_h
#define __enigma_scan_h

#include <setup_window.h>
#include <lib/gui/ewidget.h>

class eLNB;
class eListboxEntry;

class eLNBSelector: public eListBoxWindow<eListBoxEntryText>
{
	private:
		void selected( eListBoxEntryText* e);
		void init_eLNBSelector();
	public:
		eLNBSelector();
};

class eZapScan: public eSetupWindow
{
private:
	void sel_satconfig();
	void sel_rotorConfig();
	void sel_transponderEdit();
	void sel_autoScan();
	void sel_multiScan();
	void sel_manualScan();
	void sel_satfind();
	void init_eZapScan();
public:
	static eLNB* getRotorLNB(int silent);
	eZapScan();
};

#endif /* __enigma_scan_h */
