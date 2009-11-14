
#include <lib/dvb/epgcache.h>
#include <setup_epg.h>
#include <sselect.h>


eEPGSetup::eEPGSetup(): eWindow()
{
	init_eEPGSetup();
}
void eEPGSetup::init_eEPGSetup()
{
	tb_path=CreateSkinnedTextInputField("path","/hdd/","/extras/epgcachepath" );

	bt_seldir=CreateSkinnedButton("seldir");

	tb_file = CreateSkinnedTextInputField("file","epg.dat","/extras/epgfile" );

	cb_cachebouquets=CreateSkinnedCheckbox("cachebouquets",1,"/extras/cachebouquets");

	cb_infobarcache=CreateSkinnedCheckbox("infobarcache",0,"/ezap/osd/useEPGCache");

	CONNECT(CreateSkinnedButton("clear")->selected, eEPGSetup::clearCache);
	CONNECT(CreateSkinnedButton("store")->selected, eEPGSetup::storePressed);


	BuildSkin("EPGSetup");

	CONNECT(bt_seldir->selected, eEPGSetup::selectDir);

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
