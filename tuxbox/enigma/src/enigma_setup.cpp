#include "enigma_setup.h"
#include "setupnetwork.h"
#include "setupvideo.h"
#include "elistbox.h"
#include "ewindow.h"
#include "edvb.h"
#include "eskin.h"
#include "elabel.h"
#include "satconfig.h"

eZapSetup::eZapSetup()
	:eLBWindow("Setup", eListbox::tBorder, 8, eSkin::getActive()->queryValue("fontsize", 20), 220)
{
	move(ePoint(150, 136));
	CONNECT((new eListboxEntryText(list, "[Zurück]"))->selected, eZapSetup::sel_close);
	CONNECT((new eListboxEntryText(list, "Bouquets..."))->selected, eZapSetup::sel_bouquet);
	CONNECT((new eListboxEntryText(list, "Network..."))->selected, eZapSetup::sel_network);
//	CONNECT((list, "Audio..."))->selected, sel_sound);
	CONNECT((new eListboxEntryText(list, "Video..."))->selected, eZapSetup::sel_video);
	CONNECT((new eListboxEntryText(list, "Satelliten..."))->selected, eZapSetup::sel_satconfig);
/*	connect(new eListboxEntryText(list, "[Zurück]"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_close(eListboxEntry*)));
	connect(new eListboxEntryText(list, "Bouquets..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_bouquet(eListboxEntry*)));
	connect(new eListboxEntryText(list, "Network..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_network(eListboxEntry*)));
//	connect(new eListboxEntryText(list, "Audio..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_sound(eListboxEntry*)));
	connect(new eListboxEntryText(list, "Video..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_video(eListboxEntry*)));
	connect(new eListboxEntryText(list, "Satelliten..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_satconfig()));*/

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


void eZapSetup::sel_satconfig(eListboxEntry *lbe)
{
	eSatelliteConfigurationManager satconfig;
	hide();
	satconfig.show();
	satconfig.exec();
	satconfig.hide();
	show();
}
