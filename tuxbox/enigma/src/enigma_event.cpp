#include <enigma_event.h>

#include <time.h>
#include <lib/base/eerror.h>
#include <lib/driver/rc.h>
#include <lib/gdi/font.h>
#include <lib/gui/elabel.h>
#include <lib/gui/eskin.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/guiactions.h>
#include <lib/system/init_num.h>

struct enigmaEventViewActions
{
	eActionMap map;
	eAction close;
	enigmaEventViewActions():
		map("enigmaEventView", _("enigma event view")),
		close(map, "close", _("closes the Event View"), eAction::prioDialog)
	{
	}
};

eAutoInitP0<enigmaEventViewActions> i_enigmaEventViewActions(eAutoInitNumbers::actions, "enigma event view actions");

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
			else if (total && event.action == &i_cursorActions->up)
			{
				ePoint curPos = long_description->getPosition();
				if ( curPos.y() < 0 )
				{
					long_description->move( ePoint( curPos.x(), curPos.y() + descr->getSize().height() ) );
					updateScrollbar();
				}
			}
			else if (total && event.action == &i_cursorActions->down)
			{
				ePoint curPos = long_description->getPosition();
				if ( (total - descr->getSize().height() ) >= abs( curPos.y() - descr->getSize().height() ) )
				{
					long_description->move( ePoint( curPos.x(), curPos.y() - descr->getSize().height() ) );
					updateScrollbar();
				}
			}
			else if (event.action == &i_enigmaEventViewActions->close)
				close(0);
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

	scrollbar = new eProgress(this);
	scrollbar->setName("scrollbar");
	scrollbar->setStart(0);
	scrollbar->setPerc(100);

	descr = new eWidget(this);
	descr->setName("epg_description");

	eventTime = new eLabel(this);
	eventTime->setName("time");

	eventDate = new eLabel(this);
	eventDate->setName("date");

	channel = new eLabel(this);
	channel->setName("channel");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eventview"))
		eFatal("skin load of \"eventview\" failed");

	long_description=new eLabel(descr);
	long_description->setFlags(RS_WRAP);

	// try to recalc long description label... ( no broken text lines.. )
	float lineheight=fontRenderClass::getInstance()->getLineHeight( long_description->getFont() );
	int lines = descr->getSize().height() / (int)lineheight;
	int newheight = lines * (int)lineheight + (int)(round(lineheight) - (int)lineheight);
	descr->resize( eSize( descr->getSize().width(), newheight + (int)lineheight/6 ) );
	long_description->resize(eSize(descr->getSize().width(), descr->getSize().height()*4));

	if (e)
		setList(*e);
	else if (evt)
		setEvent(evt);
	addActionMap( &i_enigmaEventViewActions->map );
}

eEventDisplay::~eEventDisplay()
{
	delete events;
	delete eventlist;
}

void eEventDisplay::updateScrollbar()
{
	total = descr->getSize().height();
	int pages=1;
	while( total < long_description->getExtend().height() )
	{
		total += descr->getSize().height();
		pages++;
	}

	int start=-long_description->getPosition().y()*100/total;
	int vis=descr->getSize().height()*100/total;
	scrollbar->setStart(start);
	scrollbar->setPerc(vis);
	scrollbar->show();
	if (pages == 1)
		total=0;
}

void eEventDisplay::setEvent(EITEvent *event)
{
	long_description->hide();
	long_description->move( ePoint(0,0) );
	if (event)
	{
		eString _title=0, _long_description="";
		eString _eventDate="";
		eString _eventTime="";

		tm *begin=event->start_time!=-1?localtime(&event->start_time):0;
		if (begin)
		{
			_eventTime.sprintf("%02d:%02d", begin->tm_hour, begin->tm_min);
			_eventDate=eString().sprintf("%02d.%02d.%4d", begin->tm_mday, begin->tm_mon+1, begin->tm_year+1900);
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
			_title = _("no information is available");

		eventTime->setText(_eventTime);
		eventDate->setText(_eventDate);

		setText(_title);

		if (!_long_description)
			long_description->setText(_("no description is available"));
		else
			long_description->setText(_long_description);

		channel->setText(service);
	} 
	else
	{
		setText(service);
		long_description->setText(_("no description is available"));
	}
	updateScrollbar();
	long_description->show();
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

