#ifndef __enigma_event_h
#define __enigma_event_h

#include "ewindow.h"
#include "si.h"

class eLabel;

class eEventDisplay: public eWindow
{
	eString service;
	ePtrList<EITEvent>::iterator *events;
	ePtrList<EITEvent> *eventlist;
	eLabel *title, *long_description, *eventDate, *eventTime, *channel;
	void nextEvent();
	void prevEvent();
protected:
	int eventHandler(const eWidgetEvent &event);
public:
	eEventDisplay(eString service, const ePtrList<EITEvent>* e=0, EITEvent* evt=0);
	~eEventDisplay();
	void setList(const ePtrList<EITEvent> &events);
	void setEvent(EITEvent *event);
};

#endif /* __enigma_event_h */
