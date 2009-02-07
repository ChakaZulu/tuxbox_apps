#ifndef DISABLE_HDD
#ifndef DISABLE_FILE
/*
 * setup_harddisk.cpp
 *
 * Copyright (C) 2002 Felix Domke <tmbinc@tuxbox.org>
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
 * $Id: setup_harddisk.cpp,v 1.27 2009/02/07 10:06:31 dbluelle Exp $
 */

#include <setup_harddisk.h>
#include <enigma.h>
#include <enigma_main.h>
#include <lib/gui/emessage.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/combobox.h>
#include <lib/gui/emessage.h>
#include <lib/gui/statusbar.h>
#include <lib/dvb/epgcache.h>
#include <lib/system/info.h>
#include <sys/vfs.h> // for statfs
#include <unistd.h>
#include <signal.h>

static int getCapacity(int dev)
{
	int c='a'+dev;
	
	FILE *f=fopen(eString().sprintf("/proc/ide/hd%c/capacity", c).c_str(), "r");
	if (!f)
		return -1;
	int capacity=-1;
	fscanf(f, "%d", &capacity);
	fclose(f);
	return capacity;
}

static eString getModel(int dev)
{
	int c='a'+dev;
	char line[1024];

	FILE *f=fopen(eString().sprintf("/proc/ide/hd%c/model", c).c_str(), "r");
	if (!f)
		return "";
	*line=0;
	fgets(line, 1024, f);
	fclose(f);
	if (!*line)
		return "";
	line[strlen(line)-1]=0;
	return line;
}

int freeDiskspace(int dev, eString mp="")
{
	FILE *f=fopen("/proc/mounts", "rb");
	if (!f)
		return -1;
	eString path;
	int host=dev/4;
	int bus=!!(dev&2);
	int target=!!(dev&1);
	path.sprintf("/dev/ide/host%d/bus%d/target%d/lun0/", host, bus, target);

	while (1)
	{
		char line[1024];
		if (!fgets(line, 1024, f))
			break;
		if (!strncmp(line, path.c_str(), path.size()))
		{
			eString mountpoint=line;
			mountpoint=mountpoint.mid(mountpoint.find(' ')+1);
			mountpoint=mountpoint.left(mountpoint.find(' '));
			//eDebug("mountpoint: %s", mountpoint.c_str());
			if ( mp && mountpoint != mp )
				return -1;
			struct statfs s;
			int free;
			if (statfs(mountpoint.c_str(), &s)<0)
				free=-1;
			else
				free=s.f_bfree/1000*s.f_bsize/1000;
			fclose(f);
			return free;
		}
	}
	fclose(f);
	return -1;
}

static int numPartitions(int dev)
{
	eString path;
	int host=dev/4;
	int bus=!!(dev&2);
	int target=!!(dev&1);

	path.sprintf("ls /dev/ide/host%d/bus%d/target%d/lun0/ > /tmp/tmp.out", host, bus, target);
	system( path.c_str() );

	FILE *f=fopen("/tmp/tmp.out", "rb");
	if (!f)
	{
		eDebug("fopen failed");
		return -1;
	}

	int numpart=-1;		// account for "disc"
	while (1)
	{
		char line[1024];
		if (!fgets(line, 1024, f))
			break;
		if ( !strncmp(line, "disc", 4) )
			numpart++;
		if ( !strncmp(line, "part", 4) )
			numpart++;
	}
	fclose(f);
	system("rm /tmp/tmp.out");
	return numpart;
}

extern eString resolvSymlinks(const eString &path);

eString getPartFS(int dev, eString mp="")
{
	FILE *f=fopen("/proc/mounts", "rb");
	if (!f)
		return "";
	eString path;
	int host=dev/4;
	int bus=!!(dev&2);
	int target=!!(dev&1);
	path.sprintf("/dev/ide/host%d/bus%d/target%d/lun0/", host, bus, target);

	eString tmp=resolvSymlinks(mp);

	while (1)
	{
		char line[1024];
		if (!fgets(line, 1024, f))
			break;

		if (!strncmp(line, path.c_str(), path.size()))
		{
			eString mountpoint=line;
			mountpoint=mountpoint.mid(mountpoint.find(' ')+1);
			mountpoint=mountpoint.left(mountpoint.find(' '));
//			eDebug("mountpoint: %s", mountpoint.c_str());
			if ( tmp && mountpoint != tmp )
				continue;

			eString fs=line;
			fs=fs.mid(fs.find(' ')+1);
			fs=fs.mid(fs.find(' ')+1);
			fs=fs.left(fs.find(' '));
			eString mpath=line;
			mpath=mpath.left(mpath.find(' '));
			mpath=mpath.mid(mpath.rfind('/')+1);
			fclose(f);
			return fs+','+mpath;
		}
	}
	fclose(f);
	return "";
}

eHarddiskSetup::eHarddiskSetup()
: eListBoxWindow<eListBoxEntryText>(_("Harddisk Setup"), 5, 420)
{
	init_eHarddiskSetup();
}
void eHarddiskSetup::init_eHarddiskSetup()
{
	nr=0;
	
	move(ePoint(150, 136));
	
	for (int host=0; host<2; host++)
		for (int bus=0; bus<2; bus++)
			for (int target=0; target<2; target++)
			{
				int num=target+bus*2+host*4;
				
				int c='a'+num;
				
							// check for presence
				char line[1024];
				int ok=1;
				FILE *f=fopen(eString().sprintf("/proc/ide/hd%c/media", c).c_str(), "r");
				if (!f)
					continue;
				if ((!fgets(line, 1024, f)) || strcmp(line, "disk\n"))
					ok=0;
				fclose(f);

				if (ok)
				{
					int capacity=getCapacity(num);
					if (capacity < 0)
						continue;
						
					capacity=capacity/1000*512/1000;

					eString sharddisks;
					sharddisks=getModel(num);
					sharddisks+=" (";
					if (c&1)
						sharddisks+="master";
					else
						sharddisks+="slave";
					if (capacity)
						sharddisks+=eString().sprintf(", %d.%03d GB", capacity/1024, capacity%1024);
					sharddisks+=")";
					
					nr++;
					
					new eListBoxEntryText(&list, sharddisks, (void*)num);
				}
	}
	
	CONNECT(list.selected, eHarddiskSetup::selectedHarddisk);
}

void eHarddiskSetup::selectedHarddisk(eListBoxEntryText *t)
{
	if ((!t) || (((int)t->getKey())==-1))
	{
		close(0);
		return;
	}
	int dev=(int)t->getKey();
	
	eHarddiskMenu menu(dev);
	
	hide();
	menu.show();
	menu.exec();
	menu.hide();
	show();
}

void eHarddiskMenu::check()
{
	hide();
	ePartitionCheck check(dev);
	check.show();
	check.exec();
	check.hide();
	show();
	restartNet=true;
}

void eHarddiskMenu::extPressed()
{
	if ( visible )
	{
		gPixmap *pm = eSkin::getActive()->queryImage("arrow_down");
		if (pm)
			ext->setPixmap( pm );
		fs->hide();
		lbltimeout->hide();
		lblacoustic->hide();
		timeout->hide();
		acoustic->hide();
		store->hide();
		sbar->hide();
		resize( getSize()-eSize( 0, 80) );
		sbar->move( sbar->getPosition()-ePoint(0,80) );
		sbar->show();
		eZap::getInstance()->getDesktop(eZap::desktopFB)->invalidate( eRect( getAbsolutePosition()+ePoint( 0, height() ), eSize( width(), 80 ) ));
		visible=0;
	}
	else
	{
		gPixmap *pm = eSkin::getActive()->queryImage("arrow_up");
		if (pm)
			ext->setPixmap( pm );
		sbar->hide();
		sbar->move( sbar->getPosition()+ePoint(0,80) );
		resize( getSize()+eSize( 0, 80) );
		sbar->show();
		fs->show();
		lbltimeout->show();
		lblacoustic->show();
		timeout->show();
		acoustic->show();
		store->show();
		
		visible=1;
	}
}

void eHarddiskMenu::s_format()
{
	hide();
	do
	{
		{
			int res = eMessageBox::ShowBox(
				 _("Are you SURE that you want to format this disk?\n"),
				 _("formatting harddisk..."),
				 eMessageBox::btYes|eMessageBox::btCancel, eMessageBox::btCancel);
			if (res != eMessageBox::btYes)
				break;
		}
		if (numpart)
		{
			int res = eMessageBox::ShowBox(
				 _("There's data on this harddisk.\n"
				 "You will lose that data. Proceed?"),
				 _("formatting harddisk..."),
				 eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo);
			if (res != eMessageBox::btYes)
				break;
		}
		int host=dev/4;
		int bus=!!(dev&2);
		int target=!!(dev&1);

// kill samba server... (exporting /hdd)
#ifdef HAVE_DREAMBOX_HARDWARE
		system("killall -9 smbd");
#else
		system("killall -9 smbd nmbd");
		system("/bin/umount /hdd");
#endif
		restartNet=true;

		system(
				eString().sprintf(
#ifdef HAVE_DREAMBOX_HARDWARE
				"/bin/umount /dev/ide/host%d/bus%d/target%d/lun0/part*", host, bus, target).c_str());
#else
				"/sbin/swapoff /dev/ide/host%d/bus%d/target%d/lun0/part1", host, bus, target).c_str());
#endif
		eMessageBox msg(
			_("please wait while initializing harddisk.\nThis might take some minutes.\n"),
			_("formatting harddisk..."), 0);
		msg.show();

		FILE *f=popen(
				eString().sprintf(
				"/sbin/sfdisk -f -uM /dev/ide/host%d/bus%d/target%d/lun0/disc", host, bus, target).c_str(), "w");
		if (!f)
		{
			eMessageBox msg(
				_("sorry, couldn't find sfdisk utility to partition harddisk."),
				_("formatting harddisk..."),
				 eMessageBox::btOK|eMessageBox::iconError);
			msg.show();
			msg.exec();
			msg.hide();
			break;
		}
#ifdef HAVE_DREAMBOX_HARDWARE
		fprintf(f, "0,\n;\n;\n;\ny\n");
#else
		fprintf(f, "0,100,82\n,,,*\n;\n;\ny\n");
#endif
		fclose(f);
/*Set up Swapspace*/
#ifndef HAVE_DREAMBOX_HARDWARE
		system(eString().sprintf("/sbin/mkswap /dev/ide/host%d/bus%d/target%d/lun0/part1", host, bus, target).c_str());
#endif

#if ENABLE_REISERFS
		if ( !fs->getCurrent()->getKey() )  // reiserfs
		{
			::sync();
			if ( system( eString().sprintf(
#ifdef HAVE_DREAMBOX_HARDWARE
					"/sbin/mkreiserfs -f -f /dev/ide/host%d/bus%d/target%d/lun0/part1", host, bus, target).c_str())>>8)
#else
					"/sbin/mkreiserfs -f -f /dev/ide/host%d/bus%d/target%d/lun0/part2", host, bus, target).c_str())>>8)
#endif
				goto err;
			::sync();
			if ( system( eString().sprintf(
#ifdef HAVE_DREAMBOX_HARDWARE
					"/bin/mount -t reiserfs /dev/ide/host%d/bus%d/target%d/lun0/part1 /hdd", host, bus, target).c_str())>>8)
#else
					"/bin/mount -t reiserfs /dev/ide/host%d/bus%d/target%d/lun0/part2 /hdd", host, bus, target).c_str())>>8)
#endif
				goto err;
			::sync();
			if ( system("mkdir /hdd/movie")>>8 )
				goto err;
			::sync();
			goto noerr;
		}
		else  // ext3
#endif
		{
			::sync();
			if ( system( eString().sprintf(
#ifdef HAVE_DREAMBOX_HARDWARE
					"/sbin/mkfs.ext3 -T largefile -m0 /dev/ide/host%d/bus%d/target%d/lun0/part1", host, bus, target).c_str())>>8)
#else
					"/sbin/mkfs.ext3 -T largefile -m0 /dev/ide/host%d/bus%d/target%d/lun0/part2", host, bus, target).c_str())>>8)
#endif
				goto err;
			::sync();
			if ( system(eString().sprintf(
#ifdef HAVE_DREAMBOX_HARDWARE
					"/bin/mount -t ext3 /dev/ide/host%d/bus%d/target%d/lun0/part1 /hdd", host, bus, target).c_str())>>8)
#else
					"/bin/mount -t ext3 /dev/ide/host%d/bus%d/target%d/lun0/part2 /hdd", host, bus, target).c_str())>>8)
#endif
				goto err;
			::sync();
			if ( system("mkdir /hdd/movie")>>8 )
				goto err;
			::sync();
			goto noerr;
		}
err:
		{
			eMessageBox::ShowBox(
				_("creating filesystem failed."),
				_("formatting harddisk..."),
				 eMessageBox::btOK|eMessageBox::iconError);
			break;
		}
noerr:
		{
			eZapMain::getInstance()->clearRecordings();
			eMessageBox::ShowBox(
				_("successfully formatted your disk!"),
				_("formatting harddisk..."),
				 eMessageBox::btOK|eMessageBox::iconInfo);
		}
		readStatus();
	} while (0);
	show();
}

void eHarddiskMenu::readStatus()
{
	if (!(dev & 1))
		bus->setText("master");
	else
		bus->setText("slave");

	eString mod=getModel(dev);
	setText(mod);
	model->setText(mod);
	int cap=getCapacity(dev)/1000*512/1000;
	
	if (cap != -1)
		capacity->setText(eString().sprintf("%d.%03d GB", cap/1024, cap%1024));
		
	numpart=numPartitions(dev);
	int fds;
	
	if (numpart == -1)
		status->setText(_("(error reading information)"));
	else if (!numpart)
		status->setText(_("uninitialized - format it to use!"));
	else if ((fds=freeDiskspace(dev)) != -1)
		status->setText(eString().sprintf(_("in use, %d.%03d GB (~%d minutes) free"), fds/1024, fds%1024, fds/33 ));
	else
		status->setText(_("initialized, but unknown filesystem"));
}
// Function to store settings
void eHarddiskMenu::storevalues()
{
	eConfig::getInstance()->setKey("/extras/hdparm-s", timeout->getNumber()*12);
	eConfig::getInstance()->setKey("/extras/hdparm-m", acoustic->getNumber());

	eMessageBox::ShowBox(_("The settings have been saved successfully"), _("Harddisk"),eMessageBox::btOK);
}

// Function to send HDD to standby immediately
void eHarddiskMenu::hddstandby()
{
	system(eString().sprintf("/sbin/hdparm -y /dev/ide/host0/bus0/target0/lun0/disc").c_str());
} 

eHarddiskMenu::eHarddiskMenu(int dev): dev(dev), restartNet(false)
{
	init_eHarddiskMenu();
}
void eHarddiskMenu::init_eHarddiskMenu()
{
	visible=0;
	status=new eLabel(this); status->setName("status");
	model=new eLabel(this); model->setName("model");
	capacity=new eLabel(this); capacity->setName("capacity");
	bus=new eLabel(this); bus->setName("bus");
	
	standby=new eButton(this); standby->setName("standby");
	format=new eButton(this); format->setName("format");
	bcheck=new eButton(this); bcheck->setName("check");
	ext=new eButton(this); ext->setName("ext");

	fs=new eComboBox(this,2); fs->setName("fs"); fs->hide();

	lbltimeout=new eLabel(this); lbltimeout->setName("lbltimeout");lbltimeout->hide();
	lblacoustic=new eLabel(this); lblacoustic->setName("lblacoustic");lblacoustic->hide();
	timeout=new eNumber(this,1,0, 20, 3, 0, 0); timeout->setName("timeout");timeout->hide();
	acoustic=new eNumber(this,1,0,254, 3, 0, 0); acoustic->setName("acoustic");acoustic->hide();
	store=new eButton(this); store->setName("store");store->hide();


	sbar = new eStatusBar(this); sbar->setName("statusbar");

	new eListBoxEntryText( *fs, ("ext3"), (void*) 1 );
#ifdef ENABLE_REISERFS
	new eListBoxEntryText( *fs, ("reiserfs"), (void*) 0 );
#endif
	fs->setCurrent((void*)1);
  
	if (eSkin::getActive()->build(this, "eHarddiskMenu"))
		eFatal("skin load of \"eHarddiskMenu\" failed");

	gPixmap *pm = eSkin::getActive()->queryImage("arrow_down");
	if (pm)
	{
		eSize s = ext->getSize();
		ext->setPixmap( pm );
		ext->setPixmapPosition( ePoint(s.width()/2 - pm->x/2, s.height()/2 - pm->y/2) );
	}

	readStatus();

	int hddstandby = 60;
	if( (eConfig::getInstance()->getKey("/extras/hdparm-s", hddstandby)) )
		timeout->setNumber(hddstandby/12);
	else
		timeout->setNumber(hddstandby/12);

	int hddacoustic=128;
	if( (eConfig::getInstance()->getKey("/extras/hdparm-m", hddacoustic)) )
		acoustic->setNumber(hddacoustic);
	else
		acoustic->setNumber(hddacoustic);

	CONNECT(ext->selected, eHarddiskMenu::extPressed);
	CONNECT(format->selected, eHarddiskMenu::s_format);
	CONNECT(bcheck->selected, eHarddiskMenu::check);
	CONNECT(standby->selected, eHarddiskMenu::hddstandby);
	CONNECT(store->selected, eHarddiskMenu::storevalues);
}

ePartitionCheck::ePartitionCheck( int dev )
:eWindow(1), dev(dev), fsck(0)
{
	void init_ePartitionCheck();
}
void ePartitionCheck::init_ePartitionCheck()
{
	lState = new eLabel(this);
	lState->setName("state");
	bClose = new eButton(this);
	bClose->setName("close");
	CONNECT( bClose->selected, ePartitionCheck::accept );
	if (eSkin::getActive()->build(this, "ePartitionCheck"))
		eFatal("skin load of \"ePartitionCheck\" failed");
	bClose->hide();
}

int ePartitionCheck::eventHandler( const eWidgetEvent &e )
{
	switch(e.type)
	{
		case eWidgetEvent::execBegin:
		{
			system("killall nmbd smbd");
			eEPGCache::getInstance()->messages.send(eEPGCache::Message(eEPGCache::Message::pause));
			eEPGCache::getInstance()->messages.send(eEPGCache::Message(eEPGCache::Message::flush));
			eString fs = getPartFS(dev,"/hdd"),
							part = fs.mid( fs.find(",")+1 );

			fs = fs.left( fs.find(",") );

			eDebug("part = %s, fs = %s", part.c_str(), fs.c_str() );

			int host=dev/4;
			int bus=!!(dev&2);
			int target=!!(dev&1);

			// kill samba server... (exporting /hdd)
			system("killall -9 smbd");

			if ( system("/bin/umount /hdd") >> 8)
			{
				eMessageBox msg(
				_("could not unmount the filesystem... "),
				_("check filesystem..."),
				 eMessageBox::btOK|eMessageBox::iconError);
				close(-1);
			}
			if ( fs == "ext3" )
			{
				eWindow::globalCancel(eWindow::OFF);
				fsck = new eConsoleAppContainer( eString().sprintf("/sbin/fsck.ext3 -f -y /dev/ide/host%d/bus%d/target%d/lun0/%s", host, bus, target, part.c_str()) );

				if ( !fsck->running() )
				{
					eMessageBox::ShowBox(
						_("sorry, couldn't find fsck.ext3 utility to check the ext3 filesystem."),
						_("check filesystem..."),
						eMessageBox::btOK|eMessageBox::iconError);
					close(-1);
				}
				else
				{
					eDebug("fsck.ext3 opened");
					CONNECT( fsck->dataAvail, ePartitionCheck::getData );
					CONNECT( fsck->appClosed, ePartitionCheck::fsckClosed );
				}
			}
			else if ( fs == "ext2" )
			{
				eWindow::globalCancel(eWindow::OFF);
				fsck = new eConsoleAppContainer( eString().sprintf("/sbin/fsck.ext2 -f -y /dev/ide/host%d/bus%d/target%d/lun0/%s", host, bus, target, part.c_str()) );

				if ( !fsck->running() )
				{
					eMessageBox::ShowBox(
						_("sorry, couldn't find fsck.ext2 utility to check the ext2 filesystem."),
						_("check filesystem..."),
						eMessageBox::btOK|eMessageBox::iconError);
					close(-1);
				}
				else
				{
					eDebug("fsck.ext2 opened");
					CONNECT( fsck->dataAvail, ePartitionCheck::getData );
					CONNECT( fsck->appClosed, ePartitionCheck::fsckClosed );
				}
			}
			else if ( fs == "reiserfs" && eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000 )
			{
				eWindow::globalCancel(eWindow::OFF);
				fsck = new eConsoleAppContainer( eString().sprintf("/sbin/reiserfsck -y --fix-fixable /dev/ide/host%d/bus%d/target%d/lun0/%s", host, bus, target, part.c_str()) );

				if ( !fsck->running() )
				{
					eMessageBox::ShowBox(
						_("sorry, couldn't find reiserfsck utility to check the reiserfs filesystem."),
						_("check filesystem..."),
						eMessageBox::btOK|eMessageBox::iconError);
					close(-1);
				}
				else
				{
					eDebug("reiserfsck opened");
					CONNECT( fsck->dataAvail, ePartitionCheck::getData );
					CONNECT( fsck->appClosed, ePartitionCheck::fsckClosed );
					fsck->write("Yes\n",4);
				}
			}
			else
			{
				eMessageBox::ShowBox(
					_("not supported filesystem for check."),
					_("check filesystem..."),
					eMessageBox::btOK|eMessageBox::iconError);
				close(-1);
			}
		}
		break;

		case eWidgetEvent::execDone:
			eWindow::globalCancel(eWindow::ON);
			if (fsck)
				delete fsck;
			eEPGCache::getInstance()->messages.send(eEPGCache::Message(eEPGCache::Message::restart));
			eDVB::getInstance()->restartSamba();
		break;

		default:
			return eWindow::eventHandler( e );
	}
	return 1;	
}

void ePartitionCheck::onCancel()
{
	if (fsck)
		fsck->kill();
}

void ePartitionCheck::fsckClosed(int state)
{
	int host=dev/4;
	int bus=!!(dev&2);
	int target=!!(dev&1);

#ifdef HAVE_DREAMBOX_HARDWARE
	if ( system( eString().sprintf("/bin/mount /dev/ide/host%d/bus%d/target%d/lun0/part1 /hdd", host, bus, target).c_str() ) >> 8 )
		eDebug("mount hdd after check failed");
#else
	if ( system( eString().sprintf("/bin/mount /dev/ide/host%d/bus%d/target%d/lun0/part2 /hdd", host, bus, target).c_str() ) >> 8 )
		eDebug("mount hdd after check failed");
#endif

	if (fsck)
	{
		delete fsck;
		fsck=0;
	}

	bClose->show();
}

void ePartitionCheck::getData( eString str )
{
	str.removeChars('\x8');
	if ( str.find("<y>") != eString::npos )
		fsck->write("y",1);
	else if ( str.find("[N/Yes]") != eString::npos )
		fsck->write("Yes",3);
	eString tmp = lState->getText();
	tmp+=str;

	eSize size=lState->getSize();
	int height = size.height();
	size.setHeight(height*2);
	eLabel l(this);
	l.hide();
	l.resize(size);
	l.setText(tmp);
	if ( l.getExtend().height() > height )
		tmp=str;

	lState->setText(tmp);
}

#endif // DISABLE_FILE
#endif // DISABLE_HDD
