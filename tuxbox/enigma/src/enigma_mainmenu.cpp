#include "enigma_mainmenu.h"
#include "eskin.h"
#include "eavswitch.h"
#include "emessage.h"
#include "enigma_setup.h"
#include "enigma_plugins.h"
#include "enigma_info.h"
#include "enigma_lcd.h"
#include "elabel.h"
#include "epgcache.h"

#include <core/base/i18n.h>

eMainMenu::eMainMenu()
{
	eZapLCD *pLCD=eZapLCD::getInstance();
	window=new eLBWindow("enigma" , eListbox::tBorder, 12, eSkin::getActive()->queryValue("fontsize", 20), 400);
	window->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	window->move(ePoint(70, 150));
//	CONNECT((new eListboxEntryText(window->list, _("TV mode")))->selected, eMainMenu::sel_close);
	CONNECT((new eListboxEntryText(window->list, _("[back]")))->selected, eMainMenu::sel_close);
	CONNECT((new eListboxEntryText(window->list, _("VCR mode")))->selected, eMainMenu::sel_vcr);
	CONNECT((new eListboxEntryText(window->list, _("Plugins")))->selected, eMainMenu::sel_plugins);	
	CONNECT((new eListboxEntryText(window->list, _("Infos")))->selected, eMainMenu::sel_info);
	CONNECT((new eListboxEntryText(window->list, _("Setup")))->selected, eMainMenu::sel_setup);
	CONNECT((new eListboxEntryText(window->list, _("Shutdown")))->selected, eMainMenu::sel_quit);	
}

int eMainMenu::exec()
{
	window->show();
	int res=window->exec();
	window->hide();
	return res;
}

eMainMenu::~eMainMenu()
{
	delete window;
}

void eMainMenu::sel_close(eListboxEntry *lbe)
{
	window->close(0);
}

void eMainMenu::sel_vcr(eListboxEntry *lbe)
{
	window->hide();
	eAVSwitch::getInstance()->setInput(1);
	eMessageBox mb("If you can read this, your scartswitch doesn't work", "VCR");
	mb.show();
	eZapLCD *pLCD=eZapLCD::getInstance();
	pLCD->lcdMenu->hide();
	pLCD->lcdScart->show();
	mb.exec();
	pLCD->lcdScart->hide();
	pLCD->lcdMenu->show();
	mb.hide();
	eAVSwitch::getInstance()->setInput(0);
	window->show();
}

void eMainMenu::sel_info(eListboxEntry *)
{
	eZapLCD *pLCD=eZapLCD::getInstance();
	eZapInfo info;
	info.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	window->hide();
	info.show();
	info.exec();
	info.hide();
	window->show();
}

void eMainMenu::sel_setup(eListboxEntry *)
{
	eZapLCD *pLCD=eZapLCD::getInstance();
	eZapSetup setup;
	setup.setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	window->hide();
	setup.show();
	setup.exec();
	setup.hide();
	window->show();
}

void eMainMenu::sel_plugins(eListboxEntry *)
{
	eZapLCD *pLCD=eZapLCD::getInstance();
	eZapPlugins plugins(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	window->hide();
	plugins.exec();
	window->show();
}

void eMainMenu::sel_quit(eListboxEntry *)
{
	window->close(1);
}

void eMainMenu::setLCD(eWidget *a, eWidget *b)
{
	window->setLCD(a, b);
}
