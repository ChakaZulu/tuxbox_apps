#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <timer.h>
#include <engrab.h>
#include <elirc.h>
#include <enigma_main.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/servicestructure.h>
#include <lib/gui/emessage.h>
#include <lib/gdi/font.h>
#include <lib/gui/textinput.h>
#include <lib/gui/enumber.h>
#include <lib/gui/combobox.h>
#include <lib/gui/echeckbox.h>
#include <lib/system/econfig.h>

#define TIMER_LOGFILE CONFIGDIR "/enigma/timer.log"

static const unsigned char monthdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
/* bug fix - at localization, 
   macro the type _ ("xxxxx") for a constant does not work, 
   if it is declared outside of the function
   
static const char *monthStr[12] = { _("January"), _("February"), _("March"),
													_("April"), _("May"), _("June"),	_("July"),
													_("August"), _("September"), _("October"),
													_("November"), _("December") };
static const char *dayStr[7] = { _("Sunday"), _("Monday"), _("Tuesday"), _("Wednesday"),
											 _("Thursday"), _("Friday"), _("Saturday") };
const char *dayStrShort[7] = { _("Sun"), _("Mon"), _("Tue"), _("Wed"),
											 _("Thu"), _("Fri"), _("Sat") };
*/
eTimerManager* eTimerManager::instance=0;

void normalize( struct tm & );

bool Overlapping( const ePlaylistEntry &e1, const ePlaylistEntry &e2 );

static time_t getNextEventStartTime( time_t t, int duration, int type, bool notToday )
{
	if ( type < ePlaylistEntry::isRepeating )
		return 0;

	time_t now = time(0)+eDVB::getInstance()->time_difference;
	tm tmp = *localtime( &now ),  // now
		tmp2 = *localtime( &t );    // activation hour:min

// calc mask for today weekday
	int i = ePlaylistEntry::Su;
	for ( int x = 0; x < tmp.tm_wday; x++ )
		i*=2;

	bool found=false;

	for ( int d=0; d<8; d++,
											i*=2,
											tmp.tm_mday++ )
	{
		if ( i > ePlaylistEntry::Sa )  // wrap around..
			i = ePlaylistEntry::Su;

		if ( type & i )
		{
			if ( !d ) // if this today ?
			{
				int begTimeSec = tmp2.tm_hour*3600+tmp2.tm_min*60+tmp2.tm_sec;
				int nowTimeSec = tmp.tm_hour*3600+tmp.tm_min*60+tmp.tm_sec;

				if ( nowTimeSec > begTimeSec+duration || notToday )
					continue;
			}
			found = true;
			break;
		}
	}
	if ( !found )  // No day(s) selected
		return 0;
	tmp.tm_hour = tmp2.tm_hour;
	tmp.tm_min = tmp2.tm_min;
	tmp.tm_sec = tmp2.tm_sec;
	normalize( tmp );
	return mktime(&tmp);
}

static eString getRight(const eString& str, char c )
{
	unsigned int found = str.find(c);
	unsigned int beg = ( found != eString::npos ? found : 0 );
	unsigned int len = str.length();
	if ( found != eString::npos )
		beg++;
	return str.mid( beg, len-beg );
}

static int getDate()
{
	time_t tmp = time(0)+eDVB::getInstance()->time_difference;
	tm now = *localtime(&tmp);
	return ((100+now.tm_mday)*1000000)+((100+now.tm_mon+1)*1000)+now.tm_year;
}

static eString getLeft( const eString& str, char c )
{
	unsigned int found = str.find(c);
	return found != eString::npos ? str.left(found):str;
}

static const eString& getEventDescrFromEPGCache( const eServiceReference &_ref, time_t time )
{
	static eString descr;
	descr="";
	const eServiceReferenceDVB &ref = (eServiceReferenceDVB&)_ref;
	// parse EPGCache to get event informations
	EITEvent *tmp = eEPGCache::getInstance()->lookupEvent( ref, time );
	if (tmp)
	{
		for (ePtrList<Descriptor>::const_iterator d(tmp->descriptor); d != tmp->descriptor.end(); ++d)
		{
			if ( d->Tag() == DESCR_SHORT_EVENT)
			{
				ShortEventDescriptor *s=(ShortEventDescriptor*)*d;
				descr=s->event_name;
				if ((s->text.length() > 0) && (s->text!=descr))
					descr+=" - "+s->text;
				break;
			}
		}
		delete tmp;
	}
	return descr;
}

static eString buildDayString(int type)
{
	eString tmp;
	if ( type & ePlaylistEntry::isRepeating )
	{
		const char *dayStrShort[7] = { _("Sun"), _("Mon"), _("Tue"), _("Wed"), _("Thu"), _("Fri"), _("Sat") };
		int mask = ePlaylistEntry::Su;
		for ( int i = 0; i < 7; i++ )
		{
			if ( type & mask )
			{
				tmp+=dayStrShort[i];
				if ( i != 2 && i != 3 && i != 4 )
					tmp.erase( tmp.length()-1 );
				tmp+=' ';
			}
			mask*=2;
		}
		if ( tmp )
			tmp.erase(tmp.length()-1);
	}
	return tmp;
}

void eTimerManager::writeToLogfile( const char *str )
{
	if ( logfile && str )
	{
		time_t tmp = time(0)+eDVB::getInstance()->time_difference;
		tm now = *localtime( &tmp );
		fprintf(logfile, "%02d.%02d, %02d:%02d - %s\n", now.tm_mday, now.tm_mon+1, now.tm_hour, now.tm_min, str );
		fflush(logfile);
	}
}

void eTimerManager::writeToLogfile( eString str )
{
	if ( str.length() )
		writeToLogfile( str.c_str() );
}

eTimerManager::eTimerManager()
	:actionTimer(eApp), timer(eApp)
{
	if (!instance)
		instance = this;

	eServicePlaylistHandler::getInstance()->addNum( 5 );
	timerlistref=eServiceReference( eServicePlaylistHandler::ID, eServiceReference::flagDirectory, 0, 5);
	timerlist=(ePlaylist*)eServiceInterface::getInstance()->addRef(timerlistref);
	ASSERT(timerlist);
	timerlist->service_name=_("Timerlist");
	timerlist->load(CONFIGDIR "/enigma/timer.epl");
	nextStartingEvent=timerlist->getList().end();
	CONNECT( actionTimer.timeout, eTimerManager::actionHandler );
	conn = CONNECT( timer.timeout, eTimerManager::waitClock );
	waitClock();

	struct stat tmp;
	if ( stat( TIMER_LOGFILE, &tmp ) != -1 )
	{
		if ( tmp.st_size > 102400 )
		{
			eDebug("timer logfile is bigger than 100Kbyte.. shrink to 32kByte");
			rename(TIMER_LOGFILE, "/var/timer.old");
		}
	}
	logfile = fopen(TIMER_LOGFILE, "a" );

	FILE *old = fopen("/var/timer.old", "r");
	if ( old )
	{
		fseek(old, tmp.st_size-32768, SEEK_SET );
		char buf[32768];
		int rbytes=0;
		if( ( rbytes = fread( buf, 1, 32768, old ) ) )
			fwrite( buf, 1, rbytes, logfile );
		fclose(old);
		unlink("/var/timer.old");
	}
	writeToLogfile("Timer is comming up");
}

// called only once... at start of eTimerManager
void eTimerManager::waitClock()
{
	if (eDVB::getInstance()->time_difference)
	{
		eDebug("[eTimerManager] timeUpdated");
		writeToLogfile("Time is updated");
		nextAction = setNextEvent;
		actionTimer.start(0, true);
	}
	else
	{
		eDebug("[eTimerManager] wait for clock update");
		timer.start(1000, true);  // next check in 1 sec
	}
}

void eTimerManager::loadTimerList()
{
	writeToLogfile("--> loadTimerList()");
	timerlist->load(CONFIGDIR "/enigma/timer.epl");
	if ( !(nextStartingEvent->type & ePlaylistEntry::stateRunning) )
	{
		nextAction = setNextEvent;
		actionTimer.start(0,true);
	}
	writeToLogfile("<-- loadTimerList()");
}

void eTimerManager::saveTimerList()
{
	writeToLogfile("saveTimerlist()");
	timerlist->save();
}

void eTimerManager::timeChanged()
{
	writeToLogfile("--> timeChanged()");
	if ( nextStartingEvent != timerlist->getConstList().end()
		&& !(nextStartingEvent->type & ePlaylistEntry::stateRunning) )
	{
		nextAction=setNextEvent;
		actionTimer.start(0, true);
	}
	writeToLogfile("<-- timeChanged()");
}

#define WOL_PREPARE_TIME 240
#define ZAP_BEFORE_TIME 60
#define HDD_PREPARE_TIME 10

void eTimerManager::actionHandler()
{
	static int calldepth=0;
	switch( nextAction )
	{
		case zap:
// check if the time to next starting ev
			eDebug("[eTimerManager] zapToChannel");
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d zap", ++calldepth));
			if ( !(nextStartingEvent->type&ePlaylistEntry::doFinishOnly) )
			{
				writeToLogfile("call eZapMain::getInstance()->handleStandby()");
				eZapMain::getInstance()->handleStandby();
			}
			if ( nextStartingEvent->service &&
					eServiceInterface::getInstance()->service != nextStartingEvent->service )
			{
				eDebug("[eTimerManager] change to the right service");
				writeToLogfile("must zap to new service");
				conn = CONNECT( eDVB::getInstance()->switchedService, eTimerManager::switchedService );
				eString save = nextStartingEvent->service.descr;
				nextStartingEvent->service.descr = getLeft( nextStartingEvent->service.descr, '/' );

				// get Parentallocking state
				int pLockActive = eConfig::getInstance()->pLockActive() && nextStartingEvent->service.isLocked();

				if ( pLockActive ) // P Locking is active ?
				{
					writeToLogfile("Parentallocking is active.. disable");
					eConfig::getInstance()->locked=false;  // then disable for zap
				}

				// switch to service
				eZapMain::getInstance()->playService( nextStartingEvent->service, eZapMain::psSetMode );

				if ( pLockActive )  // reenable Parental locking
				{
					writeToLogfile("reenable parentallocking");
					eConfig::getInstance()->locked=true;
				}

				nextStartingEvent->service.descr=save;
			}
			else
			{
				writeToLogfile("we are always on the correct service");
				eDebug("[eTimerManager] we are always on the right service... do not change service");
				nextAction=startCountdown;
				actionTimer.start(0, true);
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d", calldepth--));
			break;

#ifndef DISABLE_NETWORK
		case prepareEvent:
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d prepareEvent", ++calldepth));
			eDebug("[eTimerManager] prepareEvent");
			if ( nextStartingEvent->type & ePlaylistEntry::recNgrab )
			{
				writeToLogfile("recNgrab");
				char *serverMAC=0;
				if ( !eConfig::getInstance()->getKey("/elitedvb/network/hwaddress", serverMAC ) )
				{
					if ( system( eString().sprintf("etherwake -i eth0 %s", serverMAC ).c_str() ) )
					{
						writeToLogfile("could not execute etherwake");
						eDebug("[eTimerManager] could not execute etherwake");
					}
					else
					{
						writeToLogfile("Wake On Lan sent...");
						eDebug("[eTimerManager] Wake On Lan sent to %s", serverMAC );
					}
					free( serverMAC );
				}
				long t = getSecondsToBegin();
				t -= ZAP_BEFORE_TIME;
				eDebug("[eTimerManager] zap in %d seconds", t );
				writeToLogfile(eString().sprintf("zap in %d seconds",t));
				nextAction=zap;
				actionTimer.start( t*1000, true );
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d prepareEvent", calldepth--));
			break;
#endif

		case startCountdown:
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d startCountdown", ++calldepth));
			if ( nextStartingEvent->type & ePlaylistEntry::isRepeating )
			{
				writeToLogfile("reset state flags for repeating event");
				nextStartingEvent->type &= ~(
					ePlaylistEntry::stateError |
					ePlaylistEntry::stateFinished |
					ePlaylistEntry::statePaused |
					ePlaylistEntry::errorNoSpaceLeft |
					ePlaylistEntry::errorUserAborted |
					ePlaylistEntry::errorZapFailed|
					ePlaylistEntry::errorOutdated );

				nextStartingEvent->type |= ePlaylistEntry::stateWaiting;
			}
			if ( conn.connected() )
				conn.disconnect();
			if ( nextStartingEvent->type & ePlaylistEntry::doFinishOnly &&
				!nextStartingEvent->service )
				writeToLogfile("this is Sleeptimer... don't toggle TimerMode");
			else
			{
				writeToLogfile("call eZapMain::getInstance()->toggleTimerMode()");
				eZapMain::getInstance()->toggleTimerMode();
				// now in eZapMain the RemoteControl should be handled for TimerMode...
				// any service change stops now the Running Event and set it to userAborted
//				conn = CONNECT( eDVB::getInstance()->leaveService, eTimerManager::leaveService );
			}
			if ( nextStartingEvent->type & ePlaylistEntry::isSmartTimer )
			{
				writeToLogfile("is smarttimer event");
				conn2 = CONNECT( eDVB::getInstance()->tEIT.tableReady, eTimerManager::EITready );
				EITready(0);  // check current eit now !
			}
			else
			{
				long t = getSecondsToBegin();
				nextAction=startEvent;
				if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
				{
					writeToLogfile("attend HDD_PREPARE_TIME");
					t -= HDD_PREPARE_TIME;  // for HDD Wakeup or whatever
				}
				actionTimer.start( t*1000 , true );
				writeToLogfile(eString().sprintf("startEvent in %d seconds",t));
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d startCountdown", calldepth--));
			break;

		case startEvent:
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d startEvent", ++calldepth));
			eDebug("[eTimerManager] startEvent");
			if ( nextStartingEvent->type & ePlaylistEntry::isRepeating )
			{
				writeToLogfile("set last_activation for repeated event");
				nextStartingEvent->last_activation = getDate();
			}
			nextStartingEvent->type &= ~ePlaylistEntry::stateWaiting;

			nextStartingEvent->type |= ePlaylistEntry::stateRunning;

			if (nextStartingEvent->type & ePlaylistEntry::doFinishOnly )
			{
				writeToLogfile("only Finish an running event");
				eDebug("[eTimerManager] only Finish an running event");
			}
			else if (nextStartingEvent->type & ePlaylistEntry::RecTimerEntry)
			{
				nextAction = startRecording;
				actionHandler();
			}
			else if (nextStartingEvent->type & ePlaylistEntry::SwitchTimerEntry)
			{
				writeToLogfile("SwitchTimerEvent... do nothing");
			}

			if ( !(nextStartingEvent->type & ePlaylistEntry::isSmartTimer) )
			{
				nextAction = stopEvent;
				actionTimer.start( getSecondsToEnd() * 1000, true );
				writeToLogfile(eString().sprintf("stopEvent in %d seconds", getSecondsToEnd()) );
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d startEvent", calldepth--));
			break;

		case pauseEvent:
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d pauseEvent", ++calldepth));
			eDebug("[eTimerManager] pauseEvent");
			if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
			{
				nextStartingEvent->type &= ~ePlaylistEntry::stateRunning;
				nextStartingEvent->type |= ePlaylistEntry::statePaused;
				nextAction = pauseRecording;
				actionHandler();
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d pauseEvent", calldepth--));
			break;

		case restartEvent:
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d restartEvent", ++calldepth));
			eDebug("[eTimerManager] restartEvent");
			if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
			{
				nextStartingEvent->type &= ~ePlaylistEntry::statePaused; // we delete current state...
				nextStartingEvent->type |= ePlaylistEntry::stateRunning;
				nextAction=restartRecording;
				actionHandler();
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d restartEvent", calldepth--));
			break;

		case stopEvent:
			if( nextStartingEvent->type & ePlaylistEntry::stateRunning )
			{
				writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d stopEvent", ++calldepth));
				eDebug("[eTimerManager] stopEvent");

				nextStartingEvent->type &= ~ePlaylistEntry::stateRunning;
	   // when no ErrorCode is set the we set the state to finished
				if ( !(nextStartingEvent->type & ePlaylistEntry::stateError) )
				{
					writeToLogfile("set to stateFinished");
					nextStartingEvent->type |= ePlaylistEntry::stateFinished;
				}
				if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
				{
					nextAction=stopRecording;
					actionHandler();
				}
				else // SwitchTimer
				{
					writeToLogfile("SwitchTimer... do nothing");
				}

				if ( nextStartingEvent->type & ePlaylistEntry::doFinishOnly
					&& !nextStartingEvent->service
					&& nextStartingEvent->type & ePlaylistEntry::stateError )
					writeToLogfile("abort running sleeptimer don't handleStandby");
				else
				{
					int i=0;
					if ( nextStartingEvent->type&ePlaylistEntry::doShutdown )
						i = 2;
					else if ( nextStartingEvent->type&ePlaylistEntry::doGoSleep )
						i = 3;
					writeToLogfile(eString().sprintf("call eZapMain::handleStandby(%d)",i));
					eZapMain::getInstance()->handleStandby(i);
				}

				if ( nextStartingEvent->type & ePlaylistEntry::doFinishOnly
					&& !nextStartingEvent->service )
					writeToLogfile("this is a Sleeptimer... dont toggle TimerMode");
				else
				{
					writeToLogfile("call eZapMain::getInstance()->toggleTimerMode()");
					eZapMain::getInstance()->toggleTimerMode();
				}
				if ( eConfig::getInstance()->pLockActive() && eServiceInterface::getInstance()->service.isLocked() )
				{
					writeToLogfile("this service is parentallocked... stop service now");
					eServiceInterface::getInstance()->stop();
				}
				writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d stopEvent", calldepth--));
			}
			nextAction=setNextEvent;	// we set the Next Event... a new beginning *g*
			actionTimer.start(0, true);
			break;

		case setNextEvent:
		{
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d setNextEvent", ++calldepth));
			eDebug("[eTimerManager] setNextEvent");
			if (conn.connected() )
				conn.disconnect();
			if (conn2.connected() )
				conn2.disconnect();
			nextStartingEvent=timerlist->getList().end();
			int timeToNextEvent=INT_MAX, count=0;
			// parse events... invalidate old, set nextEvent Timer
			for ( std::list< ePlaylistEntry >::iterator i(timerlist->getList().begin()); i != timerlist->getList().end(); )
			{
				writeToLogfile(eString().sprintf("(%d), descr = %s",count, i->service.descr.c_str()));
				time_t nowTime=time(0)+eDVB::getInstance()->time_difference;
				if ( i->type & ePlaylistEntry::isRepeating )
				{
					writeToLogfile(eString().sprintf(" - time_begin = %d, getDate()=%d, lastActivation=%d", i->time_begin,
						getDate(),
						i->last_activation ));
					time_t tmp = getNextEventStartTime( i->time_begin, i->duration, i->type, getDate() == i->last_activation );
					tm evtTime = *localtime( &tmp );
					writeToLogfile(eString().sprintf(" - isRepeating event (days %s)", buildDayString(i->type).c_str() ));
					writeToLogfile(eString().sprintf(" - starts at %d (%02d.%02d, %02d:%02d)", tmp, evtTime.tm_mday, evtTime.tm_mon+1, evtTime.tm_hour, evtTime.tm_min));
					if ( tmp-nowTime < timeToNextEvent && nowTime < tmp+i->duration )
					{
						nextStartingEvent=i;
						timeToNextEvent = tmp-nowTime;
						writeToLogfile(eString().sprintf(" - is our new nextEvent... timeToNextEvent is %d", timeToNextEvent));
					}
					count++;
				}
				else if ( i->type & ePlaylistEntry::stateWaiting )
				{
					tm tmp = *localtime(&i->time_begin);
					writeToLogfile(" - is Single Event");
					writeToLogfile(eString().sprintf(" - starts at %d (%02d.%02d, %02d:%02d)", i->time_begin, tmp.tm_mday, tmp.tm_mon+1, tmp.tm_hour, tmp.tm_min));
					writeToLogfile(" - have stateWaiting");
					if ( i->type & ePlaylistEntry::stateError )
					{
						writeToLogfile(" - have stateError... unset stateWaiting");
						i->type &= ~ePlaylistEntry::stateWaiting;
					}
					else if ( i->time_begin+i->duration < nowTime ) // old event found
					{
						writeToLogfile(" - is an outdated event... set to stateError");
						i->type &= ~ePlaylistEntry::stateWaiting;
						i->type |= (ePlaylistEntry::stateError|ePlaylistEntry::errorOutdated);
					}
					else if ( i->type & ePlaylistEntry::doFinishOnly )
					{
						writeToLogfile(" - is doFinishOnly.. use this... dont check other events");
						nextStartingEvent=i;
						timeToNextEvent = i->time_begin - nowTime;
						// dont check the other events...
						break;
					}
					else if( (i->time_begin - nowTime) < timeToNextEvent )
					{
						nextStartingEvent=i;
						timeToNextEvent = i->time_begin - nowTime;
						writeToLogfile(eString().sprintf(" - is our new nextEvent... timeToNextEvent is %d", timeToNextEvent));
						count++;
					}
					else
					{
						count++;
						writeToLogfile(" - is not the nextEvent... ignore");
					}
				}
				else if ( i->type & ePlaylistEntry::doFinishOnly )
				{
					writeToLogfile(" - is an old doFinishOnly Event... remove this");
					eDebug("old doFinishOnly");
					// all timers they only finish a running action/event
					// are remove from the list after they ended
					i = timerlist->getList().erase(i);  // alten ShutOffTimer aus liste entfernen...
					continue;
				}
				i++;
			}
			eDebug("[eTimerManager] updated ( %d waiting events in list )", count );
			timerlist->save();
			if ( nextStartingEvent != timerlist->getList().end() )
			{
				tm evtTime;
				writeToLogfile("our new nextStartingEvent:");
				if ( nextStartingEvent->type & ePlaylistEntry::isRepeating )
				{
					writeToLogfile(" - is a repeating event");

					if ( nextStartingEvent->last_activation != -1 )
						writeToLogfile(eString().sprintf(" - lastActivation was at %02d.%02d.%04d", 
								(nextStartingEvent->last_activation / 1000000)-100,
								(nextStartingEvent->last_activation % 100000)/1000,
								(nextStartingEvent->last_activation % 100) + 2000 ));

					time_t tmp = getNextEventStartTime( nextStartingEvent->time_begin, nextStartingEvent->duration, nextStartingEvent->type, getDate() == nextStartingEvent->last_activation );
					if ( tmp )
						evtTime=*localtime( &tmp );
				}
				else
					evtTime=*localtime( &nextStartingEvent->time_begin );
				eDebug("[eTimerManager] next event starts at %02d.%02d, %02d:%02d", evtTime.tm_mday, evtTime.tm_mon+1, evtTime.tm_hour, evtTime.tm_min );
				writeToLogfile(eString().sprintf(" - descr/service is %s",nextStartingEvent->service.descr.length()?nextStartingEvent->service.descr.c_str():"" ));
				writeToLogfile(eString().sprintf(" - starts at %02d.%02d, %02d:%02d", evtTime.tm_mday, evtTime.tm_mon+1, evtTime.tm_hour, evtTime.tm_min));
				writeToLogfile(eString().sprintf(" - event type is %d, %08X", nextStartingEvent->type, nextStartingEvent->type ));
				long t = getSecondsToBegin();
/*				int prepareTime = 0;
				if ( nextStartingEvent->type & ePlaylistEntry::isSmartTimer )
				{  // EIT related zapping ....
					if ( t > prepareTime )
					{
						nextAction=prepareEvent;
						actionTimer.start( (t - 360) * 1000, true ); // set the Timer to eventBegin - 6 min
					}
					else  // time to begin is under 6 min or the event is currently running
					{
						nextAction=zap;
						actionHandler();
					}
				}
				else*/
				{
					nextAction=zap;
					if ( nextStartingEvent->type & ePlaylistEntry::RecTimerEntry )
					{
						writeToLogfile(" - is a RecTimerEnty");
#ifndef DISABLE_NETWORK
						if ( nextStartingEvent->type & ePlaylistEntry::recNgrab )
						{
							writeToLogfile(" - is a recNgrab Event.. attend WOL_PREPARE_TIME");
							t -= WOL_PREPARE_TIME; // for send WOL Signal
							nextAction=prepareEvent;
							writeToLogfile("nextAction = prepareEvent");
						}
						else
#endif
						{
							t -= ZAP_BEFORE_TIME;
							writeToLogfile("nextAction = zap");
						}
					}
					else
						writeToLogfile("nextAction = zap");
					// set the Timer to eventBegin
					writeToLogfile(eString().sprintf("   starts in %d seconds", t ));
					actionTimer.startLongTimer(t);
				}
			}
			else
			{
				writeToLogfile("no more waiting events...");
				actionTimer.stop();
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d setNextEvent", calldepth--));
		}
		break;

		case startRecording:
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d startRecording", ++calldepth));
			eDebug("[eTimerManager] start recording");
#if ( defined(DISABLE_FILE) + defined(DISABLE_NETWORK) ) < 2
			if ( nextStartingEvent->type & (ePlaylistEntry::recDVR|ePlaylistEntry::recNgrab) )
			{
				writeToLogfile("recDVR or Ngrab");
//				eDebug("nextStartingEvent->service.data[0] = %d", nextStartingEvent->service.data[0] );
//				eDebug("nextStartingEvent->service.descr = %s", nextStartingEvent->service.descr.c_str() );
				eString recordDescr;
				if ( nextStartingEvent->type & ePlaylistEntry::isRepeating )
				{
					writeToLogfile("repeating");
					time_t tmp = getNextEventStartTime( nextStartingEvent->time_begin, nextStartingEvent->duration, nextStartingEvent->type, false );
					const eString &descr=getEventDescrFromEPGCache( nextStartingEvent->service, tmp+nextStartingEvent->duration/2 );
					if ( descr ) // build Episode Information
					{
						unsigned int npos = descr.find(" - ");
						if ( npos != eString::npos )
							recordDescr = descr.mid( npos );
					}
					if ( !recordDescr )
					{  // build date instead of epoisode information
						tm evtTime = *localtime(&tmp);
						recordDescr.sprintf(" - %02d.%02d.%02d,", evtTime.tm_mday, evtTime.tm_mon+1, evtTime.tm_year%100 );
					}
				}
				eString descr = getRight( nextStartingEvent->service.descr, '/');
				if ( recordDescr )
					descr += recordDescr;
#ifndef DISABLE_FILE
				if ( nextStartingEvent->type & ePlaylistEntry::recDVR )
				{
					writeToLogfile(eString().sprintf("call eZapMain::getInstance()->recordDVR(1,0,%s)",descr.length()?descr.c_str():""));
					eDebug("[eTimerManager] start DVR");
					eZapMain::getInstance()->recordDVR(1, 0, descr.length()?descr.c_str():0 );
				}
				else
#endif
#ifndef DISABLE_NETWORK
				if (nextStartingEvent->type & ePlaylistEntry::recNgrab)
				{
					eDebug("[eTimerManager] start Ngrab");
					writeToLogfile(eString().sprintf("call ENgrab::getNew()->sendStart(%s)",descr.length()?descr.c_str():""));
					ENgrab::getNew()->sendstart(descr.length()?descr.c_str():0);
				}
#else
				{
				}
#endif
			}
			else
#endif // defined(DISABLE_FILE) + defined(DISABLE_NETWORK) < 2
			{
#ifndef DISABLE_LIRC
				if (nextStartingEvent->type & ePlaylistEntry::recVCR)
				{
					writeToLogfile("call eLirc::getNew()->sendStart()");
					eDebug("[eTimerManager] start VCR-Lirc-Recording");
					ELirc::getNew()->sendstart();
				}
#endif
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d startRecording", calldepth--));
			break;

		case stopRecording:
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d stopRecording", ++calldepth));
			eDebug("[eTimerManager] stop recording");
#ifndef DISABLE_FILE
			if (nextStartingEvent->type & ePlaylistEntry::recDVR)
			{
				writeToLogfile("call eZapMain::getInstance()->recordDVR(0,0)");
				eDebug("[eTimerManager] stop DVR");
				eZapMain::getInstance()->recordDVR(0, 0);
			}
			else
#endif
#ifndef DISABLE_NETWORK
			if (nextStartingEvent->type & ePlaylistEntry::recNgrab)
			{
				writeToLogfile("call ENgrab::getNew()->sendstop()");
				eDebug("[eTimerManager] stop Ngrab");
				ENgrab::getNew()->sendstop();
			}
			else  // insert lirc ( VCR stop ) here
#endif
			if (nextStartingEvent->type & ePlaylistEntry::recVCR)
			{
#ifndef DISABLE_LIRC
				writeToLogfile("call ELirc::getNew()->sendstop()");
				eDebug("[eTimerManager] stop VCR-Lirc-Recording");
				ELirc::getNew()->sendstop();
#endif
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d stopRecording", calldepth--));
			break;

		case restartRecording:
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d restartRecording", ++calldepth));
			eDebug("[eTimerManager] restart recording");
#ifndef DISABLE_FILE
			if (nextStartingEvent->type & ePlaylistEntry::recDVR)
			{
				writeToLogfile("recDVR");
				eServiceHandler *handler=eServiceInterface::getInstance()->getService();
				if (!handler)
					eFatal("no service Handler");
				handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStart));
			}
			else // insert lirc ( VCR START )
#endif
			{
#ifndef DISABLE_LIRC
				writeToLogfile("recVCR... not Implemented !!");
#endif
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d restartRecording", calldepth--));
			break;

		case pauseRecording:
			writeToLogfile(eString().sprintf("--> actionHandler() calldepth=%d pauseRecording", ++calldepth));
#ifndef DISABLE_FILE
			if (nextStartingEvent->type & ePlaylistEntry::recDVR)
			{
				writeToLogfile("recDVR");
				eServiceHandler *handler=eServiceInterface::getInstance()->getService();
				if (!handler)
					eFatal("no service Handler");
				handler->serviceCommand(eServiceCommand(eServiceCommand::cmdRecordStop));
			}
			else // insert lirc ( VCR PAUSE )
#endif
			{
#ifndef DISABLE_LIRC
				writeToLogfile("recvcr not implemented...");
#endif
			}
			writeToLogfile(eString().sprintf("<-- actionHandler() calldepth=%d pauseRecording", calldepth--));
			break;
		default:
		{
			writeToLogfile(eString().sprintf("unhandled timer action %d !!!", nextAction));
			eDebug("unhandled timer action %d", nextAction);
		}
	}
}

void eTimerManager::switchedService( const eServiceReferenceDVB &ref, int err)
{
	writeToLogfile("--> switchedService()");
	if ( err == -ENOSTREAM || nextStartingEvent->service != (eServiceReference&)ref )
	{
		writeToLogfile("call abortEvent");
		abortEvent( ePlaylistEntry::errorZapFailed );
	}
	else  // zap okay
	{
		writeToLogfile("call startCountdown");
		nextAction=startCountdown;
		actionTimer.start(0,true);
	}
	writeToLogfile("<-- switchedService()");
}

void eTimerManager::abortEvent( int err )
{
	if ( nextStartingEvent->type & ePlaylistEntry::stateWaiting )
	{
		nextStartingEvent->type &= ~ePlaylistEntry::stateWaiting;
		nextStartingEvent->type |= ePlaylistEntry::stateRunning;
	}
	writeToLogfile(eString().sprintf("--> abortEvent(err %d)",err));
	eDebug("[eTimerManager] abortEvent");
	nextAction=stopEvent;
	nextStartingEvent->type |= (ePlaylistEntry::stateError|err);
	actionHandler();
	writeToLogfile("<-- abortEvent");
}

void eTimerManager::leaveService( const eServiceReferenceDVB& ref )
{
	writeToLogfile(eString().sprintf("--> leaveService %s", ref.toString().c_str() ));
	eDebug("[eTimerManager] leaveService");
	abortEvent( ePlaylistEntry::errorUserAborted );
	writeToLogfile("<-- leaveService");
}

void eTimerManager::EITready( int error )
{
	eDebug("[eTimerManager] EITready %s", strerror(-error));
	if (!error)
	{
		EIT *eit = eDVB::getInstance()->getEIT();
		if ( eit )
		{
			for (ePtrList<EITEvent>::const_iterator event(eit->events); event != eit->events.end(); ++event)		// always take the first one
			{
				if ( nextStartingEvent != timerlist->getList().end() && event->event_id == nextStartingEvent->event_id )
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
							// HERE WE MUST LATER HANDLE REPEATING EVENTS !!
							// stateWaiting is removed, when the event was running once
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
			eit->unlock();
		}
	}
}

long eTimerManager::getSecondsToBegin()
{
	time_t nowTime = time(0)+eDVB::getInstance()->time_difference;
	time_t tmp=0;
	if ( (tmp = getNextEventStartTime( nextStartingEvent->time_begin, nextStartingEvent->duration, nextStartingEvent->type, getDate() == nextStartingEvent->last_activation ) ) )
		return tmp - nowTime;
	return nextStartingEvent->time_begin - nowTime;
}

long eTimerManager::getSecondsToEnd()
{
	time_t nowTime = time(0)+eDVB::getInstance()->time_difference;
	time_t tmp=0;
	if ( (tmp = getNextEventStartTime( nextStartingEvent->time_begin, nextStartingEvent->duration, nextStartingEvent->type, false ) ) )
		return (tmp + nextStartingEvent->duration) - nowTime;
	return (nextStartingEvent->time_begin + nextStartingEvent->duration) - nowTime;
}

eTimerManager::~eTimerManager()
{
	// deep standby wakeup only on dbox2 at moment
	switch ( eSystemInfo::getInstance()->getHwType() )
	{
		case eSystemInfo::dbox2Nokia:
		case eSystemInfo::dbox2Philips:
		case eSystemInfo::dbox2Sagem:
			if ( nextStartingEvent != timerlist->getList().end() )
			{
				int min = getSecondsToBegin() / 60;
				min -= 6;   // we start the box 6 minutes before event begin

				if ( min < 1 )
					min = 1;

				int erg;
				int fd = open("/dev/dbox/fp0", O_RDWR);
				if((erg=ioctl(fd, FP_IOCTL_SET_WAKEUP_TIMER, &min))<0)
				{
					if(erg==-1) // Wakeup not supported
						eDebug("[eTimerManager] deepstandby wakeup not supported");
					else
						eDebug("[eTimerManager] error setting wakeup");
				}
				else
					eDebug("[eTimerManager] deepStandby wakeup in %d minutes programmed", min );
				close(fd);
			}
		default:
			eDebug("no support for wakeup from deepstandby yet... ");
	}
	writeToLogfile("~eTimerManager()");
	eDebug("[eTimerManager] down ( %d events in list )", timerlist->getList().size() );
	if (this == instance)
		instance = 0;
	timerlist->save();
	eServiceInterface::getInstance()->removeRef(timerlistref);
}

ePlaylistEntry* eTimerManager::findEvent( eServiceReference *service, EITEvent *evt )
{
// is the middle of the event overlapping with this event in timerlist?
// with this we prevent that the event before and after the "main" event
// are marked as "in timer" in the epg list
	ePlaylistEntry tmp(*service,
		evt->start_time + (evt->duration/2) -1,
		3, evt->event_id);

	for ( std::list<ePlaylistEntry>::iterator i( timerlist->getList().begin() ); i != timerlist->getList().end(); i++)
		if ( *service == i->service && Overlapping(*i, tmp ) )
			return &*i;

	return 0;
}

bool Overlap( time_t beginTime1, int duration1, time_t beginTime2, int duration2 )
{
	eRect movie1( ePoint(beginTime1, 0), eSize( duration1, 10) );
	eRect movie2( ePoint(beginTime2, 0), eSize( duration2, 10) );

	return movie1.intersects(movie2);
}

bool msOverlap( const ePlaylistEntry &m, const ePlaylistEntry &s )
{
	struct tm multiple = *localtime( &m.time_begin ),
				 Entry = *localtime( &s.time_begin );

	int todayDate = getDate();
	if ( m.last_activation == todayDate && m.type & ePlaylistEntry::stateError )
	{
		tm evtTime = *localtime(&s.time_begin);
		int evtDate = ((100+evtTime.tm_mday)*1000000)+((100+evtTime.tm_mon+1)*1000)+evtTime.tm_year;
		if ( evtDate == todayDate )
			return false;
	}

/*				eDebug("multiple %02d:%02d, duration = %d, entry %02d:%02d, duration = %d",
					multiple.tm_hour, multiple.tm_min, i->duration,
					Entry.tm_hour, Entry.tm_min, entry.duration );*/
	int mask = ePlaylistEntry::Su;
	for ( int x=0; x < Entry.tm_wday; x++ )
		mask*=2;
	if ( m.type & mask )
		return Overlap( Entry.tm_hour*3600+Entry.tm_min*60, s.duration,
								multiple.tm_hour*3600+multiple.tm_min*60, m.duration );
	return false;
}

bool mmOverlap( const ePlaylistEntry &m1, const ePlaylistEntry &m2 )
{
	struct tm multiple = *localtime( &m1.time_begin ),
		     Entry = *localtime( &m2.time_begin );

/*				eDebug("multiple %02d:%02d, duration = %d, entry %02d:%02d, duration = %d",
					multiple.tm_hour, multiple.tm_min, i->duration,
					Entry.tm_hour, Entry.tm_min, entry.duration );*/
	if ( Overlap( Entry.tm_hour*3600+Entry.tm_min*60, m2.duration,
								multiple.tm_hour*3600+multiple.tm_min*60, m1.duration ) )
	{
		int tmp = ePlaylistEntry::Su|ePlaylistEntry::Mo|ePlaylistEntry::Tue|
							ePlaylistEntry::Wed|ePlaylistEntry::Thu|ePlaylistEntry::Fr|
							ePlaylistEntry::Sa;
		return (m1.type&tmp) & (m2.type&tmp);
	}
	return false;
}

bool Overlapping( const ePlaylistEntry &e1, const ePlaylistEntry &e2 )
{
	bool overlap=false;
	if ( e1.type & ePlaylistEntry::isRepeating && e2.type & ePlaylistEntry::isRepeating )
		overlap = mmOverlap( e1, e2 );
	else if ( e1.type & ePlaylistEntry::isRepeating )
		overlap = msOverlap( e1, e2 );
	else if ( e2.type & ePlaylistEntry::isRepeating )
		overlap = msOverlap( e2, e1 );
	else overlap = ( !( e1.type & (ePlaylistEntry::stateError|ePlaylistEntry::stateFinished) )
		&& Overlap( e2.time_begin, e2.duration, e1.time_begin, e1.duration) );
	return overlap;
}

bool eTimerManager::removeEventFromTimerList( eWidget *sel, const ePlaylistEntry& entry, int type )
{
	for ( std::list<ePlaylistEntry>::iterator i( timerlist->getList().begin() ); i != timerlist->getList().end(); i++)
		if ( *i == entry )
		{
			sel->hide();
			eString str1, str2, str3;
			if (type == erase)
			{
				str1 = _("You would to delete the running event..\nthis stops the timer mode (recording)!");
				str2 = _("Delete the event from the timerlist");
				str3 = _("Really delete this event?");
			}
			else if (type == update)
			{
				str1 = _("You would to update the running event.. \nthis stops the timer mode (recording)!");
				str2 = _("Update event in timerlist");
				str3 = _("Really update this event?");
			}
			
/*			// show messageBox blasel.. running event...
			if ( &(*nextStartingEvent) == &entry && entry.type & ePlaylistEntry::stateRunning  )
			{
				eMessageBox box(str1, str2, eMessageBox::btOK|eMessageBox::iconWarning );
				box.show();
				box.exec();
				box.hide();
			}*/
			int ret = eMessageBox::btNo;
			if ( &(*nextStartingEvent) == &entry && entry.type & ePlaylistEntry::stateRunning  )
			{
				eMessageBox box(str1+'\n'+str3, str2, eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo);
				box.show();
				ret=box.exec();
				box.hide();
			}
			else
			{
				eMessageBox box(str3, str2, eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo);
				box.show();
				ret=box.exec();
				box.hide();
			}
			if (ret == eMessageBox::btYes)
			{
				timerlist->getList().erase(i);
				if ( &(*nextStartingEvent) == &entry )
				{
					if ( nextStartingEvent->type & ePlaylistEntry::stateRunning )
					{
						nextAction=stopEvent;
						nextStartingEvent->type |= (ePlaylistEntry::stateError | ePlaylistEntry::errorUserAborted);
						actionHandler();
					}
					else if ( !(nextStartingEvent->type & ePlaylistEntry::stateRunning) )
					{
						nextAction=setNextEvent;
						actionHandler();
					}
				}
				sel->show();
				return true;
			}
			sel->show();
			break;
		}
	return false;
}
/*
bool eTimerManager::updateRunningTimerEvent( eWidget *sel, const ePlaylistEntry& entry )
{
	for ( std::list<ePlaylistEntry>::iterator i( timerlist->getList().begin() ); i != timerlist->getList().end(); i++)
		if ( *i == entry )
		{
			sel->hide();
			eString str2 = _("Update event in timerlist");
			eString str3 = _("Really update this event?");

			int ret = eMessageBox::btNo;
			eMessageBox box(str3, str2, eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo);
			box.show();
			ret=box.exec();
			box.hide();

			if (ret == eMessageBox::btYes)
			{
				timerlist->getList().erase(i);
				if ( &(*nextStartingEvent) == &entry )
				{
					if ( nextStartingEvent->type & ePlaylistEntry::stateRunning )
					{
						nextAction=stopEvent;
						nextStartingEvent->type |= (ePlaylistEntry::stateError | ePlaylistEntry::errorUserAborted);
						actionHandler();
					}
					else if ( !(nextStartingEvent->type & ePlaylistEntry::stateRunning) )
					{
						nextAction=setNextEvent;
						actionHandler();
					}
				}
				sel->show();
				return true;
			}
			sel->show();
			break;
		}
	return false;
}*/

bool eTimerManager::removeEventFromTimerList( eWidget *sel, const eServiceReference *ref, const EITEvent *evt )
{
	ePlaylistEntry tmp(*ref, evt->start_time, evt->duration, evt->event_id);
	for ( std::list<ePlaylistEntry>::iterator i( timerlist->getList().begin() ); i != timerlist->getList().end(); i++)
		if ( *ref == i->service && Overlapping(*i, tmp ) )
			return removeEventFromTimerList( sel, *i );
	return false;
}

bool eTimerManager::eventAlreadyInList( eWidget *w, EITEvent &e, eServiceReference &ref )
{
	ePlaylistEntry tmp(ref, e.start_time, e.duration, e.event_id);
	for ( std::list<ePlaylistEntry>::iterator i( timerlist->getList().begin() ); i != timerlist->getList().end(); i++)
		if ( ref == i->service && Overlapping(*i, tmp ) )
		{
			eMessageBox box(
				_("This event is already in the timerlist."),
				_("Add event to timerlist"),
				eMessageBox::iconWarning|eMessageBox::btOK);
			w->hide();
			box.show();
			box.exec();
			box.hide();
			w->show();
			return true;
		}
	return false;
}

bool eTimerManager::addEventToTimerList( eWidget *sel, const ePlaylistEntry& entry, const ePlaylistEntry *exclude )
{
	for ( std::list<ePlaylistEntry>::iterator i( timerlist->getList().begin() ); i != timerlist->getList().end(); i++)
	{
		if ( exclude && *exclude == *i )
			continue;
		if ( Overlapping(*i, entry) )
		{
			if ( entry.type & ePlaylistEntry::doFinishOnly )
			{
				eMessageBox box(_("The Endtime overlaps with another event in the timerlist"), _("Set Stop Time"), eMessageBox::iconWarning|eMessageBox::btOK);
				sel->hide();
				box.show();
				box.exec();
				box.hide();
				sel->show();
			}
			else
			{
				eMessageBox box(_("This event cannot added to the timerlist.\n"
					"The event overlaps with another event in the timerlist\n"
					"Please check the timerlist manually."), _("Add event to timerlist"), eMessageBox::iconWarning|eMessageBox::btOK);
				sel->hide();
				box.show();
				box.exec();
				box.hide();
				sel->show();
			}
			return false;
		}
	}
	if (!exclude)
	{
		timerlist->getList().push_back( entry );
		if ( nextStartingEvent == timerlist->getList().end() ||
				!(nextStartingEvent->type & ePlaylistEntry::stateRunning) )
		{
			nextAction = setNextEvent;
			actionHandler();
		}
	}
	return true;
}

bool eTimerManager::addEventToTimerList( eWidget *sel, const eServiceReference *ref, const EITEvent *evt, int type, const ePlaylistEntry *exclude )
{
	eServiceReference *subref=0;
// add the event description

//	eString descr	= _("no description is available");

	eSubServiceSelector subservicesel(false);

	for (ePtrList<Descriptor>::const_iterator d(evt->descriptor); d != evt->descriptor.end(); ++d)
	{
		if (d->Tag()==DESCR_LINKAGE)
		{
			LinkageDescriptor *ld=(LinkageDescriptor*)*d;
			if (ld->linkage_type==0xB0)
				subservicesel.add(((eServiceReferenceDVB*)ref)->getDVBNamespace(), ld);
		}
	}
	if ( subservicesel.getSelected() )  // channel have subservices?
	{
		sel->hide();
		subservicesel.show();
		if ( !subservicesel.exec() )
			subref=subservicesel.getSelected();
		subservicesel.hide();
		sel->show();
	}
	if ( subref )
		subref->descr+=ref->descr;

	ePlaylistEntry e( subref?*subref:*ref, evt->start_time, evt->duration, evt->event_id, type );
//	eDebug("e.service.descr = %s", e.service.descr.c_str() );
//	eDebug("descr = %s", descr.c_str() );
//	eString tmp = getLeft(e.service.descr, '/');
//	eDebug("tmp = %s", tmp.c_str() );
//	e.service.descr = tmp + '/' + descr;
	return addEventToTimerList( sel, e, exclude );
}

eAutoInitP0<eTimerManager> init_eTimerManager(eAutoInitNumbers::osd-1, "Timer Manager");

gFont eListBoxEntryTimer::TimeFont;
gFont eListBoxEntryTimer::DescrFont;
gPixmap *eListBoxEntryTimer::ok=0;
gPixmap *eListBoxEntryTimer::failed=0;
int eListBoxEntryTimer::timeXSize=0;
int eListBoxEntryTimer::dateXSize=0;

struct eTimerViewActions
{
	eActionMap map;
	eAction addTimerEntry, removeTimerEntry;
	eTimerViewActions():
		map("timerView", _("timerView")),
		addTimerEntry(map, "addTimerEntry", _("add new event to Timerlist"), eAction::prioDialog ),
		removeTimerEntry(map, "removeTimerEntry", _("remove this entry from timer list"), eAction::prioDialog )
	{
	}
};
eAutoInitP0<eTimerViewActions> i_TimerViewActions(eAutoInitNumbers::actions, "timer view actions");

eListBoxEntryTimer::~eListBoxEntryTimer()
{
	if (paraTime)
		paraTime->destroy();

	if (paraDate)
		paraDate->destroy();

	if (paraDescr)
		paraDescr->destroy();

	if (paraService)
		paraService->destroy();
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
		tmp->renderString( "00.00," );
		dateXSize = tmp->getBoundBox().width();
		tmp->destroy();
	}
	return (calcFontHeight(DescrFont)+4)*2;
}

eListBoxEntryTimer::eListBoxEntryTimer( eListBox<eListBoxEntryTimer> *listbox, ePlaylistEntry* entry )
		:eListBoxEntry((eListBox<eListBoxEntry>*)listbox),
		paraDate(0), paraTime(0), paraDescr(0), paraService(0), entry(entry)
{
}

const eString &eListBoxEntryTimer::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited)
{
	drawEntryRect(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, hilited);

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

	if (!paraDescr)
	{
		hlp = getRight( entry->service.descr, '/' );
		paraDescr = new eTextPara( eRect( 0 ,0, rect.width(), rect.height()/2) );
		paraDescr->setFont( DescrFont );
		paraDescr->renderString( hlp );
		DescrYOffs = ((rect.height()/2 - paraDescr->getBoundBox().height()) / 2 ) - paraDescr->getBoundBox().top();
	}
	rc->renderPara(*paraDescr, ePoint( xpos, rect.top() + DescrYOffs + rect.height()/2 ) );

	if (!paraDate)
	{
		paraDate = new eTextPara( eRect( 0, 0, entry->type&ePlaylistEntry::isRepeating?200:dateXSize, rect.height()/2) );
		paraDate->setFont( TimeFont );
		eString tmp = buildDayString( entry->type );
		if ( !tmp )
			tmp.sprintf("%02d.%02d,", start_time.tm_mday, start_time.tm_mon+1);
		paraDate->renderString( tmp );
		TimeYOffs = ((rect.height()/2 - paraDate->getBoundBox().height()) / 2 ) - paraDate->getBoundBox().top();
		hlp=tmp+' '+hlp;
	}
	dateXSize = paraDate->getBoundBox().width();
	rc->renderPara(*paraDate, ePoint( xpos, rect.top() + TimeYOffs ) );
	xpos+=dateXSize+paraDate->getBoundBox().height();

	if (!paraTime)
	{
		paraTime = new eTextPara( eRect( 0, 0, timeXSize, rect.height()/2) );
		paraTime->setFont( TimeFont );
		eString tmp;
		tmp.sprintf("%02d:%02d - %02d:%02d", start_time.tm_hour, start_time.tm_min, stop_time.tm_hour, stop_time.tm_min);
		paraTime->renderString( tmp );
		hlp=hlp+' '+tmp;
	}
	rc->renderPara(*paraTime, ePoint( xpos, rect.top() + TimeYOffs ) );
	xpos+=timeXSize+paraTime->getBoundBox().height();

	if (!paraService)
	{
		eString sname = getLeft(entry->service.descr,'/');
		if (!sname)
		{
			eService *service = eServiceInterface::getInstance()->addRef( entry->service );
			if ( service )
			{
				sname = service->service_name;
				eServiceInterface::getInstance()->removeRef( entry->service );
			}
		}
		if (sname)
		{
			paraService = new eTextPara( eRect( 0, 0, rect.width()-xpos, rect.height()/2) );
			paraService->setFont( TimeFont );
			paraService->renderString( sname );
		}
	}
	if ( paraService )
		rc->renderPara(*paraService, ePoint( xpos, rect.top() + TimeYOffs ) );

	return hlp;
}

static int weekday (int d, int m, int y)
{
	static char table[13] = {0,0,3,2,5,0,3,5,1,4,6,2,4};
	if (m<3)
		--y;
	return (y+y/4-y/100+y/400+table[m]+d)%7;
}

void normalize( struct tm &t )
{
	while ( t.tm_sec > 59 )
	{
		t.tm_sec -= 60;
		t.tm_min++;
	}
	while ( t.tm_min > 59 )
	{
		t.tm_min -= 60;
		t.tm_hour++;
	}
	while ( t.tm_hour > 23 )
	{
		t.tm_hour-=24;
		t.tm_mday++;
	}
	int days = monthdays[t.tm_mon];
	if ( days==28 && __isleap(t.tm_year) )
		days++;
	if ( t.tm_mday > days )
	{
		t.tm_mday -= days;
		t.tm_mon++;
	}
	while ( t.tm_mon > 11 )
	{
		t.tm_year++;
		t.tm_mon-=11;
	}
	t.tm_wday=-1;
	t.tm_yday=-1;
}

eTimerListView::eTimerListView()
	:eWindow(0)
{
	events = new eListBox<eListBoxEntryTimer>(this);
	events->setName("events");
	events->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));
	CONNECT(events->selected, eTimerListView::entrySelected );

	add = new eButton( this );
	add->setName("add");
	CONNECT( add->selected, eTimerListView::addPressed );

	erase = new eButton( this );
	erase->setName("remove");
	CONNECT( erase->selected, eTimerListView::erasePressed );

	if (eSkin::getActive()->build(this, "eTimerListView"))
		eWarning("Timer view widget build failed!");

	setText(_("Timer list"));

	fillTimerList();

	addActionMap( &i_TimerViewActions->map );
	setHelpID(97);
}

void eTimerListView::erasePressed()
{
	if ( events->getCount() && eTimerManager::getInstance()->removeEventFromTimerList( this, *events->getCurrent()->entry ) )
		fillTimerList();
}

void eTimerListView::addPressed()
{
	hide();
	eTimerEditView e(0);
	e.show();
	if ( !e.exec() )
		fillTimerList();
	e.hide();
	show();
}

void eTimerListView::entrySelected(eListBoxEntryTimer *entry)
{
	// FinishOnly Timers can not edited
	if ( entry && entry->entry->service &&
		!(entry->entry->type&ePlaylistEntry::doFinishOnly) )
	{
		hide();
		eTimerEditView e( entry->entry );
#ifndef DISABLE_LCD
		e.setLCD( LCDTitle, LCDElement );
#endif
		e.show();
		if ( !e.exec() )
			fillTimerList();
		e.hide();
		show();
	}
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

void eTimerListView::fillTimerList()
{
	events->beginAtomic();
	events->clearList();
	eTimerManager::getInstance()->forEachEntry( addToView(events) );
	events->sort();
	events->endAtomic();
}

struct TimerEditActions
{
	eActionMap map;
	eAction incBegTime, decBegTime, incEndTime, decEndTime;
	TimerEditActions():
		map("timerEdit", _("Timer Edit View")),
		incBegTime(map, "incBegTime", _("increase the event begin time in 1 minute steps"), eAction::prioDialog ),
		decBegTime(map, "decBegTime", _("decrease the event begin time in 1 minute steps"), eAction::prioDialog ),
		incEndTime(map, "incEndTime", _("increase the event end time in 1 minute steps"), eAction::prioDialog ),
		decEndTime(map, "decEndTime", _("decrease the event end time in 1 minute steps"), eAction::prioDialog )
	{
	}
};

eAutoInitP0<TimerEditActions> i_TimerEditActions(eAutoInitNumbers::actions, "timer edit actions");

void eTimerEditView::createWidgets()
{
	const char *monthStr[12] = { _("January"), _("February"), _("March"),
										_("April"), _("May"), _("June"), _("July"),
										_("August"), _("September"), _("October"),
										_("November"), _("December") };
	event_name = new eTextInputField(this);
	event_name->setName("event_name");
	event_name->setMaxChars(100);

	multiple = new eCheckbox(this);
	multiple->setName("multiple");
	CONNECT(multiple->checked, eTimerEditView::multipleChanged);

	cMo = new eCheckbox(this);
	cMo->setName("Mo");

	cTue = new eCheckbox(this);
	cTue->setName("Tue");

	cWed = new eCheckbox(this);
	cWed->setName("Wed");

	cThu = new eCheckbox(this);
	cThu->setName("Thu");

	byear = new eComboBox(this);
	byear->setName("b_year");

	bmonth = new eComboBox(this);
	bmonth->setName("b_month");

	bday = new eComboBox(this);
	bday->setName("b_day");

	lBegin = new eLabel(this);
	lBegin->setName("lBegin");

	btime = new eNumber( this, 2, 0, 59, 2, 0, 0, lBegin);
	btime->setName("b_time");
	btime->setFlags( eNumber::flagDrawPoints|eNumber::flagFillWithZeros|eNumber::flagTime );
	CONNECT( btime->selected, eTimerEditView::focusNext );

	cFr = new eCheckbox(this);
	cFr->setName("Fr");

	cSa = new eCheckbox(this);
	cSa->setName("Sa");

	cSu = new eCheckbox(this);
	cSu->setName("Su");

	eyear = new eComboBox(this);
	eyear->setName("e_year");

	emonth = new eComboBox(this);
	emonth->setName("e_month");

	eday = new eComboBox(this);
	eday->setName("e_day");

	lEnd = new eLabel(this);
	lEnd->setName("lEnd");

	etime = new eNumber( this, 2, 0, 59, 2, 0, 0, lEnd );
	etime->setName("e_time");
	etime->setFlags( eNumber::flagDrawPoints|eNumber::flagFillWithZeros|eNumber::flagTime );
	CONNECT( etime->selected, eTimerEditView::focusNext );

	type = new eComboBox( this );
	type->setName("type");

	bSelectService = new eButton( this );
	bSelectService->setName("select_service");
	CONNECT( bSelectService->selected, eTimerEditView::showServiceSelector );

	bApply = new eButton( this );
	bApply->setName("apply");
	CONNECT( bApply->selected, eTimerEditView::applyPressed );

	bScanEPG = new eButton(this);
	bScanEPG->setName("scanEPG");
	CONNECT( bScanEPG->selected, eTimerEditView::scanEPGPressed);

	CONNECT(byear->selchanged_id, eTimerEditView::comboBoxClosed);
	CONNECT(bmonth->selchanged_id, eTimerEditView::comboBoxClosed);
	CONNECT(bday->selchanged_id, eTimerEditView::comboBoxClosed);
	CONNECT(eyear->selchanged_id, eTimerEditView::comboBoxClosed);
	CONNECT(emonth->selchanged_id, eTimerEditView::comboBoxClosed);
	CONNECT(eday->selchanged_id, eTimerEditView::comboBoxClosed);

	if (eSkin::getActive()->build(this, "eTimerEditView"))
		eWarning("Timer view widget build failed!");

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
#ifndef DISABLE_FILE
	if ( eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000 )
		new eListBoxEntryText( *type, _("record DVR"), (void*) (ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recDVR) );
#endif
#ifndef DISABLE_NETWORK
	if ( eSystemInfo::getInstance()->hasNetwork() )
		new eListBoxEntryText( *type, _("Ngrab"), (void*) (ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recNgrab) );
#endif
#ifndef DISABLE_LIRC
	switch ( eSystemInfo::getInstance()->getHwType() )
	{
		case eSystemInfo::dbox2Nokia:
		case eSystemInfo::dbox2Philips:
		case eSystemInfo::dbox2Sagem:
			new eListBoxEntryText( *type, _("Record VCR"), (void*) (ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recVCR) );
		default:
			break;
	}
#endif
	addActionMap( &i_TimerEditActions->map );
}

eTimerEditView::eTimerEditView( ePlaylistEntry* e)
	:curEntry(e)
{
	createWidgets();

	if ( e )
	{
		multipleChanged( e->type&ePlaylistEntry::isRepeating );
		fillInData( e->time_begin, e->duration, e->type, e->service );
	}
	else
	{
		multipleChanged( 0 );
		time_t now = time(0)+eDVB::getInstance()->time_difference;
		int type =
			eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM5600 ?
				ePlaylistEntry::SwitchTimerEntry :
			eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000 ?
#ifndef DISABLE_FILE
				ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recDVR :
#else
				ePlaylistEntry::SwitchTimerEntry :
#endif
#ifndef DISABLE_NETWORK
				ePlaylistEntry::RecTimerEntry|ePlaylistEntry::recNgrab;
#else
				ePlaylistEntry::SwitchTimerEntry;
#endif
		fillInData( now, 0, type, eServiceInterface::getInstance()->service );
	}
	setHelpID(98);
}

eTimerEditView::eTimerEditView( const EITEvent &e, int type, eServiceReference ref )
	:curEntry(0)
{
	createWidgets();
	fillInData( e.start_time, e.duration, type, ref );
	multipleChanged(0);
	scanEPGPressed();
}

void eTimerEditView::fillInData( time_t begin_time, int duration, int ttype, eServiceReference& ref )
{
	eString descr = getRight( ref.descr, '/' );
	if ( descr.length() )
		event_name->setText(descr);
	else
		event_name->setText(_("no description is available"));
	beginTime = *localtime( &begin_time );
	time_t tmp = begin_time + duration;
	endTime = *localtime( &tmp );
	updateDateTime( beginTime, endTime, 3 );
	type->setCurrent( (void*) ( ttype & (ePlaylistEntry::RecTimerEntry|ePlaylistEntry::SwitchTimerEntry|ePlaylistEntry::recDVR|ePlaylistEntry::recVCR|ePlaylistEntry::recNgrab ) ) );
	tmpService = ref;

	eString sname = getLeft( tmpService.descr, '/' );
	if ( sname )
		bSelectService->setText(sname);
	else
	{
		eService *service = eServiceInterface::getInstance()->addRef( tmpService );
		if (service)
		{
			bSelectService->setText( service->service_name );
			eServiceInterface::getInstance()->removeRef( tmpService );
		}
		else
			bSelectService->setText(_("unknown service"));
	}
	setMultipleCheckboxes( ttype );
}

void eTimerEditView::setMultipleCheckboxes( int type )
{
	if ( type & ePlaylistEntry::isRepeating )
	{
		multiple->setCheck(1);
		cMo->setCheck(type&ePlaylistEntry::Mo);
		cTue->setCheck(type&ePlaylistEntry::Tue);
		cWed->setCheck(type&ePlaylistEntry::Wed);
		cThu->setCheck(type&ePlaylistEntry::Thu);
		cFr->setCheck(type&ePlaylistEntry::Fr);
		cSa->setCheck(type&ePlaylistEntry::Sa);
		cSu->setCheck(type&ePlaylistEntry::Su);
	}
	else
	{
		multiple->setCheck(0);
		cMo->setCheck(0);
		cTue->setCheck(0);
		cWed->setCheck(0);
		cThu->setCheck(0);
		cFr->setCheck(0);
		cSa->setCheck(0);
		cSu->setCheck(0);
	}
}

extern bool checkPin( int pin, const char * text );

void eTimerEditView::applyPressed()
{
	if ( eConfig::getInstance()->pLockActive() && tmpService.isLocked() &&
			!checkPin( eConfig::getInstance()->getParentalPin(), _("parental")))
		return;

	EITEvent evt;
	time_t newEventBegin;
	int newEventDuration;
	int ttype = ( (int) type->getCurrent()->getKey() ) |
		ePlaylistEntry::stateWaiting;
	if ( multiple->isChecked() )
	{
		ttype |= ePlaylistEntry::isRepeating;
		if ( cMo->isChecked() )
			ttype |= ePlaylistEntry::Mo;
		if ( cTue->isChecked() )
			ttype |= ePlaylistEntry::Tue;
		if ( cWed->isChecked() )
			ttype |= ePlaylistEntry::Wed;
		if ( cThu->isChecked() )
			ttype |= ePlaylistEntry::Thu;
		if ( cFr->isChecked() )
			ttype |= ePlaylistEntry::Fr;
		if ( cSa->isChecked() )
			ttype |= ePlaylistEntry::Sa;
		if ( cSu->isChecked() )
			ttype |= ePlaylistEntry::Su;
	}
	else
	{
		ttype &= ~ePlaylistEntry::isRepeating;
		if ( cMo->isChecked() )
			ttype &= ~ePlaylistEntry::Mo;
		if ( cTue->isChecked() )
			ttype &= ~ePlaylistEntry::Tue;
		if ( cWed->isChecked() )
			ttype &= ~ePlaylistEntry::Wed;
		if ( cThu->isChecked() )
			ttype &= ~ePlaylistEntry::Thu;
		if ( cFr->isChecked() )
			ttype &= ~ePlaylistEntry::Fr;
		if ( cSa->isChecked() )
			ttype &= ~ePlaylistEntry::Sa;
		if ( cSu->isChecked() )
			ttype &= ~ePlaylistEntry::Su;
	}
	if ( getData( newEventBegin, newEventDuration) )  // all is okay... we add the event..
	{
		// parse EPGCache to get event informations
		EITEvent *tmp = eEPGCache::getInstance()->lookupEvent( (eServiceReferenceDVB&)tmpService, newEventBegin+newEventDuration / 2 );
		if (tmp)
		{
			evt = *tmp;
			evt.descriptor.setAutoDelete(true);
			tmp->descriptor.setAutoDelete(false); // Problem Ptrlist....
			delete tmp;
		}
		else  // ohh.. not found...
		{
			evt.running_status = -1;
			evt.free_CA_mode = -1;
			evt.event_id = -1;
		}
		int timeroffset = 0;
		if ((eConfig::getInstance()->getKey("/enigma/timeroffset", timeroffset)) != 0)
			timeroffset = 0;
			
		evt.start_time = newEventBegin - (timeroffset * 60);
		evt.duration = newEventDuration + (2* timeroffset * 60);

		eString sname = getLeft(tmpService.descr,'/');
		eString descr = event_name->getText();

		// On Multiple Events.. kill Episode Infos..
		unsigned int pos=0;
		if ( multiple->isChecked() && ( pos = descr.find(" - ") ) != eString::npos )
			descr.erase( pos, descr.length() - pos );

		if ( descr.length() )
			tmpService.descr='/'+descr;
		if ( sname.length() )
			tmpService.descr=sname+tmpService.descr;

		bool ret = !curEntry;
		if ( curEntry )  // remove old event from list...
		{
			// this is a fake call to addEventToTimerList..
			// this only checks if the new event can added ! (overlapp check only)
			if ( eTimerManager::getInstance()->addEventToTimerList( this, &tmpService, &evt, ttype, curEntry ) )
			{
				// now we can delete the old event without any problem :)
				ret = eTimerManager::getInstance()->removeEventFromTimerList( this, *curEntry, eTimerManager::update );
			}
		}
		// this now adds the event
		if ( ret && eTimerManager::getInstance()->addEventToTimerList( this, &tmpService, &evt, ttype ) )
			close(0);
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

bool eTimerEditView::getData( time_t &bTime, int &duration )
{
	if ( multiple->isChecked() )
	{
		beginTime.tm_year = 70;  // 1.1.1970
		beginTime.tm_mon = 0;
		beginTime.tm_mday = 1;

		endTime.tm_year = 70;  // 1.1.1970
		endTime.tm_mon = 0;
		endTime.tm_mday = 1;
	}
	else
	{
		beginTime.tm_year = (int)byear->getCurrent()->getKey();
		beginTime.tm_mon = (int)bmonth->getCurrent()->getKey();
		beginTime.tm_mday = (int)bday->getCurrent()->getKey();

		endTime.tm_year = (int)eyear->getCurrent()->getKey();
		endTime.tm_mon = (int)emonth->getCurrent()->getKey();
		endTime.tm_mday = (int)eday->getCurrent()->getKey();
	}

	beginTime.tm_hour = btime->getNumber(0);
	beginTime.tm_min = btime->getNumber(1);
	beginTime.tm_sec = 0;

	endTime.tm_hour = etime->getNumber(0);
	endTime.tm_min = etime->getNumber(1);
	endTime.tm_sec = 0;

	if ( multiple->isChecked() &&   // endTime after 0:00
			endTime.tm_hour*60+endTime.tm_min <
			beginTime.tm_hour*60+beginTime.tm_min )
	{
		endTime.tm_mday++;
	}

	bTime = mktime( &beginTime );
	time_t tmp = mktime( &endTime );
	duration = tmp - bTime;
	return duration > 0;
}

void eTimerEditView::updateDateTime( const tm& beginTime, const tm& endTime, int what )
{
	if ( what & 1 )
	{
		updateDay( bday, beginTime.tm_year+1900, beginTime.tm_mon+1, beginTime.tm_mday );

		btime->setNumber( 0, beginTime.tm_hour );
		btime->setNumber( 1, beginTime.tm_min );

		byear->setCurrent( (void*) beginTime.tm_year );
		bmonth->setCurrent( (void*) beginTime.tm_mon );
	}
	if ( what & 2 )
	{
		updateDay( eday, endTime.tm_year+1900, endTime.tm_mon+1, endTime.tm_mday );

		etime->setNumber( 0, endTime.tm_hour );
		etime->setNumber( 1, endTime.tm_min );

		eyear->setCurrent( (void*) endTime.tm_year );
		emonth->setCurrent( (void*) endTime.tm_mon );
	}
}

void eTimerEditView::multipleChanged( int i )
{
	if ( i )
	{
		byear->hide();
		eyear->hide();
		bmonth->hide();
		emonth->hide();
		bday->hide();
		eday->hide();
		lBegin->show();
		lEnd->show();
		cMo->show();
		cTue->show();
		cWed->show();
		cThu->show();
		cFr->show();
		cSa->show();
		cSu->show();
	}
	else
	{
		cMo->hide();
		cMo->setCheck(0);
		cTue->hide();
		cTue->setCheck(0);
		cWed->hide();
		cWed->setCheck(0);
		cThu->hide();
		cThu->setCheck(0);
		cFr->hide();
		cFr->setCheck(0);
		cSa->hide();
		cSa->setCheck(0);
		cSu->hide();
		cSu->setCheck(0);
		lBegin->hide();
		lEnd->hide();
		byear->show();
		eyear->show();
		bmonth->show();
		emonth->show();
		bday->show();
		eday->show();
	}
}

void eTimerEditView::updateDay( eComboBox* dayCombo, int year, int month, int day )
{
	const char *dayStr[7] = { _("Sunday"), _("Monday"), _("Tuesday"), _("Wednesday"),
											 _("Thursday"), _("Friday"), _("Saturday") };
	dayCombo->clear();
	int wday = weekday( 1, month, year );
	int days = monthdays[ month-1 ];
	days += (days == 28 && __isleap( year ) );
	for ( int i = wday; i < wday+days; i++ )
		new eListBoxEntryText( *dayCombo, eString().sprintf("%s, %02d", dayStr[i%7], i-wday+1), (void*) (i-wday+1) );
	dayCombo->setCurrent( day>days ? 0 : (void*) day );
}

void eTimerEditView::comboBoxClosed( eComboBox *combo,  eListBoxEntryText* )
{
	if ( combo == bmonth || combo == byear )
		updateDay( bday, (int) byear->getCurrent()->getKey()+1900, (int) bmonth->getCurrent()->getKey()+1, (int) bday->getCurrent()->getKey() );
	else if ( combo == emonth || combo == eyear )
		updateDay( eday, (int) eyear->getCurrent()->getKey()+1900, (int) emonth->getCurrent()->getKey()+1, (int) eday->getCurrent()->getKey() );
}

void eTimerEditView::showServiceSelector()
{
	if ( !tmpService )  // Sleeptimer...
		return;
	eServiceSelector sel;
#ifndef DISABLE_LCD
	sel.setLCD(LCDTitle, LCDElement);
#endif
	hide();
	sel.getRoot.connect( slot( *eZapMain::getInstance(), &eZapMain::getRoot) );
	sel.setPath(eServiceReference(eServiceReference::idDVB,
				eServiceReference::flagDirectory|eServiceReference::shouldSort,
				-2, (1<<4)|(1<<1), 0xFFFFFFFF ),eServiceReference() );
	sel.setStyle(eServiceSelector::styleSingleColumn);

/*	if ( tmpService )
		sel.selectServiceRecursive( tmpService );*/

	const eServiceReference *ref = sel.choose(-1);

	if (ref)
	{
		if ( ref->data[0] == 4 ) // NVOD
		{
			eMessageBox box(_("Sorry, you can not add a time shifted service manually to the timer.\nPlease close the Timer and use the EPG of the service you wish to add!"), _("Information"), eMessageBox::iconInfo|eMessageBox::btOK);
			hide();
			box.show();
			box.exec();
			box.hide();
			show();
		}
		else if (tmpService != *ref)
		{
			tmpService = *ref;
			if ( ref->descr.length() )
				bSelectService->setText(ref->descr);
			else
			{
				eService *service = eServiceInterface::getInstance()->addRef( tmpService );
				if ( service )
				{
					bSelectService->setText(service->service_name);
					eServiceInterface::getInstance()->removeRef( tmpService );
				}
				else
					bSelectService->setText(_("unknown"));
			}
		}
	}
	show();
	setFocus(bSelectService);
}

void eTimerEditView::scanEPGPressed()
{
	time_t newEventBegin;
	int newEventDuration;
	if ( getData( newEventBegin, newEventDuration ) )  // all is okay... we add the event..
	{
		if ( multiple->isChecked() )
			newEventBegin = getNextEventStartTime( newEventBegin, newEventDuration, ePlaylistEntry::isRepeating, false );
		const eString &descr = getEventDescrFromEPGCache( tmpService, newEventBegin+newEventDuration/2);
		if ( descr )
		{
			event_name->setText( descr );
			return;
		}
		if ( !event_name->getText() )
			event_name->setText(_("no description found"));
	}
}

int eTimerEditView::eventHandler( const eWidgetEvent &event )
{
	switch ( event.type )
	{
		case eWidgetEvent::evtAction:
			if (event.action == &i_TimerEditActions->incBegTime && !event_name->inEditMode() )
				changeTime(-1);
			else if (event.action == &i_TimerEditActions->decBegTime && !event_name->inEditMode() )
				changeTime(-2);
			else if (event.action == &i_TimerEditActions->incEndTime && !event_name->inEditMode() )
				changeTime(+2);
			else if ( event.action == &i_TimerEditActions->decEndTime && !event_name->inEditMode() )
				changeTime(+1);
			else
				return eWindow::eventHandler( event );
			break;
		default:
			return eWindow::eventHandler( event );
	};
	return 1;
}

void eTimerEditView::changeTime( int dir )
{
	time_t curBegin;
	int duration;
	getData( curBegin, duration );
	switch( dir )
	{
		case -2:  // dec begTime
			curBegin-=60;
		break;
		case -1:  // inc begTime
			curBegin+=60;
		break;
		case +1:  // dec duration
			duration-=60;
		break;
		case +2:  // inc duration
			duration+=60;
		break;
	}
	if ( dir > 0 )
	{
		curBegin+=duration;
		endTime = *localtime( &curBegin );
		updateDateTime( beginTime, endTime, 2 );
	}
	else
	{
		beginTime = *localtime( &curBegin );
		updateDateTime( beginTime, endTime, 1 );
	}
}
