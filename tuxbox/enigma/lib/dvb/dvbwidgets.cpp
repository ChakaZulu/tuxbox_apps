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
	init_eTransponderWidget(parent, edit, type);
}

void eTransponderWidget::init_eTransponderWidget(eWidget *parent, int edit, int type)
{
#ifndef DISABLE_LCD
	LCDTitle=parent->LCDTitle;
	LCDElement=parent->LCDElement;
#endif

	if ( type & deliverySatellite )
	{
		lsat =CreateSkinnedLabel("lSat");

		sat=CreateSkinnedComboBox("sat", 4, 0, edit);

		std::map<int,eSatellite*> sats;
		for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
			for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
				sats[s->getOrbitalPosition()]=s;
		for (std::map<int,eSatellite*>::iterator it = sats.begin(); it != sats.end(); ++it)
				new eListBoxEntryText(*sat, it->second->getDescription().c_str(), (void*) it->second);

		CONNECT(sat->selchanged, eTransponderWidget::updated1);
	}


	int init[5]={0,0,0,0,0};

	frequency=CreateSkinnedNumberWithLabel("frequency",0,type & deliveryTerrestrial ? 6 : 5, 0, 9, 1, init, 0, "lFreq", edit);

	inversion=CreateSkinnedCheckbox("inversion");

	polarity=new eListBox<eListBoxEntryText>(this, CreateSkinnedLabel("lPol"), edit);
	polarity->setName("polarity");
	if ( type & deliverySatellite )
	{
		polarityEntry[0]=new eListBoxEntryText(polarity, _("horizontal"), (void*)0);
		polarityEntry[1]=new eListBoxEntryText(polarity, _("vertical"), (void*)1);
	}
	else if ( type & deliveryCable ) // modulation
	{
		polarityEntry[0]=new eListBoxEntryText(polarity, _("Auto"), (void*)0);
		polarityEntry[1]=new eListBoxEntryText(polarity, _("16-QAM"), (void*)1);
		polarityEntry[2]=new eListBoxEntryText(polarity, _("32-QAM"), (void*)2);
		polarityEntry[3]=new eListBoxEntryText(polarity, _("64-QAM"), (void*)3);
		polarityEntry[4]=new eListBoxEntryText(polarity, _("128-QAM"), (void*)4);
		polarityEntry[5]=new eListBoxEntryText(polarity, _("256-QAM"), (void*)5);
	}
	else if ( type & deliveryTerrestrial )  // constellation
	{
		polarityEntry[0]=new eListBoxEntryText(polarity, _("Auto"), (void*)-1);
		polarityEntry[1]=new eListBoxEntryText(polarity, _("QPSK"), (void*)0);
		polarityEntry[2]=new eListBoxEntryText(polarity, _("16-QAM"), (void*)1);
		polarityEntry[3]=new eListBoxEntryText(polarity, _("64-QAM"), (void*)2);
	}

	fec=new eListBox<eListBoxEntryText>(this, CreateSkinnedLabel("lFec"), edit);
	fec->setName("fec");
	if ( type & (deliverySatellite|deliveryCable) )
	{
		fecEntry[0]=new eListBoxEntryText(fec, "Auto", (void*)0);
		fecEntry[1]=new eListBoxEntryText(fec, "1/2", (void*)1);
		fecEntry[2]=new eListBoxEntryText(fec, "2/3", (void*)2);
		fecEntry[3]=new eListBoxEntryText(fec, "3/4", (void*)3);
		fecEntry[4]=new eListBoxEntryText(fec, "5/6", (void*)4);
		fecEntry[5]=new eListBoxEntryText(fec, "7/8", (void*)5);
		fecEntry[6]=new eListBoxEntryText(fec, "8/9", (void*)6);
	}
	else if ( type & deliveryTerrestrial )  // guard interval
	{
		fecEntry[0]=new eListBoxEntryText(fec, "Auto", (void*)-1);
		fecEntry[1]=new eListBoxEntryText(fec, "1/32", (void*)0);
		fecEntry[2]=new eListBoxEntryText(fec, "1/16", (void*)1);
		fecEntry[3]=new eListBoxEntryText(fec, "1/8", (void*)2);
		fecEntry[4]=new eListBoxEntryText(fec, "1/4", (void*)3);
	}
	CONNECT(fec->selchanged, eTransponderWidget::updated1);
	CONNECT(polarity->selchanged, eTransponderWidget::updated1);

	if ( type & deliveryTerrestrial )
	{
		bandwidth=new eListBox<eListBoxEntryText>(this, CreateSkinnedLabel("lBandwidth"), edit);
		bandwidth->setName("Bandwidth");
		bandwidthEntry[0]=new eListBoxEntryText(bandwidth, "8 MHz", (void*)0);
		bandwidthEntry[1]=new eListBoxEntryText(bandwidth, "7 MHz", (void*)1);
		bandwidthEntry[2]=new eListBoxEntryText(bandwidth, "6 MHz", (void*)2);
		CONNECT(bandwidth->selchanged, eTransponderWidget::updated1);
		tmMode=new eListBox<eListBoxEntryText>(this, CreateSkinnedLabel("ltmMode"), edit);
		tmMode->setName("tmMode");
		tmModeEntry[0]=new eListBoxEntryText(tmMode, "Auto", (void*)-1);
		tmModeEntry[1]=new eListBoxEntryText(tmMode, "2k", (void*)0);
		tmModeEntry[2]=new eListBoxEntryText(tmMode, "8k", (void*)1);
		CONNECT(tmMode->selchanged, eTransponderWidget::updated1);
		codeRateLP=new eListBox<eListBoxEntryText>(this, CreateSkinnedLabel("lCodeRateLP"), edit);
		codeRateLP->setName("codeRateLP");
		codeRateLPEntry[0]=new eListBoxEntryText(codeRateLP, "Auto", (void*)-1);
		codeRateLPEntry[1]=new eListBoxEntryText(codeRateLP, "1/2", (void*)0);
		codeRateLPEntry[2]=new eListBoxEntryText(codeRateLP, "2/3", (void*)1);
		codeRateLPEntry[3]=new eListBoxEntryText(codeRateLP, "3/4", (void*)2);
		codeRateLPEntry[4]=new eListBoxEntryText(codeRateLP, "5/6", (void*)3);
		codeRateLPEntry[5]=new eListBoxEntryText(codeRateLP, "7/8", (void*)4);
		CONNECT(codeRateLP->selchanged, eTransponderWidget::updated1);
		codeRateHP=new eListBox<eListBoxEntryText>(this, CreateSkinnedLabel("lCodeRateHP"), edit);
		codeRateHP->setName("codeRateHP");
		codeRateHPEntry[0]=new eListBoxEntryText(codeRateHP, "Auto", (void*)-1);
		codeRateHPEntry[1]=new eListBoxEntryText(codeRateHP, "1/2", (void*)0);
		codeRateHPEntry[2]=new eListBoxEntryText(codeRateHP, "2/3", (void*)1);
		codeRateHPEntry[3]=new eListBoxEntryText(codeRateHP, "3/4", (void*)2);
		codeRateHPEntry[4]=new eListBoxEntryText(codeRateHP, "5/6", (void*)3);
		codeRateHPEntry[5]=new eListBoxEntryText(codeRateHP, "7/8", (void*)4);
		CONNECT(codeRateHP->selchanged, eTransponderWidget::updated1);
	}
	else
	{
		symbolrate=CreateSkinnedNumberWithLabel("symbolrate",0, 5, 0, 9, 1, init, 0, "lSymb", edit);
		CONNECT_1_0(symbolrate->numberChanged, eTransponderWidget::updated2, 0);	
		CONNECT(symbolrate->selected, eTransponderWidget::nextField0);
	}

	CONNECT_1_0(frequency->numberChanged, eTransponderWidget::updated2, 0);
	CONNECT(frequency->selected, eTransponderWidget::nextField0);
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
	switch (type&7)
	{
	case deliveryCable:
		name+="cable";
		break;
	case deliverySatellite:
		name+="satellite";
		break;
	case deliveryTerrestrial:
		name+="terrestrial";
		break;
	default:
		return -2;
	}
	BuildSkin(name.c_str());
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
	
	switch (type&7)
	{
	case deliveryCable:
		if (!transponder->cable.valid)
			return -1;
		if (transponder->satellite.fec >= 0 && transponder->satellite.fec < 7)
			fec->setCurrent(fecEntry[transponder->satellite.fec]);
		else
			fec->setCurrent(fecEntry[0]);
		frequency->setNumber(transponder->cable.frequency/1000);
		symbolrate->setNumber(transponder->cable.symbol_rate/1000);
		inversion->setCheck(transponder->cable.inversion==1);
		if ( transponder->cable.modulation >=1 && transponder->cable.modulation < 6 )
			polarity->setCurrent(polarityEntry[transponder->cable.modulation]);
		else
			polarity->setCurrent(polarityEntry[0]);
		break;
	case deliverySatellite:
		if (!transponder->satellite.valid)
			return -1;
		frequency->setNumber(transponder->satellite.frequency/1000);

		if (transponder->satellite.fec >= 0 && transponder->satellite.fec < 7)
			fec->setCurrent(fecEntry[transponder->satellite.fec]);
		else
			fec->setCurrent(fecEntry[0]);

		polarity->setCurrent(polarityEntry[transponder->satellite.polarisation&1]);
		symbolrate->setNumber(transponder->satellite.symbol_rate/1000);
		inversion->setCheck(transponder->satellite.inversion==1);

		if ( sat->forEachEntry(selectSat(transponder, sat)) != eListBoxBase::OK )
			sat->setCurrent(0);
		break;
	case deliveryTerrestrial:
		if (!transponder->terrestrial.valid)
			return -1;
		frequency->setNumber(transponder->terrestrial.centre_frequency/1000);
		inversion->setCheck(transponder->terrestrial.inversion==1);

		if (transponder->terrestrial.constellation >= 0 && transponder->terrestrial.constellation < 3)
			polarity->setCurrent(polarityEntry[transponder->terrestrial.constellation+1]);
		else
			polarity->setCurrent(polarityEntry[0]);

		if (transponder->terrestrial.guard_interval >= 0 && transponder->terrestrial.guard_interval < 4)
			fec->setCurrent(fecEntry[transponder->terrestrial.guard_interval+1]);
		else
			fec->setCurrent(fecEntry[0]);

		if (transponder->terrestrial.bandwidth >= 0 && transponder->terrestrial.bandwidth < 3)
			bandwidth->setCurrent(bandwidthEntry[transponder->terrestrial.bandwidth]);
		else
			bandwidth->setCurrent(bandwidthEntry[0]);

		if ( transponder->terrestrial.transmission_mode >= 0 && transponder->terrestrial.transmission_mode < 3)
			tmMode->setCurrent(tmModeEntry[transponder->terrestrial.transmission_mode+1]);
		else
			tmMode->setCurrent(tmModeEntry[0]);

		if (transponder->terrestrial.code_rate_lp >= 0 && transponder->terrestrial.code_rate_lp < 5)
			codeRateLP->setCurrent(codeRateLPEntry[transponder->terrestrial.code_rate_lp+1]);
		else
			codeRateLP->setCurrent(codeRateLPEntry[0]);

		if (transponder->terrestrial.code_rate_hp >= 0 && transponder->terrestrial.code_rate_hp < 5)
			codeRateHP->setCurrent(codeRateHPEntry[transponder->terrestrial.code_rate_hp+1]);
		else
			codeRateHP->setCurrent(codeRateHPEntry[0]);
		break;
	default:
		break;
	}
	return 0;
}

int eTransponderWidget::getTransponder(eTransponder *transponder)
{
	switch (type&7)
	{
	case deliveryCable:
		eDebug("deliveryCable");
		transponder->setCable(frequency->getNumber()*1000, symbolrate->getNumber()*1000, inversion->isChecked(), (int)polarity->getCurrent()->getKey(), (int)fec->getCurrent()->getKey() );
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
			(int)polarity->getCurrent()->getKey(), 
			(int)fec->getCurrent()->getKey(),
			sat->getCurrent() && sat->getCurrent()->getKey() ?
				((eSatellite*)sat->getCurrent()->getKey())->getOrbitalPosition()
				: 0,
			inversion->isChecked());
		return 0;
	case deliveryTerrestrial:
		transponder->setTerrestrial(
			frequency->getNumber()*1000,
			(int)bandwidth->getCurrent()->getKey(),
			(int)polarity->getCurrent()->getKey(), //constellation
			transponder->terrestrial.hierarchy_information,
			(int)codeRateLP->getCurrent()->getKey(),
			(int)codeRateHP->getCurrent()->getKey(),
			(int)fec->getCurrent()->getKey(), // guard_interval
			(int)tmMode->getCurrent()->getKey(),
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
	init_eFEStatusWidget();
}
void eFEStatusWidget::init_eFEStatusWidget()
{
	p_snr=CreateSkinnedProgress("snr");
	p_agc=CreateSkinnedProgress("agc");
	p_ber=CreateSkinnedProgress("ber");
	c_sync=CreateSkinnedCheckbox("sync",0, 0, 0);
	c_lock=CreateSkinnedCheckbox("lock",0, 0, 0);
	lsnr_num=CreateSkinnedLabel("snr_num");
	lsync_num=CreateSkinnedLabel("agc_num");
	lber_num=CreateSkinnedLabel("ber_num");
	CONNECT(updatetimer.timeout, eFEStatusWidget::update);
	BuildSkin("eFEStatusWidget");
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
