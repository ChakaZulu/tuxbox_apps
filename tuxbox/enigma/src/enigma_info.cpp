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
 * $Id: enigma_info.cpp,v 1.8 2002/11/11 21:51:25 tmbinc Exp $
 */

#include <enigma_info.h>

#include <streaminfo.h>
#include <showbnversion.h>

#include <lib/dvb/edvb.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/gui/emessage.h>
#include <lib/gui/ebutton.h>
#include <lib/base/i18n.h>

eZapInfo::eZapInfo()
	:eListBoxWindow<eListBoxEntryMenu>(_("Infos"), 8, 220)
{
	move(ePoint(150, 136));
	CONNECT((new eListBoxEntryMenu(&list, _("[back]")))->selected, eZapInfo::sel_close);
	CONNECT((new eListBoxEntryMenu(&list, _("Streaminfo")))->selected, eZapInfo::sel_streaminfo);
#if 0
	CONNECT((new eListBoxEntryMenu(&list, _("Show BN version")))->selected, eZapInfo::sel_bnversion);
#endif
	CONNECT((new eListBoxEntryMenu(&list, _("About...")))->selected, eZapInfo::sel_about);
	
}

eZapInfo::~eZapInfo()
{
}

void eZapInfo::sel_close()
{
	close(0);
}

void eZapInfo::sel_streaminfo()
{
	hide();	
	eStreaminfo si;
	si.setLCD(LCDTitle, LCDElement);
	si.show();
	si.exec();
	si.hide();
	show();
}

void eZapInfo::sel_bnversion()
{
	hide();	
	ShowBNVersion bn;
	bn.setLCD(LCDTitle, LCDElement);
	bn.show();
	bn.exec();
	bn.hide();
	show();
}

static eString getVersionInfo(const char *info)
{
	FILE *f=fopen("/.version", "rt");
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
	eLabel *machine, *processor, *frontend, *harddisks, *vendor, *dreamlogo, *version;
	eButton *okButton;
public:
	eAboutScreen()
	{
		int mID=atoi( eDVB::getInstance()->getInfo("mID").c_str());
		
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

		version=new eLabel(this);
		version->setName("version");

		if (eSkin::getActive()->build(this, "eAboutScreen"))
			eFatal("skin load of \"eAboutScreen\" failed");
		
		dreamlogo->hide();
		
		switch (mID)
		{
		case 1:
			vendor->setText("Nokia");
			break;
		case 2:
			vendor->setText("Philips");
			break;
		case 3:
			vendor->setText("Sagem");
			break;
		case 4:
			vendor->setText(_("Unknown"));
		break;
		case 5:
		case 6:
			vendor->setText("Dream-Multimedia-TV");
			dreamlogo->show();
			break;
		default:
			vendor->setText(_("my"));
			break;
		}
		
		switch (mID)
		{
		case 1:
		case 2:
		case 3:
			processor->setText(_("Processor: XPC823, 66MHz"));
			break;
		case 4:
			processor->setText(_("Processor: XPC823, 80MHz"));
			break;
		case 5:
			processor->setText(_("Processor: STB04500, 252MHz"));
			break;
		case 6:
			processor->setText(_("Processor: STB25xxx, 252MHz"));
			break;
		}
		
		switch (mID)
		{
		case 1:
		case 2:
		case 3:
			machine->setText("d-Box 2");
			break;
		case 4:
			machine->setText("d-Box");
			break;
		case 5:
			machine->setText("DM7000");
			break;
		case 6:
			machine->setText("DM5600");
			break;
		}
		
		int fe=atoi(eDVB::getInstance()->getInfo("fe").c_str());
		switch (fe)
		{
		case 1:
			frontend->setText(_("Frontend: Satellite"));
			break;
		case 2:
			frontend->setText(_("Frontend: Cable"));
			break;
		case 3:
			frontend->setText(_("Frontend: Terrestric"));
			break;
		case 4:
			frontend->setText(_("Frontend: Simulator"));
			break;
		}
		
		eString sharddisks;
		
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
		
		if (sharddisks == "")
			sharddisks=_("keine");
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
				version->setText(
					eString(typea[type%3]) + eString(" ") + ver[0] + "." + ver[1] + "." + ver[2]
						+ ", " + date.mid(6, 2) + "." + date.mid(4,2) + "." + date.left(4));
			}
		}

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
