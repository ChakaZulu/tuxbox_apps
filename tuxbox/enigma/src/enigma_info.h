#ifndef __enigma_info_h
#define __enigma_info_h

#include "ewidget.h"
#include "elbwindow.h"

class eListboxEntry;

class eZapInfo: public eLBWindow
{
//	Q_OBJECT
private:// slots:
	void sel_close(eListboxEntry *);
	void sel_streaminfo(eListboxEntry *);
	void sel_bnversion(eListboxEntry *);
	void sel_about(eListboxEntry *);	

public:
	eZapInfo();
	~eZapInfo();
};

#endif /* __enigma_info_h */
