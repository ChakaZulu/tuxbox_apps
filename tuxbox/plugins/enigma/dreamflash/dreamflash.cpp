/*
 * $Id: dreamflash.cpp,v 1.2 2005/10/21 20:43:13 digi_casi Exp $
 *
 * (C) 2005 by mechatron
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#include "dreamflash.h"

#define VDF "v2.6"

#define CONFIGFILE "/var/tuxbox/config/enigma/bootmenue.conf"
#define TMPFILE "/tmp/.dftemp"

extern "C" int plugin_exec( PluginParam *par );
extern eString getInfo(const char *file, const char *info);

int fd, timeoutValue=10, videoformat=1, kill_inetd=1;
eString mpoint, skin_path, skin_name;
eListBox<eListBoxEntryText> *liste;

//----------------------------------------------------------------------------------

bool dateiedit(std::string dateiname, std::string oldstring, std::string newstring)
{
	FILE *f=fopen(dateiname.c_str(),"rt");
	if(!f) return false;

	FILE *ft=fopen(TMPFILE, "w+");
	if(!ft) return false;

	char line[128];
	while( fgets( line, 128, f ) )
	{
		std::string tmp=line;
		if( (tmp.find(oldstring))!= std::string::npos) fprintf(ft, newstring.c_str());
		else fprintf(ft, "%s", line);
	}
	fclose(ft);fclose(f);

	f=fopen(dateiname.c_str(),"w+"); ft=fopen(TMPFILE,"rt");

	while(fgets(line,128,ft))
		fprintf(f,"%s",line);

	fclose(ft); fclose(f);
	unlink(TMPFILE);
	return true;
}

//----------------------------------------------------------------------------------

static std::string resolvSymlinks(const char *path)
{
	char buffer[128];
	std::string tmpPath;
	char *tok, *str, *org;
	str=org=strdup( path ? path : "");
	if ( *str == '/' ) str++;
	while(1)
	{
		tmpPath+='/';
		tok=strchr(str, '/');
		if ( tok ) *tok=0;
		tmpPath+=str;
		while(1)
		{
			struct stat s;
			lstat(tmpPath.c_str(), &s);
			if (S_ISLNK(s.st_mode))
			{
				int count = readlink(tmpPath.c_str(), buffer, 255);
				if (buffer[0] == '/') tmpPath.assign(buffer,count);
				else tmpPath.replace(tmpPath.rfind('/')+1,sizeof(str),std::string(buffer,count));
			}
			else break;
		}
		if (tok) { str=tok;str++; }
		else break;
	}
	free(org);
	return tmpPath;
}

bool ismounted( std::string mountpoint )
{
	char buffer[200+1], mountDev[100], mountOn[100], mountType[20];
	std::string realPath = resolvSymlinks(mountpoint.c_str());
	if(FILE *mounts=fopen("/proc/mounts","rt"))
	{
		while(fgets(buffer, 200, mounts))
		{
			mountDev[0] = mountOn[0] = mountType[0] = 0;
			sscanf(buffer,"%s %s %s ", mountDev, mountOn, mountType);
			if( realPath == mountOn )
			{
				fclose(mounts);

				std::string tmp = "ln -s /tmp " + mountpoint + "/tmp_test";
				if(system(tmp.c_str())>>8) return false;
				else
				{
					tmp = mountpoint + "/tmp_test";
					unlink(tmp.c_str());
					return true;
				}
			}
		}
		fclose(mounts);
	}
	return false;
}

//----------------------------------------------------------------------------------

void msgerror(eString errorstring)
{
	eMessageBox msg(errorstring,_("Error"), eMessageBox::btOK|eMessageBox::iconError); msg.show(); msg.exec(); msg.hide();
}

void msgok(eString okstring)
{
	eMessageBox msg(okstring,_("information"), eMessageBox::btOK); msg.show(); msg.exec(); msg.hide();
}
//----------------------------------------------------------------------------------
bool loadconfig()
{
	mpoint = getInfo(CONFIGFILE, "mountpoint");
	skin_path = getInfo(CONFIGFILE, "skin-path");
	skin_name = getInfo(CONFIGFILE, "skin-name");

	if(FILE *in=fopen(CONFIGFILE, "rt"))
	{
		char line[256];
		while(fgets(line, 256, in))
		{
			if(line[strlen(line)-2]=='\r') line[strlen(line)-2]=0;
			else if(line[strlen(line)-1]=='\n')line[strlen(line)-1]=0;

			if (!strncmp(line, "timeout", 7))		timeoutValue = atoi(line+8);
			else if (!strncmp(line, "kill_inetd", 10))	kill_inetd = atoi(line+11);
		}
		fclose(in);
	}
	else return false;

	return true;
}

bool image_liste(int func)
{
	bool get=false;
	
	if(func)
	{
		eString dir[2] = {"/image", "/fwpro"};
		struct stat s;

		for (int i = 0; i < 2; i++)
		{
			if(DIR *d=opendir(eString(mpoint + dir[i]).c_str()))
			{
				while (struct dirent *e=readdir(d))
				{
					if (!(strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))) continue;

					eString name = mpoint + dir[i] + "/" + eString(e->d_name);
					stat(name.c_str(),&s);
					if (S_ISDIR(s.st_mode))
					{
						if(func == 2)
						{
							new eListBoxEntryText(liste, e->d_name, (void*)new eString(name));
							get = true;
						}
						else
						{
							eString tmp = name + "/go";
							if(FILE *f=fopen(tmp.c_str(), "r"))
							{
								fclose(f);
								new eListBoxEntryText(liste, e->d_name, (void*)new eString(name));
								get = true;
							}
							else
							{
								if(func == 4) new eListBoxEntryText(liste, e->d_name, (void*)new eString("_#"));
							}
						}
					}
				}
				closedir(d);
			}
		}
	}
	else
	{
		if(DIR *p=opendir(mpoint.c_str()))
		{
			while(struct dirent *e=readdir(p))
			{
				eString name = mpoint + "/" + e->d_name;
				eString tmp  = name; tmp.upper();
				if(tmp.find(".IMG")!=eString::npos)
				{
					new eListBoxEntryText(liste, e->d_name, (void*)new eString(name));
					get = true;
				}
			}
			closedir(p);
		}
	}

	return get;
}
//----------------------------------------------------------------------------------

setup_df::setup_df()
{
	cmove(ePoint(200, 120));
	cresize(eSize(330, 370));
	setText(_("Setup"));

	eLabel *a=new eLabel(this);
	a->move(ePoint(10, 15));
	a->resize(eSize(90, fd+10));
	a->setText("Medium:");

	mliste=new eListBox<eListBoxEntryText>(this,a);
	mliste->move(ePoint(100, 10));
	mliste->resize(eSize(220, fd+15));
	mliste->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	mliste->setHelpText(_("press left or right to change"));
	mliste->loadDeco();

	eLabel *b=new eLabel(this);
	b->move(ePoint(10, 60));
	b->resize(eSize(260, fd+10));
	b->setText("Timeout startmenu in sec:");

	starttimer=new eNumber(this, 1, 1, 19, 2, &timeoutValue, 0, b);
	starttimer->move(ePoint(270, 55));
	starttimer->resize(eSize(50, fd+15));
	starttimer->setFlags(eNumber::flagDrawPoints);
	starttimer->setHelpText(_("press ok to change"));
	starttimer->loadDeco();

	ch_inetd=new eCheckbox(this, kill_inetd);
	ch_inetd->move(ePoint(10, 100));
	ch_inetd->resize(eSize(clientrect.width()-20, fd+10));
	ch_inetd->setText("  kill process inetd");
	ch_inetd->setHelpText("enables is default");

	ed_skin_path=new eTextInputField(this);
	ed_skin_path->move(ePoint(10, clientrect.height()-220));
	ed_skin_path->resize(eSize(clientrect.width()-20, fd+15));
	ed_skin_path->setHelpText("press ok to enter the Skin-Directory");
	ed_skin_path->setEditHelpText("Entering Path (Yellow a>A)");
	ed_skin_path->loadDeco();
	CONNECT(ed_skin_path->selected, setup_df::load_sliste);

	eLabel *c=new eLabel(this);
	c->move(ePoint(10, clientrect.height()-170));
	c->resize(eSize(90, fd+10));
	c->setText(_("Skins:"));

	sliste=new eListBox<eListBoxEntryText>(this,a);
	sliste->move(ePoint(100, clientrect.height()-170));
	sliste->resize(eSize(220, fd+15));
	sliste->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	sliste->setHelpText(_("press left or right to change"));
	sliste->loadDeco();

	eButton *ok=new eButton(this);
	ok->move(ePoint((clientrect.width()/2)-75, clientrect.height()-105));
	ok->resize(eSize(150, fd+15));
	ok->loadDeco();
	ok->setText(_("save"));
	ok->setHelpText(_("save changes and return"));
	ok->hide();
	CONNECT(ok->selected, setup_df::okselected);

	status = new eStatusBar(this);
	status->move( ePoint(0, clientrect.height()-50) );status->resize( eSize( clientrect.width(), 50) );status->loadDeco();

	if(ismounted("/var/mnt/usb")) {new eListBoxEntryText(mliste, "USB-Stick", (void*)new eString("/var/mnt/usb")); ok->show(); }
	if(ismounted("/var/mnt/cf"))  {new eListBoxEntryText(mliste, "Compact Flash", (void*)new eString("/var/mnt/cf")); ok->show(); }
	if(ismounted("/hdd"))	      {new eListBoxEntryText(mliste, _("Harddisk"), (void*)new eString("/hdd")); ok->show(); }
	mliste->setCurrent(0);

	if(skin_path)
	{
		ed_skin_path->setText(skin_path);
		load_sliste();
	}
	else ed_skin_path->setText("/var/tuxbox/config");

}
void setup_df::load_sliste()
{
	eListBoxEntryText* selection=0;
	sliste->beginAtomic(); sliste->clearList();

	if(DIR *d=opendir(ed_skin_path->getText().c_str()))
	{
		while (struct dirent *e=readdir(d))
		{
			if (!(strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))) continue;

			eString tmp(e->d_name); tmp.upper();
			if ( tmp.find(".SKIN")!= eString::npos)
			{
				eString tmp1=e->d_name; tmp1=tmp1.left(tmp1.length()-5);
				eListBoxEntryText *s=new eListBoxEntryText(sliste,tmp1,(void*)new eString(e->d_name));
				if(skin_name == e->d_name) selection=s;
			}
		}
		closedir(d);
	}

	if (selection) sliste->setCurrent(selection);

	sliste->endAtomic();
}

void setup_df::new_init()
{
	bool cr_init=false;
	if(FILE *a=fopen("/var/etc/init","r"))
	{
		char line[256];
		const char *such="/tmp/bm.sh";
		while((fgets(line,256, a)!=NULL))
		{
			if (!strncmp(line, such, strlen(such)))
			{
				cr_init=false;
				break;
			}
			cr_init=true;
		}
		fclose(a);
	}
	else cr_init=true;

	if(cr_init)
	{
		if(FILE *c=fopen("/var/etc/init","a"))
		{
			fprintf(c,"/bin/bootmenue && /tmp/bm.sh\n");
			fclose(c);
		}
	}
	system("chmod 755 /var/etc/init");
}

void setup_df::okselected()
{
	//init erstellen
	new_init();
	//value
	mpoint		= ((eString*) mliste->getCurrent()->getKey())->c_str();
	timeoutValue	= starttimer->getNumber();
	skin_path	= ed_skin_path->getText();

	if (sliste->getCount())	skin_name = ((eString*) sliste->getCurrent()->getKey())->c_str();
	else			skin_name="";

	if(ch_inetd->isChecked())	kill_inetd=1;
	else				kill_inetd=0;
	//videonorm
	unsigned int colorformat;
	if (eConfig::getInstance()->getKey("/elitedvb/video/colorformat", colorformat)) colorformat = 1;
	switch(colorformat)
	{
		case 1: videoformat = 1; break;
		case 2: videoformat = 0; break;
		case 3: videoformat = 2; break;
		case 4: videoformat = 3; break;
	}
	//save config
	if(FILE *f=fopen(CONFIGFILE, "w"))
	{
		fprintf(f,"#DreamFlash-Config\n\n");
		fprintf(f,"version=%s\nmountpoint=%s\n", VDF, mpoint.c_str());
		fprintf(f,"timeout=%d\nvideocolor=%d\n", timeoutValue, videoformat);
		fprintf(f,"kill_inetd=%d\nselentry=Flash-Image\n",kill_inetd);
		fprintf(f,"skin-path=%s\nskin-name=%s\n", skin_path.c_str(), skin_name.c_str());
		fclose(f);
	}

	close(0);
}

//----------------------------------------------------------------------------------

info_df::info_df()
{
	cmove(ePoint(140, 100)); cresize(eSize(440, 320)); setText(_("information"));

	eLabel *l=new eLabel(this);
	l->move(ePoint(10, 10));
	l->resize(eSize(clientrect.width()-20, fd+4));
	if(mpoint == "/var/mnt/usb")		l->setText("USB-Stick on \"/var/mnt/usb\"");
	else if(mpoint == "/var/mnt/cf")	l->setText("Compact Flash on \"/var/mnt/cf\"");
	else if(mpoint == "/hdd")		l->setText("Harddisk on \"/hdd\"");

	liste=new eListBox<eListBoxEntryText>(this);
	liste->move(ePoint(10, 45));
	liste->resize(eSize(clientrect.width()-20, 170));
	new eListBoxEntryText(liste, "Flash-Image", (void*)new eString(""));
	image_liste(4);
	liste->loadDeco();
	CONNECT( liste->selchanged, info_df::Listeselchanged);

	x=new eLabel(this); x->move(ePoint(10, clientrect.height()-100)); x->resize(eSize(clientrect.width()-20, 100));
	ver = "DreamFlash "; ver += VDF; ver += "   by Mechatron";
	x->setText(ver);
}

void info_df::Listeselchanged(eListBoxEntryText *item)
{
	if (item)
	{
		eString loc=((eString*) liste->getCurrent()->getKey())->c_str();
#if 0
		if(loc)
		{
			if(loc == "_#") x->setText("Image is corrupted");
			else
			{
				system(eString().sprintf("/bin/du -s \"%s\" > %s", loc.c_str(), TMPFILE).c_str());
				if (FILE *d=fopen(TMPFILE,"r"))
				{
					char duinfo[256];
					fgets(duinfo, 256, d);
					fclose(d);
					unlink(TMPFILE);

					eString duwert = duinfo; duwert=duwert.left(duwert.find('/')-1);
					eString tmp = "Image-Size: " + duwert + " kB";
					x->setText(tmp);
				}
			}
		}
		else 
#endif
			x->setText(ver);
	}
}

//----------------------------------------------------------------------------------

image_df::image_df( int was)
{
	was1=was;

	struct statfs s;
	if (statfs(mpoint.c_str(), &s)>=0) free_space=(s.f_bavail * (s.f_bsize / 1024));

	cmove(ePoint(210, 150));

	switch(was)
	{
		case 0: cresize(eSize(300, 270)); setText(_("add")); break;
		case 1: cresize(eSize(300, 270)); setText(_("rename")); break;
		case 2: cresize(eSize(300, 200)); setText(_("delete")); break;
	}

	liste=new eListBox<eListBoxEntryText>(this);
	liste->move(ePoint(10, 10));
	liste->resize(eSize(clientrect.width()-20, fd+15));
	liste->setFlags(eListBox<eListBoxEntryText>::flagNoUpDownMovement);
	liste->setHelpText(_("press left or right to change"));
	bool is=image_liste(was);
	liste->setCurrent(0);
	liste->loadDeco();
	CONNECT(liste->selchanged, image_df::Listeselchanged);

	eLabel *l=new eLabel(this);
	l->setText(_("Enter new name:"));
	l->move(ePoint(10, 80));
	l->resize(eSize(clientrect.width()-20, fd+4));

	Iname=new eTextInputField(this, l);
	Iname->move(ePoint(10, 110));
	Iname->resize(eSize(clientrect.width()-20, fd+15));
	Iname->setMaxChars(15);
	//Iname->setUseableChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890-+:.");
	Iname->setHelpText(_("press ok to enter the name"));
	Iname->setEditHelpText("Entering Name (Yellow a>A)");
	Iname->loadDeco();

	ok=new eButton(this);
	ok->move(ePoint((clientrect.width()-140)/2, clientrect.height()-95));
	ok->resize(eSize(140, fd+15));
	ok->setText(_("ok"));
	ok->loadDeco();
	CONNECT(ok->selected, image_df::oksel);

	status = new eStatusBar(this);
	status->move( ePoint(0, clientrect.height()-45) );
	status->resize( eSize( clientrect.width(), 45) );
	status->loadDeco();

	if(was == 2) { l->hide(); Iname->hide(); }

	if(!is) ok->hide();
	else { liste->goNext(); liste ->goPrev(); }

	if(was)	ok->setHelpText(_("press ok to change"));
	else	ok->setHelpText(eString().sprintf("free Memory is %d kB", free_space).c_str());
}

void image_df::Listeselchanged(eListBoxEntryText *item)
{
	if ( !item ){close(0); return;}

	eString tmp = item->getText();
	if(!was1) tmp = tmp.left(tmp.length()-4);
	Iname->setText(tmp);
}

void image_df::oksel()
{
	eString loc  = ((eString*) liste->getCurrent()->getKey())->c_str();
	eString namet = Iname->getText();
	eString path = mpoint + "/image/" + namet;


	switch(was1)
	{
		case 0://new_install
		{
			//delete vor install
			if(DIR *d=opendir(path.c_str()))
			{
				closedir(d);
				system(eString().sprintf("rm -rf \"%s\"", path.c_str()).c_str());
			}

			image_install(path.c_str(), loc.c_str());
			break;
		}
		case 1://rename
		{
			if(loc == path) break;

			if(system(eString().sprintf("mv \"%s\" \"%s\"", loc.c_str(),path.c_str()).c_str())>>8)
			{
				hide(); msgerror("rename Error!"); show();
			}
			else { hide(); msgok(_("Done.")); show(); }

			break;
		}
		case 2://delete
		{
			if(system(eString().sprintf("rm -rf \"%s\"", loc.c_str()).c_str())>>8)
			{
				hide(); msgerror("delete Error!"); show();
			}
			else { hide(); msgok(_("Done.")); show(); }
			break;
		}
	}
	//new_go
	if(was1 < 2)
	{
		path += "/go";
		if(FILE *f=fopen(path.c_str(), "w"))
		{
			fprintf(f, "#!/bin/sh\n\n#mountpoint=%s\n#name=%s\n\n", mpoint.c_str(), namet.c_str());
			fprintf(f, "mount -t devfs dev /dev\n\n");
			fprintf(f, "if [ -d /lib/modules/$(uname -r) ] ; then\n");
			fprintf(f, "\tlsmod > /tmp/c_mod\n\twhile read line; do\n\t\tset -- $(echo $line)\n");
			fprintf(f, "\t\trmmod $1\n\tdone < /tmp/c_mod\n\trm -f /tmp/c_mod\n");
			fprintf(f, "fi\n\n/etc/init.d/rcS &\n");

			fclose(f);

			eString tmp = "chmod 755 " + path;
			system(tmp.c_str());
		}
	}

	close(0);
}

bool image_df::image_install(const char *i_dir, const char *f_name)
{
	ok->setHelpText(_("One moment please..."));

	if(free_space < 20000) { hide(); msgerror("Discspace  < 20 MB !!!"); show(); return false;}

	if(DIR *d=opendir(eString().sprintf("%s/image", mpoint.c_str()).c_str())) closedir(d);
	else
	{
		eString tmp = mpoint + "/image";
		mkdir( tmp.c_str() , 0777);
	}
	//image entpacken
	eString sq_name=mpoint + "/squashfs.img";
	if(system(eString().sprintf("dd if=\"%s\" of=%s bs=1024 skip=1152 count=4992", f_name, sq_name.c_str()).c_str()) >> 8)
	{
		hide(); msgerror("couldn't cut squash_part"); show(); return false;
	}
	unlink(f_name);
	//mounten
	mkdir("/tmp/image", 0777);
	if(system(eString().sprintf("mount -t squashfs %s /tmp/image -o loop", sq_name.c_str()).c_str()) >> 8)
	{
		unlink(sq_name.c_str());
		hide(); msgerror("couldn't mount squash_part"); show(); return false;
	}
	//copy
	if(system(eString().sprintf("cp -Rd /tmp/image \"%s\"", i_dir).c_str()) >> 8)
	{
		umount2("/tmp/image", MNT_FORCE);
		unlink(sq_name.c_str());
		hide(); msgerror("couldn't copy squash_part"); show(); return false;
	}
	//umount
	umount2("/tmp/image", MNT_FORCE);
	unlink(sq_name.c_str());
	//var
	system(eString().sprintf("rm -rf \"%s/var\" \"%s/var_init/tmp/init\"", i_dir, i_dir).c_str());
	system(eString().sprintf("mv \"%s/var_init\" \"%s/var\"", i_dir, i_dir).c_str());
	//root
	system(eString().sprintf("mkdir \"%s/root\"", i_dir).c_str());
	//rcS
	system(eString().sprintf("cp \"%s/etc/init.d/rcS\" \"%s/etc/init.d/rcS_org\"", i_dir, i_dir).c_str());
	eString rcS_pl=i_dir; rcS_pl+="/etc/init.d/rcS";
	if( ! dateiedit(rcS_pl,"mount -t jffs2","#remove from DF\n"))	{hide(); msgerror("couldn't find rcS"); show(); return false;}
	if( ! dateiedit(rcS_pl,"hdparm -c","#remove from DF\n"))	{hide(); msgerror("couldn't find rcS"); show(); return false;}
	//end
	hide(); msgok(_("Done.")); show();
	return true;
}
//----------------------------------------------------------------------------------

df_main::df_main()
{
	gPixmap *img=loadPNG("/tmp/flash.png");

	cmove(ePoint(200, 150));

	if(img)
	{
		cresize(eSize(280, 310));

		gPixmap *mp = &gFBDC::getInstance()->getPixmap();
		gPixmapDC mydc(img);
		gPainter p(mydc);
		p.mergePalette(*mp);

		eLabel *logo=new eLabel(this);
		logo->move(ePoint(70, clientrect.height()-90));
		logo->resize(eSize(142, 75));
		logo->setName("logo");
		logo->setPixmap(img);
	}
	else cresize(eSize(280, 220));

	setText("DreamFlash");

	eLabel *laba= new eLabel(this);
	laba->move(ePoint(100, 10));
	laba->resize(eSize(100, fd+15));
	laba->setText("Image ...");

	menuliste=new eListBox<eListBoxEntryText>(this);
	menuliste->move(ePoint(10, 40));
	menuliste->resize(eSize(clientrect.width()-20, 200));
	menuliste->setFlags(eListBoxBase::flagNoPageMovement);
	new eListBoxEntryText(menuliste, _("add"), (void*)0);
	new eListBoxEntryText(menuliste, _("rename"), (void*)1);
	new eListBoxEntryText(menuliste, _("delete"), (void*)2);
	new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)menuliste, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	new eListBoxEntryText(menuliste, _("Setup"), (void*)3);
	new eListBoxEntryText(menuliste, _("information"), (void*)4);
	CONNECT(menuliste->selected, df_main::Listeselected);
}

void df_main::Listeselected(eListBoxEntryText *item)
{
	if ( !item ){close(0); return;}

	if((int)item->getKey() < 3)	{ hide(); image_df dlg((int)item->getKey());dlg.show();dlg.exec();dlg.hide();show(); }
	else if((int)item->getKey()==3)	{ hide(); setup_df dlg; dlg.show(); dlg.exec(); dlg.hide();show(); }
	else				{ hide();  info_df dlg; dlg.show(); dlg.exec(); dlg.hide();show(); }
}

//----------------------------------------------------------------------------------

int plugin_exec( PluginParam *par )
{
	fd=eSkin::getActive()->queryValue("fontsize", 20);

	static unsigned char logo[] = {
	#include "flash_pic.h"
	};
	int in = open ("/tmp/flash.png", O_CREAT | O_WRONLY);
	write(in,logo,sizeof(logo));
	close(in);

	mpoint = "";
	if(!loadconfig()) { setup_df dlg; dlg.show(); dlg.exec(); dlg.hide(); }
	if(mpoint == "") return 0;

	df_main dlg; dlg.show(); dlg.exec(); dlg.hide();

	unlink("/tmp/flash.png");
	return 0;
}

