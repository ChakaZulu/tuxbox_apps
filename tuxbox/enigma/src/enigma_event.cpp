#include <time.h>
#include "enigma_event.h"
#include "rc.h"
#include "font.h"
#include "elabel.h"
#include "enigma.h"


void eEventDisplay::keyDown(int rc)
{
	switch (rc)
	{
	case eRCInput::RC_RIGHT:
		++(*events);
		if (!events->current())
			events->toFirst();
		setEvent(events->current());
		break;
	case eRCInput::RC_LEFT:
		--(*events);
		if (!events->current())
			events->toLast();
		setEvent(events->current());
		break;
	}
}

void eEventDisplay::keyUp(int rc)
{
	switch (rc)
	{
	case eRCInput::RC_OK:
	case eRCInput::RC_HELP:
		close(0);
	}
}

eEventDisplay::eEventDisplay(QString service, const QList<EITEvent> &e): eWindow(1), service(service)
	/*
			kleine anmerkung:
			
			(liste mit) pointer übergeben ist scheisse, weil dazu THEORETISCH die eit gelockt werden MÜSSTE (wird sie aber nicht,
			weil "wenn lock dann kein exec"), wenn sich die eit ändert während wir hier im exec sind gibts nen CRASH.
			
			have fun.
			
			korrekterweise müsste man hier ne au-eit übergeben kriegen, sich auf dessen update connecten. das wiederrum suckt für
			senderübergreifendes EI.
			
			durch das "setList" wurde das Problem zwar nicht gefixt, aber wenigstens crasht es jetzt nicht mehr, was aber nur funktioniert
			weil wir single-threading benutzen.
	*/
{
	eventlist=0;
	events=0;
	move(QPoint(54, 70));
	resize(QSize(612, 430));
		// search for the components
	text=new eLabel(this);
	text->setFlags(RS_WRAP);
	text->move(QPoint(0, 0));
	text->resize(getClientSize());

	setList(e);
}

eEventDisplay::~eEventDisplay()
{
	delete events;
	delete eventlist;
}

void eEventDisplay::setEvent(EITEvent *event)
{

	if (event)
	{
		QString title=0, long_description="";
		QString eventDate="";
		QString eventTime="";

		tm *begin=event->start_time!=-1?localtime(&event->start_time):0;
		time_t endtime=event->start_time+event->duration;
		tm *end=event->start_time!=-1?localtime(&endtime):0;

		if (begin)
			eventTime.sprintf("%02d:%02d", begin->tm_hour, begin->tm_min);
		if (end)
			eventTime+=QString().sprintf(" - %02d:%02d", end->tm_hour, end->tm_min);

		if (begin)
			eventDate=QString().sprintf("%d.%d.%4d", begin->tm_mday, begin->tm_mon+1, begin->tm_year+1900);
	

		for (QListIterator<Descriptor> d(event->descriptor); d.current(); ++d)
		{
			if (d.current()->Tag()==DESCR_SHORT_EVENT)
			{
				ShortEventDescriptor *s=(ShortEventDescriptor*)d.current();
				title=s->event_name;
				if ((s->text.length() > 0) && (s->text!=title))
				{
					long_description+=s->text;
					long_description+="\n\n";
				}
			} else if (d.current()->Tag()==DESCR_EXTENDED_EVENT)
			{
				ExtendedEventDescriptor *ss=(ExtendedEventDescriptor*)d.current();
				long_description+=ss->item_description;
			}
		}

		QString str="";
		if (!title)
			title ="keine Information verfügbar";


		str = long_description;
	    if (eventDate.length())
			str+="\n\nSendezeit: " + eventDate+ ", " + eventTime;

		qDebug("setting string: %s", (const char*)long_description);
		setText(title);
		text->setText(str);
	} 
	else
	{
		setText(service);
		text->setText("keine Beschreibung verfügbar");
	}
}

void eEventDisplay::setList(const QList<EITEvent> &e)
{
	if (eventlist)
		delete eventlist;
	if (events)
		delete events;
	eventlist=new QList<EITEvent>(e);
	events=new QListIterator<EITEvent>(*eventlist);
	setEvent(events->current());
}

