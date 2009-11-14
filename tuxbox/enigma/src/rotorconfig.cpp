#include <rotorconfig.h>

#include <lib/base/i18n.h>
#include <lib/system/init_num.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/emessage.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/actions.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/dvb/edvb.h>
#include <lib/system/info.h>

RotorConfig::RotorConfig(eLNB *lnb )
	:lnb(lnb)
{
	init_RotorConfig(lnb);
}

void RotorConfig::init_RotorConfig(eLNB *lnb )
{
#ifndef DISABLE_LCD
	LCDTitle=parent->LCDTitle;
	LCDElement=parent->LCDElement;
#endif
	useRotorInPower = CreateSkinnedCheckbox("useRotorInPower");

	lDeltaA = CreateSkinnedLabel("lDeltaA");
	lDeltaA->hide();
	DeltaA = CreateSkinnedNumber("DeltaA",0, 1, 0, 200, 3, 0, 0, lDeltaA);
	DeltaA->hide();

	useGotoXX = CreateSkinnedCheckbox("useGotoXX");

	lLongitude = CreateSkinnedLabel("lLongitude");
	lLongitude->hide();

	Longitude = CreateSkinnedNumber("Longitude",0, 2, 0, 360, 3, 0, 0, lLongitude );
	Longitude->setFlags( eNumber::flagFixedNum );
	Longitude->hide();

	LoDirection = CreateSkinnedComboBox("LoDirection", 2 );
	LoDirection->hide();
	new eListBoxEntryText( *LoDirection, _("East"), (void*)eDiSEqC::EAST, 0, _("East") );
	new eListBoxEntryText( *LoDirection, _("West"), (void*)eDiSEqC::WEST, 0, _("West") );

	lLatitude = CreateSkinnedLabel("lLatitude");
	lLatitude->hide();

	Latitude = CreateSkinnedNumber("Latitude",0, 2, 0, 360, 3, 0, 0, lLatitude );
	Latitude->setFlags( eNumber::flagFixedNum );
	Latitude->hide();

	LaDirection = CreateSkinnedComboBox("LaDirection", 2 );
	LaDirection->hide();
	new eListBoxEntryText( *LaDirection, _("North"), (void*)eDiSEqC::NORTH, 0, _("North") );
	new eListBoxEntryText( *LaDirection, _("South"), (void*)eDiSEqC::SOUTH, 0, _("South") );

	positions = new eListBox< eListBoxEntryText >( this );
	positions->setFlags( eListBoxBase::flagLostFocusOnFirst|eListBoxBase::flagLostFocusOnLast );
	positions->setName("positions");
	positions->hide();

	lStoredRotorNo = CreateSkinnedLabel("lStoredRotorNo");
	lStoredRotorNo->hide();
	number = CreateSkinnedNumber("StoredRotorNo",0, 1, 0, 255, 3, 0, 0, lStoredRotorNo);
	number->hide();

	lOrbitalPosition = CreateSkinnedLabel("lOrbitalPosition");
	lOrbitalPosition->hide();
	orbital_position = CreateSkinnedNumber("OrbitalPosition",0, 1, 0, 3600, 4, 0, 0, lOrbitalPosition);
	orbital_position->hide();

	lDirection = CreateSkinnedLabel("lDirection");
	lDirection->hide();
	direction = CreateSkinnedComboBox("Direction", 2, lDirection );
	direction->hide();
	new eListBoxEntryText( *direction, _("East"), (void*)0, 0, _("East") );
	new eListBoxEntryText( *direction, _("West"), (void*)1, 0, _("West") );

	add = CreateSkinnedButton("add");
	add->hide();

	remove = CreateSkinnedButton("remove", 0, 0 );
	remove->hide();

	save = CreateSkinnedButton("save");

	next = CreateSkinnedButton("next");

	BuildSkin("RotorConfig");

	orbital_position->setHelpText(_("enter orbital position without dot (19.2\xC2\xB0 = 192)"));
	CONNECT( orbital_position->selected, RotorConfig::numSelected );
	CONNECT( Longitude->selected, RotorConfig::numSelected );
	CONNECT( Latitude->selected, RotorConfig::numSelected );
	CONNECT( number->selected, RotorConfig::numSelected );
	CONNECT( DeltaA->selected, RotorConfig::numSelected );
	CONNECT( add->selected, RotorConfig::onAdd );
	CONNECT( remove->selected, RotorConfig::onRemove );
	CONNECT( positions->selchanged, RotorConfig::posChanged );
	CONNECT( useGotoXX->checked, RotorConfig::gotoXXChanged );
	CONNECT( useRotorInPower->checked, RotorConfig::useRotorInPowerChanged );	

	CONNECT( save->selected, RotorConfig::onSavePressed );
	CONNECT( next->selected, RotorConfig::onNextPressed );

	addActionMap(&i_focusActions->map);

	if (lnb)
		setLNBData(lnb);

	if ( !eSystemInfo::getInstance()->canMeasureLNBCurrent() )
	{
		eDebug("useRotorInputPower can only used on dreambox");
		useRotorInPower->hide();
	}
}

struct savePosition
{
	std::map<int,int,Orbital_Position_Compare> &map;

	savePosition(std::map<int,int,Orbital_Position_Compare> &map): map(map)
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

void RotorConfig::onSavePressed()
{
	lnb->getDiSEqC().useGotoXX = useGotoXX->isChecked();
	lnb->getDiSEqC().useRotorInPower = useRotorInPower->isChecked()?1:0;
	lnb->getDiSEqC().useRotorInPower |= DeltaA->getNumber()<<8;
	lnb->getDiSEqC().gotoXXLaDirection = (int) LaDirection->getCurrent()->getKey();
	lnb->getDiSEqC().gotoXXLoDirection = (int) LoDirection->getCurrent()->getKey();
	lnb->getDiSEqC().gotoXXLatitude = Latitude->getFixedNum();
	lnb->getDiSEqC().gotoXXLongitude = Longitude->getFixedNum();
	lnb->getDiSEqC().RotorTable.clear();
	positions->forEachEntry( savePosition( lnb->getDiSEqC().RotorTable ) );
	eTransponderList::getInstance()->writeLNBData();	
	eConfig::getInstance()->flush();
	close(0);
}

void RotorConfig::useRotorInPowerChanged( int state )
{
	eDebug("useRotorInPowerChanged to %d", state);
	if (state)
	{
		lDeltaA->show();
		DeltaA->show();
	}
	else
	{
		lDeltaA->hide();
		DeltaA->hide();
	}
}

void RotorConfig::gotoXXChanged( int state )
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

void RotorConfig::setLNBData( eLNB *lnb )
{
	positions->beginAtomic();
	positions->clearList();
	eDiSEqC &DiSEqC = lnb->getDiSEqC();

	if ( lnb )
	{
		for ( std::map<int, int>::iterator it ( DiSEqC.RotorTable.begin() ); it != DiSEqC.RotorTable.end(); it++ )
			new eListBoxEntryText( positions, eString().sprintf(" %d / %d.%d%c", it->second, abs(it->first)/10, abs(it->first)%10, it->first > 0 ? 'E' : 'W'), (void*) it->first );

		useGotoXX->setCheck( (int) (lnb->getDiSEqC().useGotoXX & 1 ? 1 : 0) );
		gotoXXChanged( (int) lnb->getDiSEqC().useGotoXX & 1 );
		if ( eSystemInfo::getInstance()->canMeasureLNBCurrent() )
		{
			useRotorInPower->setCheck( (int) lnb->getDiSEqC().useRotorInPower & 1 );
			useRotorInPowerChanged( (int) lnb->getDiSEqC().useRotorInPower & 1 );
		}
		else
		{
			useRotorInPower->setCheck( 0 );
			useRotorInPowerChanged( 0 );
		}

		Latitude->setFixedNum( lnb->getDiSEqC().gotoXXLatitude );
		LaDirection->setCurrent( (void*) lnb->getDiSEqC().gotoXXLaDirection );
		Longitude->setFixedNum( lnb->getDiSEqC().gotoXXLongitude );
		LoDirection->setCurrent( (void*) lnb->getDiSEqC().gotoXXLoDirection );
		DeltaA->setNumber( (lnb->getDiSEqC().useRotorInPower & 0x0000FF00) >> 8 );
	}
	else
	{
		Latitude->setFixedNum(0);
		LaDirection->setCurrent(0);
		Longitude->setFixedNum(0);
		LoDirection->setCurrent(0);
		DeltaA->setNumber(40);
		useGotoXX->setCheck( 1 );
		useRotorInPower->setCheck( 0 );
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

void RotorConfig::posChanged( eListBoxEntryText *e )
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

void RotorConfig::numSelected(int*)
{
	focusNext( eWidget::focusDirNext );
}

void RotorConfig::onAdd()
{
	positions->beginAtomic();

	new eListBoxEntryText( positions,eString().sprintf(" %d / %d.%d%c",
																											number->getNumber(),
																											orbital_position->getNumber()/10,
																											orbital_position->getNumber()%10,
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

void RotorConfig::onRemove()
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

void RotorConfig::onNextPressed()
{
	if (lnb)
	{
		hide();
		eRotorManual c(lnb);
#ifndef DISABLE_LCD
		c.setLCD( LCDTitle, LCDElement );
#endif
		c.show();
		c.exec();
		if (c.changed)
		{
			setLNBData(lnb);
			useGotoXX->setCheck(0);
			positions->forEachEntry( savePosition( lnb->getDiSEqC().RotorTable ) );
			eTransponderList::getInstance()->writeLNBData();
		}
		c.hide();
		show();
	}
}

struct rotorMenuActions
{
	eActionMap map;
	eAction east, eastFine, eastStop, west, westFine, westStop;
	rotorMenuActions():
		map("rotorMenu", "rotorMenu"),
		east(map, "driveEast", _("drive Motor East"), eAction::prioWidget),
		eastFine(map, "driveEastStep", _("drive Motor East.. one step"), eAction::prioWidget),
		eastStop(map, "driveEastStop", _("stop Motor drive East"), eAction::prioWidget),
		west(map, "driveWest", _("drive Motor East"), eAction::prioWidget),
		westFine(map, "driveWestStep", _("drive Motor West.. one step"), eAction::prioWidget),
		westStop(map, "driveWestStop", _("stop Motor drive West"), eAction::prioWidget)
	{
	}
};

eAutoInitP0<rotorMenuActions> i_rotorMenuActions(eAutoInitNumbers::actions, "rotor menu actions");

eRotorManual::eRotorManual(eLNB *lnb)
	:lnb(lnb), retuneTimer(new eTimer(eApp)), transponder(0), changed(0)
{
	init_eRotorManual(lnb);
}

void eRotorManual::init_eRotorManual(eLNB *lnb)
{
	lSat = CreateSkinnedLabel("lSat");

	lTransponder = CreateSkinnedLabel("lTransponder");

	lDirection = CreateSkinnedLabel("lDirection");

	lCounter = CreateSkinnedLabel("lCounter");

	running=false;

	Mode = CreateSkinnedComboBoxWithLabel("Mode", 4, "lMode" );
	CONNECT(Mode->selchanged, eRotorManual::modeChanged);

	num = CreateSkinnedNumber("num",1, 1, 1, 80, 2, 0, 0, lSat );

	// send no more DiSEqC Commands on transponder::tune to Rotor
	eFrontend::getInstance()->disableRotor();

	Sat = CreateSkinnedComboBox("Sat", 7, lSat );
	CONNECT(Sat->selchanged, eRotorManual::satChanged );

	Transponder = CreateSkinnedComboBox("Transponder", 5, lTransponder );
	CONNECT(Transponder->selchanged, eRotorManual::tpChanged);

	Direction = CreateSkinnedButton("Direction");

	CONNECT( CreateSkinnedButton("Exit")->selected, eRotorManual::reject );

	Save = CreateSkinnedButton("Save");
	CONNECT(Save->selected, eRotorManual::onButtonPressed );

	CONNECT( CreateSkinnedButton("Search")->selected, eRotorManual::onScanPressed );

	status = new eFEStatusWidget( this, eFrontend::getInstance() );
	status->setName("Status");

	BuildSkin("RotorManual");

	Direction->setText("<    Stop    >");

	eTransponderList::getInstance()->reloadNetworks();

	new eListBoxEntryText(*Mode, _("position"), (void*) 0, 0, _("store new sat positions"));
	new eListBoxEntryText(*Mode, _("drive to stored pos"), (void*) 1, 0, _("drive to stored position"));
	new eListBoxEntryText(*Mode, _("drive to satellite"), (void*) 8, 0, _("drive to stored satellite"));
	new eListBoxEntryText(*Mode, _("drive to 0\xC2\xB0"), (void*) 2, 0, _("drives to 0\xB0"));
	new eListBoxEntryText(*Mode, _("recalculate"), (void*) 3, 0, _("recalculate stored positions rel. to current pos"));
	new eListBoxEntryText(*Mode, _("set east limit"), (void*) 4, 0, _("set east soft limit"));
	new eListBoxEntryText(*Mode, _("set west limit"), (void*) 5, 0, _("set west soft limit"));
	new eListBoxEntryText(*Mode, _("disable limits"), (void*) 6, 0, _("disable soft limits"));
	new eListBoxEntryText(*Mode, _("enable limits"), (void*) 7, 0, _("enable soft limits"));
	Mode->setCurrent(0,true);

	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
  
	if (sapi && sapi->transponder)
		transponder = sapi->transponder;

	eListBoxEntryText *sel=0;

	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin() ); it != eTransponderList::getInstance()->getLNBs().end(); it++)
		if ( it->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 )
			for ( ePtrList<eSatellite>::iterator s ( it->getSatelliteList().begin() ); s != it->getSatelliteList().end(); s++)
				if ( transponder && s->getOrbitalPosition() == transponder->satellite.orbital_position )
					sel = new eListBoxEntryText(*Sat, s->getDescription().c_str(), (void*) *s);
				else
					new eListBoxEntryText(*Sat, s->getDescription().c_str(), (void*) *s);

	if ( sel )
		Sat->setCurrent(sel,true);
	else
		Sat->setCurrent(0,true);

	CONNECT( retuneTimer->timeout, eRotorManual::retune );
	addActionMap(&i_rotorMenuActions->map);

	CONNECT(num->selected, eRotorManual::nextfield);
}

void eRotorManual::modeChanged( eListBoxEntryText *e)
{
 eString buttonText, helpText;

	switch((int)e->getKey())
	{
		default:
		case 0:
			helptext=_("store current pos in motor");
			buttonText=_("store");
		break;
		case 1:
			helptext=_("drive motor to stored pos");
			buttonText=_("go");
		break;
		case 2:
			helptext=_("drive motor to reference position");
			buttonText=_("go");
		break;
		case 3:
			helptext=_("recalculate all stored positions");
			buttonText=_("recalc");
		break;
		case 4:
			helptext=_("store current pos as east soft limit");
			buttonText=_("store");
		break;
		case 5:
			helptext=_("store current pos as west soft limit");
			buttonText=_("store");
		break;
		case 6:
			helptext=_("disable soft limits");
			buttonText=_("do it");
		break;
		case 7:
			helptext=_("enable soft limits");
			buttonText=_("do it");
		break;
		case 8:
			helptext=_("drive motor to satellite");
			buttonText=_("go");
		break;
	}
	Save->setHelpText(helptext);
	Save->setText(buttonText);

	switch((int)e->getKey())
	{
		default:

		case 0: // store new positions
		case 4: // set east limit
		case 5: // set west limit
/*			lRecalcParams->hide();
			num1->hide();
			num2->hide();
			num3->hide();*/
		case 3: // recalculate
			num->hide();
		break;

		case 6: // clear limits
		case 7: // set limits
		case 2: // goto ref pos
/*			lRecalcParams->hide();
			num1->hide();
			num2->hide();
			num3->hide();*/
			num->hide();
		case 1: // go to stored pos
			lSat->hide();
			Sat->hide();
		case 8: // goto satellite
			lTransponder->hide();
			Transponder->hide();
			Direction->hide();
			lDirection->hide();
		break;
	}

	switch((int)e->getKey())
	{
		default:
		case 3: // recalculate
/*			lRecalcParams->show();
			num1->show();
			num2->show();
			num3->show();*/
		case 0: // store new positions
		case 4: // set east limit
		case 5: // set west limit
			lTransponder->show();
			Transponder->show();
			Direction->show();
			lDirection->show();
		case 8: // goto sat pos
			num->hide();
			lSat->setText(_("Satellite:"));
			lSat->show();
			Sat->show();
		break;

		case 1: // go to stored pos
			lSat->show();
			lSat->setText("Position:");
			num->show();
		case 6: // clear limits
		case 7: // set limits
		case 2: // goto ref pos
		break;
	}
}

void eRotorManual::retune()
{
	if( transponder && !eFrontend::getInstance()->Locked() )
		transponder->tune();
}

void eRotorManual::satChanged( eListBoxEntryText *sat)
{
	Transponder->clear();
	if (sat && sat->getKey())
	{
		eListBoxEntryText *sel=0;

		eSatellite *Sat = (eSatellite*) (sat->getKey());

		for ( std::list<tpPacket>::const_iterator i( eTransponderList::getInstance()->getNetworks().begin() ); i != eTransponderList::getInstance()->getNetworks().end(); i++ )
		{
			if ( i->orbital_position == Sat->getOrbitalPosition() )
			{
				for (std::list<eTransponder>::const_iterator it( i->possibleTransponders.begin() ); it != i->possibleTransponders.end(); it++)
					if ( transponder && *transponder == *it )
						sel = new eListBoxEntryText( *Transponder, eString().sprintf("%d / %d / %c", it->satellite.frequency/1000, it->satellite.symbol_rate/1000, it->satellite.polarisation?'V':'H' ), (void*)&(*it) );
					else
						new eListBoxEntryText( *Transponder, eString().sprintf("%d / %d / %c", it->satellite.frequency/1000, it->satellite.symbol_rate/1000, it->satellite.polarisation?'V':'H' ), (void*)&(*it) );
			}
		}

		if (Transponder->getCount())
		{
			if ( sel )
				Transponder->setCurrent(sel,true);
			else
				Transponder->setCurrent(0,true);
		}
	}
}

void eRotorManual::tpChanged( eListBoxEntryText *tp )
{
	if (tp && tp->getKey() )
	{
		if ( transponder && *transponder == *((eTransponder*)tp->getKey()))
			return;
		transponder = (eTransponder*)(tp->getKey());
		transponder->tune();
	}
	else
		transponder = 0;
}

void eRotorManual::onButtonPressed()
{
	switch((int)Mode->getCurrent()->getKey())
	{
		default:
		case 0: // store current pos in Rotor
		{
			eStoreWindow w( lnb, ((eSatellite*) Sat->getCurrent()->getKey())->getOrbitalPosition() );
			hide();
			w.show();
			int ret = w.exec();
			if (ret && ret != -1)
			{
				eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6A, eString().sprintf("%02x",ret) );
				changed=1;
			}
			w.hide();
			show();
		}
		break;
		case 1: //drive rotor to stored pos
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6B, eString().sprintf("%02x",num->getNumber()).c_str() );
		break;
		case 2: //driver rotor to reference position
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6B, "00");
		break;
		case 3: //recalculate all stored positions
		{
			eMessageBox mb( _("Wrong use of this function can corrupt all stored sat positions.\n"
				"Are you sure you want to use this function?"), _("Warning"), eMessageBox::iconWarning|eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo );
			hide();
			mb.show();
			switch( mb.exec() )
			{
				case eMessageBox::btYes:
					eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6F, "00");
					mb.hide();
					show();
					break;
				case eMessageBox::btNo:
					mb.hide();
					show();
				return;
				break;
			}
		}
		break;
		case 4: //store current pos as east soft limit
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x66 );
		break;
		case 5: //store current pos as west soft limit
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x67 );
		break;
		case 6: //disable soft limits
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x63 );
		break;
		case 7: //enable soft limits
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6A, "00");
		break;
		case 8: //goto sat pos
			eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x6B, eString().sprintf("%02x", lnb->getDiSEqC().RotorTable[ ((eSatellite*)Sat->getCurrent()->getKey())->getOrbitalPosition()] ) );
		break;
	}
	retune();
}

void eRotorManual::onScanPressed()
{
#ifndef DISABLE_LCD
	TransponderScan setup(LCDTitle, LCDElement);
#else
	TransponderScan setup;
#endif
	hide();
	setup.exec();
	show();
}

int eRotorManual::eventHandler( const eWidgetEvent& e)
{
	switch (e.type)
	{
		case eWidgetEvent::execDone:
			// enable send DiSEqC Commands to Rotor on eTransponder::tune
			eFrontend::getInstance()->enableRotor();
		break;

		case eWidgetEvent::evtAction:
			if ( focus == Direction )
			{
				if (e.action == &i_rotorMenuActions->eastFine)
				{
					if (!running)
					{
						Direction->setText(_("one step East..."));
						eDebug("east Fine");
						eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x68, "FF" );
					}
				}
				else if (e.action == &i_rotorMenuActions->westFine)
				{
					if (!running)
					{
						Direction->setText(_("one step west..."));
						eDebug("west fine");
						eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x69, "FF" );
					}
				}
				else if (e.action == &i_rotorMenuActions->east)
				{
					if ( !running )
					{
						running=true;
						eDebug("east");
						eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x68, "00" );
						retuneTimer->start(500, false);
					}
					Direction->setText(_("turning to east..."));
				}
				else if (e.action == &i_rotorMenuActions->west)
				{
					if ( !running )
					{
						running=true;
						eDebug("west");
						eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x69, "00" );
						retuneTimer->start(500, false);
					}
					Direction->setText(_("turning to west..."));
				}
				else if (e.action == &i_rotorMenuActions->eastStop || e.action == &i_rotorMenuActions->westStop )
				{
					if (running)
					{
							eDebug("send stop");
							eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x60 );
							eFrontend::getInstance()->sendDiSEqCCmd( 0x31, 0x60 );
							running=false;
							retuneTimer->stop();
					}
					retune();
					Direction->setText(_("<    Stop    >"));
				}
				else
					break;
			}
			else
				break;
		return 1;
		
		default:
		break;
	}
	return eWindow::eventHandler(e);	
}

eRotorManual::~eRotorManual()
{
	if (retuneTimer)
		delete retuneTimer;
}

void eRotorManual::nextfield(int*)
{
	eDebug("focusDirNext");
	focusNext(eWidget::focusDirNext);
}


eStoreWindow::eStoreWindow(eLNB *lnb, int orbital_pos)
	:lnb(lnb), orbital_pos(orbital_pos)
{
	init_eStoreWindow();
}
void eStoreWindow::init_eStoreWindow()
{

	StorageLoc = CreateSkinnedNumberWithLabel("StorageLoc",0, 1, 1, 80, 2, 0, 0, "lStorageLoc" );
	CONNECT(StorageLoc->selected, eStoreWindow::nextfield );
	CONNECT( CreateSkinnedButton("store")->selected, eStoreWindow::onStorePressed );
	BuildSkin("eStoreWindow");
	int i = 0;
	while (1)
	{
		i++;
		std::map<int,int>::iterator it(lnb->getDiSEqC().RotorTable.begin());
		for ( ; it != lnb->getDiSEqC().RotorTable.end(); it++ )
			if ( it->second == i )
				break;

		if (it != lnb->getDiSEqC().RotorTable.end() )
		{
			if ( it->first != orbital_pos )
				continue;
		}

		StorageLoc->setNumber(i);
		break;
	}
}

void eStoreWindow::nextfield(int*)
{
	focusNext(eWidget::focusDirNext);
}

void eStoreWindow::onStorePressed()
{
	std::map<int,int>::iterator it = lnb->getDiSEqC().RotorTable.find( orbital_pos );
	if ( it != lnb->getDiSEqC().RotorTable.end() )
	{
		int ret = eMessageBox::btYes;
		if ( StorageLoc->getNumber() != it->second )
		{
			hide();
			ret = eMessageBox::ShowBox( eString().sprintf(_("%d.%d\xC2\xB0%c is currently stored at location %d!\nWhen you store this now at Location %d, we must remove the old Location.\nAre you sure you want to do this?"),abs(orbital_pos)/10, abs(orbital_pos)%10, orbital_pos>0?'E':'W', it->second, StorageLoc->getNumber() ), _("Warning"), eMessageBox::iconWarning|eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo );
			show();
		}
		switch( ret )
		{
			case eMessageBox::btYes:
				lnb->getDiSEqC().RotorTable.erase(it);
				lnb->getDiSEqC().useGotoXX=0;
				lnb->getDiSEqC().RotorTable[orbital_pos] = StorageLoc->getNumber();
				close(StorageLoc->getNumber());
			break;
			case eMessageBox::btNo:
			default:
				break;
		}
	}
	else
	{
		eMessageBox mb( eString().sprintf(_("Store %d.%d\xC2\xB0%c at location %d.\n"
			"If you want another location, then say no and change the location manually.\n"
			"Are you sure you want to store at this location?"),abs(orbital_pos)/10, abs(orbital_pos)%10, orbital_pos>0?'E':'W', StorageLoc->getNumber() ), _("Information"), eMessageBox::iconWarning|eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo );
		hide();
		mb.show();
		switch( mb.exec() )
		{
			case eMessageBox::btYes:
				lnb->getDiSEqC().useGotoXX=0;
				lnb->getDiSEqC().RotorTable[orbital_pos] = StorageLoc->getNumber();
				mb.hide();
				show();
				close(StorageLoc->getNumber());
			break;
			default:
			case eMessageBox::btNo:
				mb.hide();
				show();
				return;
			break;
		}
	}
}
