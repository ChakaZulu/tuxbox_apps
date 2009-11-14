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
#include <lib/system/info.h>
#include <math.h>

eSatfind::eSatfind(eFrontend *fe)
	:eWindow(0), updateTimer(eApp), fe(fe), current(0)
{
	init_eSatfind();
}
void eSatfind::init_eSatfind()
{
	p_snr=CreateSkinnedProgress("snr");

	p_agc=CreateSkinnedProgress("agc");

	p_ber=CreateSkinnedProgress("ber");

	c_sync=CreateSkinnedCheckbox("sync", 0,0, 0);

	c_lock=CreateSkinnedCheckbox("lock", 0,0, 0);

	lsnr_num=CreateSkinnedLabel("snr_num");

	lsync_num=CreateSkinnedLabel("agc_num");

	lber_num=CreateSkinnedLabel("ber_num");

	sat = CreateSkinnedComboBox("sat", 3);
	CONNECT(sat->selchanged, eSatfind::satChanged );

	transponder = CreateSkinnedComboBox("transponder");

	CONNECT(updateTimer.timeout, eSatfind::update);

	eLabel *l = CreateSkinnedLabel("lSat");

	BuildSkin("eSatfind");

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

	if (sapi && sapi->transponder)
		current = sapi->transponder;

	eListBoxEntryText *sel=0;

	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feTerrestrial )
	{
		setText(_("Signalfind"));
		l->setText(_("Region:"));
	}

	std::map<int,eSatellite*> sats;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			sats[s->getOrbitalPosition()]=s;

	for ( std::list<tpPacket>::const_iterator i(eTransponderList::getInstance()->getNetworks().begin()); i != eTransponderList::getInstance()->getNetworks().end(); ++i)
		if ( ( sats.find(i->orbital_position) != sats.end()) || (eSystemInfo::getInstance()->getFEType() != eSystemInfo::feSatellite) )
		{
			if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite && current && i->orbital_position == current->satellite.orbital_position )
				sel = new eListBoxEntryText(*sat, i->name, (void*)&*i);
			else
				new eListBoxEntryText(*sat, i->name, (void*)&*i);
		}

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

void eSatfind::satChanged(eListBoxEntryText *sat)
{
	transponder->clear();
	if (sat && sat->getKey())
	{
		eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
		eTransponder *tp=0;

		if (sapi && sapi->transponder)
			tp = sapi->transponder;

		eListBoxEntryText *sel=0;

		tpPacket *i = (tpPacket*) (sat->getKey());

		switch(eSystemInfo::getInstance()->getFEType())
		{
			case eSystemInfo::feSatellite:
				for (std::list<eTransponder>::const_iterator it( i->possibleTransponders.begin() ); it != i->possibleTransponders.end(); it++)
				{
					if ( tp && *tp == *it )
						sel = new eListBoxEntryText( *transponder, eString().sprintf("%d / %d / %c", it->satellite.frequency/1000, it->satellite.symbol_rate/1000, it->satellite.polarisation?'V':'H' ), (void*)&(*it) );
					else
						new eListBoxEntryText( *transponder, eString().sprintf("%d / %d / %c", it->satellite.frequency/1000, it->satellite.symbol_rate/1000, it->satellite.polarisation?'V':'H' ), (void*)&(*it) );
				}
				break;
			case eSystemInfo::feTerrestrial:
				for (std::list<eTransponder>::const_iterator it( i->possibleTransponders.begin() ); it != i->possibleTransponders.end(); it++)
				{
					if ( tp && *tp == *it )
						sel = new eListBoxEntryText( *transponder, eString().sprintf("%d kHz", it->terrestrial.centre_frequency/1000), (void*)&(*it) );
					else
						new eListBoxEntryText( *transponder, eString().sprintf("%d kHz", it->terrestrial.centre_frequency/1000), (void*)&(*it) );
				}
				break;
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
	update();
}

void eSatfind::update()
{                    
	int snr=fe->SNR(),
			agc=fe->SignalStrength(),
			ber=fe->BER();
	eDebug("[Satfind] SNR %d(%s) AGC %d(%s) BER %d(%s) %s %s",
		snr,
		lsnr_num->getText().c_str(),
		agc,
		lsync_num->getText().c_str(),
		ber,
		lber_num->getText().c_str(),
		c_lock->isChecked()?"LOCK":"NOLOCK",
		c_sync->isChecked()?"SYNC":"NOSYNC");
	snr=snr*100/65535;
	agc=agc*100/65535;
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
	updateTimer.start(250,true);
}
