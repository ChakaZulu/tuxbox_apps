#include <apps/enigma/enigma.h>
#include <apps/enigma/enigma_mainmenu.h>
#include <apps/enigma/enigma_setup.h>
#include <apps/enigma/enigma_plugins.h>
#include <apps/enigma/enigma_info.h>
#include <apps/enigma/enigma_lcd.h>
#include <apps/enigma/enigma_vcr.h>

#include <core/gui/eskin.h>
#include <core/driver/eavswitch.h>
#include <core/gui/elabel.h>
#include <core/dvb/epgcache.h>
#include <core/base/i18n.h>
#include <core/gui/guiactions.h>

void eMainMenu::setActive(int i)
{
	for (int a=0; a<7; a++)
	{
		int c=(i+a+7-3)%7;
		if (a != 3)
			label[a]->setPixmap(pixmaps[c][0]);	// unselected
		else
			label[a]->setPixmap(pixmaps[c][1]); // selected
	}
	
	switch (i)
	{
	case 0:
		description->setText(_("audiovisuell."));
		break;
	case 1:
		description->setText(_("audio."));
		break;
	case 2:
		description->setText(_("informell."));
		break;
	case 3:
		description->setText(_("genug."));
		break;
	case 4:
		description->setText(_("anders."));
		break;
	case 5:
		description->setText(_("abwechslung."));
		break;
	case 6:
		description->setText(_("analog."));
		break;
	}
}

eMainMenu::eMainMenu()
	: eWidget(0, 1)
{
	addActionMap(&i_cursorActions->map);
	eLabel *background=new eLabel(this);
	background->setName("background");

	label[0]=new eLabel(this);
	label[0]->setName("l3");
	label[1]=new eLabel(this);
	label[1]->setName("l2");
	label[2]=new eLabel(this);
	label[2]->setName("l1");
	label[3]=new eLabel(this);
	label[3]->setName("m");
	label[4]=new eLabel(this);
	label[4]->setName("r1");
	label[5]=new eLabel(this);
	label[5]->setName("r2");
	label[6]=new eLabel(this);
	label[6]->setName("r3");
	
	description=new eLabel(this);
	description->setName("description");

	if (eSkin::getActive()->build(this, "eZapMainMenu"))
		eFatal("unable to load main menu");
	
	char *pixmap_name[]={"tv", "radio", "info", "shutdown", "setup", "games", "scart"};

	for (int i=0; i<7; i++)
	{
		pixmaps[i][0]=eSkin::getActive()->queryImage(eString("mainmenu.") + eString(pixmap_name[i]) );
		pixmaps[i][1]=eSkin::getActive()->queryImage(eString("mainmenu.") + eString(pixmap_name[i]) + ".sel" );
		if (!pixmaps[i][0])
			eFatal("error, mainmenu bug, mainmenu.%s not defined", pixmap_name[i]);
		if (!pixmaps[i][1])
			eFatal("error, mainmenu bug, mainmenu.%s.sel not defined", pixmap_name[i]);
	}
	
	setActive(active=0);
}

void eMainMenu::sel_tv()
{
  eZap::getInstance()->setMode(eZap::TV);
	close(0);
}

void eMainMenu::sel_radio()
{
  eZap::getInstance()->setMode(eZap::Radio);
	close(0);
}

void eMainMenu::sel_vcr()
{
	hide();
	eAVSwitch::getInstance()->setInput(1);
	enigmaVCR mb("If you can read this, your scartswitch doesn't work", "VCR");
	mb.show();
	eZapLCD *pLCD=eZapLCD::getInstance();
	pLCD->lcdMenu->hide();
	pLCD->lcdScart->show();
	mb.exec();
	pLCD->lcdScart->hide();
	pLCD->lcdMenu->show();
	mb.hide();
	eAVSwitch::getInstance()->setInput(0);
	show();
}

void eMainMenu::sel_info()
{
	eZapLCD *pLCD=eZapLCD::getInstance();
	eZapInfo info;
	info.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	hide();
	info.show();
	info.exec();
	info.hide();
	show();
}

void eMainMenu::sel_setup()
{
	eZapLCD *pLCD=eZapLCD::getInstance();
	eZapSetup setup;
	setup.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	hide();
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eMainMenu::sel_plugins()
{
	eZapLCD *pLCD=eZapLCD::getInstance();
	eZapPlugins plugins(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	hide();
	plugins.exec();
	show();
}

void eMainMenu::sel_quit()
{
	close(1);
}

int eMainMenu::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_cursorActions->left)
		{
			active+=6;
			active%=7;
			setActive(active);
		} else if (event.action == &i_cursorActions->right)
		{
			active++;
			active%=7;
			setActive(active);
		} else if (event.action == &i_cursorActions->ok)
		{
			switch (active)
			{
			case 0:
				sel_tv();
				break;
			case 1:
				sel_radio();
				break;
			case 2:
				sel_info();
				break;
			case 3:
				sel_quit();
				break;
			case 4:
				sel_setup();
				break;
			case 5:
				sel_plugins();
				break;
			case 6:
				sel_vcr();
				break;
			}
		} else if (event.action == &i_cursorActions->cancel)
			close(0);
		else
			break;
		return 1;
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

void eMainMenu::eraseBackground(gPainter *, const eRect &where)
{
}
