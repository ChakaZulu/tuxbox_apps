#ifndef __enigma_setup_h
#define __enigma_setup_h

#include "ewidget.h"
#include "elbwindow.h"

class eListboxEntry;

class eZapSetup: public eWidget
{
	Q_OBJECT
	eLBWindow *window;
private slots:
	void sel_close(eListboxEntry *);
	void sel_bouquet(eListboxEntry *);
	void sel_network(eListboxEntry *);
	void sel_sound(eListboxEntry *);
	void sel_video(eListboxEntry *);
public:
	eZapSetup(eWidget* lcdTitle = 0, eWidget* lcdElement = 0);
	~eZapSetup();
	int exec();
};

#endif /* __enigma_setup_h */
