/*
 * Ripper's setup for dreambox
 * Copyright (c) 2009 Ripper <Mario.Senska@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifdef ENABLE_IPKG
#include "enigma_ipkg.h"
char exe[256];
char *Executable;
char RUN_MESSAGE[128] = "Script is running, please wait!";
char NO_OUTPUT_MESSAGE[128] = "Success!";
ePackageManagerRunApp::ePackageManagerRunApp():
eWindow (0)
{
	init_ePackageManagerRunApp();
}
void ePackageManagerRunApp::init_ePackageManagerRunApp()
{
	eDebug("%s is running, please wait\n", Executable);

	bCancel = CreateSkinnedButton("bCancel");
	CONNECT( bCancel->selected, ePackageManagerRunApp::onCancel );

	bClose = CreateSkinnedButton("bClose");
	CONNECT( bClose->selected, ePackageManagerRunApp::accept );
	
	scrollbar= CreateSkinnedProgress("scrollbar",0,100);
	eWidget* visible=new eWidget(this);
	visible->setName("visible");

	label=new eLabel(visible);
	label->setFlags(RS_WRAP);

	BuildSkin("ePackageManagerRunApp");
	
	float lineheight = fontRenderClass::getInstance()->getLineHeight(label->getFont());
	int lines = (int) (visible->getSize().height() / lineheight);
	pageHeight = (int) (lines * lineheight);
	visible->resize(eSize(visible->getSize().width(), pageHeight + (int) (lineheight / 6)));
	label->resize(eSize(visible->getSize().width(), pageHeight * 16));
	
	label->move(ePoint(0,0));

}

void ePackageManagerRunApp::updateScrollbar()
{
	total = pageHeight;
	int pages = 1;
	while (total < label->getExtend().height())
	{
		total += pageHeight;
		pages++;
	}
	int start = -label->getPosition().y() * 100 / total;
	int vis = pageHeight * 100 / total;
	scrollbar->setParams(start, vis);
	scrollbar->show();
	if (pages == 1)
	total = 0;
}

int ePackageManagerRunApp::eventHandler(const eWidgetEvent & event)
{
   switch (event.type)
   {
	case eWidgetEvent::execBegin:
		app = NULL;
		output = 0;
		bCancel->show ();
		app = new eConsoleAppContainer (Executable);
		bClose->hide ();

		if (!app->running ())
		  {
			eMessageBox::ShowBox(_("sorry, start script."), _("sorry, start script."), eMessageBox::btOK | eMessageBox::iconError);
			close (-1);
		  }
		else
		  {
		    CONNECT (app->dataAvail, ePackageManagerRunApp::getData);
		    CONNECT (app->appClosed, ePackageManagerRunApp::appClosed);
		  }
		return 1;

	case eWidgetEvent::evtAction:
          
		if (total && event.action == &i_cursorActions->up)
		{
		    ePoint curPos = label->getPosition();
		    if (curPos.y() < 0)
		    {
		      label->move(ePoint(curPos.x(), curPos.y() + pageHeight));
		      updateScrollbar();
		    }
		}
		else if (total && event.action == &i_cursorActions->down)
		{
		    ePoint curPos = label->getPosition();
		    if ((total - pageHeight) >= abs(curPos.y() - pageHeight))
		    {
		      label->move(ePoint(curPos.x(), curPos.y() - pageHeight));
		      updateScrollbar();
		    }
		}
		else if (event.action == &i_cursorActions->cancel)
		    close(0);
		else
		    break;
		return 1;
	default:
		break;
   }
   return eWindow::eventHandler(event);
}

void ePackageManagerRunApp::getData (eString str)
{
     setText (eString ().sprintf ("%s is running, please wait", Executable));
  char *begin, *ptr;
  int c;

  eString buf;
   buf = label->getText () + str;
  begin = (char *) buf.c_str ();
  ptr = begin + strlen (begin);
  c = 0;
  while (ptr > begin && c <= 17)
    {
      ptr--;
      if (ptr[0] == '\n')
        c++;
    }
  if (ptr[0] == '\n')
    ptr++;
	label->setText (ptr);
  output = 1;
}

void ePackageManagerRunApp::appClosed (int i)
{
  if (!output)
     label->setText (NO_OUTPUT_MESSAGE);
     setText (eString ().sprintf ("%s is running, please wait", Executable));
  if (i != 0)
  {
    setText (eString ().sprintf ("%s failing", Executable));
     label->setText (_("An error occured during exucution"));
   }else
   {
   setText (eString ().sprintf ("%s Success", Executable));
   }
  bClose->show ();
  bCancel->hide ();
}

void ePackageManagerRunApp::onCancel ()
{
  if (app)
    app->kill ();
  close (1);
}

long getVarPercentUsed()
{
	long blocks_used = 0;
	long blocks_percent_used = 0;
	struct statfs s;
	const char *mount_point = "/var";
	if (statfs(mount_point, &s) != 0)
		return 0;
	if ((s.f_blocks > 0)) {
		blocks_used = s.f_blocks - s.f_bfree;
		blocks_percent_used = 0;
		if (blocks_used + s.f_bavail) {
			blocks_percent_used = (((long long) blocks_used) * 100
								   + (blocks_used + s.f_bavail)/2
								   ) / (blocks_used + s.f_bavail);
		}
	}
	return blocks_percent_used;
}

bool checkAvailSpace()
{
	bool ret = true;
	long percentUsed = getVarPercentUsed();
	if (percentUsed > 90)
	{
		ret = false;
		eString msg;
		msg.sprintf(_("WARNING: Your /var is already %d%% full. Continue?"), percentUsed);
		int res = eMessageBox::ShowBox(
			msg.c_str(),
			_("Continue"),
			eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion);   
		if (res == eMessageBox::btYes)
			ret = true;
	}
	return ret;
}

eString extractNameFromPathIPK(const char *path)
{
	eString name;
	char str[255];
	
	strcpy(str, path);
	
	char *start = str;
	char *position = strrchr(str, '/');
	
	if (position != 0)
		start = position + 1;
	
	position = strstr(start, ".ipk");

	if (position != 0)
		*position = 0;
	
	name = start;

	eDebug("**********************************************");
	eDebug(name.c_str());
	eDebug("**********************************************");
	
	return name;
}

eString extractNameFromPathControl(const char *path)
{
	eString name;
	char str[255];
	
	strcpy(str, path);
	
	char *start = str;
	char *position = strrchr(str, '/');
	
	if (position != 0)
		start = position + 1;
	
	position = strstr(start, ".control");

	if (position != 0)
		*position = 0;
	
	name = start;

	eDebug("**********************************************");
	eDebug(name.c_str());
	eDebug("**********************************************");
	
	return name;
}

eMainmenuPackageManager::eMainmenuPackageManager()
:eSetupWindow(_("Package Manager"), 6, 330)
{
	init_eMainmenuPackageManager();
}
void eMainmenuPackageManager::init_eMainmenuPackageManager()
{
	move(ePoint(170, 30));
	int entry=0;
	CONNECT((new eListBoxEntryMenu(&list, _("Package Settings"), eString().sprintf("(%d) %s", ++entry, _("open package manager settings")) ))->selected, eMainmenuPackageManager::PackageManagerSettings);
	new eListBoxEntryMenuSeparator(&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	CONNECT((new eListBoxEntryMenu(&list, _("Update package list"), eString().sprintf("(%d) %s", ++entry, _("reload package list from server")) ))->selected, eMainmenuPackageManager::PackageManagerUpdate);
	CONNECT((new eListBoxEntryMenu(&list, _("Install package (Online)"), eString().sprintf("(%d) %s", ++entry, _("Install package from server")) ))->selected, eMainmenuPackageManager::PackageManagerInstallOnline);
	CONNECT((new eListBoxEntryMenu(&list, _("Install package (Offline)"), eString().sprintf("(%d) %s", ++entry, _("Install package from box")) ))->selected, eMainmenuPackageManager::PackageManagerInstall);
	CONNECT((new eListBoxEntryMenu(&list, _("Remove package"), eString().sprintf("(%d) %s", ++entry, _("Remove installed package")) ))->selected, eMainmenuPackageManager::PackageManagerRemove);
}

void eMainmenuPackageManager::PackageManagerSettings()
{
	hide(); ePackageSettings setup; setup.show(); setup.exec(); setup.hide(); show();
}

void eMainmenuPackageManager::PackageManagerUpdate()
{
	sprintf (exe, "ipkg update");
	Executable = exe;
	printf ("E: %s\n", Executable);
	strcpy (RUN_MESSAGE, "");
	hide ();
	ePackageManagerRunApp run;
	run.show();
	run.exec();
	run.hide();
 	show ();
}

void eMainmenuPackageManager::PackageManagerInstallOnline()
{
	hide ();
	if ((access("/var/ipkg/lists/lists", F_OK) == 0))
	{
 		ePackageHandler setup(2); setup.show(); setup.exec(); setup.hide();
 	}else
 	{
		eMessageBox::ShowBox(_("There was no list found. Please perform package update first!"),_("INFO"),eMessageBox::btOK|eMessageBox::iconError);
 	}
 	show ();
}

void eMainmenuPackageManager::PackageManagerInstall()
{
	hide(); ePackageHandler setup(0); setup.show(); setup.exec(); setup.hide(); show();
}

void eMainmenuPackageManager::PackageManagerRemove()
{
	hide(); ePackageHandler setup(1); setup.show(); setup.exec(); setup.hide(); show();
}

struct ePackageSettings::selectComboEntry : public std::unary_function<const eListBoxEntryText&, void>
{
	const char *key;
	eComboBox *cb;

	selectComboEntry(const char* key, eComboBox* cb): key(key), cb(cb)
	{
	}

	bool operator()(const eListBoxEntryText& s)
	{
		if (strcmp(key, (char *)s.getKey()) == 0)
		{
			cb->setCurrent(&s,true);
			return 1;
		}
		return 0;
	}
};

ePackageSettings::ePackageSettings()
{
	init_ePackageSettings();
}
void ePackageSettings::init_ePackageSettings()
{
	IPKGInstallOption=CreateSkinnedComboBox("IPKGInstallOption", 4);
	new eListBoxEntryText( *IPKGInstallOption, "install", (char*)"install");
	new eListBoxEntryText( *IPKGInstallOption, "-force-defaults install", (char*)"-force-defaults install");
	new eListBoxEntryText( *IPKGInstallOption, "-force-reinstall install", (char*)"-force-reinstall install");
	new eListBoxEntryText( *IPKGInstallOption, "-force-overwrite install", (char*)"-force-overwrite install");

	path = CreateSkinnedTextInputField("path", "/tmp/");
	path->inEditMode();
	path->setUseableChars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/");

	seldir=CreateSkinnedButton("seldir");
	CONNECT(seldir->selected,ePackageSettings::selectDir);
	

	lServer = CreateSkinnedTextInputField("lServer",0);
	lServer->inEditMode();
	lServer->setUseableChars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/");

	CONNECT(CreateSkinnedButton("bClose")->selected, ePackageSettings::accept);
	
	CONNECT(CreateSkinnedButton("bSave")->selected, ePackageSettings::SaveSettings);
	
	BuildSkin("ePackageSettings");
	
	LoadSettings();
	
}

void ePackageSettings::SaveSettings()
{
	eListBoxEntryText * e;
	e = IPKGInstallOption->getCurrent();
	eConfig::getInstance()->setKey("/elitedvb/ipkg/install", (char *)e->getKey());

	char tagValue[300];
	sprintf(tagValue, "%s", path->getText().c_str());
	eConfig::getInstance()->setKey("/elitedvb/ipkg/target", tagValue);

	FILE *fd_conf;
	if (!(fd_conf = fopen("/etc/ipkg.conf" , "wb")))
	{
		return;
	}
	fprintf(fd_conf,"src lists %s\n", lServer->getText().c_str());
	fclose(fd_conf);

	eConfig::getInstance()->flush();
	close(1);
}

void ePackageSettings::selectDir()
{
	eFileSelector sel(path->getText());
	hide();
	const eServiceReference *ref = sel.choose(-1);
	if (ref)
		path->setText(sel.getPath().current().path);
	show();
	setFocus(seldir);
}

void ePackageSettings::LoadSettings()
{
	char* install="install";
	eConfig::getInstance()->getKey("/elitedvb/ipkg/install", install);
	IPKGInstallOption->forEachEntry(selectComboEntry(install, IPKGInstallOption));

	char* target="/tmp/";
	eConfig::getInstance()->getKey("/elitedvb/ipkg/target", target);
	path->setText(target);
	
	char line_buffer[256], c_server[256]="http://192.168.178.34/package";
	char *ptr;
	FILE *fp;
	if(fp = fopen("/etc/ipkg.conf", "r+"))
	{
		while(fgets(line_buffer, sizeof(line_buffer), fp))
		{	
			if((ptr = strstr(line_buffer, "src lists ")))
			{
				sscanf(ptr + 10, "%s", c_server);
			}
		}
		fclose (fp);
	}
	lServer->setText(c_server);
}
ePackageHandler::ePackageHandler(int type):type(type)
{
	init_ePackageHandler();
}
void ePackageHandler::init_ePackageHandler()
{
	items=new eListBox<eListBoxEntryText>(this);
	items->setName("items");
	items->setFlags( eListBoxBase::flagLostFocusOnFirst|eListBoxBase::flagLostFocusOnLast );
	CONNECT(items->selected,ePackageHandler::PackageChanged );
	long percent = getVarPercentUsed();
	eString buffer;
	buffer.sprintf("%d%%", percent);

	CreateSkinnedProgress("varSpace",0,percent);

	CreateSkinnedLabel("lVarPercent",buffer.c_str());

	BuildSkin("ePackageHandler");
	ListPackage();
	switch (type)
	{
	  case 0:
		setText(_("Install package (Offline)"));
		items->setHelpText(_("select package to install"));
		break;
	  case 1:
		setText(_("Remove package"));
		items->setHelpText(_("select package to remove"));
		break;
	  case 2:
		setText(_("Install package (Online)"));
		items->setHelpText(_("select package to install"));
		break;
	}
}
void ePackageHandler::PackageChanged( eListBoxEntryText *e )
{
	hide();
	if (items->getCurrent())
	{
		eListBoxEntryText *pitem = items->getCurrent();
		if (pitem && checkAvailSpace())
		{
		  	eString command;
			eString ask;
			eString title;
			switch (type)
			{
			  case 0:
			  {
				char* install="install";
				eConfig::getInstance()->getKey("/elitedvb/ipkg/install", install);
				char* target="/tmp/";
				eConfig::getInstance()->getKey("/elitedvb/ipkg/target", target);
				ask.sprintf(_("Do you want to install package %s offline?"), pitem->getText().c_str());
				command.sprintf("ipkg %s %s%s.ipk",install,target,pitem->getText().c_str());
				title = _("Install package");
				break;
			  }
			  case 1:
			  {
				ask.sprintf(_("Do you really want to remove package %s?"), pitem->getText().c_str());
				command.sprintf("ipkg remove %s",pitem->getText().c_str());
				title = _("Remove package");
				break;
			  }
			  case 2:
			  {
				char* install="install";
				eConfig::getInstance()->getKey("/elitedvb/ipkg/install", install);
				ask.sprintf(_("Do you want to install package %s online?"), pitem->getText().c_str());
				command.sprintf("ipkg %s %s", install, pitem->getText().c_str());
				title = _("Install package");
				break;
			  }
			}
			int res = eMessageBox::ShowBox(ask.c_str(),title.c_str(),eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion, eMessageBox::btNo, 30);
			if (res == eMessageBox::btYes)
			{
				sprintf (exe, command.c_str());
				Executable = exe;
				printf ("E: %s\n", Executable);
				strcpy (RUN_MESSAGE, "");
				ePackageManagerRunApp run;
				run.show();
				run.exec();
				run.hide();
			}
			ListPackage();
		}
	}
	show();
}
void ePackageHandler::ListPackage()
{
	switch (type)
	{
	  case 0:
	  {
		char* target="/tmp/";
		eConfig::getInstance()->getKey("/elitedvb/ipkg/target", target);
		DIR * directory;
		struct dirent * entry;
		directory = opendir(target);
		if(directory != 0)
		{
			items->beginAtomic();
			items->clearList();
			while((entry = readdir(directory)) != 0)
			{
				if(strstr(entry->d_name, ".ipk") == 0)
					continue;
				new eListBoxEntryText((eListBox<eListBoxEntryText>*) items, extractNameFromPathIPK(entry->d_name).c_str());
			}
			items->endAtomic();
			closedir(directory);
		}
		break;
	  }
	  case 1:
	  {
		items->beginAtomic();
		items->clearList();
		char line_buffer[256], c_server[256];
		char *ptr;
		FILE *fp;
		if(fp = fopen("/var/ipkg/status", "r"))
		{
			while(fgets(line_buffer, sizeof(line_buffer), fp))
			{	
				if((ptr = strstr(line_buffer, "Package:")))
				{
					sscanf(ptr + 8, "%s", c_server);
					new eListBoxEntryText((eListBox<eListBoxEntryText>*) items, c_server );
				}
			}
			fclose (fp);
		}
		items->endAtomic();
		break;
	  }
	  case 2:
	  {
		items->beginAtomic();
		items->clearList();
		char line_buffer[256], c_server[256];
		char *ptr;
		FILE *fp;
		if(fp = fopen("/var/ipkg/lists/lists", "r"))
		{
			while(fgets(line_buffer, sizeof(line_buffer), fp))
			{	
				if((ptr = strstr(line_buffer, "Package:")))
				{
					sscanf(ptr + 8, "%s", c_server);
					new eListBoxEntryText((eListBox<eListBoxEntryText>*) items, c_server );
				}
			}
			fclose (fp);
		}
		items->endAtomic();
		break;
	  }
	}
}
#endif
