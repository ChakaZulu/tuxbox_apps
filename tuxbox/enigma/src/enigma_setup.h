#ifndef __enigma_setup_h
#define __enigma_setup_h

#include <core/gui/listbox.h>

class eZapSetup: public eListBoxWindow<eListBoxEntryMenu>
{
private:
	void sel_close();
	void sel_channels();	
	void sel_network();
	void sel_sound();
	void sel_video();
	void sel_language();
	void sel_skin();
	void sel_osd();
	void sel_lcd();
	
public:
	eZapSetup();
	~eZapSetup();
};

#endif /* __enigma_setup_h */
