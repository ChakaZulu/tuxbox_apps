#ifndef __engrab_h
#define __engrab_h

#include <lib/gui/listbox.h>
#include <lib/gui/ewindow.h>
#include <lib/socket/socket.h>

class ENgrab: public Object
{
	eString sendStr;
	eString startxml();
	eString stopxml();
	void sending();
	void connected();
	void connectionClosed();
	void connectionTimeouted();
	void dataWritten( int );
	eSocket *sd;
	eTimer timeout;
	ENgrab();
	~ENgrab();
public:
	static ENgrab *getNew() { return new ENgrab(); }
	void sendstart();
	void sendstop();
};

class ENgrabWnd:public eWindow
{
	eListBox<eListBoxEntryMenu> lb;	
	void onBackSelected();
	void manualStart() { ENgrab::getNew()->sendstart(); }
	void manualStop() { ENgrab::getNew()->sendstop(); }
public:
	ENgrabWnd();
};

#endif
