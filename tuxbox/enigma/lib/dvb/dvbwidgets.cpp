#include <math.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvb.h>
#include <lib/gui/eskin.h>
#include <lib/gui/enumber.h>
#include <lib/gui/combobox.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/eprogress.h>

eTransponderWidget::eTransponderWidget(eWidget *parent, int edit, int type)
	:eWidget(parent), type(type), edit(edit)
{
#ifndef DISABLE_LCD
	LCDTitle=parent->LCDTitle;
	LCDElement=parent->LCDElement;
#endif

	eLabel *l = 0;

	if ( type & deliverySatellite )
	{          
		lsat = new eLabel(this);
		lsat->setName( "lSat" );

		sat=new eComboBox(this, 4, l, edit);
		sat->setName("sat");

		for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
			for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
				new eListBoxEntryText(*sat, s->getDescription().c_str(), (void*) *s);

		CONNECT(sat->selchanged, eTransponderWidget::updated1);
	}

	l = new eLabel(this);
	l->setName( "lFreq" );

	int init[5]={0,0,0,0,0};
	frequency=new eNumber(this, 5, 0, 9, 1, init, 0, l, edit);
	frequency->setName("frequency");

	if ( type & deliverySatellite )
	{
		inversion=new eCheckbox(this);
		inversion->setName("inversion");

		l = new eLabel(this);
		l->setName( "lPol" );

		polarity=new eListBox<eListBoxEntryText>(this, l, edit);
		polarity->setName("polarity");
		polarityEntry[0]=new eListBoxEntryText(polarity, _("vertical"), (void*)0);
		polarityEntry[1]=new eListBoxEntryText(polarity, _("horizontal"), (void*)1);

		l = new eLabel(this);
		l->setName( "lFec" );

		fec=new eListBox<eListBoxEntryText>(this, l, edit);
		fec->setName("fec");
		fecEntry[0]=new eListBoxEntryText(fec, "Auto", (void*)0);
		fecEntry[1]=new eListBoxEntryText(fec, "1/2", (void*)1);
		fecEntry[2]=new eListBoxEntryText(fec, "2/3", (void*)2);
		fecEntry[3]=new eListBoxEntryText(fec, "3/4", (void*)3);
		fecEntry[4]=new eListBoxEntryText(fec, "5/6", (void*)4);
		fecEntry[5]=new eListBoxEntryText(fec, "7/8", (void*)5);

		CONNECT(fec->selchanged, eTransponderWidget::updated1);
		CONNECT(polarity->selchanged, eTransponderWidget::updated1);
	}

	l = new eLabel(this);
	l->setName( "lSymb" );
	symbolrate=new eNumber(this, 5, 0, 9, 1, init, 0, l, edit);
	symbolrate->setName("symbolrate");

	if ( !(type & deliverySatellite) )
	{
		inversion=new eCheckbox(this);
		inversion->setName("inversion");
	}

	CONNECT_1_0(frequency->numberChanged, eTransponderWidget::updated2, 0);
	CONNECT_1_0(symbolrate->numberChanged, eTransponderWidget::updated2, 0);
	CONNECT(frequency->selected, eTransponderWidget::nextField0);
	CONNECT(symbolrate->selected, eTransponderWidget::nextField0);
	CONNECT(inversion->checked, eTransponderWidget::updated2);

}

void eTransponderWidget::nextField0(int *)
{
	focusNext(eWidget::focusDirNext);
}

void eTransponderWidget::updated1(eListBoxEntryText *)
{
 updated();
}

void eTransponderWidget::updated2(int)
{
	updated();
}

int eTransponderWidget::load()
{
	eString name="transpondersettings.";
	switch (type&3)
	{
	case deliveryCable:
		name+="cable";
		break;
	case deliverySatellite:
		name+="satellite";
		break;
	default:
		return -2;
	}
	if (eSkin::getActive()->build(this, name.c_str()))
		return -1;
	if ( type & flagNoSat )
	{
		lsat->hide();
		sat->hide();
	}
	if ( type & flagNoInv )
		inversion->hide();
	return 0;
}

struct selectSat
{
	const eTransponder* t;
	eComboBox *l;

	selectSat(const eTransponder *t, eComboBox* l ): t(t), l(l)
	{
	}

	bool operator()(eListBoxEntryText& e)
	{
//		eDebug("we have %d, we want %d",((eSatellite*)e.getKey())->getOrbitalPosition(), t->satellite.orbital_position );
		if ( ((eSatellite*)e.getKey())->getOrbitalPosition() == t->satellite.orbital_position )
		{
			l->setCurrent(&e);
			return 1;
		}
		return 0;
	}
};

int eTransponderWidget::setTransponder(const eTransponder *transponder)
{
	if (!transponder)
		return -1;
	switch (type&3)
	{
	case deliveryCable:
		if (!transponder->cable.valid)
			return -1;
		frequency->setNumber(transponder->cable.frequency/1000);

		symbolrate->setNumber(transponder->cable.symbol_rate/1000);
		
		inversion->setCheck(transponder->cable.inversion);
	break;
	case deliverySatellite:
	{
		if (!transponder->satellite.valid)
			return -1;
		frequency->setNumber(transponder->satellite.frequency/1000);

		if (transponder->satellite.fec >= 0 && transponder->satellite.fec < 6)
			fec->setCurrent(fecEntry[transponder->satellite.fec]);
		else
			fec->setCurrent(fecEntry[0]);

		polarity->setCurrent(polarityEntry[!transponder->satellite.polarisation]);
		symbolrate->setNumber(transponder->satellite.symbol_rate/1000);
		
		inversion->setCheck(transponder->satellite.inversion);

		if ( sat->forEachEntry(selectSat(transponder, sat)) != eListBoxBase::OK )
			sat->setCurrent(0);

		break;
	}
	default:
		break;
	}
	return 0;
}

int eTransponderWidget::getTransponder(eTransponder *transponder)
{
	switch (type&3)
	{
	case deliveryCable:
		eDebug("deliveryCable");
		transponder->setCable(frequency->getNumber()*1000, symbolrate->getNumber()*1000, inversion->isChecked(), 3 );
		return 0;
	case deliverySatellite:
		eDebug("deliverySatellite");
		eDebug("setting to: %d %d %d %d %d %d",
			frequency->getNumber(),
			symbolrate->getNumber(),
			(int)polarity->getCurrent()->getKey(),
			(int)fec->getCurrent()->getKey(),
			sat->getCurrent() && sat->getCurrent()->getKey()?
				((eSatellite*)sat->getCurrent()->getKey())->getOrbitalPosition() : 0,
			inversion->isChecked());
		transponder->setSatellite(
			frequency->getNumber()*1000, 
			symbolrate->getNumber()*1000,
			!((int)polarity->getCurrent()->getKey()), 
			((int)fec->getCurrent()->getKey()),
			sat->getCurrent() && sat->getCurrent()->getKey() ?
				((eSatellite*)sat->getCurrent()->getKey())->getOrbitalPosition()
				: 0,
			inversion->isChecked());
		return 0;
	default:
		return -1;
	}
}

int eFEStatusWidget::eventHandler(const eWidgetEvent &event)
{
//	eDebug("fe status widget: event %d", event.type);
	switch (event.type)
	{
	case eWidgetEvent::gotFocus:
	case eWidgetEvent::willShow:
		updatetimer.start(500);
		break;
	case eWidgetEvent::lostFocus:
	case eWidgetEvent::willHide:
		updatetimer.stop();
		break;
	default:
		return eWidget::eventHandler(event);
	}
	return 1;
}

eFEStatusWidget::eFEStatusWidget(eWidget *parent, eFrontend *fe): eWidget(parent), fe(fe), updatetimer(eApp)
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


	CONNECT(updatetimer.timeout, eFEStatusWidget::update);

	if (eSkin::getActive()->build(this, "eFEStatusWidget"))
		return;
}

void eFEStatusWidget::update()
{
	int snr=fe->SNR()*100/65536,
			agc=fe->SignalStrength()*100/65536,
			ber=fe->BER();
	p_agc->setPerc((agc));
	p_snr->setPerc((snr));
	p_ber->setPerc((int)log2(ber));
	lsnr_num->setText(eString().sprintf("%d%%",snr));
	lsync_num->setText(eString().sprintf("%d%%",agc));
	lber_num->setText(eString().sprintf("%d",ber));	
	int status=fe->Status();
	c_lock->setCheck(!!(status & FE_HAS_LOCK));
	c_sync->setCheck(!!(status & FE_HAS_SYNC));
}
