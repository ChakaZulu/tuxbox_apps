#include "enigma_setup.h"
#include "setupaudio.h"
#include "setupnetwork.h"
#include "setupvideo.h"
#include "setup_osd.h"
#include "elistbox.h"
#include "ewindow.h"
#include "edvb.h"
#include "enigma.h"

eZapSetup::eZapSetup()
{
	window=new eLBWindow("SETUP", eListbox::tBorder, 8, eZap::FontSize, 220);
	window->move(QPoint(150, 136));
	connect(new eListboxEntryText(window->list, "[Zurück]"), SIGNAL(selected(eListboxEntry*)), SLOT(sel_close(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Bouquets..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_bouquet(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Network..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_network(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Audio..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_sound(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "Video..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_video(eListboxEntry*)));
	connect(new eListboxEntryText(window->list, "OSD..."), SIGNAL(selected(eListboxEntry*)), SLOT(sel_osd(eListboxEntry*)));
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
	eZapNetworkSetup setup;
	window->hide();
	setup.show();
	setup.exec();
	setup.hide();
	window->show();
}

void eZapSetup::sel_sound(eListboxEntry *lbe)
{
	eZapAudioSetup setup;
	window->hide();
	setup.show();
	setup.exec();
	setup.hide();
	window->show();
}

void eZapSetup::sel_video(eListboxEntry *lbe)
{
	eZapVideoSetup setup;
	window->hide();
	setup.show();
	setup.exec();
	setup.hide();
	window->show();
}

void eZapSetup::sel_osd(eListboxEntry *lbe)
{
	eZapOsdSetup setup;
	window->hide();
	setup.show();
	setup.exec();
	setup.hide();
	window->show();
}

