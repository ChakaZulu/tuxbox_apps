#include <epgwindow.h>
#include <enigma_event.h>

#include <algorithm>

#include <timer.h>
#include <enigma_lcd.h>
#include <lib/gui/eskin.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/gdi/font.h>

gFont eListBoxEntryEPG::TimeFont;
gFont eListBoxEntryEPG::DescrFont;
gPixmap *eListBoxEntryEPG::inTimer=0;
gPixmap *eListBoxEntryEPG::inTimerRec=0;
int eListBoxEntryEPG::timeXSize=0;
int eListBoxEntryEPG::dateXSize=0;

struct epgSelectorActions
{
  eActionMap map;
	eAction addTimerEvent, removeTimerEvent;
	epgSelectorActions():
		map("epgSelector", _("epg selector")),
		addTimerEvent(map, "addTimerEvent", _("add this event to timer list"), eAction::prioDialog ),
		removeTimerEvent(map, "removeTimerEvent", _("remove this event from timer list"), eAction::prioDialog )
	{
	}
};
eAutoInitP0<epgSelectorActions> i_epgSelectorActions(5, "epg selector actions");

eListBoxEntryEPG::~eListBoxEntryEPG()
{
	if (paraTime)
		paraTime->destroy();

	if (paraDate)
		paraDate->destroy();

	if (paraDescr)
		paraDescr->destroy();
}

int eListBoxEntryEPG::getEntryHeight()
{
	if (!DescrFont.pointSize && !TimeFont.pointSize)
	{
		DescrFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.Description");
		TimeFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.DateTime");
		inTimer = eSkin::getActive()->queryImage("timer_symbol");
		inTimerRec = eSkin::getActive()->queryImage("timer_rec_symbol");
		eTextPara* tmp = new eTextPara( eRect(0, 0, 200, 30) );
		tmp->setFont( TimeFont );
		tmp->renderString( "00:00" );
		timeXSize = tmp->getBoundBox().width();
		tmp->destroy();
		tmp = new eTextPara( eRect(0, 0, 200, 30) );
		tmp->setFont( TimeFont );
		tmp->renderString( "Tue 09.09," );
		dateXSize = tmp->getBoundBox().width();
		tmp->destroy();
	}
	return calcFontHeight(DescrFont)+4;	
}

eListBoxEntryEPG::eListBoxEntryEPG(const eit_event_struct* evt, eListBox<eListBoxEntryEPG> *listbox, eServiceReference &ref)
		:eListBoxEntry((eListBox<eListBoxEntry>*)listbox), paraDate(0), paraTime(0), paraDescr(0), event(evt), service(ref)
{	
		for (ePtrList<Descriptor>::iterator d(event.descriptor); d != event.descriptor.end(); ++d)
		{
			Descriptor *descriptor=*d;
			if (descriptor->Tag()==DESCR_SHORT_EVENT)
			{
				start_time = *localtime(&event.start_time);
				descr = ((ShortEventDescriptor*)descriptor)->event_name;
				return;
			}
		}
		descr = "no event data avail";
}

extern const char *dayStrShort[];

eString eListBoxEntryEPG::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited)
{
	drawEntryRect(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, hilited);
	
	eString hlp;
	int xpos=rect.left()+10;
	if (!paraDate)
	{
		paraDate = new eTextPara( eRect( 0, 0, dateXSize, rect.height()) );
		paraDate->setFont( TimeFont );
		eString tmp;
		tmp.sprintf("%s %02d.%02d,", dayStrShort[start_time.tm_wday], start_time.tm_mday, start_time.tm_mon+1);
		paraDate->renderString( tmp );
		paraDate->realign( eTextPara::dirRight );
		TimeYOffs = ((rect.height() - paraDate->getBoundBox().height()) / 2 ) - paraDate->getBoundBox().top();
		hlp+=tmp;
	}
	rc->renderPara(*paraDate, ePoint( xpos, rect.top() + TimeYOffs ) );
	xpos+=dateXSize+paraDate->getBoundBox().height();

	if (!paraTime)
	{
		paraTime = new eTextPara( eRect( 0, 0, timeXSize, rect.height()) );
		paraTime->setFont( TimeFont );         		
		eString tmp;
		tmp.sprintf("%02d:%02d", start_time.tm_hour, start_time.tm_min);
		paraTime->renderString( tmp );
		paraTime->realign( eTextPara::dirRight );
		hlp+=tmp;
	}
	rc->renderPara(*paraTime, ePoint( xpos, rect.top() + TimeYOffs ) );
	xpos+=timeXSize+paraTime->getBoundBox().height();

	ePlaylistEntry* p;
	if ( (p = eTimerManager::getInstance()->findEvent( &service, &event )) )
		if ( p->type & ePlaylistEntry::SwitchTimerEntry )
		{
	  	int ypos = (rect.height() - inTimer->y) / 2;
			rc->blit( *inTimer, ePoint( xpos, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);		
			xpos+=paraTime->getBoundBox().height()+inTimer->x;
		}
		else if ( p->type & ePlaylistEntry::RecTimerEntry )
		{
	  	int ypos = (rect.height() - inTimerRec->y) / 2;
			rc->blit( *inTimerRec, ePoint( xpos, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);		
			xpos+=paraTime->getBoundBox().height()+inTimerRec->x;
		}
	
	if (!paraDescr)
	{
		paraDescr = new eTextPara( eRect( 0 ,0, rect.width(), rect.height()) );
		paraDescr->setFont( DescrFont );
		paraDescr->renderString(descr);
		DescrYOffs = 0; // ((rect.height() - paraDescr->getBoundBox().height()) / 2 ) - paraDescr->getBoundBox().top();
	}
	rc->renderPara(*paraDescr, ePoint( xpos, rect.top() + DescrYOffs ) );

	return hlp+" "+descr;
}

void eEPGSelector::fillEPGList()
{
  eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(current);
  if (service)
		setText(eString("EPG - ")+service->service_name);
 	eDebug("get EventMap for onid: %02x, sid: %02x", current.getOriginalNetworkID().get(), current.getServiceID().get());
	const eventMap* evt = eEPGCache::getInstance()->getEventMap(current);
	eventMap::const_iterator It;
	for (It = evt->begin(); It != evt->end(); It++)
		new eListBoxEntryEPG(*It->second, events, current);
}

void eEPGSelector::entrySelected(eListBoxEntryEPG *entry)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

	if (!entry || !sapi)
		close(0);
	else
	{	
		int ret;
		hide();
		eZapLCD* pLCD = eZapLCD::getInstance();
		pLCD->lcdMain->hide();
		pLCD->lcdMenu->show();
	  eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(sapi->service);
		eEventDisplay ei(service ? service->service_name.c_str() : "", 0, &entry->event);
		ei.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
		ei.show();
		while((ret = ei.exec()))
		{
			eListBoxEntryEPG* tmp;

			if (ret == 1)
				tmp=events->goPrev();
			else if (ret == 2)
				tmp=events->goNext();
			else
				break; // close EventDisplay

			if (tmp)
				ei.setEvent(&tmp->event);					
		}
		ei.hide();
		pLCD->lcdMenu->hide();
		pLCD->lcdMain->show();
		show();
	}
}

eEPGSelector::eEPGSelector(const eServiceReferenceDVB &service)
	:eWindow(0), current(service)
{
	events = new eListBox<eListBoxEntryEPG>(this);
	events->setName("events");
	events->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));

	if (eSkin::getActive()->build(this, "eEPGSelector"))
		eWarning("EPG selector widget build failed!");

	CONNECT(events->selected, eEPGSelector::entrySelected);
	fillEPGList();
	addActionMap( &i_epgSelectorActions->map );
}

int eEPGSelector::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_epgSelectorActions->addTimerEvent)
			{
				if ( eTimerManager::getInstance()->addEventToTimerList( this, (eServiceReference*)&current, &events->getCurrent()->event ) )
				{
					hide();
					eTimerView v( eTimerManager::getInstance()->findEvent( (eServiceReference*)&current, &events->getCurrent()->event ) );
					v.show();
					v.exec();
					v.hide();
					invalidate();
					show();
				}
			}
			else if (event.action == &i_epgSelectorActions->removeTimerEvent)
			{
				if ( eTimerManager::getInstance()->removeEventFromTimerList( this, &current, &events->getCurrent()->event ) )
					invalidateEntry( events->getCurrent() );
			}
			else
				break;
			return 1;
		default:
			break;
	}
	return eWindow::eventHandler(event);
}

struct findEvent: public std::unary_function<const eListBoxEntry&, void>
{
	const eListBoxEntry& entry;
	int& cnt;

	findEvent(const eListBoxEntry& e, int& cnt): entry(e), cnt(cnt)
	{
		cnt=0;	
	}

	bool operator()(const eListBoxEntry& s)
	{
		if (&entry == &s)
			return 1;
		cnt++;
		return 0;
	}
};

void eEPGSelector::invalidateEntry( eListBoxEntryEPG *e)
{
	int i;
	if( events->forEachVisibleEntry( findEvent( *e, i ) ) == eListBoxBase::OK )
		events->invalidateEntry( i );
}
