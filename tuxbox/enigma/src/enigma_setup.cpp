#include "enigma_setup.h"
#include "setupnetwork.h"
#include "setupvideo.h"
#include "elistbox.h"
#include "ewindow.h"
#include "edvb.h"
#include "eskin.h"

eZapSetup::eZapSetup( eWidget *lcdTitle, eWidget *lcdElement)
:eWidget(0,0, lcdTitle, lcdElement)
{
	window=new eLBWindow("SETUP", eListbox::tBorder, 8, eSkin::getActive()->queryValue("fontsize", 20), 220, LCDTitle, LCDElement);
	window->move(QPoint(150, 136));
	connect(new eListboxEntryText(window->list, "[Zurück]"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_close(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Bouquets..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_bouquet(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Network..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_network(eListboxEntry*)));
//	connect(new eListboxEntryText(window->list, "Audio..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_sound(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Video..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_video(eListboxEntry*)));
}

int eZapSetup::exec()
{
	window->show();
	int res=window->exec();
	window->hide();
	return res;
}

eZapSetup::~eZapSetup()
{
	delete window;
}

void eZapSetup::sel_close(eListboxEntry *lbe)
{
	window->close(0);
}

void eZapSetup::sel_bouquet(eListboxEntry *lbe)
{
	eDVB::getInstance()->sortInChannels();
}

void eZapSetup::sel_network(eListboxEntry *lbe)
{
	eZapNetworkSetup setup(LCDTitle, LCDElement);
	window->hide();
	setup.show();
	setup.exec();
	setup.hide();
	window->show();
}

void eZapSetup::sel_sound(eListboxEntry *lbe)
{
}

void eZapSetup::sel_video(eListboxEntry *lbe)
{
	eZapVideoSetup setup(LCDTitle, LCDElement);
	window->hide();
	setup.show();
	setup.exec();
	setup.hide();
	window->show();
}

