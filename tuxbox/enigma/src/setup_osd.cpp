#include "setup_osd.h"

#include <core/gui/echeckbox.h>
#include <core/gui/eskin.h>
#include <core/system/econfig.h>
#include <core/base/i18n.h>

eZapOsdSetup::eZapOsdSetup(): eWindow(0)
{
	setText("On Screen Display Setup");
	move(ePoint(150, 136));
	resize(eSize(400, 280));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	int state=0;
	eConfig::getInstance()->getKey("/ezap/osd/showOSDOnEITUpdate", state);

	showOSDOnEITUpdate = new eCheckbox(this, state, fd);
	showOSDOnEITUpdate->setText(_("Show OSD on EIT Update"));
	showOSDOnEITUpdate->move(ePoint(20, 25));
	showOSDOnEITUpdate->resize(eSize(fd+4+300, fd+4));
	showOSDOnEITUpdate->setHelpText(_("shows OSD when now/next info is changed"));

	state=0;
	eConfig::getInstance()->getKey("/ezap/osd/showConsoleOnFB", state);
	showConsoleOnFB = new eCheckbox(this, state, fd);
	showConsoleOnFB->setText(_("Show Console on Framebuffer"));
	showConsoleOnFB->move(ePoint(20, 75));
	showConsoleOnFB->resize(eSize(fd+4+300, fd+4));
	showConsoleOnFB->setHelpText(_("shows the linux console on TV"));

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->move(ePoint(20, 135));
	ok->resize(eSize(90, fd+4));
	ok->setHelpText(_("close window and save changes"));
	ok->loadDeco();
	
	CONNECT(ok->selected, eZapOsdSetup::okPressed);

	abort=new eButton(this);
	abort->setText(_("abort"));
	abort->move(ePoint(140, 135));
	abort->resize(eSize(100, fd+4));
	abort->setHelpText(_("close window (no changes are saved)"));
	abort->loadDeco();

	CONNECT(abort->selected, eZapOsdSetup::abortPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-30 ) );
	statusbar->resize( eSize( clientrect.width(), 30) );
	statusbar->loadDeco();
}

eZapOsdSetup::~eZapOsdSetup()
{
}

void eZapOsdSetup::fieldSelected(int *number)
{
	focusNext(eWidget::focusDirNext);
}

void eZapOsdSetup::okPressed()
{
	eConfig::getInstance()->setKey("/ezap/osd/showOSDOnEITUpdate", showOSDOnEITUpdate->isChecked());
	eConfig::getInstance()->setKey("/ezap/osd/showConsoleOnFB", showConsoleOnFB->isChecked());
	close(1);
}

void eZapOsdSetup::abortPressed()
{
	close(0);
}
