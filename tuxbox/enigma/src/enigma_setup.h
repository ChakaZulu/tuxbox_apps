#ifndef __enigma_setup_h
#define __enigma_setup_h

#include <core/gui/elbwindow.h>

class eListboxEntry;

class eZapSetup: public eLBWindow
{
private:
	void sel_close(eListboxEntry *);
	void sel_bouquet(eListboxEntry *);
	void sel_network(eListboxEntry *);
	void sel_sound(eListboxEntry *);
	void sel_video(eListboxEntry *);
	void sel_satconfig(eListboxEntry *);
	void sel_language(eListboxEntry *);
public:
	eZapSetup();
	~eZapSetup();
};

#endif /* __enigma_setup_h */
