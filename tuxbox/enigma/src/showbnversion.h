#ifndef __showbnversion_h_
#define __showbnversion_h_

#include "ewindow.h"

class eLabel;
class BNDirectory;

class ShowBNVersion: public eWindow
{
	eLabel *text, *res1, *res2;
	BNDirectory *bnd[2];
protected:
	void willShow();
	void willHide();
	int keyUp(int rc);
private:
	void eventOccured(int event);
public:
	ShowBNVersion();
	~ShowBNVersion();
};

#endif
