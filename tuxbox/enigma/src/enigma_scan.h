#ifndef __enigma_scan_h
#define __enigma_scan_h

#include "ewidget.h"
#include "elbwindow.h"

class eListboxEntry;

class eZapScan: public eLBWindow
{
//	Q_OBJECT
private:// slots:
	void sel_close(eListboxEntry *);
	void sel_scan(eListboxEntry *);
	void sel_bouquet(eListboxEntry *);
	void sel_satconfig(eListboxEntry *);	

public:
	eZapScan();
	~eZapScan();
};

#endif /* __enigma_scan_h */
