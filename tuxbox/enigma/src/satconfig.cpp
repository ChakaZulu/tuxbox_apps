#include <satconfig.h>

#include <lib/base/i18n.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/gui/echeckbox.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvbwidgets.h>
//#include <lib/driver/rc.h>

eSatelliteConfigurationManager::eSatelliteConfigurationManager()
	:refresh(0), deleteThisEntry( 0 )
{
	button_close=new eButton(this);
	button_close->setName("close");
	CONNECT(button_close->selected, eSatelliteConfigurationManager::closePressed);

	button_new=new eButton(this);
	button_new->setName("new");
	CONNECT(button_new->selected, eSatelliteConfigurationManager::newPressed);

	w_buttons=new eWidget(this);
	w_buttons->setName("buttons");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eSatelliteConfigurationManager"))
		eFatal("skin load of \"eSatelliteConfigurationManager\" failed");

	parseNetworks();  // load all networks from satellite.xml or cable.xml
	createControlElements();
	repositionWidgets();
}

eSatelliteConfigurationManager::~eSatelliteConfigurationManager()
{
	if (refresh)
		delete refresh;
}

eSatellite *eSatelliteConfigurationManager::getSat4SatCombo( const eComboBox *c )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
		if ( it->second.sat == c)
			break;
	return it->first;
}

eSatellite *eSatelliteConfigurationManager::getSat4HiLoCombo( const eComboBox *c )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
		if ( it->second.hilo == c)
			break;
	return it->first;
}

eSatellite *eSatelliteConfigurationManager::getSat4VoltageCombo( const eComboBox *c )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
		if ( it->second.voltage == c)
			break;
	return it->first;
}

eSatellite *eSatelliteConfigurationManager::getSat4LnbButton( const eButton *b )
{
	std::map<eSatellite*, SatelliteEntry>::iterator it ( entryMap.begin() );
	for ( ; it != entryMap.end(); it++)
		if ( it->second.lnb == b)
			break;
	return it->first;
}


#define SAT_POS_X  0
#define LNB_POS_X  200
#define HILO_POS_X  270
#define VOLTAGE_POS_X  370
#define POS_Y 0

void eSatelliteConfigurationManager::repositionWidgets()
{
	if (deleteThisEntry)
	{
		delete deleteThisEntry->sat;
		delete deleteThisEntry->lnb;
		delete deleteThisEntry->voltage;
		delete deleteThisEntry->hilo;
		// search Entry in Map;		
		std::map< eSatellite*, SatelliteEntry >::iterator it( entryMap.begin() );
		for ( ; it != entryMap.end() && &it->second != deleteThisEntry ; it++);
		if (it != entryMap.end() )
			entryMap.erase( it );		
		deleteThisEntry=0;
	}
	int sx=SAT_POS_X, y=POS_Y, hx=HILO_POS_X, vx=VOLTAGE_POS_X, lx=LNB_POS_X, count=0;

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
	{
		for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
		{
			SatelliteEntry& entry = entryMap[ *s ];
			// search eComboBox for this eSatellite and move
			if (isVisible())
			{
				entry.sat->hide();
				entry.lnb->hide();
				entry.voltage->hide();
				entry.hilo->hide();
			}
			entry.sat->move( ePoint(sx, y) );
			entry.lnb->move( ePoint(lx, y) );
			entry.lnb->setText( eString().sprintf("%i", count) );
			entry.hilo->move( ePoint(hx, y) );
			entry.voltage->move( ePoint(vx, y) );

			if (isVisible())
			{
				entry.sat->show();
				entry.lnb->show();
				entry.voltage->show();
				entry.hilo->show();
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


void eSatelliteConfigurationManager::satChanged(eComboBox* who, eListBoxEntryText *le)
{
	eSatellite *s = getSat4SatCombo(who);
	if ( le->getKey() && le->getText() )
	{
		s->setOrbitalPosition( (int) le->getKey() );
		s->setDescription( le->getText() );
	}
	else  // *delete* selected -->> satellite and empty lnbs were now deleted
	{
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

	eLabel *l = new eLabel(this);
	l->setName("lSatPos");
	eComboBox* c = new eComboBox(w_buttons, 6, l);
	sat.sat=c;
	c->loadDeco();
//			c->move(ePoint(sx,y));
	c->resize(eSize(190, 30));
	c->setHelpText( _("press ok to select another satellite, or delete this satellite"));

	new eListBoxEntryText( *c, _("*delete*"), (void*) 0 );   // this is to delete an satellite
	for (std::list<tpPacket>::const_iterator i(networks.begin()); i != networks.end(); ++i)
		if ( i->possibleTransponders.size() )
			new eListBoxEntryText( *c, i->name, (void*) i->possibleTransponders.begin()->satellite.orbital_position );

	int err;
	if ( (err = c->setCurrent( (void*) s->getOrbitalPosition() ) ) )
		if (err == eComboBox::E_COULDNT_FIND)  // hmm current entry not in Combobox... we add manually
			c->setCurrent( new eListBoxEntryText( *c, s->getDescription(), (void*) s->getOrbitalPosition() ) );
	CONNECT( c->selchanged_id, eSatelliteConfigurationManager::satChanged );

	l = new eLabel(this);
	l->setName("lLnb");
	eButton* b = new eButton(w_buttons, l);
	sat.lnb=b;
	b->loadDeco();
//			b->move(ePoint(lx, y));
	b->resize(eSize(60, 30));
	b->setHelpText( _("press ok to goto LNB config"));
	CONNECT(b->selected_id, eSatelliteConfigurationManager::lnbSelected);

	l = new eLabel(this);
	l->setName("l22khz");
	c = new eComboBox(w_buttons, 3, l);
	sat.hilo=c;
	c->loadDeco();
//			c->move( ePoint( hx, y ) );
	c->resize( eSize( 90, 30 ) );
	c->setHelpText( _("press ok to select another 22kHz mode") );
	new eListBoxEntryText( *c, "Hi/Lo", (void*)eSwitchParameter::HILO, 0, _("22kHz signal is automaticaly switched") );
	new eListBoxEntryText( *c, "On", (void*)eSwitchParameter::ON, 0, _("22kHz is always enabled (high band)") );
	new eListBoxEntryText( *c, "Off", (void*)eSwitchParameter::OFF, 0, _("22kHz is always disabled (low band)") );
	c->setCurrent( (void*) (int) s->getSwitchParams().HiLoSignal );
	CONNECT( c->selchanged_id, eSatelliteConfigurationManager::hiloChanged);

	l = new eLabel(this);
	l->setName("lVoltage");
	c = new eComboBox(w_buttons, 3, l	);
	sat.voltage=c;
	c->loadDeco();
//			c->move( ePoint( vx, y ) );
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
	if ( !eTransponderList::getInstance()->getLNBs().size() )  // lnb list is empty ?
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
		lnb->getDiSEqC().rotorOffset=0;    
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
			break;
	 return 1;

	default:
		break;
	}
	return eWindow::eventHandler(event);
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
  
	DiSEqCPage->hide();
	LNBPage->hide();
	RotorPage->hide();
	mp.addPage( LNBPage );    
	mp.addPage( DiSEqCPage );
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
	p->getDiSEqC().rotorOffset = RotorPage->RotorOffset->getNumber();
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
	lofL->invalidateNum();
	lofH->setNumber( l2 / 1000 );
	lofH->invalidateNum();
	increased_voltage->setCheck( incVoltage );
	threshold->setNumber( l3 / 1000 );
	threshold->invalidateNum();
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

	lRotorOffset = new eLabel( this );
	lRotorOffset->setName("lRotorOffset");

	RotorOffset = new eNumber(this, 1, 0, 3600, 4, 0, 0, lRotorOffset );
	RotorOffset->setFlags( eNumber::flagPosNeg );
	RotorOffset->setName("RotorOffset");

	positions = new eListBox< eListBoxEntryText >( this );  
	positions->setFlags( eListBoxBase::flagNoPageMovement );
	positions->setName("positions");
  
	eLabel *l = new eLabel(this);
	l->setName("lStoredRotorNo");
	number = new eNumber( this, 1, 0, 255, 3, 0, 0, l);
	number->setName("StoredRotorNo");

	l = new eLabel(this);
	l->setName("lOrbitalPosition");
	orbital_position = new eNumber( this, 1, 0, 450, 3, 0, 0, l);
	orbital_position->setName("OrbitalPosition");
  
	l = new eLabel(this);
	l->setName("lDirection");
	direction = new eComboBox( this, 2, l );
	direction->setName("Direction");
	new eListBoxEntryText( *direction, _("East"), (void*)0, 0, _("East") );
	new eListBoxEntryText( *direction, _("West"), (void*)1, 0, _("West") );
  
	add = new eButton( this );
	add->setName("add");

	remove = new eButton ( this );
	remove->setName("remove");
  
	prev = new eButton(this);
	prev->setName("prev");

	save = new eButton(this);
	save->setName("save");

	cancel = new eButton(this);
	cancel->setName("cancel");

	eSkin *skin=eSkin::getActive();
	if (skin->build(this, "eRotorPage"))
		eFatal("skin load of \"eRotorPage\" failed");

	CONNECT( RotorOffset->selected, eRotorPage::numSelected );    
	CONNECT( orbital_position->selected, eRotorPage::numSelected );
	CONNECT( number->selected, eRotorPage::numSelected );    
	CONNECT( add->selected, eRotorPage::onAdd );
	CONNECT( remove->selected, eRotorPage::onRemove );
	CONNECT( positions->selchanged, eRotorPage::posChanged );
  
	addActionMap(&i_focusActions->map);
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
		RotorOffset->setNumber( (int) ((eLNB*)lnb->getKey())->getDiSEqC().rotorOffset );
	}
	else
	{
		useGotoXX->setCheck( 1 );
		RotorOffset->setNumber( 0 );
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
	if ( e )
	{
		direction->setCurrent( e->getText().right( 1 ) == "E" ? 0 : 1 );
		int bla = abs ( (int) e->getKey() );
		orbital_position->setNumber( bla );
		number->setNumber( atoi( e->getText().mid( 1 , e->getText().find('/')-1 ).c_str()) );
	}
	else
	{
		orbital_position->setNumber( 0 );
		number->setNumber( 0 );
		direction->setCurrent( 0 );
	}
	orbital_position->invalidateNum();
	number->invalidateNum();
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

	positions->invalidate();
	positions->endAtomic();
}

