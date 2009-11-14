#include <src/time_correction.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvbservice.h>

eTimeCorrectionEditWindow::eTimeCorrectionEditWindow( tsref tp )
	:updateTimer(eApp), transponder(tp)
{
	init_eTimeCorrectionEditWindow(tp);
}

void eTimeCorrectionEditWindow::init_eTimeCorrectionEditWindow( tsref tp )
{

	lCurTime= CreateSkinnedLabel("curtime");
	lCurDate = CreateSkinnedLabel("curdate");

	lTpTime = CreateSkinnedLabel("tptime");
	lTpDate = CreateSkinnedLabel("tpdate");

	time_t now = time(0)+eDVB::getInstance()->time_difference;
	tm tmp = *localtime( &now );

	nTime = CreateSkinnedNumber("nTime",0, 2, 0, 59, 2, 0, 0);
	nTime->setFlags( eNumber::flagTime|eNumber::flagFillWithZeros );
	nTime->setNumber(0, tmp.tm_hour );
	nTime->setNumber(1, tmp.tm_min );
	CONNECT( nTime->selected, eTimeCorrectionEditWindow::fieldSelected );

	cday = CreateSkinnedComboBox("cday");
	cmonth = CreateSkinnedComboBox("cmonth");
	for ( int i = 0; i < 12; i++ )
		new eListBoxEntryText( *cmonth, eString().sprintf("%02d",i+1), (void*)i );
	CONNECT( cmonth->selchanged, eTimeCorrectionEditWindow::monthChanged );

	cyear = CreateSkinnedComboBox("cyear");
	for ( int i = -1; i < 4; i++ )
		new eListBoxEntryText( *cyear, eString().sprintf("%d",tmp.tm_year+(1900+i)), (void*)(tmp.tm_year+i) );

	cyear->setCurrent( (void*) tmp.tm_year );
	cmonth->setCurrent( (void*) tmp.tm_mon, true );
	cday->setCurrent( (void*) tmp.tm_mday );
	CONNECT( cyear->selchanged, eTimeCorrectionEditWindow::yearChanged );

	bSet=CreateSkinnedButton("set");
	CONNECT(bSet->selected, eTimeCorrectionEditWindow::savePressed);

	BuildSkin("timecorrection");
	CONNECT( updateTimer.timeout, eTimeCorrectionEditWindow::updateTimeDate );
}

void eTimeCorrectionEditWindow::savePressed()
{
	eDVB &dvb = *eDVB::getInstance();
	std::map<tsref,int> &tOffsMap=
		eTransponderList::getInstance()->TimeOffsetMap;
	time_t now = time(0)+dvb.time_difference;

	tm nowTime = *localtime( &now );
	nowTime.tm_isdst=-1;
	nowTime.tm_hour = nTime->getNumber(0);
	nowTime.tm_min = nTime->getNumber(1);
	nowTime.tm_mday = (int)cday->getCurrent()->getKey();
	nowTime.tm_wday = -1;
	nowTime.tm_yday = -1;
	nowTime.tm_mon = (int)cmonth->getCurrent()->getKey();
	nowTime.tm_year = (int)cyear->getCurrent()->getKey();
	time_t newTime = mktime(&nowTime);

	tOffsMap.clear();

	dvb.time_difference=1;
	eDebug("[TIME] set Linux Time");
	timeval tnow;
	gettimeofday(&tnow, 0);
	tnow.tv_sec=newTime;
	settimeofday(&tnow, 0);
	for (ePtrList<eMainloop>::iterator it(eMainloop::existing_loops)
		;it != eMainloop::existing_loops.end(); ++it)
		it->setTimerOffset(newTime-now);

	/*emit*/ dvb.timeUpdated();

// for calc new transponder correction
	eDVBServiceController *sapi = dvb.getServiceAPI();
	if ( sapi )
		sapi->startTDT();

	close(0);
}

void eTimeCorrectionEditWindow::updateTimeDate()
{
	time_t now = time(0)+eDVB::getInstance()->time_difference;
	tm ltime = *localtime( &now );
	lCurTime->setText(eString().sprintf("%02d:%02d:%02d", ltime.tm_hour, ltime.tm_min, ltime.tm_sec));
	lCurDate->setText(eString().sprintf("%02d.%02d.%04d", ltime.tm_mday, ltime.tm_mon+1, 1900+ltime.tm_year));
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if ( sapi && sapi->tdt && sapi->transponder )
	{
		time_t tpTime = now + sapi->lastTpTimeDifference;
		tm ltime = *localtime( &tpTime );
		lTpTime->setText(eString().sprintf("%02d:%02d:%02d", ltime.tm_hour, ltime.tm_min, ltime.tm_sec));
		lTpDate->setText(eString().sprintf("%02d.%02d.%04d", ltime.tm_mday, ltime.tm_mon+1, 1900+ltime.tm_year));
	}
	else
	{
		lTpTime->setText(_("no transponder time"));
		lTpDate->setText("");
	}
}

int eTimeCorrectionEditWindow::eventHandler( const eWidgetEvent &event )
{
	switch (event.type)
	{
		case eWidgetEvent::execBegin:
			updateTimer.start(1000);
			setFocus(bSet);
			break;
		case eWidgetEvent::execDone:
		{
			updateTimer.stop();
			break;
		}
		default:
			return eWindow::eventHandler( event );
	}
	return 1;
}

void eTimeCorrectionEditWindow::yearChanged( eListBoxEntryText* )
{
	cmonth->setCurrent( (int) cmonth->getCurrent()->getKey(), true );
	setFocus(cyear);
}

void eTimeCorrectionEditWindow::monthChanged( eListBoxEntryText *e )
{
	if ( e )
	{
		const unsigned char monthdays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
		int month = (int)e->getKey();
		cday->clear();
		int days = monthdays[month];
		if ( month == 1 && __isleap( (int)cyear->getCurrent()->getKey()) )
			days++;
		for ( int i = 1; i <= days ; i++ )
			new eListBoxEntryText( *cday, eString().sprintf("%02d", i), (void*)i);
		cday->setCurrent( (void*) 1 );
	}
}

