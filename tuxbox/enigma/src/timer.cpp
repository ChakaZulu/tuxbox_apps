#include <timer.h>

#include <enigma_main.h>
#include <lib/system/init.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/servicestructure.h>
#include <lib/gui/emessage.h>
#include <lib/gdi/font.h>

eTimerManager* eTimerManager::instance=0;

eTimerManager::eTimerManager()
	:actionTimer(eApp), timer(eApp)
{
	if (!instance)
		instance = this;

	timerlistref=eServiceReference(eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 1, 1);
	timerlist=(ePlaylist*)eServiceInterface::getInstance()->addRef(timerlistref);
	ASSERT(timerlist);
	timerlist->service_name=_("Timerlist");
	timerlist->load(CONFIGDIR "/enigma/timer.epl");
	CONNECT( actionTimer.timeout, eTimerManager::actionHandler );
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
		nextAction = setNextEvent;
		actionTimer.start(0, true);
	}
	else
	{
		eDebug("[eTimerManager] wait for clock update");
		timer.start(1000, true);  // next check in 1 sec
	}
}

void eTimerManager::actionHandler()
{
	switch( nextAction )
	{
		case zap:
			eDebug("[eTimerManager] zapToChannel");
			if ( eServiceInterface::getInstance()->service != nextStartingEvent->service )
			{
				eDebug("[eTimerManager] change to the right service");
				nextAction=stopEvent;
				// when service is changed we set it back to stateWaiting..
				nextStartingEvent->type &= ~ePlaylistEntry::stateWaiting;
				nextStartingEvent->type |= (ePlaylistEntry::stateError|ePlaylistEntry::errorZapFailed);
				actionTimer.start(5000, true);  // this is the zap Timeout
				conn = CONNECT( eDVB::getInstance()->enterService, eTimerManager::serviceChanged );
				eZapMain::getInstance()->playService( nextStartingEvent->service, eZapMain::psDontAdd ); // start Zap
			}
			else
			{
				eDebug("[eTimerManager] we are always on the right service... do not change service");
				nextAction=startCountdown;
				actionTimer.start(0, true);
			}
    break;

		case showMessage:
			eDebug("[eTimerManager] viewTimerMessage");
			long t;
			if ( (t = getSecondsToBegin()) ) // event is not running
			{
				nextAction=zap;
				actionTimer.start(60000, true ); // restart timer
				eDebug("[eTimerManager] event starts in 6 min");
				// here we can show a messagebox... event begin in bla minutes... ok.. abort...
				// messagebox timout 1 min..
			}
		/*	else
			{
				conn = CONNECT( timer.timeout, eTimerManager::stopEvent );
				int t = getTimeout();
			}*/
		break;
		
		case startCountdown:
			eDebug("[eTimerManager] startCountdown");
			eZapMain::getInstance()->toggleTimerMode();
			// now in eZapMain the RemoteControl should be handled for TimerMode...
			// an service change now stop the Running Event and set it to userAborted
	  	conn = CONNECT( eDVB::getInstance()->leaveService, eTimerManager::leaveService );
			if ( nextStartingEvent->type & ePlaylistEntry::typeSmartTimer )
			{
				conn2 = CONNECT( eDVB::getInstance()->tEIT.tableReady, eTimerManager::EITready );
				EITready(0);  // check current eit now !
			}
			else
			{
				long t = getSecondsToBegin();
				nextAction=startEvent;
				if (t > 0)
					actionTimer.start( t*1000 , true );
				else
					actionHandler();
			}
		break;

		case startEvent:
			nextStartingEvent->type &= ~ePlaylistEntry::stateWaiting;
			nextStartingEvent->type |= ePlaylistEntry::stateRunning;
			eDebug("[eTimerManager] startEvent");
			switch ( nextStartingEvent->type & (ePlaylistEntry::RecTimerEntry|ePlaylistEntry::SwitchTimerEntry) )
			{
				case ePlaylistEntry::SwitchTimerEntry:
				// here we must do nothing... we have always zapped...
				break;	

				case ePlaylistEntry::RecTimerEntry:
					nextAction = startRecording;
					actionHandler();
				break;
			}
			if ( !(nextStartingEvent->type & ePlaylistEntry::typeSmartTimer) )
			{
				nextAction = stopEvent;
				actionTimer.start( getSecondsToEnd() * 1000, true );
			}
		break;

  	case pauseEvent:
			eDebug("[eTimerManager] pauseEvent");
			if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
			{
				nextStartingEvent->type &= ~ePlaylistEntry::stateRunning;
				nextStartingEvent->type |= ePlaylistEntry::statePaused;
        nextAction = pauseRecording;
        actionHandler();
      }
		break;

		case restartEvent:
			eDebug("[eTimerManager] restartEvent");
			if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
			{
				nextStartingEvent->type &= ~ePlaylistEntry::statePaused; // we delete current state...
				nextStartingEvent->type |= ePlaylistEntry::stateRunning;
				nextAction=restartRecording;
				actionHandler();
			}
    break;
	
		case stopEvent:
			eDebug("[eTimerManager] stopEvent");
			if( nextStartingEvent->type & ePlaylistEntry::stateRunning )
			{
    		nextStartingEvent->type &= ~ePlaylistEntry::stateRunning;
				if ( !(nextStartingEvent->type & ePlaylistEntry::stateError) )
					nextStartingEvent->type |= ePlaylistEntry::stateFinished;
          // when no ErrorCode is set the we set the state to finished
				if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
				{
					nextAction=stopRecording;
					actionHandler();
				}
			}
			nextAction=setNextEvent;	// we set the Next Event... a new beginning *g*
			actionTimer.start(0, true);
		break;

		case setNextEvent:
		{
			if (conn.connected() )
				conn.disconnect();
			if (conn2.connected() )
				conn2.disconnect();
			eDebug("[eTimerManager] setNextEvent");
			nextStartingEvent=timerlist->list.end();
			int timeToNextEvent=0xFFFF, count=0;
			// parse events... invalidate old, set nextEvent Timer
			for (	std::list< ePlaylistEntry >::iterator i(timerlist->list.begin()); i != timerlist->list.end(); i++ )
			{
				time_t nowTime=time(0)+eDVB::getInstance()->time_difference;
				if ( i->type & ePlaylistEntry::stateWaiting )
					if ( i->type & ePlaylistEntry::stateError )
						i->type &= ~ePlaylistEntry::stateWaiting;
					else if ( i->time_begin+i->duration < nowTime ) // old event found
					{
						i->type &= ~ePlaylistEntry::stateWaiting;
						i->type |= (ePlaylistEntry::stateError|ePlaylistEntry::errorOutdated);
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
				int prepareTime = 0;
				if ( nextStartingEvent->type & ePlaylistEntry::typeSmartTimer )
				{  // EIT related zapping ....
					if ( t > prepareTime )
					{
						nextAction=showMessage;
						actionTimer.start( (t - 360) * 1000, true );		// set the Timer to eventBegin - 6 min
					}
					else  // time to begin is under 6 min or is currently running
					{
						nextAction=zap;
						actionHandler();
					}
				}
				else
				{
					nextAction=zap;
					actionTimer.start( t * 1000, true );		// set the Timer to eventBegin
				}
			}
		}
		break;

		case startRecording:
//			if (nextStartingEvent != timerlist->list.end())
      if (nextStartingEvent->type & ePlaylistEntry::recDVR)
      {
  			eZapMain::getInstance()->recordDVR(1, 0, nextStartingEvent->service.descr);
      }
      else  // insert lirc ( VCR start ) here
      {

      }
			break;

    case stopRecording:
      if (nextStartingEvent->type & ePlaylistEntry::recDVR)
      {
				eZapMain::getInstance()->toggleTimerMode();        
				eZapMain::getInstance()->recordDVR(0, 0);
      }
      else  // insert lirc ( VCR stop ) here
      {

      }
			break;

		case restartRecording:
		{
      if (nextStartingEvent->type & ePlaylistEntry::recDVR)
      {
  		 	eServiceHandler *handler=eServiceInterface::getInstance()->getService();
  			if (!handler)
	  			eFatal("no service Handler");
        handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStart));
      }
      else // insert lirc ( VCR START )
      {
        
      }
			eDebug("ok, recording...");
		}
		break;	

		case pauseRecording:
		{
      if (nextStartingEvent->type & ePlaylistEntry::recDVR)
      {
        eServiceHandler *handler=eServiceInterface::getInstance()->getService();
  			if (!handler)
  				eFatal("no service Handler");
  			handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStop));
      }
      else // insert lirc ( VCR PAUSE )
      {

      }
		}

		default:
			eDebug("unhandled timer action");
	}
}                                                                                                   

void eTimerManager::serviceChanged( const eServiceReferenceDVB& ref )
{
	eDebug("[eTimerManager] serviceChanged");
	if ( nextStartingEvent->service == (eServiceReference&)ref )
	{
		conn.disconnect();
		actionTimer.stop(); // stop zapTimeout
		nextStartingEvent->type &= ~(ePlaylistEntry::stateError|ePlaylistEntry::errorZapFailed);
		nextStartingEvent->type |= ePlaylistEntry::stateWaiting;
		nextAction=startCountdown;
		actionHandler();
	}
}

void eTimerManager::leaveService( const eServiceReferenceDVB& ref )
{
	eDebug("[eTimerManager] leaveService");
	// this works from 5 min before the event begin to end of event...
	nextAction=stopEvent;
	nextStartingEvent->type |= (ePlaylistEntry::stateError|ePlaylistEntry::errorUserAborted);
	actionTimer.start(0, true);
}

void eTimerManager::EITready( int error )
{
	eDebug("[eTimerManager] EITready %s", strerror(-error));
	EIT *eit = eDVB::getInstance()->tEIT.getCurrent();
	if (!error && eit)
	{
		for (ePtrList<EITEvent>::const_iterator event(eit->events); event != eit->events.end(); ++event)		// always take the first one
		{
			if ( nextStartingEvent != timerlist->list.end() && event->event_id == nextStartingEvent->event_id )
			{
				eDebugNoNewLine("running_status(%d) = ", event->running_status );
				switch( event->running_status )
				{
					case 0:
						eDebug("undefined");
						// premiere world sends either undefined or running
					case 1:
						eDebug("not running");					
						if ( nextStartingEvent->type & ePlaylistEntry::stateRunning )
						{
							nextAction=stopEvent;
							actionHandler();
						}
					break;

					case 2:
						eDebug("starts in few seconds");					
					break;

					case 3:
						eDebug("pausing");					
						if ( nextStartingEvent->type & ePlaylistEntry::stateRunning )
						{
							nextAction=pauseEvent;
							actionHandler();
						}
					break;

					case 4:
						eDebug("running");
						if ( nextStartingEvent->type & ePlaylistEntry::stateWaiting )
							nextAction=startEvent;
						else if ( nextStartingEvent->type & ePlaylistEntry::statePaused )
							nextAction=restartEvent;
						else
							break;
						actionHandler();
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

bool Overlap( time_t beginTime1, int duration1, time_t beginTime2, int duration2 )
{
  eRect movie1( ePoint(beginTime1, 0), eSize( duration1, 10) );
  eRect movie2( ePoint(beginTime2, 0), eSize( duration2, 10) );

  return movie1.intersects(movie2);
}

bool eTimerManager::removeEventFromTimerList( eWidget *sel, const ePlaylistEntry& entry, int type )
{
	for ( std::list<ePlaylistEntry>::iterator i( timerlist->list.begin() ); i != timerlist->list.end(); i++)
		if ( *i == entry )
		{
			sel->hide();
			eString str1, str2, str3;
			if (type == erase)
			{
				str1 = _("You would to delete the running event.. this stops the timer mode (recording)!");
				str2 = _("Delete event from timerlist");
				str3 = _("Really delete this event?");
			}
     	else if (type == update)
			{
				str1 = _("You would to update the running event.. this stops the timer mode (recording)!");
				str2 = _("Update event in timerlist");
				str3 = _("Really update this event?");
			}
	 		if ( &(*nextStartingEvent) == &entry && entry.type & ePlaylistEntry::stateRunning  )
			{
				eMessageBox box(str1, str2, eMessageBox::btOK|eMessageBox::iconWarning );
				box.show();
				box.exec();
				box.hide();
	  	}
			eMessageBox box(str3, str2, eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo);
			box.show();
			int r=box.exec();
			box.hide();
			if (r == eMessageBox::btYes)
			{
				timerlist->list.erase(i);
		 		if ( &(*nextStartingEvent) == &entry )
				{
					nextAction=stopEvent;
					nextStartingEvent->type |= (ePlaylistEntry::stateError | ePlaylistEntry::errorUserAborted);
					actionHandler();
				}
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

bool eTimerManager::addEventToTimerList( eWidget *sel, const ePlaylistEntry& entry )
{
	time_t nowTime = time(0)+eDVB::getInstance()->time_difference;
	if ( entry.time_begin < nowTime )
	{
		eMessageBox box(_("This event already began.\nYou can not add this to timerlist"), _("Add event to timerlist"), eMessageBox::iconWarning|eMessageBox::btOK);
		sel->hide();
		box.show();
		box.exec();
		box.hide();
		sel->show();
		return false;
	}
  for ( std::list<ePlaylistEntry>::iterator i( timerlist->list.begin() ); i != timerlist->list.end(); i++)
		if ( ( entry.event_id != -1 && entry.event_id == i->event_id ) ||
			   ( entry.service == i->service && entry.time_begin == i->time_begin ) )
		{
			eMessageBox box(_("This event is already in the timerlist."), _("Add event to timerlist"), eMessageBox::iconWarning|eMessageBox::btOK);
			sel->hide();
			box.show();
			box.exec();
			box.hide();
			sel->show();
			return false;
		}
		else if ( Overlap( entry.time_begin, entry.duration, i->time_begin, i->duration) )
		{
			eMessageBox box(_("This event can not added to the timerlist.\nThe event overlaps with another event in the timerlist\nPlease check manually the timerlist."), _("Add event to timerlist"), eMessageBox::iconWarning|eMessageBox::btOK);
			sel->hide();
			box.show();
			box.exec();
			box.hide();
			sel->show();
			return false;
		}

	timerlist->list.push_back( entry );
	if ( ( ( nextStartingEvent != timerlist->list.end() )	&& (nextStartingEvent->type & ePlaylistEntry::stateWaiting) )
			|| ( nextStartingEvent == timerlist->list.end() ) )
	{
		nextAction = setNextEvent;
		actionHandler();		
	}
	return true;
}

bool eTimerManager::addEventToTimerList( eWidget *sel, const eServiceReference *ref, const EITEvent *evt, int type )
{
	ePlaylistEntry e( *ref, evt->start_time, evt->duration, evt->event_id, type );
// add the event description	
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
	e.service.descr = descr;

	return addEventToTimerList( sel, e );
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

eListBoxEntryTimer::eListBoxEntryTimer( eListBox<eListBoxEntryTimer> *listbox, ePlaylistEntry* entry )
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
	else if ( entry->type & ePlaylistEntry::stateError )
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

struct _selectEvent: public std::unary_function<eListBoxEntryTimer&, void>
{
	ePlaylistEntry* entry;

	_selectEvent( ePlaylistEntry* entry ): entry(entry)
	{
	}

	bool operator()(eListBoxEntryTimer& e)
	{
		if ( *e.entry == *entry )
		{
			((eListBox<eListBoxEntryTimer>*)e.listbox)->setCurrent(&e);
			return true;
		}
		return false;
	}
};

void eTimerView::selectEvent( ePlaylistEntry* e )
{
	if ( events->forEachEntry( _selectEvent( e ) ) != eListBoxBase::OK )
		events->moveSelection( eListBox<eListBoxEntryTimer>::dirFirst );
}

struct addToView: public std::unary_function<ePlaylistEntry*, void>
{
	eListBox<eListBoxEntryTimer> *listbox;

	addToView(eListBox<eListBoxEntryTimer> *lb): listbox(lb)
	{
	}

	void operator()(ePlaylistEntry* se)
	{
		new eListBoxEntryTimer(listbox, se);		
	}
};

struct findService: public std::unary_function<const eListBoxEntryText&, void>
{
	eServiceReference service;
	const void *&entry;

	findService(const eServiceReference& e, const void *& entry): service(e), entry(entry)
	{
	}

	bool operator()(const eListBoxEntryText& s)
	{
		if (service == *(const eServiceReference*)s.getKey() )
		{
			entry=s.getKey();
			return 1;
		}
		return 0;
	}
};

void eTimerView::fillTimerList()
{
	events->beginAtomic();
	events->clearList();
	eTimerManager::getInstance()->forEachEntry( addToView(events) );
	events->sort();
	events->endAtomic();
  if (!events->getCount())
    setFocus(byear);
}

const unsigned char monthdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
const char *monthStr[12] = { _("January"), _("February"), _("March"),
													_("April"), _("May"), _("June"),	_("July"),
													_("August"), _("September"), _("October"),
													_("November"), _("December") };
const char *dayStr[7] = { _("Sunday"), _("Monday"), _("Tuesday"), _("Wednesday"),
											 _("Thursday"), _("Friday"), _("Saturday") };

int weekday (int d, int m, int y)
{
  static char table[13] = {0,0,3,2,5,0,3,5,1,4,6,2,4};
  if (m<3) --y;
  return (y+y/4-y/100+y/400+table[m]+d)%7;
}

eTimerView::eTimerView( ePlaylistEntry* e)
	:eWindow(0)
{
	events = new eListBox<eListBoxEntryTimer>(this);
	events->setName("events");
	events->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));

	byear = new eComboBox(this);
	byear->setName("b_year");

	bmonth = new eComboBox(this);
	bmonth->setName("b_month");

	bday = new eComboBox(this);
	bday->setName("b_day");

	btime = new eNumber( this, 2, 0, 59, 2, 0);
	btime->setName("b_time");
	btime->setFlags( eNumber::flagDrawPoints|eNumber::flagFillWithZeros|eNumber::flagTime );
	CONNECT( btime->selected, eTimerView::focusNext );

	eyear = new eComboBox(this);
	eyear->setName("e_year");

	emonth = new eComboBox(this);
	emonth->setName("e_month");

	eday = new eComboBox(this);
	eday->setName("e_day");

	etime = new eNumber( this, 2, 0, 59, 2, 0 );
	etime->setName("e_time");
	etime->setFlags( eNumber::flagDrawPoints|eNumber::flagFillWithZeros|eNumber::flagTime );
	CONNECT( etime->selected, eTimerView::focusNext );

	type = new eComboBox( this );
	type->setName("type");

  bSelectService = new eButton( this );
  bSelectService->setName("select_service");
  CONNECT( bSelectService->selected, eTimerView::showServiceSelector );

	bclose = new eButton( this );
	bclose->setName("close");
	CONNECT( bclose->selected, eTimerView::accept );

	update = new eButton( this );
	update->setName("update");
	CONNECT( update->selected, eTimerView::updatePressed );

	add = new eButton( this );
	add->setName("add");
	CONNECT( add->selected, eTimerView::addPressed );

	erase = new eButton( this );
	erase->setName("remove");
	CONNECT( erase->selected, eTimerView::erasePressed );

	if (eSkin::getActive()->build(this, "eTimerView"))
		eWarning("Timer view widget build failed!");

	setText(_("Timer list"));

	CONNECT(events->selected, eTimerView::entrySelected);
	CONNECT(events->selchanged, eTimerView::selChanged);
	CONNECT(byear->selchanged_id, eTimerView::comboBoxClosed);
	CONNECT(bmonth->selchanged_id, eTimerView::comboBoxClosed);
	CONNECT(bday->selchanged_id, eTimerView::comboBoxClosed);
	CONNECT(eyear->selchanged_id, eTimerView::comboBoxClosed);
	CONNECT(emonth->selchanged_id, eTimerView::comboBoxClosed);
	CONNECT(eday->selchanged_id, eTimerView::comboBoxClosed);

	fillTimerList();

	if (e)
	{
		selectEvent(e);
		setFocus( byear );
	}

	addActionMap( &i_TimerViewActions->map );
	
	time_t tmp = time(0)+eDVB::getInstance()->time_difference;
	tm now = *localtime( &tmp );

	for ( int i=0; i<10; i++ )
		new eListBoxEntryText( *byear, eString().sprintf("20%02d", now.tm_year+(i-100)), (void*) (now.tm_year+i) );

	for ( int i=0; i<=11; i++ )
		new eListBoxEntryText( *bmonth, monthStr[i], (void*) i );

	for ( int i=0; i<10; i++ )
		new eListBoxEntryText( *eyear, eString().sprintf("20%02d", now.tm_year+(i-100)), (void*) (now.tm_year+i) );

	for ( int i=0; i<=11; i++ )
		new eListBoxEntryText( *emonth, monthStr[i], (void*) i );

	new eListBoxEntryText( *type, _("switch"), (void*) ePlaylistEntry::SwitchTimerEntry );
	new eListBoxEntryText( *type, _("record DVR"), (void*) (ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recDVR) );
//	new eListBoxEntryText( *type, _("record VCR"), (void*) ePlaylistEntry::RecTimerEntry|ePlaylisteEntry::recVCR ); );  

	if ( events->getCount() )
		selChanged( events->getCurrent() );
	else
		selChanged(0);
}

void eTimerView::updatePressed()
{
	eString descr;
	EITEvent evt;
	time_t newEventBegin;
	int newEventDuration;
	if ( getData( newEventBegin, newEventDuration ) )  // all is okay... we add the event..
	{
		evt.start_time = newEventBegin;
		evt.duration = newEventDuration;
		evt.running_status = -1;
		evt.free_CA_mode = -1;
		if ( events->getCount() && events->getCurrent()->entry )
		{
			time_t oldEventBegin = events->getCurrent()->entry->time_begin;
	  	int oldEventDuration = events->getCurrent()->entry->duration;
			if ( tmpService == events->getCurrent()->entry->service
				 	&& Overlap( newEventBegin, newEventDuration, oldEventBegin, oldEventDuration ) )
			{ // we have a description...
				descr = events->getCurrent()->entry->service.descr;
				evt.event_id = events->getCurrent()->entry->event_id;
			}
			else
				evt.event_id = -1;
			eTimerManager::getInstance()->removeEventFromTimerList( this, *events->getCurrent()->entry, eTimerManager::update );
		}
		if ( eTimerManager::getInstance()->addEventToTimerList( this, &tmpService, &evt, ((int)type->getCurrent()->getKey()) | ePlaylistEntry::stateWaiting ) )
		{
			ePlaylistEntry *entry = eTimerManager::getInstance()->findEvent( &tmpService, &evt);
			if (descr)
				entry->service.descr=descr;
			events->beginAtomic();
			fillTimerList();
			selectEvent( entry );
			events->endAtomic();
		}
	}
	else
	{
		hide();
		eMessageBox box(_("Invalid begin or end time.!\nPlease check time and date"), _("Update event in timerlist"), eMessageBox::iconWarning|eMessageBox::btOK);
		box.show();
		box.exec();
		box.hide();
		show();
	}
}

void eTimerView::erasePressed()
{
	if ( eTimerManager::getInstance()->removeEventFromTimerList( this, *events->getCurrent()->entry ) )
		fillTimerList();
}

bool eTimerView::getData( time_t &bTime, int &duration )
{
  beginTime.tm_year = (int)byear->getCurrent()->getKey();
	beginTime.tm_mon = (int)bmonth->getCurrent()->getKey();
	beginTime.tm_mday = (int)bday->getCurrent()->getKey();
	beginTime.tm_hour = btime->getNumber(0);
	beginTime.tm_min = btime->getNumber(1);
	beginTime.tm_sec = 0;
	bTime = mktime( &beginTime );
  endTime.tm_year = (int)eyear->getCurrent()->getKey();
	endTime.tm_mon = (int)emonth->getCurrent()->getKey();
	endTime.tm_mday = (int)eday->getCurrent()->getKey();
	endTime.tm_hour = etime->getNumber(0);
	endTime.tm_min = etime->getNumber(1);
	endTime.tm_sec = 0;

	time_t tmp = mktime( &endTime );
	duration = tmp - bTime;

	return duration > -1;
}

void eTimerView::addPressed()
{
	EITEvent evt;
	time_t newEventBegin;
	int newEventDuration;
	if ( getData( newEventBegin, newEventDuration ) )  // all is okay... we add the event..
	{
		evt.start_time = newEventBegin;
		evt.duration = newEventDuration;
		evt.event_id = -1;
		evt.running_status = -1;
		evt.free_CA_mode = -1;
		eTimerManager::getInstance()->addEventToTimerList( this, &tmpService, &evt, ((int)type->getCurrent()->getKey()) | ePlaylistEntry::stateWaiting );
		if (descr)
			eTimerManager::getInstance()->findEvent( &tmpService, &evt)->service.descr=descr;
		fillTimerList();
	}
	else
	{
		hide();
		eMessageBox box(_("Invalid begin or end time.!\nYou can not add this to timerlist"), _("Add event to timerlist"), eMessageBox::iconWarning|eMessageBox::btOK);
		box.show();
		box.exec();
		box.hide();
		show();
	}
}

void eTimerView::selChanged( eListBoxEntryTimer *entry )
{
	if (entry && entry->entry )
	{
		beginTime = *localtime( &entry->entry->time_begin );
		time_t tmp = entry->entry->time_begin + entry->entry->duration;
		endTime = *localtime( &tmp );
		updateDateTime( beginTime, endTime );
		type->setCurrent( (void*) ( entry->entry->type & (ePlaylistEntry::RecTimerEntry|ePlaylistEntry::SwitchTimerEntry|ePlaylistEntry::recDVR|ePlaylistEntry::recVCR) ) );
    eService *service = eServiceInterface::getInstance()->addRef( entry->entry->service );
    if (service)
    {
      tmpService = entry->entry->service;
      bSelectService->setText( service->service_name );
      eServiceInterface::getInstance()->removeRef( eServiceInterface::getInstance()->service );
    }
  }
	else
	{
		time_t now = time(0)+eDVB::getInstance()->time_difference;
		tm tmp = *localtime( &now );
		updateDateTime( tmp, tmp );
		type->setCurrent( (void*)(ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recDVR) );

		eServiceReference ref = eServiceInterface::getInstance()->service;

    if (ref.type == eServiceReference::idDVB)
    {
      eService *service = eServiceInterface::getInstance()->addRef( eServiceInterface::getInstance()->service );
      if (service)
      {
        tmpService = ref;
        bSelectService->setText( service->service_name );
        eServiceInterface::getInstance()->removeRef( eServiceInterface::getInstance()->service );
      }
    }
		else  // we have no service
    {
      bSelectService->setText( _("choose service") );
      tmpService = eServiceReference();
    }
	}
}

void eTimerView::updateDateTime( const tm& beginTime, const tm& endTime )
{
		updateDay( bday, beginTime.tm_year+1900, beginTime.tm_mon+1, beginTime.tm_mday );
		updateDay( eday, endTime.tm_year+1900, endTime.tm_mon+1, endTime.tm_mday );

		btime->setNumber( 0, beginTime.tm_hour );
		btime->setNumber( 1, beginTime.tm_min );

		byear->setCurrent( (void*) beginTime.tm_year );
		bmonth->setCurrent( (void*) beginTime.tm_mon );

		etime->setNumber( 0, endTime.tm_hour );
		etime->setNumber( 1, endTime.tm_min );

		eyear->setCurrent( (void*) endTime.tm_year );
		emonth->setCurrent( (void*) endTime.tm_mon );
}

void eTimerView::updateDay( eComboBox* dayCombo, int year, int month, int day )
{
	dayCombo->clear();
	int wday = weekday( 1, month, year );
	int days = monthdays[ month-1 ];
	days += (days == 28 && __isleap( year ) );
	for ( int i = wday; i < wday+days; i++ )
		new eListBoxEntryText( *dayCombo, eString().sprintf("%s, %02d", dayStr[i%7], i-wday+1), (void*) (i-wday+1) );
  dayCombo->setCurrent( day>days ? 0 : (void*) day );
}


int eTimerView::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
		case eWidgetEvent::evtAction:
/*			if (event.action == &i_focusActions->left && focus != events)
				eWidget::focusNext(eWidget::focusDirW);
			else if (event.action == &i_focusActions->right && focus != events)
				eWidget::focusNext(eWidget::focusDirE);
			else if (event.action == &i_focusActions->up && focus != events)
				eWidget::focusNext(eWidget::focusDirN);
			else if (event.action == &i_focusActions->down && focus != events)
				eWidget::focusNext(eWidget::focusDirS);
			else*/ if (event.action == &i_TimerViewActions->removeTimerEntry)
			{
				erasePressed();
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

void eTimerView::comboBoxClosed( eComboBox *combo,  eListBoxEntryText* )
{
	if ( combo == bmonth || combo == byear )
			updateDay( bday, (int) byear->getCurrent()->getKey()+1900, (int) bmonth->getCurrent()->getKey()+1, (int) bday->getCurrent()->getKey() );
	else if ( combo == emonth || combo == eyear )
			updateDay( eday, (int) eyear->getCurrent()->getKey()+1900, (int) emonth->getCurrent()->getKey()+1, (int) eday->getCurrent()->getKey() );
}

void eTimerView::invalidateEntry( eListBoxEntryTimer *e)
{
	int i;
	if( events->forEachVisibleEntry( findEvent( *e, i ) ) == eListBoxBase::OK )
		events->invalidateEntry( i );
}

void eTimerView::entrySelected(eListBoxEntryTimer *entry)
{
	setFocus( byear );
}

void eTimerView::showServiceSelector()
{
  eServiceSelector sel;
	sel.setLCD(LCDTitle, LCDElement);
  hide();
  sel.setPath(eServiceStructureHandler::getRoot(eServiceStructureHandler::modeTvRadio),eServiceReference() );
  sel.setStyle(eServiceSelector::styleSingleColumn);

  if ( tmpService != eServiceReference() )
    sel.selectServiceRecursive( tmpService );
    
  const eServiceReference *ref = sel.choose(-1);

  if (ref)
  {
    if (tmpService != *ref)
    {
      tmpService = *ref;
      eService *service =	eServiceInterface::getInstance()->addRef( tmpService );
      bSelectService->setText(service->service_name);
      eServiceInterface::getInstance()->removeRef( tmpService );
    }
  }
  show();
  setFocus(bSelectService);
}
