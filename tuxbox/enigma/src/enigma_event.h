#ifndef __enigma_event_h
#define __enigma_event_h

#include "ewindow.h"
#include <qstring.h>
#include "si.h"

class eLabel;

class eEventDisplay: public eWindow
{
	QString service;
	ePtrList<EITEvent>::iterator *events;
	ePtrList<EITEvent> *eventlist;
	eLabel *title, *long_description, *eventDate, *eventTime, *channel;
protected:
	void keyDown(int rc);
	void keyUp(int rc);
public:
	eEventDisplay(QString service, const ePtrList<EITEvent>* e=0, EITEvent* evt=0);
	~eEventDisplay();
	
	void setEvent(EITEvent *event);
	void setList(const ePtrList<EITEvent> &events);
};

#endif /* __enigma_event_h */
