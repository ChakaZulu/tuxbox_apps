#ifndef __showbnversion_h_
#define __showbnversion_h_

#include "ewindow.h"

class eLabel;
class BNDirectory;

class ShowBNVersion: public eWindow
{
//	Q_OBJECT
	eLabel *text, *res1, *res2;
	BNDirectory *bnd[2];
protected:
	void willShow();
	void willHide();
	void keyUp(int rc);
private:// slots:
	void eventOccured(int event);
public:
	ShowBNVersion();
	~ShowBNVersion();
};

#endif
