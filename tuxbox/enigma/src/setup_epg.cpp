
#include <lib/dvb/epgcache.h>
#include <setup_epg.h>
#include <sselect.h>

eEPGSetup::eEPGSetup(): eWindow(0)
{
	init_eEPGSetup();
}
void eEPGSetup::init_eEPGSetup()
{
	eString path = eString("/hdd/");
	char* p = 0;
	eConfig::getInstance()->getKey("/extras/epgcachepath", p);
	if (p) path = eString(p);
	tb_path=new eTextInputField(this);tb_path->setName("path");
	tb_path->setText(path);

	bt_seldir=new eButton(this); bt_seldir->setName("seldir");

	eString file = eString("epg.dat");
	p = 0;
	eConfig::getInstance()->getKey("/extras/epgfile", p);
	if (p) file = eString(p);
	tb_file=new eTextInputField(this);tb_file->setName("file");
	tb_file->setText(file);

	int cachebouquets = 1;
	eConfig::getInstance()->getKey("/extras/cachebouquets", cachebouquets );
	cb_cachebouquets=new eCheckbox(this);cb_cachebouquets->setName("cachebouquets");
	cb_cachebouquets->setCheck(cachebouquets);

	int infobarchache = 0;
	eConfig::getInstance()->getKey("/ezap/osd/useEPGCache", infobarchache );
	cb_infobarcache=new eCheckbox(this);cb_infobarcache->setName("infobarcache");
	cb_infobarcache->setCheck(infobarchache);

	bt_clear = new eButton(this);bt_clear->setName("clear");
	bt_store=new eButton(this); bt_store->setName("store");


	statusbar = new eStatusBar(this); statusbar->setName("statusbar");

	if (eSkin::getActive()->build(this, "EPGSetup"))
		eFatal("skin load of \"EPGSetup\" failed");

	CONNECT(bt_seldir->selected, eEPGSetup::selectDir);
	CONNECT(bt_clear->selected, eEPGSetup::clearCache);
	CONNECT(bt_store->selected, eEPGSetup::storePressed);

}
void eEPGSetup::selectDir()
{
	eFileSelector sel(tb_path->getText());
#ifndef DISABLE_LCD
	sel.setLCD(LCDTitle, LCDElement);
#endif
	hide();

	const eServiceReference *ref = sel.choose(-1);

	if (ref)
		tb_path->setText(sel.getPath().current().path);
	show();
	setFocus(bt_seldir);
}


void eEPGSetup::clearCache()
{
	eEPGCache::getInstance()->ClearCache();
	eMessageBox::ShowBox((_("Cache has been cleared!")), _("EPG settings"), eMessageBox::iconInfo|eMessageBox::btOK);

}

void eEPGSetup::storePressed()
{
	eConfig::getInstance()->setKey("/extras/cachebouquets", cb_cachebouquets->isChecked()?255:0 );
	eConfig::getInstance()->setKey("/ezap/osd/useEPGCache", cb_infobarcache->isChecked()?255:0 );

	eString startPath =tb_path->getText();
	if (startPath.empty() || startPath[startPath.length() -1] != '/')
		startPath+= "/";
	eConfig::getInstance()->setKey("/extras/epgcachepath", startPath.c_str() );

	eString file = tb_file->getText();
	eConfig::getInstance()->setKey("/extras/epgfile", file.c_str() );

	close(0);
}

eEPGSetup::~eEPGSetup()
{
}
