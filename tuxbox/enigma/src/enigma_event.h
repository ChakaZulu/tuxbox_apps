#ifndef __enigma_event_h
#define __enigma_event_h

#include "ewindow.h"
#include <qlist.h>
#include <qstring.h>
#include "si.h"

class eLabel;

class eEventDisplay: public eWindow
{
	Q_OBJECT
	QString service;
	QListIterator<EITEvent> *events;
	QList<EITEvent> *eventlist;
	eLabel *title, *long_description, *eventDate, *eventTime, *channel;
protected:
	void keyDown(int rc);
	void keyUp(int rc);
public:
	eEventDisplay(QString service, const QList<EITEvent>* e=0, EITEvent* evt=0);
	~eEventDisplay();
	
	void setEvent(EITEvent *event);
	void setList(const QList<EITEvent> &events);
};

#endif /* __enigma_event_h */
