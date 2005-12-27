/*
 * setup_extra.cpp
 *
 * Copyright (C) 2003 Andreas Monzner <ghostrider@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: setup_extra.cpp,v 1.60 2005/12/27 12:21:32 pieterg Exp $
 */
#include <enigma.h>
#include <setup_extra.h>
#include <setupengrab.h>
#include <setupnetwork.h>
#include <software_update.h>
#include <setup_rc.h>
#include <lib/dvb/decoder.h>
#include <lib/gui/emessage.h>
#include <lib/system/info.h>

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_stop();
extern "C" void tuxtxt_close();
extern "C" int  tuxtxt_init();
extern "C" void tuxtxt_start(int tpid);
#endif

eExpertSetup::eExpertSetup()
	:eSetupWindow(_("Expert Setup"), 10, 400)
{
	init_eExpertSetup();
}

void eExpertSetup::init_eExpertSetup()
{
	cmove(ePoint(170, 115));

	int lockWebIf=1;
	if ( eConfig::getInstance()->getKey("/ezap/webif/lockWebIf", lockWebIf) )
		eConfig::getInstance()->setKey("/ezap/webif/lockWebIf", lockWebIf);

	int showSatPos=1;
	if ( eConfig::getInstance()->getKey("/extras/showSatPos", showSatPos) )
		eConfig::getInstance()->setKey("/extras/showSatPos", showSatPos);

	int entry=0;
#ifndef DISABLE_NETWORK
	if (eSystemInfo::getInstance()->hasNetwork())
	{
		CONNECT((new eListBoxEntryMenu(&list, _("Communication Setup"), eString().sprintf("(%d) %s", ++entry, _("open communication setup")) ))->selected, eExpertSetup::communication_setup);
		CONNECT((new eListBoxEntryMenu(&list, _("Ngrab Streaming Setup"), eString().sprintf("(%d) %s", ++entry, _("open ngrab server setup")) ))->selected, eExpertSetup::ngrab_setup);
		if ( eSystemInfo::getInstance()->getHwType() != eSystemInfo::DM7020 )  // no update for 7020 yet
			CONNECT((new eListBoxEntryMenu(&list, _("Software Update"), eString().sprintf("(%d) %s", ++entry, _("open software update")) ))->selected, eExpertSetup::software_update);
	}
	int startSamba=1;
	if ( eConfig::getInstance()->getKey("/elitedvb/network/samba", startSamba) )
		eConfig::getInstance()->setKey("/elitedvb/network/samba", startSamba);
#endif
	CONNECT((new eListBoxEntryMenu(&list, _("Remote Control"), eString().sprintf("(%d) %s", ++entry, _("open remote control setup")) ))->selected, eExpertSetup::rc_setup);
	if ( eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000 )
		CONNECT((new eListBoxEntryMenu(&list, _("Factory reset"), eString().sprintf("(%d) %s", ++entry, _("all settings will set to factory defaults")) ))->selected, eExpertSetup::factory_reset);
	new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	list.setFlags(list.getFlags()|eListBoxBase::flagNoPageMovement);
#ifndef DISABLE_FILE
	if ( eSystemInfo::getInstance()->canRecordTS() && !eDVB::getInstance()->recorder )
	{
		record_split_size = new eListBoxEntryMulti( (eListBox<eListBoxEntryMulti>*)&list, _("record split size (left, right)"));
		record_split_size->add("         650MB        >", 650*1024);
		record_split_size->add("<        700MB        >", 700*1024);
		record_split_size->add("<        800MB        >", 800*1024);
		record_split_size->add("<         1GB         >", 1024*1024);
		record_split_size->add("<        1,5GB        >", 1536*1024);
		record_split_size->add("<         2GB         >", 2*1024*1024);
		record_split_size->add("<         4GB         >", 4*1024*1024);
		record_split_size->add("<         8GB         >", 8*1024*1024);
		record_split_size->add("<        16GB         ", 16*1024*1024);
		int splitsize=0;
		if (eConfig::getInstance()->getKey("/extras/record_splitsize", splitsize))
			splitsize=1024*1024; // 1G
		record_split_size->setCurrent(splitsize);
		CONNECT( list.selchanged, eExpertSetup::selChanged );
	}
#endif
	timeout_infobar = new eListBoxEntryMulti( (eListBox<eListBoxEntryMulti>*)&list, _("infobar timeout (left, right)"));
	timeout_infobar->add((eString)"  " + eString().sprintf(_("Infobar timeout %d sec"), 2) + (eString)" >", 2);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 3) + (eString)" >", 3);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 4) + (eString)" >", 4);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 5) + (eString)" >", 5);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 6) + (eString)" >", 6);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 7) + (eString)" >", 7);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 8) + (eString)" >", 8);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 9) + (eString)" >", 9);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 10) + (eString)" >", 10);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 11) + (eString)" >", 11);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 12) + (eString)"  ", 12);
	int timeoutInfobar = 6;
	eConfig::getInstance()->getKey("/enigma/timeoutInfobar", timeoutInfobar);
	timeout_infobar->setCurrent(timeoutInfobar);
	CONNECT( list.selchanged, eExpertSetup::selInfobarChanged );

	CONNECT((new eListBoxEntryCheck((eListBox<eListBoxEntry>*)&list,_("Serviceselector help buttons"),"/ezap/serviceselector/showButtons",_("show colored help buttons in service selector")))->selected, eExpertSetup::colorbuttonsChanged );
	new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Show Sat position"), "/extras/showSatPos", _("show sat position in the infobar"));
	if ( eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000 )
		CONNECT((new eListBoxEntryCheck((eListBox<eListBoxEntry>*)&list,_("Enable fast zapping"),"/elitedvb/extra/fastzapping",_("enables faster zapping.. but with visible sync")))->selected, eExpertSetup::fastZappingChanged );
	new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Skip confirmations"), "/elitedvb/extra/profimode", _("enable/disable confirmations"));
	new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Hide error windows"), "/elitedvb/extra/hideerror", _("don't show zap error messages"));
	new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Auto show Infobar"), "/ezap/osd/showOSDOnEITUpdate", _("always show infobar when new event info is avail"));
	new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Show remaining Time"), "/ezap/osd/showCurrentRemaining", _("show event remaining time in the infobar"));
	CONNECT((new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Use http authentification"), "/ezap/webif/lockWebIf", _("enables the http (user/password) authentification")))->selected, eExpertSetup::reinitializeHTTPServer );
	CONNECT((new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Don't open serial port"), "/ezap/extra/disableSerialOutput", _("don't write debug messages to /dev/tts/0")))->selected, eExpertSetup::reinitializeHTTPServer );
	new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Auto bouquet change"), "/elitedvb/extra/autobouquetchange", _("change into next bouquet when end of current bouquet is reached"));
	new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Auto reconnect cahandler"), "/elitedvb/extra/cahandlerReconnect", _("try to reconnect when an external cahandler connection was lost"));
#ifndef DISABLE_NETWORK
	if ( eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000 ||
	    eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
		new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Enable file sharing"), "/elitedvb/network/samba", _("start file sharing(samba) on startup"));
#endif
#ifndef TUXTXT_CFG_STANDALONE
	CONNECT((new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Disable teletext caching"), "/ezap/extra/teletext_caching", _("don't cache teletext pages in background")))->selected, eExpertSetup::tuxtxtCachingChanged );
#endif
	new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Enable Zapping History"), "/elitedvb/extra/extzapping", _("don't care about actual mode when zapping in history list"));	
	if ( eSystemInfo::getInstance()->getHwType() < eSystemInfo::DM5600 )
		new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Disable Standby"), "/extras/fastshutdown", _("Box goes directly into Deep-Standby"));
#ifdef ENABLE_MHW_EPG
	int mhwepg=1;
	if ( eConfig::getInstance()->getKey("/extras/mhwepg", mhwepg) )
		eConfig::getInstance()->setKey("/extras/mhwepg", mhwepg);
	new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Enable MHW EPG"), "/extras/mhwepg", _("Mediahighway EPG, activate swap space when using with multiple operators"));
#endif
#ifdef HAVE_DREAMBOX_HARDWARE
	if ( eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000 )
	{
		int corefilesDisable = 0;
		if (access("/var/etc/.no_corefiles", R_OK) == 0)
			corefilesDisable = 1;
		eConfig::getInstance()->setKey("/extras/corefiles_disable", corefilesDisable);
		new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
		CONNECT_2_1((new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Disable CoreFiles"), "/extras/corefiles_disable", _("don't create 'Corefiles' after an Enigma crash")))->selected, eExpertSetup::fileToggle,"/var/etc/.no_corefiles");
#ifdef ENABLE_EXPERT_WEBIF
		int dontMountHDD = 0;
		if (access("/var/etc/.dont_mount_hdd", R_OK) == 0)
			dontMountHDD = 1;
		eConfig::getInstance()->setKey("/extras/dont_mount_hdd", dontMountHDD);
		CONNECT_2_1((new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Disable HDD mount"), "/extras/dont_mount_hdd", _("don't mount the HDD via 'rcS'")))->selected, eExpertSetup::fileToggle,"/var/etc/.dont_mount_hdd");
#endif
	}
#endif
#ifndef HAVE_DREAMBOX_HARDWARE
	new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	int bootInfo = 0;
	if (access("/var/etc/.boot_info", R_OK) == 0)
		bootInfo = 1;
	eConfig::getInstance()->setKey("/extras/bootinfo", bootInfo);
	CONNECT_2_1((new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Show Boot-Info"), "/extras/bootinfo", _("Show Boot-Infos (IP, etc.)")))->selected, eExpertSetup::fileToggle,"/var/etc/.boot_info");
	int hwSectionsDisable = 0;
	if (access("/var/etc/.hw_sections", R_OK) == 0)
		hwSectionsDisable = 1;
	eConfig::getInstance()->setKey("/extras/hw_sections_disable", hwSectionsDisable);
	CONNECT_2_1((new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Disable HW_Sections"), "/extras/hw_sections_disable", _("don't use hardware section filtering")))->selected, eExpertSetup::fileToggle,"/var/etc/.hw_sections");
	int watchdogDisable = 0;
	if (access("/var/etc/.no_watchdog", R_OK) == 0)
		watchdogDisable = 1;
	eConfig::getInstance()->setKey("/extras/watchdog_disable", watchdogDisable);
	CONNECT_2_1((new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Disable Watchdog"), "/extras/watchdog_disable", _("don't use the Watchdog")))->selected, eExpertSetup::fileToggle,"/var/etc/.no_watchdog");
	if ( eSystemInfo::getInstance()->getHwType() != eSystemInfo::dbox2Nokia )
	{
		int enxWatchdogDisable = 0;
		if (access("/var/etc/.no_enxwatchdog", R_OK) == 0)
			enxWatchdogDisable = 1;
		eConfig::getInstance()->setKey("/extras/enxwatchdog_disable", enxWatchdogDisable);
		CONNECT_2_1((new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Disable ENX-Watchdog"), "/extras/enxwatchdog_disable", _("don't use the ENX-Watchdog")))->selected, eExpertSetup::fileToggle,"/var/etc/.no_enxwatchdog");
	}
	int sptsMode = 0;
	if (access("/var/etc/.spts_mode", R_OK) == 0)
		sptsMode = 1;
	eConfig::getInstance()->setKey("/extras/spts_mode", sptsMode);
	CONNECT_2_1((new eListBoxEntryCheck( (eListBox<eListBoxEntry>*)&list, _("Enable SPTS-Mode"), "/extras/spts_mode", _("use SPTS-Mode (enables TS-recording)")))->selected, eExpertSetup::fileToggle,"/var/etc/.spts_mode");
#endif
	setHelpID(92);
}

void eExpertSetup::fileToggle(bool newState, const char* filename)
{
	FILE* test;
	test = fopen(filename,"r");
	if (test != NULL)
	{
		fclose(test);
		::unlink(filename);
	}
	else
	{
		eString cmd = "touch ";
		cmd += filename;
		system(cmd.c_str());
	}
}

#ifndef TUXTXT_CFG_STANDALONE
void eExpertSetup::tuxtxtCachingChanged(bool b)
{
	if ( b )
	{
		if (Decoder::current.tpid != -1)
			tuxtxt_stop();
		tuxtxt_close();
	}
	else
	{
		tuxtxt_init();
		if (Decoder::current.tpid != -1)
			tuxtxt_start(Decoder::current.tpid);
	}
}
#endif

#ifndef DISABLE_FILE
void eExpertSetup::selChanged(eListBoxEntryMenu* e)
{
	if ( eSystemInfo::getInstance()->canRecordTS() &&
		e == (eListBoxEntryMenu*)record_split_size )
		eConfig::getInstance()->setKey("/extras/record_splitsize", (int)e->getKey());
}
#endif

void eExpertSetup::selInfobarChanged(eListBoxEntryMenu* e)
{
	if ( e == (eListBoxEntryMenu*)timeout_infobar )
		eConfig::getInstance()->setKey("/enigma/timeoutInfobar", (int)e->getKey());
}

void eExpertSetup::colorbuttonsChanged(bool b)
{
	eServiceSelector *sel = eZap::getInstance()->getServiceSelector();
	sel->setStyle( sel->getStyle(), true );
}

void eExpertSetup::reinitializeHTTPServer(bool)
{
	eZap::getInstance()->reconfigureHTTPServer();
}

void eExpertSetup::fastZappingChanged(bool b)
{
	Decoder::setFastZap(b);
}

#ifndef DISABLE_NETWORK
void eExpertSetup::communication_setup()
{
	hide();
	eZapNetworkSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eExpertSetup::ngrab_setup()
{
	hide();
	ENgrabSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eExpertSetup::software_update()
{
	hide();
	eSoftwareUpdate setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

#endif

void eExpertSetup::rc_setup()
{
	hide();
	eZapRCSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

//implemented in upgrade.cpp
extern bool erase(char mtd[30], const char *titleText);

void eExpertSetup::factory_reset()
{
	hide();
	eMessageBox mb(
		_("When you do a factory reset, you will lose ALL your configuration data\n"
			"(including bouquets, services, satellite data ...)\n"
			"After completion of factory reset, your receiver will restart automatically!\n\n"
			"Really do a factory reset?"),
		_("Factory reset"),
		eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion,
		eMessageBox::btNo );
	mb.show();
	int ret = mb.exec();
	mb.hide();
	if ( ret == eMessageBox::btYes ) 
	{
		switch( eSystemInfo::getInstance()->getHwType() )
		{
			case eSystemInfo::DM7020:
				system("rm -R /etc/enigma && killall -9 enigma");
				break;
			case eSystemInfo::DM7000:
			case eSystemInfo::DM500:
			case eSystemInfo::DM5620:
			case eSystemInfo::DM5600:
			case eSystemInfo::TR_DVB272S:
				erase("/dev/mtd/1", _("Factory reset..."));
				system("reboot");
				break;
			default: 
				eDebug("factory reset not implemented for this hardware!!\n");
		}
	}
	show();
}
