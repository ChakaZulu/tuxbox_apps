#include "timer.h"

#include <apps/enigma/enigma_main.h>
#include <core/system/init.h>
#include <core/dvb/dvbservice.h>
#include <core/gui/emessage.h>
#include <core/gdi/font.h>

eTimerManager* eTimerManager::instance=0;

eTimerManager::eTimerManager()
	:timer(eApp)
{
	if (!instance)
		instance = this;

	timerlistref=eServiceReference(eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 1, 1);
	timerlist=(ePlaylist*)eServiceInterface::getInstance()->addRef(timerlistref);
	ASSERT(timerlist);
	timerlist->service_name=_("Timerlist");
	timerlist->load(CONFIGDIR "/enigma/timer.epl");
	conn = CONNECT( timer.timeout, eTimerManager::waitClock );
	waitClock();
}

// called only once... at start of eTimerManager
void eTimerManager::waitClock()
{
	if (eDVB::getInstance()->time_difference)	
	{
		conn.disconnect();
		eDebug("[eTimerManager] timeUpdated");	
		setNextEvent();
	}
	else
	{
		eDebug("[eTimerManager] wait for clock update");
		timer.start(1000, true);  // next check in 1 sec
	}
}

void eTimerManager::setNextEvent()
{
	eDebug("[eTimerManager] setNextEvent()");
	nextStartingEvent=timerlist->list.end();
	int timeToNextEvent=0xFFFF, count=0;
	// parse events... invalidate old, set nextEvent Timer
	for (	std::list< ePlaylistEntry >::iterator i(timerlist->list.begin()); i != timerlist->list.end(); i++ )
	{
		time_t nowTime=time(0)+eDVB::getInstance()->time_difference;
		if ( i->type & ePlaylistEntry::stateWaiting )
			if ( i->time_begin+i->duration < nowTime ) // old event found
			{
				i->type &= ~ePlaylistEntry::stateWaiting;
				i->type |= ePlaylistEntry::stateEventOutdated;
			}
			else if( (i->time_begin - nowTime) < timeToNextEvent )
			{
				nextStartingEvent=i;
				timeToNextEvent = i->time_begin - nowTime;
				count++;
			}
			else
				count++;
	}
	eDebug("[eTimerManager] updated ( %d waiting events in list )", count );
	if ( nextStartingEvent != timerlist->list.end() )
	{
		tm* evtTime = localtime( &nextStartingEvent->time_begin );
		eDebug("[eTimerManager] next event starts at %02d.%02d, %02d:%02d", evtTime->tm_mday, evtTime->tm_mon+1, evtTime->tm_hour, evtTime->tm_min );
		long t = getSecondsToBegin();
		if ( t > 360 )
		{
			timer.start( (t - 360) * 1000, true );		// set the Timer to eventBegin - 6 min
			conn = CONNECT( timer.timeout, eTimerManager::viewTimerMessage );
		}
		else
			zapToChannel();
	}
}

void eTimerManager::viewTimerMessage()
{
	eDebug("[eTimerManager] viewTimerMessage()");
	if (conn.connected())
		conn.disconnect();

	long t;
	if ( (t = getSecondsToBegin()) ) // event is not running
	{
		timer.start(60000, true ); // restart timer
		conn = CONNECT( timer.timeout, eTimerManager::zapToChannel );
		eDebug("[eTimerManager] event starts in 6 min");
		// here we can show a messagebox... event begin in bla minutes... ok.. abort...
		// messagebox timout 1 min..
	}
/*	else
	{
		conn = CONNECT( timer.timeout, eTimerManager::stopEvent );
		int t = getTimeout();
	}*/
}

void eTimerManager::zapToChannel()
{
	eDebug("[eTimerManager] zapToChannel()");
	if (conn.connected())
		conn.disconnect();

	if ( eServiceInterface::getInstance()->service != nextStartingEvent->service )
	{
		eDebug("[eTimerManager] change to the right service");
		timer.start( 5000, true );  // set zap Timeout...
		conn = CONNECT_1_0( timer.timeout, eTimerManager::stopEvent, ePlaylistEntry::stateZapFailed );
		conn2 = CONNECT( eDVB::getInstance()->switchedService, eTimerManager::serviceChanged );
		eServiceInterface::getInstance()->play( nextStartingEvent->service );			// we zap to Channel
	}
	else
	{
		eDebug("[eTimerManager] we are always on the right service... do not change service");
		startActiveTimerMode();
	}
}

void eTimerManager::serviceChanged( const eServiceReferenceDVB& ref, int err )
{
	eDebug("[eTimerManager] serviceChanged()");
	if ( nextStartingEvent->service == (eServiceReference&)ref )
	{
		timer.stop(); // stop zapTimeout
		conn.disconnect();
		conn2.disconnect();
		startActiveTimerMode();
	}
}

void eTimerManager::startActiveTimerMode()
{
		eDebug("[eTimerManager] startActiveTimerMode()");
		eZapMain::getInstance()->setState( eZapMain::stateRunningTimerEvent );
		// now in eZapMain the RemoteControl should be handled for TimerMode...
		// an service change now stop the Running Event and set it to userAborted
  	conn2 =	CONNECT( eDVB::getInstance()->leaveService, eTimerManager::leaveService );
		if ( nextStartingEvent->type & ePlaylistEntry::typeSmartTimer )
		{
			conn = CONNECT( eDVB::getInstance()->tEIT.tableReady, eTimerManager::EITready );
			EITready(0);  // fake
		}
		long t = getSecondsToBegin();
		if (t > 0)
		{
			if ( !(nextStartingEvent->type & ePlaylistEntry::typeSmartTimer) )
			{
				// we start the last 5 min countdown ...
				timer.start( t*1000 , true ); // restart timer
				conn = CONNECT( timer.timeout, eTimerManager::startEvent );
			}
		}
		else
			startEvent();
}

void eTimerManager::startEvent()
{
	eDebug("[eTimerManager] startEvent()");
	if ( conn.connected() && !(nextStartingEvent->type & ePlaylistEntry::typeSmartTimer) )
	{
		conn.disconnect();
		timer.start( nextStartingEvent->duration * 1000, true );
		conn=CONNECT_1_0( timer.timeout, eTimerManager::stopEvent, ePlaylistEntry::stateFinished );
	}
	//else we leave the eit connected...

	nextStartingEvent->type &= ~ePlaylistEntry::stateWaiting;
	nextStartingEvent->type |= ePlaylistEntry::stateRunning;

	switch ( nextStartingEvent->type & (ePlaylistEntry::RecTimerEntry|ePlaylistEntry::SwitchTimerEntry) )
	{
		case ePlaylistEntry::SwitchTimerEntry:
				// here we must do nothing... we have always zapped...
		break;	
		case ePlaylistEntry::RecTimerEntry:
		 	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
			if (!handler)
				eFatal("no service Handler");

			eDebug("start recording...");
			if (handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordOpen)))
			{
				eDebug("couldn't record... :/");
			} else
			{
				handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStart));
				eDebug("ok, recording...");
			}
		break;	
	}
}

void eTimerManager::stopEvent( int state )
{
	eDebug("[eTimerManager] stopEvent()");
	if (conn.connected())
		conn.disconnect();
	if(conn2.connected())
		conn2.disconnect();

	if( nextStartingEvent->type & ePlaylistEntry::stateRunning )
	{
		if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
		{
		 	eServiceHandler *handler=eServiceInterface::getInstance()->getService();	
			if (!handler)
				eFatal("no service Handler");
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStop));		
			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordClose));
		}
	}

	nextStartingEvent->type &= ~(ePlaylistEntry::stateRunning|
															 ePlaylistEntry::statePaused|
                               ePlaylistEntry::stateWaiting);
	nextStartingEvent->type |= state;

	setNextEvent();	// we set the Next Event... a new beginning *g*
}

void eTimerManager::EITready( int error )
{
	eDebug("[eTimerManager] EITready %s", strerror(-error));
	EIT *eit = eDVB::getInstance()->tEIT.getCurrent();
	if (!error && eit)
	{
		for (ePtrList<EITEvent>::const_iterator event(eit->events); event != eit->events.end(); ++event)		// always take the first one
		{
			eDebug("event->event_id(%d) == nextStartingEvent->event_id(%d)", event->event_id, nextStartingEvent->event_id );
			if ( nextStartingEvent != timerlist->list.end() && event->event_id == nextStartingEvent->event_id )
			{
				eDebugNoNewLine("running_status(%d) = ", event->running_status );
				switch( event->running_status )
				{
					case 0:
						eDebug("undefined");
						// premiere world sends either undefined or running
						if ( nextStartingEvent->type & ePlaylistEntry::stateRunning )
							stopEvent( ePlaylistEntry::stateFinished );
					break;
					case 1:
						eDebug("not running");					
						if ( nextStartingEvent->type & ePlaylistEntry::stateRunning )
							stopEvent( ePlaylistEntry::stateFinished );
					break;
					case 2:
						eDebug("starts in few seconds");					
					break;
					case 3:
						eDebug("pausing");					
						if ( nextStartingEvent->type & ePlaylistEntry::stateRunning )
							pauseEvent();
					break;
					case 4:
						eDebug("running");
						if ( nextStartingEvent->type & ePlaylistEntry::stateWaiting )
							startEvent();
						else if ( nextStartingEvent->type & ePlaylistEntry::statePaused )
							restartEvent();
/*						else if ( nextStartingEvent->type & ePlaylistEntry::stateRunning )
						{
							restartRecording();
							eDebug("[eTimerManager] restart HDD Recording (rst_flags -> running)");
						}
						else
							eDebug("[eTimerManager] bla	");*/
					break;
					case 5 ... 7:
						eDebug("reserved for future use");
					break;
				}
				break;
			}
		}	
	}
}

void eTimerManager::pauseEvent()
{
	eDebug("[eTimerManager] pauseEvent()");
	if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
	{
	 	eServiceHandler *handler=eServiceInterface::getInstance()->getService();	handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStop));
		if (!handler)
			eFatal("no service Handler");
		nextStartingEvent->type &= ~ePlaylistEntry::stateRunning;
		nextStartingEvent->type |= ePlaylistEntry::statePaused;	
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStop));		
	}
}

void eTimerManager::restartEvent()
{
	eDebug("[eTimerManager] restartEvent()");
	if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
	{
	 	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
		if (!handler)
			eFatal("no service Handler");
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStart));
		nextStartingEvent->type &= ~ePlaylistEntry::statePaused; // we delete current state...
		nextStartingEvent->type |= ePlaylistEntry::stateRunning;
	}
}

void eTimerManager::restartRecording()
{
	eDebug("[eTimerManager] restartRecording()");
	if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
	{
	 	eServiceHandler *handler=eServiceInterface::getInstance()->getService();	
		if (!handler)
			eFatal("no service Handler");
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStop));		
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordClose));
		handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStart));
	}
}

void eTimerManager::leaveService( const eServiceReferenceDVB& ref )
{
	eDebug("[eTimerManager] leaveService");
	if ( nextStartingEvent->service == (eServiceReference&)ref)
	{
	// this works from 5 min before the event begin to end of event...
		stopEvent( ePlaylistEntry::stateUserAborted );
	}
}

long eTimerManager::getSecondsToBegin()
{
	time_t nowTime = time(0)+eDVB::getInstance()->time_difference;
	return nextStartingEvent->time_begin - nowTime;
}

long eTimerManager::getSecondsToEnd()
{
	time_t nowTime = time(0)+eDVB::getInstance()->time_difference;
	return (nextStartingEvent->time_begin + nextStartingEvent->duration) - nowTime;
}

eTimerManager::~eTimerManager()
{
	if (this == instance)
		instance = 0;
	timerlist->save(CONFIGDIR "/enigma/timer.epl");
	eServiceInterface::getInstance()->removeRef(timerlistref);
	eDebug("[eTimerManager] down ( %d events in list )", timerlist->list.size() );
}


ePlaylistEntry* eTimerManager::findEvent( eServiceReference *service, EITEvent *evt )
{
	for ( std::list<ePlaylistEntry>::iterator i( timerlist->list.begin() ); i != timerlist->list.end(); i++)
		if ( ( evt->event_id != -1 && i->current_position == evt->event_id ) ||
			   ( *service == i->service && evt->start_time == i->time_begin ) )
			return &*i;
	
	return 0;
}

bool eTimerManager::removeEventFromTimerList( eWidget *sel, const ePlaylistEntry& entry )
{
	for ( std::list<ePlaylistEntry>::iterator i( timerlist->list.begin() ); i != timerlist->list.end(); i++)
		if ( *i == entry )
		{
			sel->hide();
			time_t t(time(0)+eDVB::getInstance()->time_difference);
	 		if ( eServiceInterface::getInstance()->service == entry.service && t >= i->time_begin && t <= i->time_begin + i->duration )
			{
				eMessageBox box(_("You would to delete the running event.. this stops the timer mode (recording)!"), _("Delete event from timerlist"), eMessageBox::btOK|eMessageBox::iconWarning );
				box.show();
				box.exec();
				box.hide();
	  	}
			eMessageBox box(_("Really delete this event?"), _("Delete event from timerlist"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo);
			box.show();
			int r=box.exec();
			box.hide();
			if (r == eMessageBox::btYes)
			{
		 		if ( eServiceInterface::getInstance()->service == entry.service && t >= i->time_begin && t <= i->time_begin + i->duration )
					stopEvent( ePlaylistEntry::stateUserAborted );  // we have warned....
				timerlist->list.erase(i);
				sel->show();
				return true;
			}
			sel->show();
			break;
		}
	return false;
}

bool eTimerManager::removeEventFromTimerList( eWidget *sel, const eServiceReference *ref, const EITEvent *evt )
{
	for ( std::list<ePlaylistEntry>::iterator i( timerlist->list.begin() ); i != timerlist->list.end(); i++)
		if ( ( i->event_id != -1 && i->event_id == evt->event_id ) || ( *ref == i->service && evt->start_time == i->time_begin ) )
			return removeEventFromTimerList( sel, *i );
	return false;
}

bool eTimerManager::addEventToTimerList( eEPGSelector *sel, const eServiceReference *ref, const EITEvent *evt )
{
	time_t nowTime = time(0)+eDVB::getInstance()->time_difference;
	for ( std::list<ePlaylistEntry>::iterator i( timerlist->list.begin() ); i != timerlist->list.end(); i++)
		if ( evt->start_time < nowTime )
		{
			eMessageBox box(_("This event already began.\nYou can not add this to timerlist"), _("Add event to timerlist"), eMessageBox::iconWarning|eMessageBox::btOK);
			sel->hide();
			box.show();
			box.exec();
			box.hide();
			sel->show();
			return false;
		}
		else if ( ( i->event_id != -1 && i->event_id == evt->event_id ) ||
			   ( *ref == i->service && evt->start_time == i->time_begin ) )
		{
			eMessageBox box(_("This event is already in the timerlist."), _("Add event to timerlist"), eMessageBox::iconWarning|eMessageBox::btOK);
			sel->hide();
			box.show();
			box.exec();
			box.hide();
			sel->show();
			return false;
		}
		else if ( evt->start_time - (i->time_begin+i->duration) < 0 )
		{
			eMessageBox box(_("This service can not added to the timerlist.\nThe event overlaps with another event in the timerlist\nPlease check manually the timerlist."), _("Add event to timerlist"), eMessageBox::iconWarning|eMessageBox::btOK);
			sel->hide();
			box.show();
			box.exec();
			box.hide();
			sel->show();
			return false;
		}
  eEPGContextMenu menu;
  sel->hide();
  menu.show();
	int type;
	switch ( menu.exec() )
	{
		case 1:
			type = ePlaylistEntry::RecTimerEntry|ePlaylistEntry::stateWaiting;
		break;

		case 2:
			type = ePlaylistEntry::SwitchTimerEntry|ePlaylistEntry::stateWaiting;
		break;

		case 0:
		default:
			return false;
	}
	menu.hide();
	sel->show();
// add the event description
	ePlaylistEntry e( *ref, evt->start_time, evt->duration, evt->event_id, type );
	timerlist->list.push_back( e );
	eString descr	= _("no description avail");
	for (ePtrList<Descriptor>::const_iterator d(evt->descriptor); d != evt->descriptor.end(); ++d)
	{
		Descriptor *descriptor=*d;
		if (descriptor->Tag() == DESCR_SHORT_EVENT)
		{
			descr = ((ShortEventDescriptor*)descriptor)->event_name;
			break;
		}
	}
	timerlist->list.back().service.descr = descr;
	if ( ( nextStartingEvent != timerlist->list.end() && nextStartingEvent->type & ePlaylistEntry::stateWaiting)
			|| nextStartingEvent == timerlist->list.end() )
	{
		setNextEvent();				
	}

	return true;
}

eEPGContextMenu::eEPGContextMenu()
	:eListBoxWindow<eListBoxEntryText>(_("Service Menu"), 4)
{
	move(ePoint(150, 200));
	new eListBoxEntryText(&list, _("back"), (void*)0);
	new eListBoxEntryText(&list, _("add as record Timer (Harddisc)"), (void*)1);
	new eListBoxEntryText(&list, _("add as switch Timer"), (void*)2);
	CONNECT(list.selected, eEPGContextMenu::entrySelected);
}

void eEPGContextMenu::entrySelected(eListBoxEntryText *test)
{
	if (!test)
		close(0);
	else
		close((int)test->getKey());
}

eAutoInitP0<eTimerManager> init_eTimerManager(8, "Timer Manager");

gFont eListBoxEntryTimer::TimeFont;
gFont eListBoxEntryTimer::DescrFont;
gPixmap *eListBoxEntryTimer::ok=0;
gPixmap *eListBoxEntryTimer::failed=0;
int eListBoxEntryTimer::timeXSize=0;
int eListBoxEntryTimer::dateXSize=0;

struct eTimerViewActions
{
  eActionMap map;
	eAction removeTimerEntry;
	eTimerViewActions():
		map("timerView", _("timerView")),
		removeTimerEntry(map, "removeTimerEntry", _("remove this entry from timer list"), eAction::prioDialog )
	{
	}
};
eAutoInitP0<eTimerViewActions> i_TimerViewActions(5, "timer view actions");

eListBoxEntryTimer::~eListBoxEntryTimer()
{
	if (paraTime)
		paraTime->destroy();

	if (paraDate)
		paraDate->destroy();

	if (paraDescr)
		paraDescr->destroy();
}

int eListBoxEntryTimer::getEntryHeight()
{
	if (!DescrFont.pointSize && !TimeFont.pointSize)
	{
		DescrFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.Description");
		TimeFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.DateTime");
		ok = eSkin::getActive()->queryImage("ok_symbol");
		failed = eSkin::getActive()->queryImage("failed_symbol");
		eTextPara* tmp = new eTextPara( eRect(0, 0, 200, 30) );
		tmp->setFont( TimeFont );
		tmp->renderString( "00:00 - 00:00" );
		timeXSize = tmp->getBoundBox().width();
		tmp->destroy();
		tmp = new eTextPara( eRect(0, 0, 200, 30) );
		tmp->setFont( TimeFont );
		tmp->renderString( "09.09," );
		dateXSize = tmp->getBoundBox().width();
		tmp->destroy();
	}
	return calcFontHeight(DescrFont)+4;	
}

eListBoxEntryTimer::eListBoxEntryTimer( eListBox<eListBoxEntryTimer> *listbox, const ePlaylistEntry* entry )
		:eListBoxEntry((eListBox<eListBoxEntry>*)listbox), paraDate(0), paraTime(0), paraDescr(0), entry(entry)
{	
}

eString eListBoxEntryTimer::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited)
{
	drawEntryRect(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, hilited);
	
	eString hlp;

	int xpos=rect.left()+10;

	if ( entry->type & ePlaylistEntry::stateFinished )
	{
  	int ypos = ( rect.height() - ok->y ) / 2;
		rc->blit( *ok, ePoint( xpos, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);		
	}
	else if ( entry->type & 15360 ) // 15360 -> all fail ePlaylistEntry states
	{
  	int ypos = (rect.height() - failed->y) / 2;
		rc->blit( *failed, ePoint( xpos, rect.top()+ypos ), eRect(), gPixmap::blitAlphaTest);		
	}
	xpos+=24+10; // i think no people want to change the ok and false pixmaps....

	tm start_time = *localtime(&entry->time_begin);
	time_t t = entry->time_begin+entry->duration;
	tm stop_time = *localtime(&t);
	if (!paraDate)
	{
		paraDate = new eTextPara( eRect( 0, 0, dateXSize, rect.height()) );
		paraDate->setFont( TimeFont );
		eString tmp;
		tmp.sprintf("%02d.%02d,", start_time.tm_mday, start_time.tm_mon+1);
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
		tmp.sprintf("%02d:%02d - %02d:%02d", start_time.tm_hour, start_time.tm_min, stop_time.tm_hour, stop_time.tm_min);
		paraTime->renderString( tmp );
		paraTime->realign( eTextPara::dirRight );
		hlp+=tmp;
	}
	rc->renderPara(*paraTime, ePoint( xpos, rect.top() + TimeYOffs ) );
	xpos+=timeXSize+paraTime->getBoundBox().height();

	eString descr;
	if (!paraDescr)
	{
		eService* s = eServiceInterface::getInstance()->addRef( entry->service );
		if (s)
		{
			descr = s->service_name;
			eServiceInterface::getInstance()->removeRef( entry->service );
		}
		if (entry->service.descr)
			descr += " - "+entry->service.descr;
		paraDescr = new eTextPara( eRect( 0 ,0, rect.width(), rect.height()) );
		paraDescr->setFont( DescrFont );
		paraDescr->renderString( descr );
		DescrYOffs = ((rect.height() - paraDescr->getBoundBox().height()) / 2 ) - paraDescr->getBoundBox().top();
	}
	rc->renderPara(*paraDescr, ePoint( xpos, rect.top() + DescrYOffs ) );

	return hlp+" "+descr;
}

struct addToView: public std::unary_function<const ePlaylistEntry*, void>
{
	eListBox<eListBoxEntryTimer> *listbox;

	addToView(eListBox<eListBoxEntryTimer> *lb): listbox(lb)
	{
	}

	void operator()(const ePlaylistEntry* se)
	{
		new eListBoxEntryTimer(listbox, se);		
	}
};


void eTimerView::fillTimerList()
{
	eTimerManager::getInstance()->forEachEntry( addToView(events) );
}

eTimerView::eTimerView()
	:eWindow(0)
{
	events = new eListBox<eListBoxEntryTimer>(this);
	events->setName("events");
	events->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));

	if (eSkin::getActive()->build(this, "eEPGSelector"))
		eWarning("EPG selector widget build failed!");

	setText(_("Timer list"));

	CONNECT(events->selected, eTimerView::entrySelected);
	fillTimerList();
	addActionMap( &i_TimerViewActions->map );
}

int eTimerView::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_TimerViewActions->removeTimerEntry)
			{
				if ( eTimerManager::getInstance()->removeEventFromTimerList( this, *events->getCurrent()->entry ) )
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

void eTimerView::invalidateEntry( eListBoxEntryTimer *e)
{
	int i;
	if( events->forEachVisibleEntry( findEvent( *e, i ) ) == eListBoxBase::OK )
		events->invalidateEntry( i );
}

void eTimerView::entrySelected(eListBoxEntryTimer *entry)
{
}
