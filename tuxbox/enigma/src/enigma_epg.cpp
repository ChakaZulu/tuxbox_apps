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
#include <lib/gui/numberactions.h>

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

eZapEPG::eZapEPG() 
	:eWidget(0,1), offs(0), focusColumn(0), hours(3)
	,numservices(8), eventWidget(0), NowTimeLineXPos(-1)
{
	eConfig::getInstance()->getKey("/elitedvb/multiepg/hours", hours);
	move(ePoint(70, 50));
	resize(eSize(590, 470));
	timeLine.setAutoDelete(true);
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
	addActionMap( &i_numberActions->map );

#ifndef DISABLE_FILE
	addActionToHelpList( &i_epgSelectorActions->addDVRTimerEvent );
#endif
#ifndef DISABLE_NETWORK
	addActionToHelpList( &i_epgSelectorActions->addNGRABTimerEvent );
#endif
	addActionToHelpList( &i_epgSelectorActions->addSwitchTimerEvent );
	addActionToHelpList( &i_epgSelectorActions->removeTimerEvent );

	Signal1<void,const eServiceReference& > callback;
	CONNECT( callback, eZapEPG::addToList );
	eZap::getInstance()->getServiceSelector()->forEachServiceRef( callback, false );
	curS = curE = services.begin();
	sbar = new eStatusBar(this);
	sbar->move( ePoint(0, clientrect.height()-50) );
	sbar->resize( eSize( clientrect.width(), 50) );
	sbar->loadDeco();
	sbar->setFlags( eStatusBar::flagOwnerDraw|RS_WRAP);

	eLabel *l = new eLabel(this);
	l->move(ePoint(100, clientrect.height()-80) );
	l->setFont( eSkin::getActive()->queryFont("eStatusBar") );
	l->resize( eSize( clientrect.width()-100, 30) );
	l->setText(_("press 1 ... 6 to select count of visible hours"));
	l->setFlags( eLabel::flagVCenter );

	setHelpID(13);
}

eZapEPG::~eZapEPG()
{
	eConfig::getInstance()->setKey("/elitedvb/multiepg/hours", hours);
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
	sbar(sbar), para(0), xOffs(0), yOffs(0)
{
	setBackgroundColor(entryColor);
};

eZapEPG::entry::~entry()
{
	if(para)
	{
		para->destroy();
		para=0;
	}
	delete event;
}

void eZapEPG::entry::redrawWidget(gPainter *target, const eRect &area)
{
	if ( !para )
	{
		para=new eTextPara( eRect(0, 0, size.width(), size.height() ) );
		para->setFont(titleFont);
		para->renderString(title, RS_WRAP);
		int bboxHeight = para->getBoundBox().height();
		int bboxWidth = para->getBoundBox().width();
		yOffs = (bboxHeight < size.height()) ? (( size.height() - bboxHeight ) / 2) - para->getBoundBox().top() : 0;
		xOffs = (bboxWidth < size.width()) ? (( size.width() - bboxWidth ) / 2) - para->getBoundBox().left() : 0;
	}

	target->renderPara(*para, ePoint( area.left()+xOffs, area.top()+yOffs) );

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
	if (backgroundColor==entryColorSelected)
		redrawed();
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
			selEntry(-1);
		else if (event.action == &i_focusActions->right)
			selEntry(+1);
		else if (event.action == &i_focusActions->up)
			selService(-1);
		else if (event.action == &i_focusActions->down)
			selService(+1);
		else if (event.action == &i_cursorActions->ok)
			close(eventvalid?0:-1);
		else if (event.action == &i_cursorActions->cancel)
			close(-1);
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
			drawTimeLines();
		}
		else if (event.action == &i_numberActions->key1)
		{
			hours=1;
			close(5);
		}
		else if (event.action == &i_numberActions->key2)
		{
			hours=2;
			close(5);
		}
		else if (event.action == &i_numberActions->key3)
		{
			hours=3;
			close(5);
		}
		else if (event.action == &i_numberActions->key4)
		{
			hours=4;
			close(5);
		}
		else if (event.action == &i_numberActions->key5)
		{
			hours=5;
			close(5);
		}
		else if (event.action == &i_numberActions->key6)
		{
			hours=6;
			close(5);
		}
		else
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
	return eWidget::eventHandler(event);
}

void eZapEPG::buildService(serviceentry &service)
{
	int width = service.pos.width();
	service.entries.setAutoDelete(1);
	eEPGCache *epgcache=eEPGCache::getInstance();
	epgcache->Lock();
	const timeMap *evmap = epgcache->getTimeMap(service.service);
	if (!evmap)
	{
		epgcache->Unlock();
		return;
	}

	timeMap::const_iterator ibegin = evmap->lower_bound(start);
	if ((ibegin != evmap->end()) && (ibegin != evmap->begin()) )
	{
		if ( ibegin->first != start )
			--ibegin;
	}
	else
		ibegin=evmap->begin();

	timeMap::const_iterator iend = evmap->lower_bound(end);

	for (timeMap::const_iterator event(ibegin); event != iend; ++event)
	{
		EITEvent *ev = new EITEvent(*event->second);
		if (((ev->start_time+ev->duration)>= start) && (ev->start_time <= end))
		{
			eString description;
			entry *e = new entry(eventWidget, timeFont, titleFont, descrFont, entryColor, entryColorSelected, sbar);
			e->service = &service;
			int xpos = (ev->start_time - start) * width / (end - start);
			int ewidth = (ev->start_time + ev->duration - start) * width / (end - start);
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
			CONNECT( e->redrawed, eZapEPG::drawTimeLines );
			service.entries.push_back(e);

			LocalEventData led;
			led.getLocalData(ev, &e->title, &description);
			tm *begin=localtime(&ev->start_time);
			while ( description[0] == ' ' )
				description.erase(0);
			if ( description != e->title )
			{
				if ( description )
					description = " - "+description;
			}
			else
				description="";
			eString tmp;
			tmp.sprintf("%02d.%02d. %02d:%02d - ",
				begin->tm_mday,
				begin->tm_mon+1,
				begin->tm_hour,
				begin->tm_min);

			time_t endTime = ev->start_time + ev->duration;
			tm *end=localtime(&endTime);
			tmp+=eString().sprintf("%02d:%02d %s%s",
				end->tm_hour, end->tm_min,
				e->title.c_str(), description.c_str());

			e->setHelpText(tmp);
			e->event = ev;
		}
		else
			delete ev;
	}
	epgcache->Unlock();
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
		last_time = l->event->start_time;
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
				if ((best == current_service->entries.end()) || abs(i->event->start_time-last_time) < best_diff)
				{
					best = i;
					best_diff = abs(i->event->start_time-last_time);
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
			direction 1  ->  up
			direction 2  ->  down
			direction 3  ->  left
			direction 4  ->  right */
	NowTimeLineXPos = -1;

	if ( eventWidget )
		eventWidget->hide();
	timeLine.clear();

	serviceentries.clear();
	current_service = serviceentries.end();

	delete eventWidget;
	eventWidget = new eWidget( this );
	eventWidget->move(ePoint(0,0));
	eSize tmps = clientrect.size();
	tmps.setHeight( clientrect.height()-80 );
	eventWidget->resize( tmps );

#ifndef DISABLE_LCD
	eventWidget->setLCD( LCDTitle, LCDElement );
#endif

	start=time(0)+eDVB::getInstance()->time_difference+offs;
	unsigned int tmp = start % 900;  // align to 15min
	start -= tmp;
	end=start+hours*3600;

	if ( direction == 1 )
		// go left.. we must count "numservices" back
	{
		std::list<eServiceReferenceDVB>::iterator s(curS);
		unsigned int cnt=0;
		do
		{
			if ( s == services.end() )
				break;
			const eit_event_struct *e = (const eit_event_struct*) eEPGCache::getInstance()->lookupEvent( *s, (time_t)(start+tmp), true );
			if ( e && (++cnt > numservices) )
					break;
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

	int width = clientrect.width();
	int serviceheight = (eventWidget->height()-40) / numservices;

	time_t tmpTime=start;
	int timeWidth = (width - 100) / (hours>3?hours:hours*2);
	for (unsigned int i=0; i < (hours>3?hours:hours*2); ++i)
	{
		tmpTime+=i?(hours>3?3600:1800):0;
		eLabel *l = new eLabel(eventWidget);
		l->move(ePoint( i*timeWidth-(timeWidth/2)+100, 0));
		l->resize(eSize(timeWidth,30));
		l->setFlags(eLabel::flagVCenter);
		l->setAlign(eTextPara::dirCenter);
		tm *bla = localtime(&tmpTime);
		l->setText(eString().sprintf("%d:%02d", bla->tm_hour, bla->tm_min));
		timeLine.push_back(l);
	}

	int p=0;
	do
	{
		if ( curE == services.end() )
			break;
		const eit_event_struct *e = (const eit_event_struct*) eEPGCache::getInstance()->lookupEvent( *curE, (time_t)(start+tmp), true );
		if ( e )
		{
			serviceentries.push_back(serviceentry());
			serviceentry &service = serviceentries.back();
			service.header = new eLabel(eventWidget);
			service.header->move(ePoint(0, p * serviceheight + 35));
			service.header->resize(eSize(90, serviceheight));
			service.pos = eRect(100, p++ * serviceheight + 35, width - 100, serviceheight );

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
			service.header->setFlags( eLabel::flagVCenter );

			buildService(service);

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

void eZapEPG::drawTimeLines()
{
	if ( eventWidget && eventWidget->isVisible() && timeLine.size() )
	{
		gPainter *p = getPainter(eRect(eventWidget->getPosition(),eventWidget->getSize()));
		int incWidth=((eventWidget->width()-100)/(hours>3?hours:hours*2));
		int pos=100;
		int lineheight = eventWidget->height();
		if ( NowTimeLineXPos != -1 )
		{
			int tmp=NowTimeLineXPos;
			NowTimeLineXPos=-1;
			invalidate( eRect( tmp,30,2,lineheight) );
		}
		p->setForegroundColor( eSkin::getActive()->queryColor("epg.timeline") );
		for (ePtrList<eLabel>::iterator it(timeLine); it != timeLine.end(); ++it)
		{
			p->fill(eRect(pos,30,1,lineheight));
			pos+=incWidth;
		}

		time_t now=time(0)+eDVB::getInstance()->time_difference;
		if ( now >= start && now < end )
		{
			int bla = ((eventWidget->width()-100)*1000) / (hours*60);
			NowTimeLineXPos = ((now/60) - (start/60)) * bla / 1000;
			NowTimeLineXPos+=100;
			p->setForegroundColor( eSkin::getActive()->queryColor("epg.timeline.now") );
			p->fill(eRect(NowTimeLineXPos,30,2,lineheight));
		}
		delete p;
	}
}
