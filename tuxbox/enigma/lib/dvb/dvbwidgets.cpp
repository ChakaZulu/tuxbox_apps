#include "dvbwidgets.h"
#include <core/gui/eskin.h>
#include <core/gui/enumber.h>
#include <core/gui/listbox.h>
#include <core/gui/echeckbox.h>
#include <core/gui/eprogress.h>
#include "frontend.h"
#include "dvb.h"

eTransponderWidget::eTransponderWidget(eWidget *parent, int edit, int type): eWidget(parent), edit(edit), type(type)
{
	int init[5]={1,2,3,4,5};
	frequency=new eNumber(this, 5, 0, 9, 1, init, 0, 0, edit);
	frequency->setName("frequency");

	polarity=new eListBox<eListBoxEntryText>(this);
	polarityEntry[0]=new eListBoxEntryText(polarity, _("vertical"), (void*)0);
	polarityEntry[1]=new eListBoxEntryText(polarity, _("horizontal"), (void*)1);
	polarity->setName("polarity");
	polarity->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	
	fec=new eListBox<eListBoxEntryText>(this);
	fecEntry[0]=new eListBoxEntryText(fec, "Auto", (void*)0);
	fecEntry[1]=new eListBoxEntryText(fec, "1/2", (void*)1);
	fecEntry[2]=new eListBoxEntryText(fec, "2/3", (void*)2);
	fecEntry[3]=new eListBoxEntryText(fec, "3/4", (void*)3);
	fecEntry[4]=new eListBoxEntryText(fec, "5/6", (void*)4);
	fecEntry[5]=new eListBoxEntryText(fec, "7/8", (void*)5);
	fec->setName("fec");
	fec->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	
	symbolrate=new eNumber(this, 5, 0, 9, 1, init, 0, 0, edit);
	symbolrate->setName("symbolrate");
	
	inversion=new eCheckbox(this);
	inversion->setName("inversion");
	
	lnb=new eNumber(this, 1, 0, 3, 1, init, 0, 0, edit);
	lnb->setName("lnb");
	
	CONNECT(frequency->selected, eTransponderWidget::nextField0);
	CONNECT(polarity->selected, eTransponderWidget::nextField1);
	CONNECT(fec->selected, eTransponderWidget::nextField1);
	CONNECT(symbolrate->selected, eTransponderWidget::nextField0);
	CONNECT(lnb->selected, eTransponderWidget::nextField0);
}

void eTransponderWidget::nextField0(int *)
{
	focusNext(eWidget::focusDirNext);
}

void eTransponderWidget::nextField1(eListBoxEntryText *)
{
	focusNext(eWidget::focusDirNext);
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

int eTransponderWidget::setTransponder(eTransponder *transponder)
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
		lnb->setNumber(transponder->satellite.lnb);
		
		break;
	}
	}
	return 0;
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
	}
	return eWidget::eventHandler(event);
}

eFEStatusWidget::eFEStatusWidget(eWidget *parent, eFrontend *fe): eWidget(parent), fe(fe), updatetimer(eApp)
{
	p_snr=new eProgress(this);
	p_snr->setName("snr");
	
	p_agc=new eProgress(this);
	p_agc->setName("agc");
	
	c_sync=new eCheckbox(this);
	c_sync->setName("sync");
	
	c_lock=new eCheckbox(this);
	c_lock->setName("lock");
	
	CONNECT(updatetimer.timeout, eFEStatusWidget::update);

	if (eSkin::getActive()->build(this, "eFEStatusWidget"))
		return;
}

void eFEStatusWidget::update()
{
	p_agc->setPerc(fe->SignalStrength()*100/65536);
	p_snr->setPerc((65536-fe->SNR())*100/65536);
	int status=fe->Status();
	c_lock->setCheck(!!(status & FE_HAS_LOCK));
	c_sync->setCheck(!!(status & FE_HAS_SYNC));
}
