/*
 * enigma_info.cpp
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
 * $Id: enigma_info.cpp,v 1.33 2006/07/15 13:22:48 ghostrider Exp $
 */

#include <enigma_info.h>
#include <unistd.h>

#include <streaminfo.h>

#include <lib/base/i18n.h>
#include <lib/dvb/service.h>
#include <lib/dvb/frontend.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/gui/ebutton.h>
#include <lib/system/info.h>
#include <lib/system/dmfp.h>

eZapInfo::eZapInfo()
	:eListBoxWindow<eListBoxEntryMenu>(_("Infos"), 7, 320)
{
	move(ePoint(150, 166));
	CONNECT((new eListBoxEntryMenu(&list, _("Streaminfo"), _("open the Streaminfo")))->selected, eZapInfo::sel_streaminfo);
	CONNECT((new eListBoxEntryMenu(&list, _("About..."), _("open the about dialog")))->selected, eZapInfo::sel_about);
}

eZapInfo::~eZapInfo()
{
}

void eZapInfo::sel_streaminfo()
{
	hide();	
	eStreaminfo si(0, eServiceInterface::getInstance()->service);
#ifndef DISABLE_LCD
	si.setLCD(LCDTitle, LCDElement);
#endif
	si.show();
	si.exec();
	si.hide();
	show();
}

static eString getVersionInfo(const char *info)
{
	FILE *f=fopen("/.version", "rt");
	if (!f)
		f = fopen("/etc/image-version", "rt");
	if (!f)
		return "";
	eString result;
	while (1)
	{
		char buffer[128];
		if (!fgets(buffer, 128, f))
			break;
		if (strlen(buffer))
			buffer[strlen(buffer)-1]=0;
		if ((!strncmp(buffer, info, strlen(info)) && (buffer[strlen(info)]=='=')))
		{
			int i = strlen(info)+1;
			result = eString(buffer).mid(i, strlen(buffer)-i);
			break;
		}
	}	
	fclose(f);
	return result;
}

class eAboutScreen: public eWindow
{
	eLabel *machine, *processor, *frontend, *harddisks, *vendor, *logo, *version, *dreamlogo, *triaxlogo, *fpversion;
	eButton *okButton;

public:
	eAboutScreen()
	{
		setHelpID(43);

		machine=new eLabel(this);
		machine->setName("machine");

		vendor=new eLabel(this);
		vendor->setName("vendor");

		processor=new eLabel(this);
		processor->setName("processor");

		frontend=new eLabel(this);
		frontend->setName("frontend");

		harddisks=new eLabel(this);
		harddisks->setName("harddisks");

		okButton=new eButton(this);
		okButton->setName("okButton");

		dreamlogo=new eLabel(this);
		dreamlogo->setName("dreamlogo");

		triaxlogo=new eLabel(this);
		triaxlogo->setName("triaxlogo");

		version=new eLabel(this);
		version->setName("version");

		fpversion=new eLabel(this);
		fpversion->setName("fp_version");

		if (eSkin::getActive()->build(this, "eAboutScreen"))
			eFatal("skin load of \"eAboutScreen\" failed");

		dreamlogo->hide();
		triaxlogo->hide();
		
		if ( !eSystemInfo::getInstance()->hasHDD() )
		{
			harddisks->hide();
			eWidget *h=search("harddisk_label");
			if(h)
				h->hide();
		}

		machine->setText(eSystemInfo::getInstance()->getModel());
		vendor->setText(eSystemInfo::getInstance()->getManufacturer());
		processor->setText(eString().sprintf("Processor: %s", eSystemInfo::getInstance()->getCPUInfo()));

		switch (eSystemInfo::getInstance()->getFEType())
		{
			case eSystemInfo::feSatellite:
				frontend->setText(_("Frontend: Satellite"));
				break;
			case eSystemInfo::feCable:
				frontend->setText(_("Frontend: Cable"));
				break;
			case eSystemInfo::feTerrestrial:
				frontend->setText(_("Frontend: Terrestrial"));
				break;
			default:
				frontend->setText(_("Frontend: Unknown"));
		}

		eString sharddisks;
#ifndef DISABLE_FILE
		if ( eSystemInfo::getInstance()->hasHDD() )
		{
			for (int c='a'; c<'h'; c++)
			{
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
					FILE *f=fopen(eString().sprintf("/proc/ide/hd%c/model", c).c_str(), "r");
					if (!f)
						continue;
					*line=0;
					fgets(line, 1024, f);
					fclose(f);
					if (!*line)
						continue;
					line[strlen(line)-1]=0;
					sharddisks+=line;
					f=fopen(eString().sprintf("/proc/ide/hd%c/capacity", c).c_str(), "r");
					if (!f)
						continue;
					int capacity=0;
					fscanf(f, "%d", &capacity);
					fclose(f);
					sharddisks+=" (";
					if (c&1)
						sharddisks+="master";
					else
						sharddisks+="slave";
					if (capacity)
						sharddisks+=eString().sprintf(", %d MB", capacity/2048);
					sharddisks+=")\n";
				}
			}
		}
#endif //DISABLE_FILE
		if (sharddisks == "")
			sharddisks=_("none");
		harddisks->setText(sharddisks);

		{
			eString verid=getVersionInfo("version");
			if (!verid)
				version->setText(_("unknown"));
			else
			{
				int type=atoi(verid.left(1).c_str());
				char *typea[3];
				typea[0]=_("release");
				typea[1]=_("beta");
				typea[2]=_("internal");
				eString ver=verid.mid(1, 3);
				eString date=verid.mid(4, 8);
//				eString time=verid.mid(12, 4);
				if ( eSystemInfo::getInstance()->getHwType() >= eSystemInfo::DM7000 )
					version->setText(
						eString(typea[type%3]) + eString(" ") + ver[0] + "." + ver[1] + "." + ver[2]
							+ ", " + date.mid(6, 2) + "." + date.mid(4,2) + "." + date.left(4));
				else
					version->setText(
														eString().sprintf
															("%s %c.%d. %s", typea[type%3], ver[0],
															atoi( eString().sprintf("%c%c",ver[1],ver[2]).c_str()	),
															(date.mid(6, 2) + "." + date.mid(4,2) + "." + date.left(4)).c_str()
															)
													);
			}
		}

		if ( !strcmp(eSystemInfo::getInstance()->getManufacturer(),"Triax") )
			triaxlogo->show();
		else if ( !strcmp(eSystemInfo::getInstance()->getManufacturer(),"Dream-Multimedia-TV") )
			dreamlogo->show();

		if ( eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7000
			|| eSystemInfo::getInstance()->getHwType() == eSystemInfo::DM7020)
		{
			eString fp_version = fpversion->getText();
			fp_version += eString().sprintf(" 1.%02d", eDreamboxFP::getFPVersion());
			eDebug("%s", fp_version.c_str());
			fpversion->setText(fp_version);
		} 
		else
			fpversion->hide();

		CONNECT(okButton->selected, eWidget::accept);
	}
};

void eZapInfo::sel_about()
{
	hide();
	eAboutScreen about;
	about.show();
	about.exec();
	about.hide();
	show();
}
