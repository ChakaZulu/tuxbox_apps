#include <satconfig.h>

#include <lib/base/i18n.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/gui/echeckbox.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/dvb/edvb.h>
//#include <lib/driver/rc.h>

eSatelliteConfigurationManager::eSatelliteConfigurationManager()
	:refresh(0), deleteThisEntry( 0 )
{
	// send no more DiSEqC Commands on transponder::tune to Rotor
	eFrontend::getInstance()->disableRotor();

	lSatPos = new eLabel(this);
	lSatPos->setName("lSatPos");

	lLNB = new eLabel(this);
	lLNB->setName("lLnb");
	lLNB->hide();

	l22Khz = new eLabel(this);
	l22Khz->setName("l22khz");
	l22Khz->hide();

	lVoltage = new eLabel(this);
	lVoltage->setName("lVoltage");
	lVoltage->hide();

	button_new=new eButton(this);
	button_new->setName("new");
	button_new->hide();
	CONNECT(button_new->selected, eSatelliteConfigurationManager::newPressed);

	button_erase=new eButton(this);
	button_erase->setName("erase");
	button_erase->hide();
	CONNECT(button_erase->selected, eSatelliteConfigurationManager::erasePressed);

	button_close=new eButton(this);
	button_close->setName("close");
	CONNECT(button_close->selected, eSatelliteConfigurationManager::closePressed);
  	
	buttonWidget=new eWidget(this);
	buttonWidget->setName("buttons");	
	
	combo_type=new eComboBox(this, 4);
	combo_type->setName("type");
	CONNECT(combo_type->selchanged, eSatelliteConfigurationManager::typeChanged);
	new eListBoxEntryText( *combo_type, _("one single satellite"), (void*)0, 0, _("one directly connected LNB"));
	new eListBoxEntryText( *combo_type, _("2 satellites via DiSEqC A/B"), (void*)1, 0, _("2 LNBs via Diseqc"));
	new eListBoxEntryText( *combo_type, _("4 satellites via DiSEqC OPT A/B"), (void*)2, 0, _("3 LNBs via Diseqc"));
	new eListBoxEntryText( *combo_type, _("non-standard user defined cofiguration..."), (void*)3, 0, _("special"));
	
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eSatelliteConfigurationManager"))
		eFatal("skin load of \"eSatelliteConfigurationManager\" failed");

	eSize s = buttonWidget->getSize();
	s.setHeight( s.height()*8 );

	w_buttons = new eWidget(buttonWidget);
	w_buttons->resize( s );
	w_buttons->move( ePoint(0,0) );

	parseNetworks();  // load all networks from satellite.xml or cable.xml
	createControlElements();

	complexity=checkComplexity();
	eConfig::getInstance()->getKey("/elitedvb/DVB/config/lnbs/type", complexity);
	combo_type->setCurrent( (void*)complexity );

	repositionWidgets();

	CONNECT( eWidget::focusChanged, eSatelliteConfigurationManager::focusChanged );
}

void eSatelliteConfigurationManager::focusChanged( const eWidget* focus )
{
	if ( focus && focus->getName() == "satWidget" )
	{
// get Current Sat Widget Position
		const ePoint &wPosition = w_buttons->getPosition();

// get Visible Rect
		eRect rcVisible( wPosition, buttonWidget->getSize() );

		ePoint relFocusPos(focus->getPosition() );
		relFocusPos+=ePoint( 0, wPosition.y()*2 );
    
		if ( relFocusPos.y()+40 > rcVisible.bottom() )
			w_buttons->move( ePoint( wPosition.x(), wPosition.y()-40 ) );
		else if ( relFocusPos.y() < rcVisible.top() ) // we must scroll down
			w_buttons->move( ePoint( wPosition.x(), wPosition.y()+40 ) );
	}
}

void eSatelliteConfigurationManager::typeChanged(eListBoxEntryText* newtype)
{
	int newcomplexity=(int)newtype->getKey();
	// check if the new type is less complex than our current setup...
	if ( checkComplexity() > newcomplexity)
	{
		eMessageBox mb(_("Configuration contains some elements\nwhich don't fit into new DiSEqC-Type. Drop these items?"), _("Change DiSEqC-Type..."), eMessageBox::iconWarning|eMessageBox::btYes|eMessageBox::btCancel);
		hide();
		mb.show();
		int res=mb.exec();
		mb.hide();
		show();
		if (res == eMessageBox::btCancel)
		{
			combo_type->setCurrent((void*)complexity);
			return;
		}
	}
	setComplexity(complexity=newcomplexity);
}

void eSatelliteConfigurationManager::setComplexity(int complexity)
{
	int i=0;
	if (complexity < 3)
	{
		lLNB->hide();
		l22Khz->hide();
		lVoltage->hide();
		lSatPos->hide();
		lSatPos->move( ePoint( 75, lSatPos->getPosition().y() ) );
		lSatPos->show();
		button_erase->hide();
		button_new->hide();
	}
	else
	{
		lSatPos->hide();
		lSatPos->move( ePoint( 0, lSatPos->getPosition().y() ) );
		lSatPos->show();
		lLNB->show();
		l22Khz->show();
		lVoltage->show();
		button_new->show();
		button_erase->show();
	}

	switch (complexity)
	{
	case 0:
		deleteSatellitesAbove(1);
		while (eTransponderList::getInstance()->getLNBs().size() < 1)
			newPressed();
		for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); ++it, ++i)
			setSimpleDiseqc(it->getSatelliteList().first(), i);
		break;
	case 1:
		deleteSatellitesAbove(2);
		while (eTransponderList::getInstance()->getLNBs().size() < 2)
			newPressed();
		for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); ++it, ++i)
			setSimpleDiseqc(it->getSatelliteList().first(), i);
		break;
	case 2:
		deleteSatellitesAbove(4);
		while (eTransponderList::getInstance()->getLNBs().size() < 4)
			newPressed();
		for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); ++it, ++i)
			setSimpleDiseqc(it->getSatelliteList().first(), i);
		break;
	case 3:
		break;
	}
	checkComplexity();
	repositionWidgets();
}

int eSatelliteConfigurationManager::checkComplexity()
{
	int c=0, comp=0;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
	{
		if (it->getSatelliteList().size() != 1)
		{
			eDebug("complexity is 3 since lnb %d has more than one satellite.", c);
			return 3;
		} else
			++c;
		int dc=checkDiseqcComplexity(it->getSatelliteList().first());
		eDebug("LNB %d has %d", c, dc);
		if (dc > comp)
			comp=dc;
	}
	if (c > 4)
		comp=3;
		
	if ((comp < 2) && c>2)
		comp=2;
	if ((comp < 1) && c>1)
		comp=1;

	eDebug("complexity is %d", comp);
	return comp;
}

int eSatelliteConfigurationManager::checkDiseqcComplexity(eSatellite *s)
{
	SatelliteEntry *se=0;
	
	if (entryMap.count(s))
		se=&entryMap[s];
	
	if (s->getSwitchParams().VoltageMode != eSwitchParameter::HV)
	{
		eDebug("voltage mode unusual");
		return 3;
	}
	if (s->getSwitchParams().HiLoSignal != eSwitchParameter::HILO)
	{
		eDebug("sig22 mode unusual");
		return 3;
	}
	if (s->getLNB()->getDiSEqC().DiSEqCMode == eDiSEqC::NONE)
	{
		if (se)
			se->description->setText(_("direct connection"));
		return 0;
	}

	if (s->getLNB()->getDiSEqC().DiSEqCMode > eDiSEqC::V1_0)
	{
		eDebug("diseqc mode > 1.0");
		return 3;
	}
	if (s->getLNB()->getDiSEqC().DiSEqCParam > 3)
	{
		eDebug("unusual diseqc parameter");
		return 3;
	}
	
	if (se)
	{
		switch (s->getLNB()->getDiSEqC().DiSEqCParam)
		{
		case 0:
			se->description->setText("DiSEqC A");
			break;
		case 1:
			se->description->setText("DiSEqC B");
			break;
		case 2:
			se->description->setText("DiSEqC BA");
			break;
		case 3:
			se->description->setText("DiSEqC BB");
			break;
		}
	}
	
			// we have simple 1.0 .. 
	if (s->getLNB()->getDiSEqC().DiSEqCParam > 1)
		return 2;
	
	if (s->getLNB()->getDiSEqC().DiSEqCParam)
		return 1;
	
	return 0;
}

void eSatelliteConfigurationManager::deleteSatellitesAbove(int n)
{
			// it's all about invalidating ptrlist's iterators on delete :/
start:
		int index=0;
		for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); (it != eTransponderList::getInstance()->getLNBs().end()); ++it)
			for ( ePtrList<eSatellite>::iterator si = it->getSatelliteList().begin() ; si != it->getSatelliteList().end(); si++)
				if (index++ >= n)
				{
					delSatellite(si);
					goto start;
				}
}

void eSatelliteConfigurationManager::setSimpleDiseqc(eSatellite *s, int diseqcnr)
{
	eLNB *lnb=s->getLNB();

	lnb->setLOFHi(10600000);
	lnb->setLOFLo(9750000);
	lnb->setLOFThreshold(11700000);
	lnb->setIncreasedVoltage(0);
	lnb->getDiSEqC().MiniDiSEqCParam=eDiSEqC::NO;
	if (complexity)		// if we have diseqc at all
	{
		lnb->getDiSEqC().DiSEqCParam=eDiSEqC::AA+diseqcnr;
		lnb->getDiSEqC().DiSEqCMode=eDiSEqC::V1_0;
	} else
	{
		lnb->getDiSEqC().DiSEqCParam=0;
		lnb->getDiSEqC().DiSEqCMode=eDiSEqC::NONE;
	}
	lnb->getDiSEqC().DiSEqCRepeats=0;
	lnb->getDiSEqC().SeqRepeat=0;
	lnb->getDiSEqC().uncommitted_switch=0;
	lnb->getDiSEqC().uncommitted_gap=0;
	lnb->getDiSEqC().useGotoXX=1;
	lnb->getDiSEqC().gotoXXLatitude=0.0;
	lnb->getDiSEqC().gotoXXLaDirection=eDiSEqC::NORTH;
	lnb->getDiSEqC().gotoXXLongitude=0.0;
	lnb->getDiSEqC().gotoXXLoDirection=eDiSEqC::EAST;
}

eSatelliteConfigurationManager::~eSatelliteConfigurationManager()
{
	if (refresh)
		delete refresh;

// enable send DiSEqC Commands to Rotor on eTransponder::tune
	eFrontend::getInstance()->enableRotor();
}

eSatellite *eSatelliteConfigurationManager::getSat4SatCombo( const eComboBox *c )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
		if ( it->second.sat == c)
			break;
	return it != entryMap.end()?it->first:0;
}

eSatellite *eSatelliteConfigurationManager::getSat4HiLoCombo( const eComboBox *c )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
		if ( it->second.hilo == c)
			break;
	return it != entryMap.end()?it->first:0;
}

eSatellite *eSatelliteConfigurationManager::getSat4VoltageCombo( const eComboBox *c )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
		if ( it->second.voltage == c)
			break;
	return it != entryMap.end()?it->first:0;
}

eSatellite *eSatelliteConfigurationManager::getSat4LnbButton( const eButton *b )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
		if ( it->second.lnb == b)
			break;
	return it != entryMap.end()?it->first:0;
}

#define DESC_POS_X 350
#define SAT_POS_X  0
#define LNB_POS_X  260
#define HILO_POS_X  330
#define VOLTAGE_POS_X  430

#define POS_Y 0
void eSatelliteConfigurationManager::repositionWidgets()
{
	if (deleteThisEntry)
	{
		delete deleteThisEntry->sat;
		delete deleteThisEntry->lnb;
		delete deleteThisEntry->voltage;
		delete deleteThisEntry->hilo;
		delete deleteThisEntry->fixed;
		delete deleteThisEntry->description;
		// search Entry in Map;		
		std::map< eSatellite*, SatelliteEntry >::iterator it( entryMap.begin() );
		for ( ; it != entryMap.end() && &it->second != deleteThisEntry ; it++);
		if (it != entryMap.end() )
			entryMap.erase( it );		
		deleteThisEntry=0;
	}
	int count=0, y=POS_Y;

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
	{
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
		{
			SatelliteEntry& entry = entryMap[ *s ];
			// search eComboBox for this eSatellite and move

			entry.sat->hide();
			entry.lnb->hide();
			entry.voltage->hide();
			entry.hilo->hide();
			entry.fixed->hide();
			entry.description->hide();

			entry.sat->move( ePoint((complexity==3?SAT_POS_X:75), y) );
			entry.fixed->move( ePoint(0, y) );
			entry.description->move( ePoint(DESC_POS_X, y) );
			entry.lnb->move( ePoint(LNB_POS_X, y) );
			entry.lnb->setText( eString().sprintf("%i", count) );
			entry.hilo->move( ePoint(HILO_POS_X, y) );
			entry.voltage->move( ePoint(VOLTAGE_POS_X, y) );

			entry.sat->show();
			if (complexity == 3) // user defined..
			{
				entry.lnb->show();
				entry.voltage->show();
				entry.hilo->show();
			}
			else
			{
				entry.fixed->show();
				entry.description->show();
			}
			y+=40;
		}
		count++;
	}
}


void eSatelliteConfigurationManager::createControlElements()
{
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
			addSatellite( *s );
}

void eSatelliteConfigurationManager::lnbSelected(eButton* who)
{
	eLNBSetup sel( getSat4LnbButton(who), LCDTitle, LCDElement );

	hide();
	sel.show();

	if ( sel.exec() )  // the we must reposition widgets
		repositionWidgets();

	sel.hide();
	show();

	return;
}

void eSatelliteConfigurationManager::erasePressed()
{
	eSatellite *s=0;
	if ( focus && focus->getName() == "satWidget" )
	{
		s = getSat4SatCombo( (eComboBox*)focus );
		if (!s)
			s = getSat4HiLoCombo( (eComboBox*)focus );
		if (!s)
			s = getSat4VoltageCombo( (eComboBox*)focus );
		if (!s)
			s = getSat4LnbButton( (eButton*)focus );
	}
	if (!s)
		eDebug("Widget not found");
	else
	{
		eDebug("call delSatellite(s)");
		delSatellite(s);
	}
}

void eSatelliteConfigurationManager::delSatellite( eSatellite* s )
{
	eDVB::getInstance()->settings->removeOrbitalPosition(s->getOrbitalPosition());
	eLNB* lnb = s->getLNB();
	lnb->deleteSatellite( s );
	eDebug("Satellite is now removed");
	if ( !lnb->getSatelliteList().size() )   // the lnb that have no more satellites must be deleted
	{
		eDebug("delete no more used lnb");
		eTransponderList::getInstance()->getLNBs().remove( *s->getLNB() );
	}
	else
		eDebug("do not delete lnb");

	deleteThisEntry = &entryMap[ s ];
	if (!refresh)
	{
		refresh = new eTimer( eApp );
		CONNECT( refresh->timeout, eSatelliteConfigurationManager::repositionWidgets );
	}
	refresh->start(50, true );
}

void eSatelliteConfigurationManager::satChanged(eComboBox* who, eListBoxEntryText *le)
{
	eSatellite *s=getSat4SatCombo( who );

	if ( le->getKey() && le->getText() )
	{
			// delete old orbital position services
		if ((int)le->getKey() != s->getOrbitalPosition())
			eDVB::getInstance()->settings->removeOrbitalPosition(s->getOrbitalPosition());
		s->setOrbitalPosition( (int) le->getKey() );
		s->setDescription( le->getText() );
	}
	else if (s) // *delete* selected -->> satellite and empty lnbs were now deleted
		delSatellite(s);
}

void eSatelliteConfigurationManager::hiloChanged(eComboBox* who, eListBoxEntryText *le)
{
	getSat4HiLoCombo(who)->getSwitchParams().HiLoSignal = (eSwitchParameter::SIG22) (int) le->getKey();
}

void eSatelliteConfigurationManager::voltageChanged(eComboBox* who, eListBoxEntryText *le)
{
	getSat4VoltageCombo(who)->getSwitchParams().VoltageMode = (eSwitchParameter::VMODE) (int) le->getKey();
}

void eSatelliteConfigurationManager::closePressed()
{
	close(0);
}

void eSatelliteConfigurationManager::addSatellite( eSatellite *s )
{
	SatelliteEntry sat;

	eComboBox* c = new eComboBox(w_buttons, 6, lSatPos );
	c->setName("satWidget");
	sat.sat=c;

	c->loadDeco();
	c->resize(eSize(250, 30));
	c->setHelpText( _("press ok to select another satellite, or delete this satellite"));

	if (complexity == 3)
		new eListBoxEntryText( *c, _("*delete*"), (void*) 0 );   // this is to delete an satellite
	for (std::list<tpPacket>::const_iterator i(networks.begin()); i != networks.end(); ++i)
		if ( i->possibleTransponders.size() )
			new eListBoxEntryText( *c, i->name, (void*) i->possibleTransponders.begin()->satellite.orbital_position );

	int err;
	if ( (err = c->setCurrent( (void*) s->getOrbitalPosition() ) ) )
		if (err == eComboBox::E_COULDNT_FIND)  // hmm current entry not in Combobox... we add it manually
			c->setCurrent( new eListBoxEntryText( *c, s->getDescription(), (void*) s->getOrbitalPosition() ) );
	CONNECT( c->selchanged_id, eSatelliteConfigurationManager::satChanged );

	eButton* b = new eButton(w_buttons, lLNB);
	sat.lnb=b;
	b->setName("satWidget");
	b->loadDeco();

	b->resize(eSize(60, 30));
	b->setHelpText( _("press ok to goto LNB config"));
	CONNECT(b->selected_id, eSatelliteConfigurationManager::lnbSelected);

	int index=entryMap.size()+1;
	sat.fixed=new eLabel(w_buttons);
//	sat.fixed->setText(eString().sprintf(_("Sat %d"), index));
		// don't ask...
	sat.fixed->setShortcut(eString().sprintf("%d", index));
	sat.fixed->setShortcutFocus(sat.sat);
	sat.fixed->setShortcutPixmap(eString().sprintf("%d", index));
	sat.fixed->resize(eSize(130, 30));

	sat.description=new eLabel(w_buttons);
	sat.description->resize(eSize(230, 30));

	c = new eComboBox(w_buttons, 3, l22Khz);
	c->setName("satWidget");
	sat.hilo=c;
	c->loadDeco();

	c->resize( eSize( 90, 30 ) );
	c->setHelpText( _("press ok to select another 22kHz mode") );
	new eListBoxEntryText( *c, "Hi/Lo", (void*)eSwitchParameter::HILO, 0, _("22kHz signal is automaticaly switched") );
	new eListBoxEntryText( *c, "On", (void*)eSwitchParameter::ON, 0, _("22kHz is always enabled (high band)") );
	new eListBoxEntryText( *c, "Off", (void*)eSwitchParameter::OFF, 0, _("22kHz is always disabled (low band)") );
	c->setCurrent( (void*) (int) s->getSwitchParams().HiLoSignal );
	CONNECT( c->selchanged_id, eSatelliteConfigurationManager::hiloChanged);

	c = new eComboBox(w_buttons, 3, lVoltage );
	c->setName("satWidget");
	sat.voltage=c;
	c->loadDeco();

	c->resize( eSize( 90, 30 ) );
	c->setHelpText( _("press ok to select another LNB Voltage mode") );
	new eListBoxEntryText( *c, "H/V", (void*)eSwitchParameter::HV, 0, _("Voltage is automaticaly changed") );
	new eListBoxEntryText( *c, "14V", (void*)eSwitchParameter::_14V, 0, _("Voltage is always 14V (vertical)") );
	new eListBoxEntryText( *c, "18V", (void*)eSwitchParameter::_18V, 0, _("Voltage is always 18V (horizontal") );
	new eListBoxEntryText( *c, "off", (void*)eSwitchParameter::_0V, 0, _("Voltage is always off") );
	c->setCurrent( (void*) (int) s->getSwitchParams().VoltageMode );
	CONNECT( c->selchanged_id, eSatelliteConfigurationManager::voltageChanged);
	entryMap.insert( std::pair <eSatellite*, SatelliteEntry> ( s, sat ) );
}

void eSatelliteConfigurationManager::newPressed()
{
	std::list<tpPacket>::const_iterator i(networks.begin());
	// we search the next unused Satellite in list...
	int found=0;
	for (; i != networks.end(); ++i)
	{
		std::map< eSatellite*, SatelliteEntry > :: iterator it ( entryMap.begin() );
		for ( ; it != entryMap.end(); it++)
		{
			if ( i->possibleTransponders.begin()->satellite.orbital_position == it->first->getOrbitalPosition() )
				break;  // test the next...
		}
		if ( it == entryMap.end() )  // all Entrys have been checked...
		{
			found++;
			break;
		}
	}
	if ( !found )
	{
		eMessageBox("No more unused satellites in satellites.xml. Please add a new satellite with possible transponder(s).","Could not add satellite." );
		return;
	}


	eLNB *lnb;
			// if not in user-defined mode, 
	if ( (complexity < 3) || !eTransponderList::getInstance()->getLNBs().size() )  // lnb list is empty ?
	{
		eTransponderList::getInstance()->getLNBs().push_back( eLNB(*eTransponderList::getInstance()) );
		lnb = &eTransponderList::getInstance()->getLNBs().back();
		lnb->setLOFHi(10600000);
		lnb->setLOFLo(9750000);
		lnb->setLOFThreshold(11700000);
		lnb->setIncreasedVoltage(0);
		lnb->getDiSEqC().MiniDiSEqCParam=eDiSEqC::NO;
		lnb->getDiSEqC().DiSEqCParam=eDiSEqC::AA;
		lnb->getDiSEqC().DiSEqCMode=eDiSEqC::V1_0;
		lnb->getDiSEqC().DiSEqCRepeats=0;
		lnb->getDiSEqC().SeqRepeat=0;
		lnb->getDiSEqC().uncommitted_switch=0;
		lnb->getDiSEqC().uncommitted_gap=0;
		lnb->getDiSEqC().useGotoXX=1;
		lnb->getDiSEqC().gotoXXLatitude=0.0;
		lnb->getDiSEqC().gotoXXLaDirection=eDiSEqC::NORTH;
		lnb->getDiSEqC().gotoXXLongitude=0.0;
		lnb->getDiSEqC().gotoXXLoDirection=eDiSEqC::EAST;
	}
	else // we use the last lnb in the list for the new Satellite
		lnb = &eTransponderList::getInstance()->getLNBs().back();

	eSatellite *satellite = lnb->addSatellite( i->possibleTransponders.begin()->satellite.orbital_position );
	satellite->setDescription(i->name);
	eSwitchParameter &sParams = satellite->getSwitchParams();
	sParams.VoltageMode = eSwitchParameter::HV;
	sParams.HiLoSignal = eSwitchParameter::HILO;

	// here we must add the new Comboboxes and the button to the hash maps...
	addSatellite(satellite);
	repositionWidgets();
}

int eSatelliteConfigurationManager::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::execBegin:
		w_buttons->setLCD( LCDTitle, LCDElement );
	break;
	case eWidgetEvent::evtAction:
		if (event.action == &i_focusActions->left)
			focusNext(eWidget::focusDirW);
		else if (event.action == &i_focusActions->right)
			focusNext(eWidget::focusDirE);
		else if (event.action == &i_focusActions->up)
			focusNext(eWidget::focusDirN);
		else if (event.action == &i_focusActions->down)
			focusNext(eWidget::focusDirS);
		else
			return eWindow::eventHandler(event);
	break;

	default:
		return eWindow::eventHandler(event);
		break;
	}
  return 1;
}

eLNBSetup::eLNBSetup( eSatellite* sat, eWidget* lcdTitle, eWidget* lcdElement )
	:sat(sat)
{
	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eLNBSetup"))
		eFatal("skin load of \"eLNBSetup\" failed");

	DiSEqCPage = new eDiSEqCPage( this, sat );
	DiSEqCPage->setLCD( lcdTitle, lcdElement );
	LNBPage = new eLNBPage( this, sat );
	LNBPage->setLCD( lcdTitle, lcdElement );
	RotorPage = new eRotorPage( this, sat );
	RotorPage->setLCD( lcdTitle, lcdElement );
//	ManuallyRotorPage = new eManuallyRotorPage( this );
	RotorPage->setLCD( lcdTitle, lcdElement );
    
	DiSEqCPage->hide();
	LNBPage->hide();
	RotorPage->hide();
//	ManuallyRotorPage->hide();
	mp.addPage( LNBPage );    
	mp.addPage( DiSEqCPage );
//	mp.addPage( ManuallyRotorPage );
	mp.addPage( RotorPage );

// here we can not use the Makro CONNECT ... slot (*this, .... is here not okay..
	LNBPage->lnb_list->selchanged.connect( slot( *LNBPage, &eLNBPage::lnbChanged) );
	LNBPage->lnb_list->selchanged.connect( slot( *DiSEqCPage, &eDiSEqCPage::lnbChanged) );
	LNBPage->lnb_list->selchanged.connect( slot( *RotorPage, &eRotorPage::lnbChanged) );
   
	CONNECT( DiSEqCPage->next->selected, eLNBSetup::onNext );
	CONNECT( LNBPage->next->selected, eLNBSetup::onNext );
	CONNECT( DiSEqCPage->prev->selected, eLNBSetup::onPrev );  
	CONNECT( LNBPage->save->selected, eLNBSetup::onSave );
	CONNECT( LNBPage->cancel->selected, eLNBSetup::reject);
	CONNECT( DiSEqCPage->save->selected, eLNBSetup::onSave);
	CONNECT( DiSEqCPage->cancel->selected, eLNBSetup::reject);
	CONNECT( RotorPage->prev->selected, eLNBSetup::onPrev );
	CONNECT( RotorPage->save->selected, eLNBSetup::onSave );
	CONNECT( RotorPage->cancel->selected, eLNBSetup::reject);
//	CONNECT( ManuallyRotorPage->prev->selected, eLNBSetup::onPrev );
} 

struct savePosition: public std::unary_function< eListBoxEntryText&, void>
{
	std::map<int,int> &map;

	savePosition(std::map<int,int> &map): map(map)
	{
	}

	bool operator()(eListBoxEntryText& s)
	{
		if ( (int)s.getKey() == 0xFFFF )
			return 0; // ignore sample Entry... delete me...

		int num = atoi( s.getText().left( s.getText().find('/') ).c_str() );
		map[ (int)s.getKey() ] = num;
		return 0;
	}
};

void eLNBSetup::onSave()
{
	eLNB *p = (eLNB*) LNBPage->lnb_list->getCurrent()->getKey();

	if ( !p )  // then we must create new LNB; (New is selected)
	{
		eTransponderList::getInstance()->getLNBs().push_back( eLNB( *eTransponderList::getInstance() ) );  // add new LNB
		p = &eTransponderList::getInstance()->getLNBs().back();   // get adresse from the new lnb
//		eDebug("now we have a new LNB Created = %p", p );
	}
/*	else
		eDebug("do not create LNB");*/

	p->setLOFLo( LNBPage->lofL->getNumber() * 1000 );
	p->setLOFHi( LNBPage->lofH->getNumber() * 1000 );
	p->setLOFThreshold( LNBPage->threshold->getNumber() * 1000 );
	p->setIncreasedVoltage( LNBPage->increased_voltage->isChecked() );

	p->getDiSEqC().MiniDiSEqCParam = (eDiSEqC::tMiniDiSEqCParam) (int) DiSEqCPage->MiniDiSEqCParam->getCurrent()->getKey();
	p->getDiSEqC().DiSEqCMode = (eDiSEqC::tDiSEqCMode) (int) DiSEqCPage->DiSEqCMode->getCurrent()->getKey();
	p->getDiSEqC().DiSEqCParam = (int) DiSEqCPage->DiSEqCParam->getCurrent()->getKey();
	p->getDiSEqC().DiSEqCRepeats = (int) DiSEqCPage->DiSEqCRepeats->getCurrent()->getKey();
	p->getDiSEqC().SeqRepeat = DiSEqCPage->SeqRepeat->isChecked();
	p->getDiSEqC().uncommitted_switch = DiSEqCPage->uncommitted->isChecked();
	p->getDiSEqC().uncommitted_gap = DiSEqCPage->uncommitted_gap->isChecked();
	p->getDiSEqC().useGotoXX = RotorPage->useGotoXX->isChecked();
	p->getDiSEqC().gotoXXLaDirection = (int)RotorPage->LaDirection->getCurrent()->getKey();
	p->getDiSEqC().gotoXXLoDirection = (int)RotorPage->LoDirection->getCurrent()->getKey();
	p->getDiSEqC().gotoXXLatitude = RotorPage->Latitude->getFixedNum();
	p->getDiSEqC().gotoXXLongitude = RotorPage->Longitude->getFixedNum();
	p->getDiSEqC().RotorTable.clear();
	RotorPage->positions->forEachEntry( savePosition( p->getDiSEqC().RotorTable ) );
                
	if ( p != sat->getLNB() )  // the satellite must removed from the old lnb and inserts in the new
	{
		p->addSatellite( sat->getLNB()->takeSatellite( sat ) );
/*		eDebug("remove satellite from lnb... now %i satellites left", sat->getLNB()->getSatelliteList().size() );
		eDebug("added satellite to lnb... now %i satellites in lnb", p->getSatelliteList().size() );		*/

		if ( !sat->getLNB()->getSatelliteList().size() )   // the lnb that have no more satellites must delete
		{
//			eDebug("delete no more used lnb");
		  eTransponderList::getInstance()->getLNBs().remove( *sat->getLNB() );
		}
/*		else
			eDebug("lnb not deleted");*/

		// now we must set the LNB Pointer in eSatellite...
		sat->setLNB(p);

		close(-1); // we must reposition control elements in eSatelliteConfigurationManager
	}
	else
		close(0); // we must not reposition...

	eDebug("flush");
	eTransponderList::getInstance()->writeLNBData();
	eFrontend::getInstance()->Reset();
}

int eLNBSetup::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::execBegin:
		mp.first();
		LNBPage->lnbChanged( LNBPage->lnb_list->getCurrent() );   // fake selchanged for initialize lofl, lofh, threshold...
		DiSEqCPage->lnbChanged( LNBPage->lnb_list->getCurrent() );   // fake selchanged for initialize lofl, lofh, threshold...
		RotorPage->lnbChanged( LNBPage->lnb_list->getCurrent() );   // fake selchanged for initialize lofl, lofh, threshold...        
	break;
	default:
		break;
	}
	return eWindow::eventHandler(event);
}

struct eLNBPage::selectlnb: public std::unary_function<const eListBoxEntryText&, void>
{
	const eLNB *lnb;
	eListBox<eListBoxEntryText> *lb;

	selectlnb(const eLNB* lnb, eListBox<eListBoxEntryText>* lb): lnb(lnb), lb(lb)
	{
	}

	bool operator()(const eListBoxEntryText& s)
	{
		if ( lnb == (eLNB*)s.getKey() )
		{
			lb->setCurrent(&s);
			return 1;
		}
		return 0;
	}
};

eLNBPage::eLNBPage( eWidget *parent, eSatellite* sat )
  :eWidget(parent), sat(sat)
{
	LCDTitle=parent->LCDTitle;
	LCDElement=parent->LCDElement;
  
	lnb_list = new eListBox<eListBoxEntryText>(this);
	lnb_list->setFlags( eListBoxBase::flagNoPageMovement );
	lnb_list->setName("lnblist");

	eLabel *l = new eLabel(this);
	l->setName("lLofL");
	lofL = new eNumber(this, 5, 0, 9, 1, 0, 0, l);  // todo descr label im skin mit name versehen für lcd anzeige
	lofL->setName("lofL");

	l = new eLabel(this);
	l->setName("lLofH");
	lofH = new eNumber(this, 5, 0, 9, 1, 0, 0, l);  // todo descr label im skin mit name versehen für lcd anzeige
	lofH->setName("lofH");

	l = new eLabel(this);
	l->setName("lThreshold");
	threshold = new eNumber(this, 5, 0 ,9, 1, 0, 0, l);
	threshold->setName("threshold");

	increased_voltage = new eCheckbox( this );
	increased_voltage->setName("increased_voltage");
                                       
	save = new eButton(this);
	save->setName("save");

	cancel = new eButton(this);
	cancel->setName("cancel");

	next = new eButton(this);
	next->setName("next");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eLNBPage"))
		eFatal("skin load of \"eLNBPage\" failed");

	// add all LNBs

	int i=0;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		new eListBoxEntryText(lnb_list, eString().sprintf("LNB %i", i++), (void*)&(*it) );

	// add a None LNB
	new eListBoxEntryText(lnb_list, _("New"), (void*) 0 );

	lnb_list->forEachEntry(	selectlnb( sat->getLNB(), lnb_list ) );

	CONNECT( lofL->selected, eLNBPage::numSelected);
	CONNECT( lofH->selected, eLNBPage::numSelected);
	CONNECT( threshold->selected, eLNBPage::numSelected);
	CONNECT( lnb_list->selected, eLNBPage::lnbSelected);
	// on exec we begin in eventHandler execBegin
}

void eLNBPage::lnbSelected( eListBoxEntryText*)
{
	focusNext( eWidget::focusDirNext );
}

void eLNBPage::lnbChanged( eListBoxEntryText *lnb )
{
	int l1 = 9750000;
	int l2 = 10600000;
	int l3 = 11700000;
	int incVoltage = 0;
	if ( lnb && lnb->getKey() )
	{
		l1 = ((eLNB*)lnb->getKey())->getLOFLo();
		l2 = ((eLNB*)lnb->getKey())->getLOFHi();
		l3 = ((eLNB*)lnb->getKey())->getLOFThreshold();
		incVoltage = ((eLNB*)lnb->getKey())->getIncreasedVoltage();
	}
	lofL->setNumber( l1 / 1000 );
	lofH->setNumber( l2 / 1000 );
	increased_voltage->setCheck( incVoltage );
	threshold->setNumber( l3 / 1000 );
}

void eLNBPage::numSelected(int*)
{
	focusNext( eWidget::focusDirNext );
}

eDiSEqCPage::eDiSEqCPage( eWidget *parent, eSatellite *sat)
	:eWidget(parent), sat(sat)
{
	LCDTitle=parent->LCDTitle;
	LCDElement=parent->LCDElement;

	eLabel *l = new eLabel(this);
	l->setName("lMiniDiSEqCPara");
	MiniDiSEqCParam = new eComboBox( this, 4, l );
	MiniDiSEqCParam->setName("MiniDiSEqCParam");
	new eListBoxEntryText( *MiniDiSEqCParam, _("None"), (void*)eDiSEqC::NO, 0, _("sends no tone burst") );
	new eListBoxEntryText( *MiniDiSEqCParam, "A", (void*)eDiSEqC::A, 0, _("sends modulated tone burst") );
	new eListBoxEntryText( *MiniDiSEqCParam, "B", (void*)eDiSEqC::B, 0, _("sends unmodulated tone burst") );

	l = new eLabel(this);
	l->setName("lDiSEqCMode");
	DiSEqCMode = new eComboBox( this, 4, l );
	DiSEqCMode->setName("DiSEqCMode");
	// *DiSEqCMode... here we use the operator eListBox* from eComboBox !
	new eListBoxEntryText( *DiSEqCMode, "None", (void*)eDiSEqC::NONE, 0, _("Disable DiSEqC") );
	new eListBoxEntryText( *DiSEqCMode, "Version 1.0", (void*)eDiSEqC::V1_0, 0, _("Use DiSEqC Version 1.0") );
	new eListBoxEntryText( *DiSEqCMode, "Version 1.1", (void*)eDiSEqC::V1_1, 0, _("Use DiSEqC Version 1.1") );
	new eListBoxEntryText( *DiSEqCMode, "Version 1.2", (void*)eDiSEqC::V1_2, 0, _("Use DiSEqC Version 1.2") );
	// no SMATV at the moment... we can do this when anyone ask...
	// 	new eListBoxEntryText( *DiSEqCMode, "SMATV", (void*)eDiSEqC::SMATV, 0, _("Use SMATV Remote Tuning") );

	lDiSEqCParam = new eLabel(this);
	lDiSEqCParam->setName("lDiSEqCParam");
	DiSEqCParam = new eComboBox( this, 4, lDiSEqCParam );
	DiSEqCParam->setName("DiSEqCParam");
	new eListBoxEntryText( *DiSEqCParam, "A/A", (void*)eDiSEqC::AA, 0, _("sends DiSEqC cmd A/A") );
	new eListBoxEntryText( *DiSEqCParam, "A/B", (void*)eDiSEqC::AB, 0, _("sends DiSEqC cmd A/B") );
	new eListBoxEntryText( *DiSEqCParam, "B/A", (void*)eDiSEqC::BA, 0, _("sends DiSEqC cmd B/A") );
	new eListBoxEntryText( *DiSEqCParam, "B/B", (void*)eDiSEqC::BB, 0, _("sends DiSEqC cmd B/B") );
	new eListBoxEntryText( *DiSEqCParam, "1", (void*)0xF0, 0, _("sends switch cmd 1") );
	new eListBoxEntryText( *DiSEqCParam, "2", (void*)0xF1, 0, _("sends switch cmd 2") );
	new eListBoxEntryText( *DiSEqCParam, "3", (void*)0xF2, 0, _("sends switch cmd 3") );
	new eListBoxEntryText( *DiSEqCParam, "4", (void*)0xF3, 0, _("sends switch cmd 4") );
	new eListBoxEntryText( *DiSEqCParam, "5", (void*)0xF4, 0, _("sends switch cmd 5") );
	new eListBoxEntryText( *DiSEqCParam, "6", (void*)0xF5, 0, _("sends switch cmd 6") );
	new eListBoxEntryText( *DiSEqCParam, "7", (void*)0xF6, 0, _("sends switch cmd 7") );
	new eListBoxEntryText( *DiSEqCParam, "8", (void*)0xF7, 0, _("sends switch cmd 8") );
	new eListBoxEntryText( *DiSEqCParam, "9", (void*)0xF8, 0, _("sends switch cmd 9") );
	new eListBoxEntryText( *DiSEqCParam, "10", (void*)0xF9, 0, _("sends switch cmd 10") );
	new eListBoxEntryText( *DiSEqCParam, "11", (void*)0xFA, 0, _("sends switch cmd 11") );
	new eListBoxEntryText( *DiSEqCParam, "12", (void*)0xFB, 0, _("sends switch cmd 12") );
	new eListBoxEntryText( *DiSEqCParam, "13", (void*)0xFC, 0, _("sends switch cmd 13") );
	new eListBoxEntryText( *DiSEqCParam, "14", (void*)0xFD, 0, _("sends switch cmd 14") );
	new eListBoxEntryText( *DiSEqCParam, "15", (void*)0xFE, 0, _("sends switch cmd 15") );
	new eListBoxEntryText( *DiSEqCParam, "16", (void*)0xFF, 0, _("sends switch cmd 16") );

	lDiSEqCRepeats = new eLabel(this);
	lDiSEqCRepeats->setName("lDiSEqCRepeats");
	DiSEqCRepeats = new eComboBox( this, 4, lDiSEqCRepeats );
	DiSEqCRepeats->setName("DiSEqCRepeats");
	new eListBoxEntryText( *DiSEqCRepeats, _("None"), (void*)0, 0, _("sends no DiSEqC repeats") );
	new eListBoxEntryText( *DiSEqCRepeats, _("One"), (void*)1, 0, _("sends one repeat") );
	new eListBoxEntryText( *DiSEqCRepeats, _("Two"), (void*)2, 0, _("sends two repeats") );
	new eListBoxEntryText( *DiSEqCRepeats, _("Three"), (void*)3, 0, _("sends three repeats") );
  
	SeqRepeat = new eCheckbox(this);
	SeqRepeat->setName("SeqRepeat");

	uncommitted = new eCheckbox(this);
	uncommitted->setName("uncommitted");
  
	uncommitted_gap = new eCheckbox(this);
	uncommitted_gap->setName("uncommitted_gap");

	next = new eButton(this);
	next->setName("next");
            
	prev = new eButton(this);
	prev->setName("prev");
  
	save = new eButton(this);
	save->setName("save");

	cancel = new eButton(this);
	cancel->setName("cancel");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eDiSEqCPage"))
		eFatal("skin load of \"eDiSEqCPage\" failed");

	CONNECT( DiSEqCMode->selchanged, eDiSEqCPage::DiSEqCModeChanged );
	addActionMap(&i_focusActions->map);
}

void eDiSEqCPage::numSelected(int*)
{
	focusNext( eWidget::focusDirNext );
}

void eDiSEqCPage::DiSEqCModeChanged( eListBoxEntryText* e )
{
	switch( (int) e->getKey() )
	{
		case eDiSEqC::V1_2:
			next->show();
		case eDiSEqC::V1_1:
			lDiSEqCRepeats->show();
			DiSEqCRepeats->show();
			uncommitted->show();
			uncommitted_gap->show();
		case eDiSEqC::V1_0:
			lDiSEqCParam->show();
			DiSEqCParam->show();
			SeqRepeat->show();
		break;
	}
	switch ( (int) e->getKey() )
	{
		default:
		case eDiSEqC::NONE:
			SeqRepeat->hide();
			DiSEqCParam->hide();
			lDiSEqCParam->hide();
		case eDiSEqC::V1_0:
			lDiSEqCRepeats->hide();
			DiSEqCRepeats->hide();
			uncommitted->hide();
			uncommitted_gap->hide();
		case eDiSEqC::V1_1:
			next->hide();
		case eDiSEqC::V1_2: // hide nothing
		break;
  }
}

void eDiSEqCPage::lnbChanged( eListBoxEntryText *lnb )
{
	if ( lnb && lnb->getKey() )
	{
		DiSEqCMode->setCurrent( (void*) (int) ((eLNB*)lnb->getKey())->getDiSEqC().DiSEqCMode );
		MiniDiSEqCParam->setCurrent( (void*) ((eLNB*)lnb->getKey())->getDiSEqC().MiniDiSEqCParam );
		DiSEqCParam->setCurrent( (void*) ((eLNB*)lnb->getKey())->getDiSEqC().DiSEqCParam );
		DiSEqCRepeats->setCurrent( (void*) ((eLNB*)lnb->getKey())->getDiSEqC().DiSEqCRepeats );
		SeqRepeat->setCheck( (int) ((eLNB*)lnb->getKey())->getDiSEqC().SeqRepeat );
		uncommitted->setCheck( (int) ((eLNB*)lnb->getKey())->getDiSEqC().uncommitted_switch );
		uncommitted_gap->setCheck( (int) ((eLNB*)lnb->getKey())->getDiSEqC().uncommitted_gap );
	}
	else
	{
		DiSEqCMode->setCurrent( 0 );
		DiSEqCParam->setCurrent( 0 );
		MiniDiSEqCParam->setCurrent( 0 );
		DiSEqCRepeats->setCurrent( 0 );
		SeqRepeat->setCheck( 0 );
		uncommitted->setCheck( 0 );
		uncommitted_gap->setCheck( 0 );
	}
	DiSEqCModeChanged( (eListBoxEntryText*) DiSEqCMode->getCurrent() );
}

eRotorPage::eRotorPage( eWidget *parent, eSatellite *sat )
	:eWidget(parent), sat(sat)
{
	LCDTitle=parent->LCDTitle;
	LCDElement=parent->LCDElement;

	useGotoXX = new eCheckbox(this);
	useGotoXX->setName("useGotoXX");

	lLongitude = new eLabel(this);
	lLongitude->setName("lLongitude");
	lLongitude->hide();

	Longitude = new eNumber(this, 2, 0, 360, 3, 0, 0, lLongitude );
	Longitude->setFlags( eNumber::flagFixedNum );
	Longitude->setName("Longitude");
	Longitude->hide();

	LoDirection = new eComboBox( this, 2 );
	LoDirection->setName("LoDirection");
	LoDirection->hide();
	new eListBoxEntryText( *LoDirection, _("East"), (void*)eDiSEqC::EAST, 0, _("East") );
	new eListBoxEntryText( *LoDirection, _("West"), (void*)eDiSEqC::WEST, 0, _("West") );

	lLatitude = new eLabel(this);
	lLatitude->setName("lLatitude");
	lLatitude->hide();

	Latitude = new eNumber(this, 2, 0, 360, 3, 0, 0, lLatitude );
	Latitude->setFlags( eNumber::flagFixedNum );
	Latitude->setName("Latitude");
	Latitude->hide();

	LaDirection = new eComboBox( this, 2 );
	LaDirection->setName("LaDirection");
	LaDirection->hide();
	new eListBoxEntryText( *LaDirection, _("North"), (void*)eDiSEqC::NORTH, 0, _("North") );
	new eListBoxEntryText( *LaDirection, _("South"), (void*)eDiSEqC::SOUTH, 0, _("South") );

	positions = new eListBox< eListBoxEntryText >( this );  
	positions->setFlags( eListBoxBase::flagNoPageMovement );
	positions->setName("positions");
	positions->hide();
  
	lStoredRotorNo = new eLabel(this);
	lStoredRotorNo->setName("lStoredRotorNo");
	lStoredRotorNo->hide();
	number = new eNumber( this, 1, 0, 255, 3, 0, 0, lStoredRotorNo);
	number->setName("StoredRotorNo");
	number->hide();

	lOrbitalPosition = new eLabel(this);
	lOrbitalPosition->setName("lOrbitalPosition");
	lOrbitalPosition->hide();
	orbital_position = new eNumber( this, 1, 0, 360, 3, 0, 0, lOrbitalPosition);

	orbital_position->setName("OrbitalPosition");
	orbital_position->hide();
  
	lDirection = new eLabel(this);
	lDirection->setName("lDirection");
	lDirection->hide();
	direction = new eComboBox( this, 2, lDirection );
	direction->setName("Direction");
	direction->hide();
	new eListBoxEntryText( *direction, _("East"), (void*)0, 0, _("East") );
	new eListBoxEntryText( *direction, _("West"), (void*)1, 0, _("West") );

	add = new eButton( this );
	add->setName("add");
	add->hide();

	remove = new eButton ( this );
	remove->setName("remove");
	remove->hide();
  
	prev = new eButton(this);
	prev->setName("prev");

	save = new eButton(this);
	save->setName("save");

	cancel = new eButton(this);
	cancel->setName("cancel");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eRotorPage"))
		eFatal("skin load of \"eRotorPage\" failed");

	CONNECT( orbital_position->selected, eRotorPage::numSelected );
	CONNECT( Longitude->selected, eRotorPage::numSelected );
	CONNECT( Latitude->selected, eRotorPage::numSelected );
	CONNECT( number->selected, eRotorPage::numSelected );    
	CONNECT( add->selected, eRotorPage::onAdd );
	CONNECT( remove->selected, eRotorPage::onRemove );
	CONNECT( positions->selchanged, eRotorPage::posChanged );
	CONNECT( useGotoXX->checked, eRotorPage::gotoXXChanged );
  
	addActionMap(&i_focusActions->map);
}

void eRotorPage::gotoXXChanged( int state )
{
	eDebug("gotoXXChanged to %d", state);
	if ( state )
	{
		add->hide();
		remove->hide();
		lOrbitalPosition->hide();
		orbital_position->hide();
		lStoredRotorNo->hide();
		number->hide();
		lDirection->hide();
		direction->hide();
		positions->hide();
		
		lLongitude->show();
		Longitude->show();
		LoDirection->show();
		lLatitude->show();
		Latitude->show();
		LaDirection->show();
	}
	else
	{
		lLongitude->hide();
		Longitude->hide();
		LoDirection->hide();
		lLatitude->hide();
		Latitude->hide();
		LaDirection->hide();

		positions->show();
		add->show();
		remove->show();
		lOrbitalPosition->show();
		orbital_position->show();
		lStoredRotorNo->show();
		number->show();
		lDirection->show();
		direction->show();
		positions->show();
	}
}

void eRotorPage::lnbChanged( eListBoxEntryText *lnb )
{
	curlnb=(eLNB*)lnb->getKey();

	positions->beginAtomic();

	positions->clearList();
	eDiSEqC &DiSEqC = ((eLNB*)lnb->getKey())->getDiSEqC();
  
	if ( lnb && lnb->getKey() )
	{
		for ( std::map<int, int>::iterator it ( DiSEqC.RotorTable.begin() ); it != DiSEqC.RotorTable.end(); it++ )
			new eListBoxEntryText( positions, eString().sprintf(" %d / %03d %c", it->second, abs(it->first), it->first > 0 ? 'E' : 'W'), (void*) it->first );

		useGotoXX->setCheck( (int) ((eLNB*)lnb->getKey())->getDiSEqC().useGotoXX );
		gotoXXChanged( (int) ((eLNB*)lnb->getKey())->getDiSEqC().useGotoXX );
		Latitude->setFixedNum( ((eLNB*)lnb->getKey())->getDiSEqC().gotoXXLatitude );
		LaDirection->setCurrent( (void*) ((eLNB*)lnb->getKey())->getDiSEqC().gotoXXLaDirection );
		Longitude->setFixedNum( ((eLNB*)lnb->getKey())->getDiSEqC().gotoXXLongitude );
		LoDirection->setCurrent( (void*) ((eLNB*)lnb->getKey())->getDiSEqC().gotoXXLoDirection );		
	}
	else
	{
		Latitude->setFixedNum(0);
		LaDirection->setCurrent(0);
		Longitude->setFixedNum(0);
		LoDirection->setCurrent(0);		
		useGotoXX->setCheck( 1 );
	}

	if ( positions->getCount() )
	{
		positions->sort();
		positions->moveSelection(eListBox<eListBoxEntryText>::dirFirst);
	}
	else
	{
		new eListBoxEntryText( positions, _("delete me"), (void*) 0xFFFF );
		posChanged(0);
	}

	positions->endAtomic();
}

void eRotorPage::posChanged( eListBoxEntryText *e )
{
	if ( e && (int)e->getKey() != 0xFFFF )
	{
		direction->setCurrent( e->getText().right( 1 ) == "E" ? 0 : 1 );
		orbital_position->setNumber( (int) e->getKey() );
		number->setNumber( atoi( e->getText().mid( 1 , e->getText().find('/')-1 ).c_str()) );
	}
	else
	{
		orbital_position->setNumber( 0 );
		number->setNumber( 0 );
		direction->setCurrent( 0 );
	}
}

void eRotorPage::numSelected(int*)
{
	focusNext( eWidget::focusDirNext );
}

void eRotorPage::onAdd()
{
	positions->beginAtomic();

	new eListBoxEntryText( positions,eString().sprintf(" %d / %03d %c",
																											number->getNumber(),
																											orbital_position->getNumber(),
																											direction->getCurrent()->getKey() ? 'W':'E'
																										),
													(void*) ( direction->getCurrent()->getKey()
													? - orbital_position->getNumber()
													: orbital_position->getNumber() )
												);

	positions->sort();
	positions->invalidateContent();
	positions->endAtomic();
}

void eRotorPage::onRemove()
{
	positions->beginAtomic();

	if (positions->getCurrent())
		positions->remove( positions->getCurrent() );

	if (!positions->getCount())
	{
		new eListBoxEntryText( positions, _("delete me"), (void*) 0xFFFF );
		posChanged(0);
	}

	positions->invalidate();
	positions->endAtomic();
}

eManuallyRotorPage::eManuallyRotorPage( eWidget *parent )
	:eWidget(parent), transponder(*eDVB::getInstance()->settings->getTransponders())
{
	int ft=0;
 
	switch (eFrontend::getInstance()->Type())
	{
	case eFrontend::feSatellite:
		ft=eTransponderWidget::deliverySatellite;
		break;
	case eFrontend::feCable:
		ft=eTransponderWidget::deliveryCable;
		break;
	default:
		ft=eTransponderWidget::deliverySatellite;
		break;
	}

	transponder_widget=new eTransponderWidget(this, 1, ft);
	transponder_widget->load();
	transponder_widget->setName("transponder");

	festatus_widget=new eFEStatusWidget(this, eFrontend::getInstance());
	festatus_widget->setName("festatus");

	prev = new eButton(this);
	prev->setName("prev");

	if ( eSkin::getActive()->build(this, "eRotorPageManual"))
		eFatal("skin load of \"eRotorPageManual\" failed");

	CONNECT(transponder_widget->updated, eManuallyRotorPage::retune );
}

void eManuallyRotorPage::retune()
{
	if (!transponder_widget->getTransponder(&transponder))
		transponder.tune();
}

int eManuallyRotorPage::eventHandler( const eWidgetEvent &event )
{
	eDebug("eventHandler");
	switch (event.type)
	{
		case eWidgetEvent::willShow:
		{
			eDebug("eventHandler->willShow()");
			eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

			if (sapi && sapi->transponder)
				transponder=*sapi->transponder;
			else
				switch (eFrontend::getInstance()->Type())
				{
				case eFrontend::feCable:
					transponder.setCable(402000, 6900000, 0, 3);	// some cable transponder
					break;
				case eFrontend::feSatellite:
					transponder.setSatellite(12551500, 22000000, eFrontend::polVert, 4, 0, 192 ); // some astra transponder
					break;
				default:
					break;
				}
			transponder_widget->setTransponder(&transponder);
		}
		default:
			return eWidget::eventHandler(event);
	}
	return 1;
}
