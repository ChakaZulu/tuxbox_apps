#include "setup_osd.h"

#include <core/gdi/fb.h>
#include <core/gui/echeckbox.h>
#include <core/gui/eskin.h>
#include <core/system/econfig.h>
#include <core/base/i18n.h>

eZapOsdSetup::eZapOsdSetup(): eWindow(0)
{
	setText("OSD Setup");
	move(ePoint(150, 136));
	resize(eSize(400, 250));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	int state=0;
	eConfig::getInstance()->getKey("/ezap/osd/showOSDOnEITUpdate", state);

	showOSDOnEITUpdate = new eCheckbox(this, state, fd);
	showOSDOnEITUpdate->setText(_("Show OSD on EIT Update"));
	showOSDOnEITUpdate->move(ePoint(20, 25));
	showOSDOnEITUpdate->resize(eSize(fd+4+300, fd+4));

	state=0;
	eConfig::getInstance()->getKey("/ezap/osd/showConsoleOnFB", state);
	showConsoleOnFB = new eCheckbox(this, state, fd);
	showConsoleOnFB->setText(_("Show Console on Framebuffer"));
	showConsoleOnFB->move(ePoint(20, 75));
	showConsoleOnFB->resize(eSize(fd+4+300, fd+4));

	ok=new eButton(this);
	ok->setText(_("[SAVE]"));
	ok->move(ePoint(20, 125));
	ok->resize(eSize(90, fd+4));
	
	CONNECT(ok->selected, eZapOsdSetup::okPressed);

	abort=new eButton(this);
	abort->setText(_("[ABORT]"));
	abort->move(ePoint(140, 125));
	abort->resize(eSize(100, fd+4));

	CONNECT(abort->selected, eZapOsdSetup::abortPressed);
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
