#include "enigma_setup.h"
#include "setupnetwork.h"
#include "setupvideo.h"
#include "elistbox.h"
#include "ewindow.h"
#include "edvb.h"
#include "eskin.h"
#include "elabel.h"

eZapSetup::eZapSetup()
	:eLBWindow("Setup", eListbox::tBorder, 8, eSkin::getActive()->queryValue("fontsize", 20), 220)
{
	move(QPoint(150, 136));
	connect(new eListboxEntryText(list, "[Zurück]"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_close(eListboxEntry*)));
	connect(new eListboxEntryText(list, "Bouquets..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_bouquet(eListboxEntry*)));
	connect(new eListboxEntryText(list, "Network..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_network(eListboxEntry*)));
//	connect(new eListboxEntryText(list, "Audio..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_sound(eListboxEntry*)));
	connect(new eListboxEntryText(list, "Video..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_video(eListboxEntry*)));
}

eZapSetup::~eZapSetup()
{
}

void eZapSetup::sel_close(eListboxEntry *lbe)
{
	close(0);
}

void eZapSetup::sel_bouquet(eListboxEntry *lbe)
{
	eDVB::getInstance()->sortInChannels();
}

void eZapSetup::sel_network(eListboxEntry *lbe)
{
	eZapNetworkSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	hide();
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::sel_sound(eListboxEntry *lbe)
{
}

void eZapSetup::sel_video(eListboxEntry *lbe)
{
	eZapVideoSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	hide();
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

