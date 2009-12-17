#include <parentallock.h>

#include <enigma.h>
#include <enigma_main.h>
#include <sselect.h>
#include <lib/base/i18n.h>
#include <lib/gui/emessage.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/enumber.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/eskin.h>
#include <lib/system/econfig.h>

ParentalLockWindow::ParentalLockWindow( const char* windowText, int curNum )
:eWindow(0)
{
	init_ParentalLockWindow(windowText,curNum);
}
void ParentalLockWindow::init_ParentalLockWindow(const char* windowText, int curNum )
{
	setText(windowText);

	lPin = CreateSkinnedLabel("lPin");

	nPin=CreateSkinnedNumber("nPin",curNum, 4, 0, 9, 1, 0, 0, lPin, 1);
	nPin->setFlags( eNumber::flagHideInput );
	CONNECT( nPin->selected, ParentalLockWindow::numEntered );

	BuildSkin("ParentalLockWindow");
}

void ParentalLockWindow::numEntered(int *i)
{
	close( nPin->getNumber() );
}

eParentalSetup::eParentalSetup():
	eWindow(0)
{
	init_eParentalSetup();
}
void eParentalSetup::init_eParentalSetup()
{

	loadSettings();

	parentallock =CreateSkinnedCheckbox("parentallock",sparentallock);
	CONNECT(parentallock->checked, eParentalSetup::plockChecked );

	changeParentalPin = CreateSkinnedButton("changeParentalPin");
	CONNECT(changeParentalPin->selected_id, eParentalSetup::changePin );
	if ( !sparentallock )
		changeParentalPin->hide();

	setuplock =CreateSkinnedCheckbox("setuplock",ssetuplock);
	CONNECT(setuplock->checked, eParentalSetup::slockChecked );

	changeSetupPin = CreateSkinnedButton("changeSetupPin");
	CONNECT(changeSetupPin->selected_id, eParentalSetup::changePin );
	if ( !ssetuplock )
		changeSetupPin->hide();

	hidelocked =CreateSkinnedCheckbox("hidelocked",shidelocked);
	CONNECT(hidelocked->checked, eParentalSetup::hidelockChecked );
	if ( !sparentallock )
		hidelocked->hide();

	CONNECT(CreateSkinnedButton("store")->selected, eParentalSetup::okPressed);

	BuildSkin("eParentalSetup");

	setHelpID(93);
}

void eParentalSetup::plockChecked(int i)
{
	if ( i && !changeParentalPin->isVisible() )
	{
		changeParentalPin->show();
		hidelocked->show();
	}
	else
	{
		if ( checkPin( parentalpin, _("parental") ) )
		{
			parentalpin=0;
			changeParentalPin->hide();
			hidelocked->hide();
		}
		else
		{
			hidelocked->show();
			parentallock->setCheck(1);
		}
	}
}

void eParentalSetup::slockChecked(int i)
{
	if ( i && !changeSetupPin->isVisible() )
		changeSetupPin->show();
	else
	{
		if ( checkPin( setuppin, _("setup") ) )
		{
			setuppin=0;
			changeSetupPin->hide();
		}
		else
			setuplock->setCheck(1);
	}
}

void eParentalSetup::changePin(eButton *p)
{
	const char *text = ( p == changeParentalPin ) ? _("parental") : _("setup");

	int oldpin = (p == changeParentalPin) ? parentalpin : setuppin;

	int ret = 0;

	if ( oldpin )  // let enter the oldpin.. and validate
	{
		if ( !checkPin( oldpin, text ) )
			return;
	}

	int newPin=0;
	do
	{
		{
			ParentalLockWindow w(eString().sprintf(_("New %s PIN"),text).c_str(), 0 );
			w.show();
			newPin = w.exec();
			w.hide();
			if ( newPin == -1 ) // cancel pressed
				return;
		}
		ParentalLockWindow w(eString().sprintf(_("Reenter %s PIN"),text).c_str(), 0 );
		w.show();
		ret = w.exec();
		w.hide();
		if ( ret == -1 ) // cancel pressed
			return;
		else if ( ret != newPin )
		{
			int ret = eMessageBox::ShowBox(_("The PINs are not equal!\n\nDo you want to retry?"),
					_("PIN validation failed"),
					eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion,
					eMessageBox::btYes );
			if ( ret == eMessageBox::btNo || ret == -1 )
				return;
		}
	}
	while ( newPin != ret );

	if ( p == changeParentalPin )
		parentalpin = newPin;
	else
		setuppin = newPin;

	eMessageBox::ShowBox(_("PIN change completed"),
		_("PIN changed"),
		eMessageBox::btOK|eMessageBox::iconInfo,
		eMessageBox::btOK );
}

void eParentalSetup::hidelockChecked(int i)
{
	shidelocked = i;
}

void eParentalSetup::loadSettings()
{
	parentalpin = eConfig::getInstance()->getParentalPin();

	sparentallock = parentalpin != 0;

	if (eConfig::getInstance()->getKey("/elitedvb/pins/setuplock", setuppin))
		setuppin=0;

	if (eConfig::getInstance()->getKey("/elitedvb/hidelocked", shidelocked ))
		shidelocked=0;

	ssetuplock = setuppin != 0;
}

void eParentalSetup::saveSettings()
{
	eConfig::getInstance()->setKey("/elitedvb/pins/setuplock", setuppin);
	eConfig::getInstance()->setKey("/elitedvb/hidelocked", shidelocked);
	eConfig::getInstance()->setParentalPin(parentalpin);
	eConfig::getInstance()->flush();
	eZap::getInstance()->getServiceSelector()->actualize();
}

eParentalSetup::~eParentalSetup()
{
}

void eParentalSetup::okPressed()
{
	saveSettings();
	close(1);
}

bool checkPin( int pin, const char * text )
{
	if ( !pin )
		return true;
	ParentalLockWindow w(eString().sprintf(_("current %s PIN"),text).c_str(), 0 );
	int ret=0;
	do
	{
		w.show();
		ret = w.exec();
		w.hide();
		if ( ret == -1 )  // cancel pressed
			return false;
		else if ( ret != pin )
		{
			ret = eMessageBox::ShowBox(_("The entered PIN is incorrect.\nDo you want to retry?"),
					_("PIN validation failed"),
					eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion,
					eMessageBox::btYes );
			if ( ret == eMessageBox::btNo || ret == -1 )
				return false;
		}
	}
	while ( ret != pin );
	return true;
}
