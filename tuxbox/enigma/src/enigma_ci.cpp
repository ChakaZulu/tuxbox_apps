#include <enigma_ci.h>

#include <lib/base/i18n.h>

#include <lib/driver/rc.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/eskin.h>
#include <lib/system/econfig.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/dvbci.h>

enigmaCI::enigmaCI(): eWindow(0)
{
	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	DVBCI=eDVB::getInstance()->DVBCI;

	setText(_("Common Interface Module"));
	move(ePoint(150, 136));
	resize(eSize(350, 300));

	reset=new eButton(this);
	reset->setText(_("Reset"));
	reset->move(ePoint(10, 13));
	reset->resize(eSize(330, fd+10));
	reset->setHelpText(_("reset the common interface module"));
	reset->loadDeco();

	CONNECT(reset->selected, enigmaCI::resetPressed);		

	init=new eButton(this);
	init->setText(_("Init"));
	init->move(ePoint(10, 53));
	init->resize(eSize(330, fd+10));
	init->setHelpText(_("send the ca-pmt to ci"));
	init->loadDeco();

	CONNECT(init->selected, enigmaCI::initPressed);		

	app=new eButton(this);
	app->setText(_("waiting for module"));
	app->move(ePoint(10, 93));
	app->resize(eSize(330, fd+10));
	app->setHelpText(_("enter ci menu (mmi)"));
	app->loadDeco();

	CONNECT(app->selected, enigmaCI::appPressed);		

	ok=new eButton(this);
	ok->setText(_("ok"));
	ok->move(ePoint(20, 150));
	ok->resize(eSize(90, fd+4));
	ok->setHelpText(_("leave common interface menu"));
	ok->loadDeco();

	CONNECT(ok->selected, enigmaCI::okPressed);		

	status = new eStatusBar(this);	
	status->move( ePoint(0, clientrect.height()-30) );
	status->resize( eSize( clientrect.width(), 30) );
	status->loadDeco();
	
	CONNECT(DVBCI->ci_progress, enigmaCI::updateCIinfo);		
		
}

enigmaCI::~enigmaCI()
{
	if (status)
		delete status;
}

void enigmaCI::updateCIinfo(const char *buffer)
{
	eDebug("new info %s",buffer);
	app->setText(_(buffer));

}

void enigmaCI::okPressed()
{
	close(1);
}

void enigmaCI::resetPressed()
{
	app->setText(_("resetting....please wait"));
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::reset));
}

void enigmaCI::initPressed()
{
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::init));
}

void enigmaCI::appPressed()
{
}

