#include <enigma_event.h>

#include <time.h>
#include <lib/base/eerror.h>
#include <lib/driver/rc.h>
#include <lib/gdi/font.h>
#include <lib/gui/elabel.h>
#include <lib/gui/eskin.h>
#include <lib/gui/guiactions.h>

void eEventDisplay::nextEvent()
{
	if (*events == --eventlist->end())
		*events = eventlist->begin();
	else
		++(*events);

	if (*events != eventlist->end())
		setEvent(**events);
	else
		setEvent(0);
}

void eEventDisplay::prevEvent()
{
	if (*events == eventlist->begin())
		*events = --eventlist->end();
 	else
		--(*events);	
  	
	if (*events != eventlist->end())
		setEvent(**events);
	else
		setEvent(0);
}

int eEventDisplay::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_cursorActions->left)
			{
				if (events)
					prevEvent();
				else
					close(1); // this go the prev event and call exec()   (in epgwindow.cpp)
			}
			else if (event.action == &i_cursorActions->right)
 			{
				if (events)
					nextEvent();
				else
					close(2);  // this go the next event and call exec()   (in epgwindow.cpp)
			}
			else
				break;
		return 1;
		default:
			break;
	}
	return eWindow::eventHandler(event);
}


eEventDisplay::eEventDisplay(eString service, const ePtrList<EITEvent>* e, EITEvent* evt)
: eWindow(1), service(service)
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

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eventview"))
		eFatal("skin load of \"eventview\" failed");

	ASSIGN(long_description, eLabel, "epg_description");
	long_description->setFlags(RS_WRAP);
	ASSIGN(title, eLabel, "title");
	ASSIGN(eventTime, eLabel, "time");
	ASSIGN(eventDate, eLabel, "date");
	ASSIGN(channel, eLabel, "channel");
	
	title->setText("");
	long_description->setText("");
	eventDate->setText("");
	eventTime->setText("");
	channel->setText("");
	
	if (e)
		setList(*e);
	else if (evt)
		setEvent(evt);

	addActionMap(&i_cursorActions->map);
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
		eString _title=0, _long_description="";
		eString _eventDate="";
		eString _eventTime="";

		tm *begin=event->start_time!=-1?localtime(&event->start_time):0;
		if (begin)
		{
			_eventTime.sprintf("%02d:%02d", begin->tm_hour, begin->tm_min);
			_eventDate=eString().sprintf("%d.%d.%4d", begin->tm_mday, begin->tm_mon+1, begin->tm_year+1900);
		}
		time_t endtime=event->start_time+event->duration;
		tm *end=event->start_time!=-1?localtime(&endtime):0;
		if (end)
		{
			_eventTime+=eString().sprintf(" - %02d:%02d", end->tm_hour, end->tm_min);
		}

		for (ePtrList<Descriptor>::iterator d(event->descriptor); d != event->descriptor.end(); ++d)
		{
			if (d->Tag()==DESCR_SHORT_EVENT)
			{
				ShortEventDescriptor *s=(ShortEventDescriptor*)*d;
				_title=s->event_name;
	
				if (LCDElement)
					LCDElement->setText(s->text);

				if ((s->text.length() > 0) && (s->text!=_title))
				{
					_long_description+=s->text;
					_long_description+="\n\n";
				}
			} else if (d->Tag()==DESCR_EXTENDED_EVENT)
			{
				ExtendedEventDescriptor *ss=(ExtendedEventDescriptor*)*d;
				_long_description+=ss->item_description;
			}
		}

		if (!_title)
			_title = _("no info available");

		eventTime->setText(_eventTime);
		eventDate->setText(_eventDate);

		title->setText(_title);
	
		if (!_long_description)
			long_description->setText(_("no description available"));
		else
			long_description->setText(_long_description);

		channel->setText(service);
	} 
	else
	{
		title->setText(service);
		long_description->setText(_("no description available"));
	}
}

void eEventDisplay::setList(const ePtrList<EITEvent> &e)
{
	if (eventlist)
		delete eventlist;
	if (events)
		delete events;
	eventlist=new ePtrList<EITEvent>(e);
	events=new ePtrList<EITEvent>::iterator(*eventlist);
	if (*events != eventlist->end())
		setEvent(**events);
	else
		setEvent(0);
}

