#include "dvbwidgets.h"
#include <core/gui/eskin.h>
#include <core/gui/enumber.h>
#include <core/gui/eListBox.h>
#include <core/gui/echeckbox.h>
#include "dvb.h"

eTransponderWidget::eTransponderWidget(eWidget *parent, int edit, int type): eWidget(parent, edit), edit(edit), type(type)
{
	int init[5]={1,2,3,4,5};
	frequency=new eNumber(this, 5, 0, 9, 1, init, 0, 0, edit);
	frequency->setName("frequency");
	
	fec=new eListBox<eListBoxEntryText>(this);
	fecEntry[0]=new eListBoxEntryText(fec, "Auto", (void*)0);
	fecEntry[1]=new eListBoxEntryText(fec, "1/2", (void*)1);
	fecEntry[2]=new eListBoxEntryText(fec, "2/3", (void*)2);
	fecEntry[3]=new eListBoxEntryText(fec, "3/4", (void*)3);
	fecEntry[4]=new eListBoxEntryText(fec, "5/6", (void*)4);
	fecEntry[5]=new eListBoxEntryText(fec, "7/8", (void*)5);
	fec->setName("fec");
	
	polarity=new eListBox<eListBoxEntryText>(this);
	polarityEntry[0]=new eListBoxEntryText(polarity, _("vertical"), (void*)0);
	polarityEntry[1]=new eListBoxEntryText(polarity, _("horizontal"), (void*)1);
	polarity->setName("polarity");
	
	symbolrate=new eNumber(this, 5, 0, 9, 1, init, 0, 0, edit);
	symbolrate->setName("symbolrate");
	
	inversion=new eCheckbox(this);
	inversion->setName("inversion");
	
	lnb=new eNumber(this, 1, 0, 3, 1, init, 0, 0, edit);
	lnb->setName("lnb");
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

		polarity->setCurrent(polarityEntry[0]);
		symbolrate->setNumber(transponder->satellite.symbol_rate/1000);
		
		inversion->setCheck(transponder->satellite.inversion);
		lnb->setNumber(transponder->satellite.lnb);
		
		break;
	}
	}
	return 0;
}
