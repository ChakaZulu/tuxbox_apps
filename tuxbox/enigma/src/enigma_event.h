#ifndef __enigma_event_h
#define __enigma_event_h

#include <lib/gui/ewindow.h>
#include <lib/dvb/si.h>

class eLabel;
class eProgress;

class eEventDisplay: public eWindow
{
	eString service;
	ePtrList<EITEvent>::iterator *events;
	ePtrList<EITEvent> *eventlist;
	eWidget *descr;
	eLabel *long_description, *eventDate, *eventTime, *channel;
	eProgress *scrollbar;
	void nextEvent();
	void prevEvent();
	int total;
	void updateScrollbar();
protected:
	int eventHandler(const eWidgetEvent &event);
public:
	eEventDisplay(eString service, const ePtrList<EITEvent>* e=0, EITEvent* evt=0);
	~eEventDisplay();
	void setList(const ePtrList<EITEvent> &events);
	void setEvent(EITEvent *event);
};

#endif /* __enigma_event_h */
