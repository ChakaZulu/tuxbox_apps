#include "satfind.h"
#include "enigma_lcd.h"
#include <lib/base/ebase.h>
#include <lib/gui/eprogress.h>
#include <lib/gui/elabel.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/combobox.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/dvb.h>

eSatfind::eSatfind(eFrontend *fe)
	:eWindow(0), updateTimer(eApp), fe(fe), current(0), lockcount(0)
{
	p_snr=new eProgress(this);
	p_snr->setName("snr");

	p_agc=new eProgress(this);
	p_agc->setName("agc");

	p_ber=new eProgress(this);
	p_ber->setName("ber");

	c_sync=new eCheckbox(this, 0, 0);
	c_sync->setName("sync");

	c_lock=new eCheckbox(this, 0, 0);
	c_lock->setName("lock");

	lsnr_num=new eLabel(this);
	lsnr_num->setName("snr_num");

	lsync_num=new eLabel(this);
	lsync_num->setName("agc_num");

	lber_num=new eLabel(this);
	lber_num->setName("ber_num");

	sat = new eComboBox(this, 3);
	sat->setName("sat");
	CONNECT(sat->selchanged, eSatfind::satChanged );

	transponder = new eComboBox(this, 5);
	transponder->setName("transponder");

	CONNECT(updateTimer.timeout, eSatfind::update);

	if (eSkin::getActive()->build(this, "eSatfind"))
		return;

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

	if (sapi && sapi->transponder)
		current = sapi->transponder;

	eListBoxEntryText *sel=0;

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			if ( current && s->getOrbitalPosition() == current->satellite.orbital_position )
				sel = new eListBoxEntryText(*sat, s->getDescription().c_str(), (void*) *s);
			else
				new eListBoxEntryText(*sat, s->getDescription().c_str(), (void*) *s);

	if ( sat->getCount() )
	{
		if ( sel )
			sat->setCurrent(sel,true);
		else
			sat->setCurrent(0,true);
	}

	CONNECT( eFrontend::getInstance()->s_RotorRunning, eSatfind::RotorRunning );
	CONNECT( eFrontend::getInstance()->tunedIn, eSatfind::tunedIn );

	CONNECT(transponder->selchanged, eSatfind::tpChanged);

	setHelpID(44);
}

void eSatfind::RotorRunning(int)
{
	updateTimer.stop();
}

void eSatfind::satChanged( eListBoxEntryText *sat)
{
	transponder->clear();
	if (sat && sat->getKey())
	{
		eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
		eTransponder *tp=0;

		if (sapi && sapi->transponder)
			tp = sapi->transponder;

		eListBoxEntryText *sel=0;

		eSatellite *Sat = (eSatellite*) (sat->getKey());

		for ( std::list<tpPacket>::const_iterator i( eTransponderList::getInstance()->getNetworks().begin() ); i != eTransponderList::getInstance()->getNetworks().end(); i++ )
		{
			if ( i->orbital_position == Sat->getOrbitalPosition() )
			{
				for (std::list<eTransponder>::const_iterator it( i->possibleTransponders.begin() ); it != i->possibleTransponders.end(); it++)
					if ( tp && *tp == *it )
						sel = new eListBoxEntryText( *transponder, eString().sprintf("%d / %d / %c", it->satellite.frequency/1000, it->satellite.symbol_rate/1000, it->satellite.polarisation?'V':'H' ), (void*)&(*it) );
					else
						new eListBoxEntryText( *transponder, eString().sprintf("%d / %d / %c", it->satellite.frequency/1000, it->satellite.symbol_rate/1000, it->satellite.polarisation?'V':'H' ), (void*)&(*it) );
			}
		}

		if (transponder->getCount())
		{
			if ( sel )
				transponder->setCurrent(sel,true);
			else
				transponder->setCurrent(0,true);
		}
	}
}

void eSatfind::tpChanged( eListBoxEntryText *tp )
{
	lockcount=0;
	updateTimer.stop();
	if (tp && tp->getKey())
	{
		if ( current && *current == *((eTransponder*)tp->getKey()))
			return;
		current = (eTransponder*)(tp->getKey());
		current->tune();
	}
	else
		current = 0;
}

int eSatfind::eventHandler( const eWidgetEvent& e)
{
	switch (e.type)
	{
		case eWidgetEvent::execBegin:
#ifndef DISABLE_LCD
			eZapLCD::getInstance()->lcdMenu->hide();
			eZapLCD::getInstance()->lcdSatfind->show();
#endif
			updateTimer.start(250, true);
			break;
		case eWidgetEvent::execDone:
			updateTimer.stop();
#ifndef DISABLE_LCD
			eZapLCD::getInstance()->lcdSatfind->hide();
			eZapLCD::getInstance()->lcdMenu->show();
#endif
			break;
		default:
			return eWindow::eventHandler(e);
	}
	return 0;
}

void eSatfind::tunedIn(eTransponder *, int error )
{
	lockcount=0;
	update();
	if ( error )
		retune();
}

void eSatfind::update()
{                    
	int snr=fe->SNR()*100/65535,
			agc=fe->SignalStrength()*100/65535,
			ber=fe->BER();
	p_agc->setPerc(agc);
	p_snr->setPerc(snr);
	p_ber->setPerc((int)log2(ber));
	lsnr_num->setText(eString().sprintf("%d%%",snr));
	lsync_num->setText(eString().sprintf("%d%%",agc));
	lber_num->setText(eString().sprintf("%d",ber));
	status=fe->Status();
	c_lock->setCheck(!!(status & FE_HAS_LOCK));
	c_sync->setCheck(!!(status & FE_HAS_SYNC));
#ifndef DISABLE_LCD
	eZapLCD::getInstance()->lcdSatfind->update(snr,agc);
#endif
	if (status & FE_HAS_LOCK)
		updateTimer.start(250,true);
	else if( lockcount++ == 5 )
		retune();
}

void eSatfind::retune()
{
	if (!(status & FE_HAS_LOCK))
	{
		if (current)
			current->tune();
	}
}
