#ifndef __enigma_setup_h
#define __enigma_setup_h

#include "ewidget.h"
#include "elbwindow.h"

class eListboxEntry;

class eZapSetup: public eLBWindow
{
//	Q_OBJECT
private:// slots:
	void sel_close(eListboxEntry *);
	void sel_bouquet(eListboxEntry *);
	void sel_network(eListboxEntry *);
	void sel_sound(eListboxEntry *);
	void sel_video(eListboxEntry *);
	void sel_satconfig(eListboxEntry *);
public:
	eZapSetup();
	~eZapSetup();
};

#endif /* __enigma_setup_h */
