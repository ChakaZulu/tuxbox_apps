#include "dvbwidgets.h"
#include <core/gui/eskin.h>
#include <core/gui/enumber.h>
#include <core/gui/listbox.h>
#include <core/gui/echeckbox.h>
#include <core/gui/eprogress.h>
#include "frontend.h"
#include "dvb.h"

eTransponderWidget::eTransponderWidget(eWidget *parent, int edit, int type)
	:eWidget(parent), type(type), edit(edit)
{
	LCDTitle=parent->LCDTitle;
	LCDElement=parent->LCDElement;
	eLabel *l = new eLabel(this);
	l->setName( "lSat" );
	sat=new eListBox<eListBoxEntryText>(this, l);
	sat->setName("sat");
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			new eListBoxEntryText(sat, s->getDescription().c_str(), (void*) *s);

	l = new eLabel(this);
	l->setName( "lFreq" );
	int init[5]={1,2,3,4,5};
	frequency=new eNumber(this, 5, 0, 9, 1, init, 0, l, edit);
	frequency->setName("frequency");
	
	inversion=new eCheckbox(this);
	inversion->setName("inversion");

	l = new eLabel(this);
	l->setName( "lPol" );
	polarity=new eListBox<eListBoxEntryText>(this, l);
	polarityEntry[0]=new eListBoxEntryText(polarity, _("vertical"), (void*)0);
	polarityEntry[1]=new eListBoxEntryText(polarity, _("horizontal"), (void*)1);
	polarity->setName("polarity");

	l = new eLabel(this);
	l->setName( "lFec" );
	fec=new eListBox<eListBoxEntryText>(this, l);
	fecEntry[0]=new eListBoxEntryText(fec, "Auto", (void*)0);
	fecEntry[1]=new eListBoxEntryText(fec, "1/2", (void*)1);
	fecEntry[2]=new eListBoxEntryText(fec, "2/3", (void*)2);
	fecEntry[3]=new eListBoxEntryText(fec, "3/4", (void*)3);
	fecEntry[4]=new eListBoxEntryText(fec, "5/6", (void*)4);
	fecEntry[5]=new eListBoxEntryText(fec, "7/8", (void*)5);
	fec->setName("fec");
	
	l = new eLabel(this);
	l->setName( "lSymb" );
	symbolrate=new eNumber(this, 5, 0, 9, 1, init, 0, l, edit);
	symbolrate->setName("symbolrate");

	CONNECT(frequency->selected, eTransponderWidget::nextField0);
	CONNECT(symbolrate->selected, eTransponderWidget::nextField0);
	CONNECT(fec->selchanged, eTransponderWidget::updated1);
	CONNECT(polarity->selchanged, eTransponderWidget::updated1);
	CONNECT_1_0(frequency->numberChanged, eTransponderWidget::updated2, 0);
	CONNECT_1_0(symbolrate->numberChanged, eTransponderWidget::updated2, 0);
	CONNECT(sat->selchanged, eTransponderWidget::updated1);
	CONNECT(inversion->checked, eTransponderWidget::updated2);
	CONNECT( focusChanged, eTransponderWidget::updateText);
}

void eTransponderWidget::nextField0(int *)
{
	focusNext(eWidget::focusDirNext);
}

void eTransponderWidget::updateText(const eWidget* w)  // for Statusbar....
{
	setHelpText( w->getHelpText() );
	if (parent)
		parent->focusChanged( this );
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
	switch (type)
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
	return 0;
}

struct selectSat: public std::unary_function<eListBoxEntryText&, void>
{
	const eTransponder* t;
	eListBox<eListBoxEntryText> *l;

	selectSat(const eTransponder *t, eListBox<eListBoxEntryText>* l ): t(t), l(l)
	{
	}

	bool operator()(eListBoxEntryText& e)
	{
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
	switch (type)
	{
	case deliveryCable:
		return -1;
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

		sat->forEachEntry(selectSat(transponder, sat));

		break;
	}
	default:
		break;
	}
	return 0;
}

int eTransponderWidget::getTransponder(eTransponder *transponder)
{
	switch (type)
	{
	case deliveryCable:
//	    void setCable(int frequency, int symbol_rate, int inversion)
		return -1;
	case deliverySatellite:
		eDebug("setting to: %d %d %d %d %d %d", frequency->getNumber(), symbolrate->getNumber(), (int)polarity->getCurrent()->getKey(), (int)fec->getCurrent()->getKey(), ((eSatellite*)sat->getCurrent()->getKey())->getOrbitalPosition(), inversion->isChecked());
		transponder->setSatellite(frequency->getNumber()*1000, 
			symbolrate->getNumber()*1000, !((int)polarity->getCurrent()->getKey()), 
			((int)fec->getCurrent()->getKey()), ((eSatellite*)sat->getCurrent()->getKey())->getOrbitalPosition(), inversion->isChecked());
		return 0;
	default:
		return -1;
	}
}

int eFEStatusWidget::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::willShow:
		updatetimer.start(500);
		update();
		break;
	case eWidgetEvent::willHide:
		updatetimer.stop();
		break;
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

eFEStatusWidget::eFEStatusWidget(eWidget *parent, eFrontend *fe): eWidget(parent), fe(fe), updatetimer(eApp)
{
	p_snr=new eProgress(this);
	p_snr->setName("snr");
	
	p_agc=new eProgress(this);
	p_agc->setName("agc");
	
	c_sync=new eCheckbox(this, 0, 0);
	c_sync->setName("sync");
	
	c_lock=new eCheckbox(this, 0, 0);
	c_lock->setName("lock");
	
	CONNECT(updatetimer.timeout, eFEStatusWidget::update);

	if (eSkin::getActive()->build(this, "eFEStatusWidget"))
		return;
}

void eFEStatusWidget::update()
{
	p_agc->setPerc(fe->SignalStrength()*100/65536);
	p_snr->setPerc((fe->SNR())*100/65536);
	int status=fe->Status();
	c_lock->setCheck(!!(status & FE_HAS_LOCK));
	c_sync->setCheck(!!(status & FE_HAS_SYNC));
}
