#include <enigma.h>
#include <enigma_main.h>
#include <enigma_mainmenu.h>
#include <enigma_setup.h>
#include <enigma_plugins.h>
#include <enigma_info.h>
#include <enigma_lcd.h>
#include <enigma_vcr.h>
#include <timer.h>

#include <lib/gui/eskin.h>
#include <lib/driver/eavswitch.h>
#include <lib/gui/elabel.h>
#include <lib/dvb/epgcache.h>
#include <lib/base/i18n.h>
#include <lib/gui/guiactions.h>

void eMainMenu::setActive(int i)
{
	for (int a=0; a<7; a++)
	{
		int c=(i+a+MENU_ENTRIES-3)%MENU_ENTRIES;
		if (a != 3)
			label[a]->setPixmap(pixmaps[c][0]);	// unselected
		else
			label[a]->setPixmap(pixmaps[c][1]); // selected
	}
	
	switch (i)
	{
	case 0:
		description->setText(_("TV mode"));
		break;
	case 1:
		description->setText(_("Radio mode"));
		break;
	case 2:
		description->setText(_("File mode"));
		break;
	case 3:
		description->setText(_("information"));
		break;
	case 4:
		description->setText(_("shutdown"));
		break;
	case 5:
		description->setText(_("setup"));
		break;
	case 6:
		description->setText(_("games"));
		break;
	case 7:
		description->setText(_("VCR"));
		break;
	case 8:
		description->setText(_("Timer"));
	}

	if (LCDTitle)
		LCDTitle->setText(_("Mainmenu"));

	if (LCDElement)
		LCDElement->setText( description->getText() );
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
	
	char *pixmap_name[]={"tv", "radio", "file", "info", "shutdown", "setup", "games", "scart", "timer"};

	for (int i=0; i<MENU_ENTRIES; i++)
	{
		pixmaps[i][0]=eSkin::getActive()->queryImage(eString("mainmenu.") + eString(pixmap_name[i]) );
		pixmaps[i][1]=eSkin::getActive()->queryImage(eString("mainmenu.") + eString(pixmap_name[i]) + ".sel" );
		if (!pixmaps[i][0])
			eFatal("error, mainmenu bug, mainmenu.%s not defined", pixmap_name[i]);
		if (!pixmaps[i][1])
			eFatal("error, mainmenu bug, mainmenu.%s.sel not defined", pixmap_name[i]);
	}
}

void eMainMenu::sel_tv()
{
  eZapMain::getInstance()->setMode(eZapMain::modeTV, 1);
	close(0);
}

void eMainMenu::sel_radio()
{
  eZapMain::getInstance()->setMode(eZapMain::modeRadio, 1);
	close(0);
}

void eMainMenu::sel_file()
{
  eZapMain::getInstance()->setMode(eZapMain::modeFile, 1);
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

void eMainMenu::sel_timer()
{
	eZapLCD *pLCD=eZapLCD::getInstance();
	hide();
	eTimerView setup;
	setup.setLCD(pLCD->LCDTitle, pLCD->LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
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
	case eWidgetEvent::willShow:
		setActive(active=eZapMain::getInstance()->getRealMode());		
	break;

	case eWidgetEvent::evtAction:
		if (event.action == &i_cursorActions->left)
		{
			active+=MENU_ENTRIES-1;
			active%=MENU_ENTRIES;
			setActive(active);
		} else if (event.action == &i_cursorActions->right)
		{
			active++;
			active%=MENU_ENTRIES;
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
				sel_file();
				break;
			case 3:
				sel_info();
				break;
			case 4:
				sel_quit();
				break;
			case 5:
				sel_setup();
				break;
			case 6:
				sel_plugins();
				break;
			case 7:
				sel_vcr();
				break;
			case 8:
				sel_timer();
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
