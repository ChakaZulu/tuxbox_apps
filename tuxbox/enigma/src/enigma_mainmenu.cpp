#include "enigma_mainmenu.h"
#include "eskin.h"
#include "eavswitch.h"
#include "emessage.h"
#include "scan.h"
#include "streaminfo.h"
#include "enigma_setup.h"
#include "enigma_plugins.h"
#include "showbnversion.h"
#include "enigma_lcd.h"
#include "elabel.h"
#include "epgcache.h"
#include "epixmap.h"

eMainMenu::eMainMenu()
{
	eZapLCD *pLCD=eZapLCD::getInstance();
	window=new eLBWindow("enigma 0.1" , eListbox::tBorder, 12, eSkin::getActive()->queryValue("fontsize", 20), 240);
	window->setLCD(pLCD->lcdMenu->Title, pLCD->lcdMenu->Element);
	window->move(ePoint(70, 150));
	CONNECT((new eListboxEntryText(window->list, "TV Mode"))->selected, eMainMenu::sel_close);
	CONNECT((new eListboxEntryText(window->list, "VCR Mode"))->selected, eMainMenu::sel_vcr);
	CONNECT((new eListboxEntryText(window->list, "Transponder Scan"))->selected, eMainMenu::sel_scan);
	CONNECT((new eListboxEntryText(window->list, "Setup"))->selected, eMainMenu::sel_setup);
	CONNECT((new eListboxEntryText(window->list, "Streaminfo"))->selected, eMainMenu::sel_streaminfo);
	CONNECT((new eListboxEntryText(window->list, "Show BN version"))->selected, eMainMenu::sel_bnversion);
	CONNECT((new eListboxEntryText(window->list, "Plugins"))->selected, eMainMenu::sel_plugins);
	CONNECT((new eListboxEntryText(window->list, "Quit enigma"))->selected, eMainMenu::sel_quit);
	CONNECT((new eListboxEntryText(window->list, "About..."))->selected, eMainMenu::sel_about);
/*	connect(new eListboxEntryText(window->list, "TV Mode"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_close(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "VCR Mode"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_vcr(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Transponder Scan"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_scan(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Setup"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_setup(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Streaminfo"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_streaminfo(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Show BN version"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_bnversion(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Plugins"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_plugins(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Quit enigma"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_quit(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "About..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_about(eListboxEntry*)));*/
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

void eMainMenu::sel_scan(eListboxEntry *)
{
	TransponderScan ts;
	window->hide();
	ts.exec();
	window->show();
}

void eMainMenu::sel_streaminfo(eListboxEntry *)
{
	eStreaminfo si;
	window->hide();
	si.show();
	si.exec();
	si.hide();
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

void eMainMenu::sel_bnversion(eListboxEntry *)
{
	ShowBNVersion bn;
	window->hide();
	bn.show();
	bn.exec();
	bn.hide();
	window->show();
}

void eMainMenu::sel_about(eListboxEntry *)
{
	window->hide();
	eMessageBox msgbox(
	"insert non-peinlichen ABOUT text here...",
	"About enigma");
	msgbox.show();
	msgbox.exec();
	msgbox.hide();
	window->show();
};

void eMainMenu::setLCD(eWidget *a, eWidget *b)
{
	window->setLCD(a, b);
}
