#define DIR_V
#include "enigma_epg.h"
#include "enigma.h"
#include "sselect.h"
#include "enigma_lcd.h"
#include "epgactions.h"
#include "timer.h"
#include "enigma_event.h"

#include <lib/dvb/epgcache.h>
#include <lib/dvb/service.h>
#include <lib/dvb/si.h>
#include <lib/dvb/serviceplaylist.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/gui/guiactions.h>
#include <lib/gui/statusbar.h>
#include <lib/gdi/font.h>

gPixmap *eZapEPG::entry::inTimer=0;
gPixmap *eZapEPG::entry::inTimerRec=0;

static eString buildShortName( const eString &str )
{
	eString tmp;
	static char stropen[3] = { 0xc2, 0x86, 0x00 };
	static char strclose[3] = { 0xc2, 0x87, 0x00 };
	unsigned int open=eString::npos-1;

	while ( (open = str.find(stropen, open+2)) != eString::npos )
	{
		unsigned int close = str.find(strclose, open);
		if ( close != eString::npos )
			tmp+=str.mid( open+2, close-(open+2) );
	}
	return tmp;
}

eZapEPG::eZapEPG(): 
	eWindow(1), offs(0), focusColumn(0), hours(3), numservices(5), eventWidget(0)
{
	setText(_("Programm Guide"));
	timeFont = eSkin::getActive()->queryFont("epg.time");
	titleFont = eSkin::getActive()->queryFont("epg.title");
	descrFont = eSkin::getActive()->queryFont("epg.description");
	entryColor = eSkin::getActive()->queryColor("epg.entry.background");
	entryColorSelected = eSkin::getActive()->queryColor("epg.entry.background.selected");
	entry::inTimer = eSkin::getActive()->queryImage("timer_symbol");
	entry::inTimerRec = eSkin::getActive()->queryImage("timer_rec_symbol");
	addActionMap( &i_epgSelectorActions->map );
	addActionMap( &i_focusActions->map );
	addActionMap( &i_cursorActions->map );
	Signal1<void,const eServiceReference& > callback;
	CONNECT( callback, eZapEPG::addToList );
	eZap::getInstance()->getServiceSelector()->forEachServiceRef( callback, false );
	curS = curE = services.begin();
	sbar = new eStatusBar(this);
	sbar->setFlags( eStatusBar::flagOwnerDraw );
	setHelpID(13);
}

void eZapEPG::addToList( const eServiceReference& ref )
{
	if ( ref.type == eServiceReference::idDVB )
		services.push_back( (const eServiceReferenceDVB&) ref );
}

eZapEPG::entry::entry(eWidget *parent, gFont &timeFont, gFont &titleFont, 
	gFont &descrFont, gColor entryColor, gColor entryColorSelected, eWidget *sbar)
	: eWidget(parent), timeFont(timeFont),
	titleFont(titleFont), descrFont(descrFont), entryColor(entryColor), 
	entryColorSelected(entryColorSelected),
	sbar(sbar)
{
	setBackgroundColor(entryColor);
};

eZapEPG::entry::~entry()
{
	delete event;
}

void eZapEPG::entry::redrawWidget(gPainter *target, const eRect &area)
{
	eString time="";
	tm *begin=start!=-1?localtime(&start):0;
	if (begin)
		time.sprintf("%02d:%02d (%dmin)", begin->tm_hour, begin->tm_min, duration / 60);
	target->setFont(timeFont);
	target->renderText(eRect(0, 0, size.width(), 18), time);

	target->setFont(titleFont);
	target->renderText(eRect(0, 18, size.width(), size.height()-18), title, RS_WRAP);

	ePlaylistEntry* p=0;
	if ( (p = eTimerManager::getInstance()->findEvent( &service->service, (EITEvent*)event )) )
	{
		if ( p->type & ePlaylistEntry::SwitchTimerEntry )
			target->blit( *inTimer, ePoint( size.width()-inTimer->x-1, size.height()-inTimerRec->y-1 ), eRect(), gPixmap::blitAlphaTest);
		else if ( p->type & ePlaylistEntry::RecTimerEntry )
			target->blit( *inTimerRec, ePoint(size.width()-inTimerRec->x-1, size.height()-inTimerRec->y-1), eRect(), gPixmap::blitAlphaTest);
	}

	target->setForegroundColor(gColor(entryColorSelected));
	target->fill(eRect(0, size.height()-1, size.width(), 1));
	target->fill(eRect(size.width()-1, 0, 1, size.height()));
}

void eZapEPG::entry::gotFocus()
{
	sbar->setText( helptext );
	setBackgroundColor(entryColorSelected);
}

void eZapEPG::entry::lostFocus()
{
//	setForegroundColor(normalF,false);
	setBackgroundColor(entryColor);
}

int eZapEPG::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::execBegin:
		if ( sbar->getPosition().isNull() )
		{
			sbar->move( ePoint(0, clientrect.height()-30) );
			sbar->resize( eSize( clientrect.width(), 30) );
			sbar->loadDeco();
		}
		break;
	case eWidgetEvent::evtAction:
	{
		int addtype=-1;
		int servicevalid = current_service != serviceentries.end();
		int eventvalid = 0;
		if ( servicevalid && current_service->current_entry != current_service->entries.end())
			eventvalid = 1;
		if ( (addtype = i_epgSelectorActions->checkTimerActions( event.action )) != -1 )
				;
		else if (event.action == &i_epgSelectorActions->removeTimerEvent)
		{
			if (eventvalid)
				if ( eTimerManager::getInstance()->removeEventFromTimerList( this, &current_service->service, current_service->current_entry->event ) )
            current_service->current_entry->invalidate();
		}
		else if (event.action == &i_focusActions->left)
		#ifdef DIR_V
			selService(-1);
		#else
			selEntry(-1);
		#endif
		else if (event.action == &i_focusActions->right)
		#ifdef DIR_V
			selService(+1);
		#else
			selEntry(+1);
		#endif
		else if (event.action == &i_focusActions->up)
		#ifdef DIR_V
			selEntry(-1);
		#else
			selService(-1);
		#endif
		else if (event.action == &i_focusActions->down)
		#ifdef DIR_V
			selEntry(+1);
		#else
			selService(+1);
		#endif
		else if (event.action == &i_cursorActions->ok)
			close(eventvalid?0:-1);
		else if ((event.action == &i_epgSelectorActions->showExtendedInfo) && eventvalid)
		{
			eService *service=eDVB::getInstance()->settings->getTransponders()->searchService(current_service->service);
			eEventDisplay ei(service ? service->service_name.c_str() : "", current_service->service, 0, (EITEvent*)current_service->current_entry->event);

#ifndef DISABLE_LCD
			eZapLCD* pLCD = eZapLCD::getInstance();
			pLCD->lcdMain->hide();
			pLCD->lcdMenu->show();
			ei.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
#endif
			hide();
			ei.show();
			int ret;
			while((ret = ei.exec()))
			{
				if (ret == 1)
					selEntry(-1);
				else if (ret == 2)
					selEntry(+1);
				else
					break; // close EventDisplay
	
				ei.setEvent((EITEvent*)current_service->current_entry->event);
			}
			ei.hide();
			show();
		} else
			break;
		if (eventvalid && (addtype != -1))
		{
			if ( !eTimerManager::getInstance()->eventAlreadyInList( this, *(EITEvent*)current_service->current_entry->event, current_service->service) )
			{
				hide();
				eTimerEditView v( *(EITEvent*)current_service->current_entry->event, addtype, current_service->service);
				v.show();
				v.exec();
				v.hide();
				show();
			}
		}
		return 1;
	}
	default:
		break;
	}
	return eWindow::eventHandler(event);
}


void eZapEPG::buildService(serviceentry &service, time_t start, time_t end)
{
#ifdef DIR_V
	int height = service.pos.height();
#else
	int width = service.pos.width();
#endif
	service.entries.setAutoDelete(1);
	eEPGCache *epgcache=eEPGCache::getInstance();
	const timeMap *evmap = epgcache->getTimeMap(service.service);
	if (!evmap)
		return;
	timeMap::const_iterator ibegin = evmap->lower_bound(start);
	if ((ibegin != evmap->end()) && (ibegin != evmap->begin()))
		--ibegin;
	else
		ibegin=evmap->begin();

	timeMap::const_iterator iend = evmap->upper_bound(end);
	if (iend != evmap->end())
		++iend;

	for (timeMap::const_iterator event(ibegin); event != iend; ++event)
	{
		EITEvent *ev = new EITEvent(*event->second);
		if (((ev->start_time+ev->duration)>= start) && (ev->start_time <= end))
		{
			entry *e = new entry(eventWidget, timeFont, titleFont, descrFont, entryColor, entryColorSelected, sbar);
			e->service = &service;
			e->start = ev->start_time;
			e->duration = ev->duration;
			e->event_id = ev->event_id;
#ifdef DIR_V
			int ypos = (e->start - start) * height / (end - start);
			int eheight = (e->start + e->duration - start) * height / (end - start);
			eheight -= ypos;
			
			if (ypos < 0)
			{
				eheight += ypos;
				ypos = 0;
			}
			
			if ((ypos+eheight) > height)
				eheight = height - ypos;
				
			e->move(ePoint(service.pos.x(), service.pos.y() + ypos));
			e->resize(eSize(service.pos.width(), eheight));
#else
			int xpos = (e->start - start) * width / (end - start);
			int ewidth = (e->start + e->duration - start) * width / (end - start);
			ewidth -= xpos;
			
			if (xpos < 0)
			{
				ewidth += xpos;
				xpos = 0;
			}
			
			if ((xpos+ewidth) > width)
				ewidth = width - xpos;
				
			e->move(ePoint(service.pos.x() + xpos, service.pos.y()));
			e->resize(eSize(ewidth, service.pos.height()));
#endif
			service.entries.push_back(e);
			
			for (ePtrList<Descriptor>::const_iterator d(ev->descriptor); d != ev->descriptor.end(); ++d)
				if (d->Tag()==DESCR_SHORT_EVENT)
				{
					const ShortEventDescriptor *s=(const ShortEventDescriptor*)*d;
					e->title=s->event_name;
					e->description=s->text;
					break;
				}
			tm *begin=ev->start_time!=-1?localtime(&ev->start_time):0;
			eString descr = e->description;
			while ( descr[0] == ' ' )
				descr.erase(0);
			if ( descr != e->title )
			{
				if ( descr )
					descr = " - "+descr;
			}
			else
				descr="";
			e->setHelpText( eString().sprintf("%02d.%02d. %02d:%02d %s%s",
				begin->tm_mday,
				begin->tm_mon+1,
				begin->tm_hour,
				begin->tm_min,
				e->title.c_str(),
				descr.c_str() ));
			e->event = ev;
		} else
			delete ev;
	}
}

void eZapEPG::selService(int dir)
{	
	if (serviceentries.begin() == serviceentries.end())
		return;
	int isok;
	ePtrList<entry>::iterator l = current_service->current_entry;
	isok = l != current_service->entries.end();
	if (dir == +1)
	{
		++current_service;
		if (current_service == serviceentries.end())
		{
			focusColumn=0;
			close(2);
			return;
		}
		else
			++focusColumn;
	} else if (dir == -1)
	{
		if (current_service != serviceentries.begin())
		{
			--focusColumn;
			--current_service;
		}
		else
		{
			close(1);
			focusColumn=numservices-1;
			return;
		}
	}

	time_t last_time=0;

	if (isok)
	{
		l->lostFocus();
		last_time = l->start;
	}
	
	if (current_service->current_entry != current_service->entries.end())
	{
		if (last_time)
		{
			int best_diff=0;
			ePtrList<entry>::iterator best=current_service->entries.end();
			for (ePtrList<entry>::iterator i(current_service->entries.begin()); 
					i != current_service->entries.end(); ++i)
			{
				if ((best == current_service->entries.end()) || abs(i->start-last_time) < best_diff)
				{
					best = i;
					best_diff = abs(i->start-last_time);
				}
			}
			
			if (best != current_service->entries.end())
				current_service->current_entry = best;
		}
		current_service->current_entry->gotFocus();
	}
}

void eZapEPG::selEntry(int dir)
{
	if (current_service == serviceentries.end())
	{
		if ( dir == -1 && offs >= hours*3600 )
		{
			offs -= hours*3600;
			close(3);
		}
/*		else
			eDebug("invalid service");*/
		return;
	}
	if (current_service->entries.begin() == current_service->entries.end())
	{
//		eDebug("empty service");
		return;
	}
	ePtrList<entry>::iterator l = current_service->current_entry;
	if ( dir == +1)
	{
		++current_service->current_entry;
		if (current_service->current_entry == current_service->entries.end())
		{
			if ( eventWidget->isVisible() )
			{
				offs += hours*3600;
				close(4);
			}
			else
				--current_service->current_entry;
			return;
		}
	} else
	{
		if (current_service->current_entry == current_service->entries.begin())
		{
			if ( offs >= hours*3600 )
			{
				offs -= hours*3600;
				close(3);
			}
			return;
		}
		--current_service->current_entry;
	}
	if (l != current_service->entries.end())
		l->lostFocus();
	current_service->current_entry->gotFocus();
}

void eZapEPG::buildPage(int direction)
{
	/*
			direction 1  ->  left
			direction 2  ->  right
			direction 3  ->  up
			direction 4  ->  down  */
	if ( eventWidget )
		eventWidget->hide();
	serviceentries.clear();
	current_service = serviceentries.end();
	delete eventWidget;
	eventWidget = new eWidget( this );
	eventWidget->move(ePoint(0,0));
	eSize tmp = clientrect.size();
	tmp.setHeight( clientrect.height()-30 );
	eventWidget->resize( tmp );

#ifndef DISABLE_LCD
	eventWidget->setLCD( LCDTitle, LCDElement );
#endif

	time_t now=time(0)+eDVB::getInstance()->time_difference+offs,
				 end=now+hours*3600;

	if ( direction == 1 )
		// go left.. we must count "numservices" back
	{
		std::list<eServiceReferenceDVB>::iterator s(curS);
		unsigned int cnt=0;
		do
		{
			if ( s == services.end() )
				break;
			const EITEvent *e = eEPGCache::getInstance()->lookupEvent( *s, now );
			if ( e )
			{
				delete e;
				if ( ++cnt > numservices )
					break;
			}
			if ( s != services.begin() )
				--s;
			else
			{
				s = services.end();
				if (s != services.begin())
					--s;
				else
					break;
			}
		}
		while( s != curS );
		curS=curE=s;
	}
	else if ( direction == 2 ) // right
		curS=curE;
	else if ( direction > 2 )  // up or down
		curE=curS;

#ifdef DIR_V
	int height = clientrect.height()-30; // sub statusbar height
	int servicewidth = clientrect.width() / numservices;
#else
	int width = clientrect.width();
	int serviceheight = clientrect.height() / numservices;
#endif
	
	int p = 0;
	do
	{
		if ( curE == services.end() )
			break;
		const EITEvent *e = eEPGCache::getInstance()->lookupEvent( *curE, now );
		if ( e )
		{
			delete e;
			serviceentries.push_back(serviceentry());
			serviceentry &service = serviceentries.back();
			service.header = new eLabel(eventWidget);
#ifdef DIR_V
			service.header->move(ePoint(p * servicewidth, 0));
			service.header->resize(eSize(servicewidth, 30));
			service.pos = eRect(p++ * servicewidth, 30, servicewidth, height - 30);
#else
			service.header->move(ePoint(0, p * serviceheight));
			service.header->resize(eSize(100, serviceheight));
			service.pos = eRect(100, p++ * serviceheight, width - 100, serviceheight);
#endif
			eString stext;
			if ( curE->descr )   // have changed service name?
				stext=curE->descr;  // use this...
			else
			{
				eService *sv=eServiceInterface::getInstance()->addRef(*curE);
				if ( sv )
				{
					eString shortname = buildShortName( sv->service_name );
					stext=shortname?shortname:sv->service_name;
					eServiceInterface::getInstance()->removeRef(*curE);
				}
			}
			service.service = *curE;

			// set column service name
			service.header->setText(stext);

			buildService(service, now, end);

   // set focus line
			if ( direction == 3 )  // up pressed
    // set focus to last line
				service.current_entry = --service.entries.end();
			else  // set focus to first line
				service.current_entry = service.entries.begin();
		}
		if ( ++curE == services.end() )  // need wrap ?
			curE = services.begin();
	}
	while( serviceentries.size() < numservices && curE != curS );
	if (!p)
	{
		sbar->setText("");
		return;
	}

	eventWidget->show();

	if ( !serviceentries.size() )
		return;

 // set column focus
	current_service = serviceentries.begin();
	for (unsigned int i=0; i < focusColumn; i++ )
	{
		if (current_service == --serviceentries.end())
			break;
		current_service++;
	}
	if (current_service->current_entry != current_service->entries.end())
		current_service->current_entry->gotFocus();
}
