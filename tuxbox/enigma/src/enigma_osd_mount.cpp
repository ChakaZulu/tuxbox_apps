#ifdef ENABLE_DYN_MOUNT
#include <stdio.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <fstream>
#include <pthread.h>
#include <string.h>
#include <lib/base/i18n.h>
#include <lib/gdi/font.h>
#include <lib/gui/elabel.h>
#include <lib/gui/emessage.h>
#include <lib/gui/eskin.h>
#include <lib/gui/ewidget.h>
#include <enigma_main.h>
#include <enigma_mount.h>
#include <enigma_osd_mount.h>

eMountOSD *eMountOSD::instance = 0;

gFont eListBoxEntryMountOSD::ServerFont;
gFont eListBoxEntryMountOSD::LocalFont;
gPixmap *eListBoxEntryMountOSD::ok = 0;
gPixmap *eListBoxEntryMountOSD::failed = 0;
int eListBoxEntryMountOSD::ServerXSize = 0;
int eListBoxEntryMountOSD::LocalXSize = 0;

eListBoxEntryMountOSD::~eListBoxEntryMountOSD()
{
	if (paraLocal)
		paraLocal->destroy();

	if (paraServer)
		paraServer->destroy();

	if (paraFS)
		paraFS->destroy();

	if (paraAuto)
		paraAuto->destroy();
}

int eListBoxEntryMountOSD::getEntryHeight()
{
	if (!ServerFont.pointSize && !LocalFont.pointSize)
	{
		ServerFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.Description");
		LocalFont = eSkin::getActive()->queryFont("eEPGSelector.Entry.DateTime");
		ok = eSkin::getActive()->queryImage("ok_symbol");
		failed = eSkin::getActive()->queryImage("failed_symbol");
		eTextPara* tmp = new eTextPara(eRect(0, 0, 200, 30));
		tmp->setFont(LocalFont);
		tmp->renderString("00:00 - 00:00");
		LocalXSize = tmp->getBoundBox().width();
		tmp->destroy();
		tmp = new eTextPara(eRect(0, 0, 200, 30));
		tmp->setFont(LocalFont);
		tmp->renderString("00.00,");
		ServerXSize = tmp->getBoundBox().width();
		tmp->destroy();
	}
	return (calcFontHeight(ServerFont) + 4) * 2;
}

void eListBoxEntryMountOSD::redrawEntry()
{
	paraServer->destroy();
	paraServer = 0;
	paraLocal->destroy();
	paraLocal = 0;
	paraFS->destroy();
	paraFS = 0;
	paraAuto->destroy();
	paraAuto = 0;
}

eListBoxEntryMountOSD::eListBoxEntryMountOSD(eListBox<eListBoxEntryMountOSD> *listbox, int id)
		:eListBoxEntry((eListBox<eListBoxEntry>*)listbox),
		paraLocal(0), paraServer(0), paraFS(0), paraAuto(0), id(id)
{
}

const eString &eListBoxEntryMountOSD::redraw(gPainter *rc, const eRect& rect, gColor coActiveB, gColor coActiveF, gColor coNormalB, gColor coNormalF, int hilited)
{
	t_mount entry = eMountMgr::getInstance()->getMountPointData(id);
	drawEntryRect(rc, rect, coActiveB, coActiveF, coNormalB, coNormalF, hilited);

	int xpos = rect.left() + 10;
	if (entry.mounted)
	{
		int ypos = (rect.height() - ok->y) / 2;
		rc->blit(*ok, ePoint(xpos, rect.top() + ypos), eRect(), gPixmap::blitAlphaTest);
	}
	else
	{
		int ypos = (rect.height() - failed->y) / 2;
		rc->blit(*failed, ePoint(xpos, rect.top() + ypos), eRect(), gPixmap::blitAlphaTest);
	}
	xpos += 24 + 10; // i think no people want to change the ok and false pixmaps....
	if (!paraServer)
	{
		paraServer = new eTextPara(eRect(0 ,0, rect.width(), rect.height() / 2));
		paraServer->setFont(ServerFont);
		eString dummy, dummy2, dummy3;
		dummy2.sprintf("%s", entry.mountDir.c_str());
		if (dummy2.size() > 35)
			dummy3.sprintf("%s..%s", dummy2.left(15).c_str(), dummy2.right(20).c_str());
		else
			dummy3 = dummy2;
		if (entry.fstype != 2)
			paraServer->renderString(dummy.sprintf("%d.%d.%d.%d > %s", entry.ip[0], entry.ip[1], entry.ip[2], entry.ip[3], dummy3.c_str()));
		else
			paraServer->renderString(dummy.sprintf(_("Device > %s"), dummy3.c_str()));
		ServerYOffs = ((rect.height() / 2 - paraServer->getBoundBox().height()) / 2) - paraServer->getBoundBox().top();
	}
	rc->renderPara(*paraServer, ePoint(xpos, rect.top() + ServerYOffs + rect.height() / 2));
	if (!paraLocal)
	{
		paraLocal = new eTextPara(eRect(0, 0, LocalXSize, rect.height() / 2));
		paraLocal->setFont(LocalFont);
		eString dummy, dummy2;
		dummy.sprintf("%s", entry.localDir.c_str());
		if (entry.localDir.size() > 15)
			dummy2.sprintf("%s..%s", dummy.left(6).c_str(), dummy.right(6).c_str());
		else
			dummy2.sprintf("%s", dummy.c_str());
		paraLocal->renderString(dummy2);
		LocalYOffs = ((rect.height() / 2 - paraLocal->getBoundBox().height()) / 2) - paraLocal->getBoundBox().top();
	}
	LocalXSize = paraLocal->getBoundBox().width();
	rc->renderPara(*paraLocal, ePoint(xpos, rect.top() + LocalYOffs));
	xpos += LocalXSize + paraLocal->getBoundBox().height();
	//FIXME positions should be always the same one
	if (!paraFS)
	{
		paraFS = new eTextPara(eRect(0, 0, LocalXSize, rect.height() / 2));
		paraFS->setFont(LocalFont);
		if (entry.fstype == 0)
			paraFS->renderString("NFS");
		else 
		if (entry.fstype == 1)
			paraFS->renderString("CIFS");
		else
			paraFS->renderString(_("DEVICE"));
	}
	rc->renderPara(*paraFS, ePoint(xpos, rect.top()));
	xpos += LocalXSize + paraFS->getBoundBox().height();
	if (!paraAuto)
	{
		paraAuto = new eTextPara(eRect(0, 0, rect.width() - xpos, rect.height() / 2));
		paraAuto->setFont(LocalFont);
		paraAuto->renderString(entry.automount ? "auto" : "noauto");
	}
	rc->renderPara(*paraAuto, ePoint(xpos, rect.top()));

	dummy2 = "DUMMY [TM]";
	return dummy2;
}

/************************************************
*		MAIN MOUNT MANAGER              *
************************************************/
eMountOSD::eMountOSD():	eWindow(0)
{
	if (!instance)
		instance = this;
	setText(_("Mount Manager"));
	cmove(ePoint(75, 100));
	cresize(eSize(560, 400));

	mountList = new eListBox<eListBoxEntryMountOSD>(this);
	mountList->move(ePoint(10, 10));
	mountList->resize(eSize(540, 290));
	mountList->loadDeco();
	mountList->setHelpText(_("press ok to edit mount entry"));
	mountList->setActiveColor(eSkin::getActive()->queryScheme("eServiceSelector.highlight.background"), eSkin::getActive()->queryScheme("eServiceSelector.highlight.foreground"));
	CONNECT(mountList->selected, eMountOSD::mountSelected);

	updateList();

	remmount = new eButton(this);
	remmount->setText(_("remove"));
	remmount->move(ePoint(10, 330));
	remmount->resize(eSize(125, 28));
	remmount->setShortcut("red");
	remmount->setShortcutPixmap("red");
	remmount->loadDeco();
	remmount->setHelpText(_("remove selected mount point"));
	CONNECT(remmount->selected, eMountOSD::removeMount);

	newmount = new eButton(this);
	newmount->setText(_("add"));
	newmount->move(ePoint(140, 330));
	newmount->resize(eSize(130, 28));
	newmount->setShortcut("green");
	newmount->setShortcutPixmap("green");
	newmount->loadDeco();
	newmount->setHelpText(_("add new mount point"));
	CONNECT(newmount->selected, eMountOSD::addMount);

	unmount = new eButton(this);
	unmount->setText(_("unmount"));
	unmount->move(ePoint(275, 330));
	unmount->resize(eSize(125, 28));
	unmount->setShortcut("yellow");
	unmount->setShortcutPixmap("yellow");
	unmount->loadDeco();
	unmount->setHelpText(_("unmount selected mount point"));
	CONNECT(unmount->selected, eMountOSD::unmountNow);

	mountnow = new eButton(this);
	mountnow->setText(_("mount"));
	mountnow->move(ePoint(405, 330));
	mountnow->resize(eSize(125, 28));
	mountnow->setShortcut("blue");
	mountnow->setShortcutPixmap("blue");
	mountnow->loadDeco();
	mountnow->setHelpText(_("mount selected mount point"));
	CONNECT(mountnow->selected, eMountOSD::mountNow);

	statusbar = new eStatusBar(this);
	statusbar->move(ePoint(0, clientrect.height() - 30));
	statusbar->resize(eSize(clientrect.width(), 30));
	statusbar->loadDeco();
}

void eMountOSD::mountSelected(eListBoxEntryMountOSD *sel)
{
	if (sel)
	{
		hide();
		eMountOSDWindow wndMnt = eMountOSDWindow(mountList->getCurrent()->id);
		wndMnt.show();
		wndMnt.exec();
		wndMnt.hide();
		show();
		mountList->getCurrent()->redrawEntry();
		mountList->redraw();
	}
}

void eMountOSD::removeMount()
{
	if (mountList->getCurrent())
	{	
		t_mount mp = eMountMgr::getInstance()->getMountPointData(mountList->getCurrent()->id);
		if (mp.mounted)
		{
			if (eMountMgr::getInstance()->unmountMountPoint(mountList->getCurrent()->id))
			{
				eMessageBox box(_("Unmount failed! Remove mount point from list?"), _("Unmount Error"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconWarning, eMessageBox::btNo);
				box.show();
				int answer = box.exec();
				box.hide();
				if (answer == eMessageBox::btNo)
					return;
			}
		}
		eMountMgr::getInstance()->removeMountPoint(mountList->getCurrent()->id);
		updateList();
		setFocus(mountList);
	}
}

void eMountOSD::addMount()
{
	hide();
	t_mount mp;
	sscanf("//0.0.0.0/nothing", "//%d.%d.%d.%d/%*s", &mp.ip[0], &mp.ip[1], &mp.ip[2], &mp.ip[3]);
	mp.mountDir   = "";
	mp.localDir   = "";
	mp.fstype     = 0;
	mp.password   = "";
	mp.userName   = "";
	mp.automount  = 0;
	mp.rsize      = 0;
	mp.wsize      = 0;
	mp.options    = "";
	mp.ownOptions = "";
	mp.mounted    = false;
	mp.id         = -1; //don't care
	eMountOSDWindow newMountWindow = eMountOSDWindow(eMountMgr::getInstance()->addMountPoint(mp));
	newMountWindow.show();
	newMountWindow.exec();
	newMountWindow.hide();
	show();
	updateList();
	setFocus(mountList);
}

void eMountOSD::unmountNow()
{
	if (mountList->getCurrent())
	{
		if (eMountMgr::getInstance()->unmountMountPoint(mountList->getCurrent()->id))
		{
			eMessageBox box(_("Unmount failed!"), _("Unmount Error"), eMessageBox::btOK|eMessageBox::iconWarning, eMessageBox::btOK);
			box.show();
			box.exec();
			box.hide();
			return;
		}

		mountList->redraw();
		setFocus(mountList);
	}
	return;
}

void eMountOSD::mountNow()
{
	t_mount mp = eMountMgr::getInstance()->getMountPointData(mountList->getCurrent()->id);
	if (mountList->getCurrent())
	{
		eString message;
		switch(eMountMgr::getInstance()->mountMountPoint(mountList->getCurrent()->id))
		{
			case -1:
					message = _("This mount point is allready mounted!");
					break;
			case -2:
					message = _("Local directory is allready used as mount point!");
					break;
			case -3:
					message = _("CIFS is not supported!");
					break;
			case -4:
					message = _("NFS is not supported!");
					break;
			case -5:
					message = _("Mounting failed (timeout)!");
					break;
			case -10:
					createDirectory(mp.localDir);
					break;
			default:
					mountList->redraw();
					break;
		}

		if (message)
		{
			eMessageBox box1(message.c_str(), _("Mount Error"), eMessageBox::btOK|eMessageBox::iconInfo, eMessageBox::btOK);
			box1.show();
			box1.exec();
			box1.hide();
		}
		
		setFocus(mountList);
	}
}

void eMountOSD::createDirectory(eString directory)
{
	eMessageBox box(_("Local directory does not exist! Create it now?"), _("Mount Error"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconWarning, eMessageBox::btYes);
	box.show();
	int answer = box.exec();
	box.hide();
	if (answer == eMessageBox::btYes)
	{
		mkdir(directory.c_str(), 777);
		mountNow();
	}
	return;
}

eMountOSD::~eMountOSD()
{
}

void eMountOSD::updateList()
{
	mountList->beginAtomic();
	mountList->clearList();
	eMountMgr::getInstance()->listMountPoints(mountList);
	mountList->endAtomic();
	mountList->redraw();
	return;
}

void eMountOSD::fieldSelected(int *number)
{
	focusNext(eWidget::focusDirNext);
}

/*********************
*  MAIN MOUNT WINDOW *
*********************/
eMountOSDWindow::eMountOSDWindow(int id): eWindow(0)
{
	mp = eMountMgr::getInstance()->getMountPointData(mp.id);

	int fd = eSkin::getActive()->queryValue("fontsize", 20);

	setText(_("Mount..."));
	cmove(ePoint(75, 125));
	cresize(eSize(530, 400));

	eLabel *l = new eLabel(this);
	l->setText(_("Mode"));
	l->move(ePoint(10, 10));
	l->resize(eSize(80, fd+4));

	fstype = new eComboBox(this, 4, l);
	fstype->move(ePoint(130, 10));
	fstype->resize(eSize(300, 35));
	fstype->setHelpText(_("select your mount fs (ok)"));
	fstype->loadDeco();
	eListBoxEntryText *entry[3];
	entry[0] = new eListBoxEntryText(*fstype,_("NFS (Network FS)"), new eString("NFS"));
	entry[1] = new eListBoxEntryText(*fstype,_("CIFS (Common Internet FS)"), new eString("CIFS"));
	entry[2] = new eListBoxEntryText(*fstype,_("Block device"), new eString("Device"));
	fstype->setCurrent(entry[mp.fstype]);
	CONNECT(fstype->selchanged, eMountOSDWindow::mountFSChanged);

	lserver = new eLabel(this);
	lserver->setText(_("Server IP"));
	lserver->move(ePoint(10, 50));
	lserver->resize(eSize(150, fd + 4));
	if (mp.fstype == 2)
		lserver->hide();

	ip = new eNumber(this, 4, 0, 255, 3, mp.ip, 0, l);
	ip->move(ePoint(130, 50));
	ip->resize(eSize(200, fd + 10));
	ip->setFlags(eNumber::flagDrawPoints);
	ip->setHelpText(_("enter IP address from server"));
	ip->loadDeco();
	CONNECT(ip->selected, eMountOSDWindow::fieldSelected);
	if (mp.fstype == 2)
		ip->hide();

	l = new eLabel(this);
	l->setText(_("Localdir"));
	l->move(ePoint(10, 90));
	l->resize(eSize(150, fd + 4));

	localdir = new eTextInputField(this);
	localdir->setText(mp.localDir.c_str());
	localdir->setAlign(0);
	localdir->move(ePoint(130, 90));
	localdir->resize(eSize(300, 35));
	localdir->setHelpText(_("enter local directory where the system should be mounted"));
	localdir->loadDeco();
	localdir->setMaxChars(60);

	l = new eLabel(this);
	l->setText(_("Mountdir"));
	l->move(ePoint(10, 130));
	l->resize(eSize(150, fd + 4));

	mountdir = new eTextInputField(this);
	mountdir->setText(mp.mountDir.c_str());
	mountdir->setAlign(0);
	mountdir->move(ePoint(130, 130));
	mountdir->resize(eSize(300, 35));
	mountdir->setHelpText(_("enter mount directory of the server"));
	mountdir->loadDeco();
	mountdir->setMaxChars(60);

	lusername = new eLabel(this);
	lusername->setText(_("Username"));
	lusername->move(ePoint(10, 170));
	lusername->resize(eSize(150, fd + 4));
	if (mp.fstype != 1)
		lusername->hide();

	tusername = new eTextInputField(this);
	tusername->setText(mp.userName.c_str());
	tusername->setAlign(0);
	tusername->move(ePoint(130, 170));
	tusername->resize(eSize(300, 35));
	tusername->setHelpText(_("enter username for server (only if needed)"));
	tusername->loadDeco();
	tusername->setMaxChars(60);
	if (mp.fstype != 1)
		tusername->hide();

	lpassword = new eLabel(this);
	lpassword->setText(_("Password"));
	lpassword->move(ePoint(10, 210));
	lpassword->resize(eSize(150, fd + 4));
	if (mp.fstype != 1)
		lpassword->hide();

	tpassword = new eTextInputField(this);
	if (mp.password.size())
		tpassword->setText("******");
	tpassword->setAlign(0);
	tpassword->move(ePoint(130, 210));
	tpassword->resize(eSize(300, 35));
	tpassword->setHelpText(_("enter password for server (only if needed)"));
	tpassword->loadDeco();
	tpassword->setMaxChars(60);
	if (mp.fstype != 1)
		tpassword->hide();
	CONNECT(tpassword->selected, eMountOSDWindow::passwordSelected);

	l = new eLabel(this);
	l->setText(_("Options"));
	l->move(ePoint(10, 250));
	l->resize(eSize(150, fd + 4));

	eString optionsdummy;
	optionsdummy = mp.options;
	if (mp.ownOptions.length() != 0)
	{
		if (mp.options.length() != 0)
			optionsdummy += ",";
		optionsdummy += mp.ownOptions;
	}
	loptions = new eLabel(this);
	loptions->setText(optionsdummy.c_str());
	loptions->move(ePoint(130, 250));
	loptions->resize(eSize(300, fd + 4));
	loptions->loadDeco();

	automount = new eCheckbox(this, mp.automount, 1);
	automount->setText(_("automount at startup"));
	automount->move(ePoint(130, 280));
	automount->resize(eSize(fd + 4 + 240, fd + 4));
	automount->setHelpText(_("do mount at enigma startup"));

	editoptions = new eButton(this);
	editoptions->setText(_("Options"));
	editoptions->setAlign(0);
	editoptions->setShortcut("yellow");
	editoptions->setShortcutPixmap("yellow");
	editoptions->move(ePoint(10, 320));
	editoptions->resize(eSize(130, 35));
	editoptions->setHelpText(_("edit mount options"));
	editoptions->loadDeco();
	CONNECT(editoptions->selected, eMountOSDWindow::editOptions);

	savemount = new eButton(this);
	savemount->setText(_("save"));
	savemount->setAlign(0);
	savemount->setShortcut("green");
	savemount->setShortcutPixmap("green");
	savemount->move(ePoint(150, 320));
	savemount->resize(eSize(130, 35));
	savemount->setHelpText(_("save mount options"));
	savemount->loadDeco();
	CONNECT(savemount->selected, eMountOSDWindow::saveMount);

	abort = new eButton(this);
	abort->setText(_("abort"));
	abort->setAlign(0);
	abort->setShortcut("red");
	abort->setShortcutPixmap("red");
	abort->move(ePoint(290, 320));
	abort->resize(eSize(130, 35));
	abort->setHelpText(_("close window without saving"));
	abort->loadDeco();
	CONNECT(abort->selected, eWidget::accept);

	statusbar = new eStatusBar(this);
	statusbar->move(ePoint(0, clientrect.height() - 30));
	statusbar->resize(eSize(clientrect.width(), 30));
	statusbar->loadDeco();
}

eMountOSDWindow::~eMountOSDWindow()
{
}

void eMountOSDWindow::passwordSelected()
{
	if (tpassword->inEditMode())
	{
		if (tpassword->getText().size())
		{
			eMessageBox box(_("For security reasons you can only enter a new pass, but not edit the old one! Do you want to input a new password now?"), _("Mountmanager"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconWarning, eMessageBox::btYes);
			box.show();
			int answer = box.exec();
			box.hide();
			if (answer == eMessageBox::btNo)
				return;
		}
		tpassword->setText("");
	}
	else
	{
		mp.password = tpassword->getText();
		tpassword->setText("******");
		eMountMgr::getInstance()->changeMountPoint(mp.id, mp);
	}
}

void eMountOSDWindow::mountFSChanged(eListBoxEntryText *sel)
{
	if (sel->getKey() == "CIFS")
	{
		tusername->show();
		tpassword->show();
		lusername->show();
		lpassword->show();
		ip->show();
		lserver->show();
	}
	else 
	if (sel->getKey() == "NFS")
	{
		tusername->hide();
		tpassword->hide();
		lusername->hide();
		lpassword->hide();
		ip->show();
		lserver->show();
	}
	else
	{
		tusername->hide();
		tpassword->hide();
		lusername->hide();
		lpassword->hide();
		ip->hide();
		lserver->hide();
	}
}

void eMountOSDWindow::editOptions()
{
	hide();
	eMountOSDOptionsWindow *options = new eMountOSDOptionsWindow(mp.id);
	options->show();
	options->exec();
	options->hide();
	show();
	eString optionsdummy;
	optionsdummy = mp.options;
	if (mp.ownOptions.length() != 0)
	{
		if (mp.options.length() != 0)
			optionsdummy += ",";
		optionsdummy += mp.ownOptions;
	}
	loptions->setText(optionsdummy.c_str());
}

void eMountOSDWindow::saveMount()
{
	if (mp.mounted)
	{
		eMessageBox box(_("You can only change an unmounted mountpoint! Unmount now?"), _("Save Error"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconWarning, eMessageBox::btYes);
		box.show();
		int answer = box.exec();
		box.hide();
		if (answer == eMessageBox::btNo)
			close(0);
		else
		{
			if (eMountMgr::getInstance()->unmountMountPoint(mp.id))
				return;
		}
	}

	if (access(localdir->getText().c_str(), 0))
	{
		eMessageBox box(_("Local directory does not exist! Create it now?"), _("Mount Error"), eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconWarning, eMessageBox::btYes);
		box.show();
		int answer = box.exec();
		box.hide();
		if (answer == eMessageBox::btNo)
			return;
		else
		{
			mkdir(localdir->getText().c_str(), 777);
			if (access(localdir->getText().c_str(), 0))
			{
				eMessageBox box1(_("Failed to create directory!"), _("Mount Error"), eMessageBox::btOK|eMessageBox::iconWarning, eMessageBox::btOK);
				box1.show();
				box1.exec();
				box1.hide();
				return;
			}
		}
	}
	
	mp.localDir = localdir->getText();
	mp.mountDir = mountdir->getText();
	if (fstype->getCurrent()->getKey() == "NFS")
		mp.fstype = 0;
	else
		mp.fstype = 1;
	mp.userName = tusername->getText();
	if (automount->isChecked())
		mp.automount = 1;
	else
		mp.automount = 0;

	for (int j = 0; j <= 3; j++)
		mp.ip[j] = ip->getNumber(j);

	eMountMgr::getInstance()->changeMountPoint(mp.id, mp);
		
	close(0);
}

void eMountOSDWindow::fieldSelected(int *number)
{
	focusNext(eWidget::focusDirNext);
}

/**********************************************************
*		MOUNT OPTIONS WINDOW                      *
**********************************************************/
eMountOSDOptionsWindow::eMountOSDOptionsWindow(int id): eWindow(0)
{
	mp = eMountMgr::getInstance()->getMountPointData(mp.id);

	int fd = eSkin::getActive()->queryValue("fontsize", 20);

	setText(_("Mount options"));
	cmove(ePoint(75, 125));
	cresize(eSize(520, 400));

	eLabel *opt = new eLabel(this);
	opt->setText("Options:");
	opt->move(ePoint(10, 5));
	opt->resize(eSize(100, fd + 4));
	opt->loadDeco();

	ownopt = new eLabel(this);

	ownopt->setText(mp.ownOptions.c_str());
	ownopt->move(ePoint(100, 5));
	ownopt->resize(eSize(500, fd + 4));
	ownopt->loadDeco();

	async = new eCheckbox(this, checkOptions(mp.ownOptions, "async"), 1);
	async->setText(_("async"));
	async->move(ePoint(10, 40));
	async->resize(eSize(100, fd + 4));
	async->setHelpText(_("All I/O to the file system should be done asynchronously"));

	sync = new eCheckbox(this, checkOptions(mp.ownOptions, "sync"), 1);
	sync->setText(_("sync"));
	sync->move(ePoint(10, 80));
	sync->resize(eSize(100, fd + 4));
	sync->setHelpText(_("All  I/O to the file system should be done synchronously"));

	atime = new eCheckbox(this, checkOptions(mp.ownOptions, "atime"), 1);
	atime->setText(_("atime"));
	atime->move(ePoint(10, 120));
	atime->resize(eSize(100, fd + 4));
	atime->setHelpText(_("Update  inode  access  time  for each access. This is the default"));

	autom = new eCheckbox(this, checkOptions(mp.ownOptions, "auto"), 1);
	autom->setText(_("auto"));
	autom->move(ePoint(10, 160));
	autom->resize(eSize(100, fd + 4));
	autom->setHelpText(_("Can be mounted with the -a option"));

	execm = new eCheckbox(this, checkOptions(mp.ownOptions, "exec"), 1);
	execm->setText(_("exec"));
	execm->move(ePoint(10, 200));
	execm->resize(eSize(100, fd + 4));
	execm->setHelpText(_("Permit execution of binaries"));

	noexec = new eCheckbox(this, checkOptions(mp.ownOptions, "noexec"), 1);
	noexec->setText(_("noexec"));
	noexec->move(ePoint(10, 240));
	noexec->resize(eSize(100, fd + 4));
	noexec->setHelpText(_("Do  not  allow  execution  of any binaries on the mounted file system"));

	ro = new eCheckbox(this, checkOptions(mp.ownOptions, "ro"), 1);
	ro->setText(_("ro"));
	ro->move(ePoint(200, 40));
	ro->resize(eSize(100, fd + 4));
	ro->setHelpText(_("Mount the file system read-only"));

	rw = new eCheckbox(this, checkOptions(mp.ownOptions, "rw"), 1);
	rw->setText(_("rw"));
	rw->move(ePoint(200, 80));
	rw->resize(eSize(100, fd + 4));
	rw->setHelpText(_("Mount the file system read-write"));

	users = new eCheckbox(this, checkOptions(mp.ownOptions, "users"), 1);
	users->setText(_("users"));
	users->move(ePoint(200, 120));
	users->resize(eSize(100, fd + 4));
	users->setHelpText(_("Allow every user to mount and unmount  the  file  system"));

	nolock = new eCheckbox(this, checkOptions(mp.ownOptions, "nolock"), 1);
	nolock->setText(_("nolock"));
	nolock->move(ePoint(200, 160));
	nolock->resize(eSize(100, fd + 4));
	nolock->setHelpText(_("Prevent the Linux NFS client from notifying the server's lock manager."));

	intr = new eCheckbox(this, checkOptions(mp.ownOptions, "intr"), 1);
	intr->setText(_("intr"));
	intr->move(ePoint(200, 200));
	intr->resize(eSize(100, fd + 4));
	intr->setHelpText(_("Interrupts allowed on hard mount."));

	soft = new eCheckbox(this, checkOptions(mp.ownOptions, "soft"), 1);
	soft->setText(_("soft"));
	soft->move(ePoint(200, 240));
	soft->resize(eSize(100, fd + 4));
	soft->setHelpText(_("Use soft mount in place of hard mount."));

	udp = new eCheckbox(this, checkOptions(mp.ownOptions, "udp"), 1);
	udp->setText(_("udp"));
	udp->move(ePoint(200, 280));
	udp->resize(eSize(100, fd + 4));
	udp->setHelpText(_("Use UDP connection."));

	newoptions = new eButton(this);
	newoptions->setText("input options");
	newoptions->move(ePoint(10, 320));
	newoptions->resize(eSize(140, fd + 4));
	newoptions->setShortcut("blue");
	newoptions->setShortcutPixmap("blue");
	newoptions->loadDeco();
	newoptions->setHelpText(_("enter new options"));
	CONNECT(newoptions->selected, eMountOSDOptionsWindow::enterOptions);

	restore = new eButton(this);
	restore->setText(_("default"));
	restore->move(ePoint(160, 320));
	restore->resize(eSize(160, fd + 4));
	restore->setShortcut("yellow");
	restore->setShortcutPixmap("yellow");
	restore->loadDeco();
	restore->setHelpText(_("restore default settings"));
	CONNECT(restore->selected, eMountOSDOptionsWindow::setDefaultOptions);

	save = new eButton(this);
	save->setText(_("save"));
	save->move(ePoint(330, 320));
	save->resize(eSize(120, fd + 4));
	save->setShortcut("green");
	save->setShortcutPixmap("green");
	save->loadDeco();
	save->setHelpText(_("save changes and close window"));
	CONNECT(save->selected, eMountOSDOptionsWindow::saveOptions);

	abort = new eButton(this);
	abort->setText(_("abort"));
	abort->move(ePoint(460, 320));
	abort->resize(eSize(120, fd + 4));
	abort->setShortcut("red");
	abort->setShortcutPixmap("red");
	abort->loadDeco();
	abort->setHelpText(_("close window without saving"));
	CONNECT(abort->selected, eWidget::accept);

	statusbar = new eStatusBar(this);
	statusbar->move(ePoint(0, clientrect.height() - 30));
	statusbar->resize(eSize(clientrect.width(), 30));
	statusbar->loadDeco();
}

eMountOSDOptionsWindow::~eMountOSDOptionsWindow()
{
}

int eMountOSDOptionsWindow::checkOptions(eString options, eString option)
{
	return (options.find(option) != eString::npos) ? 1 : 0;
}

void eMountOSDOptionsWindow::enterOptions()
{
	hide();
	eTextInput *textinput = new eTextInput();
	mp.ownOptions = textinput->showTextInput(_("Options"),_("Enter your own mount options, comma separated"), NULL);
	eMountMgr::getInstance()->changeMountPoint(mp.id, mp);
	ownopt->setText(mp.ownOptions.c_str());
	show();
}

void eMountOSDOptionsWindow::setDefaultOptions()
{
	async->setCheck(0);
	sync->setCheck(0);
	atime->setCheck(0);
	autom->setCheck(0);
	execm->setCheck(0);
	noexec->setCheck(0);
	ro->setCheck(0);
	rw->setCheck(0);
	users->setCheck(0);
	nolock->setCheck(1);
	intr->setCheck(1);
	soft->setCheck(1);
	udp->setCheck(1);

	mp.ownOptions = "";
}

void eMountOSDOptionsWindow::saveOptions()
{
	mp.options = "";
	mp.options += async->isChecked() ? "async," : "";
	mp.options += sync->isChecked() ? "sync," : "";
	mp.options += atime->isChecked() ? "atime," : "";
	mp.options += autom->isChecked() ? "auto," : "";
	mp.options += execm->isChecked() ? "exec," : "";
	mp.options += noexec->isChecked() ? "noexec," : "";
	mp.options += ro->isChecked() ? "ro," : "";
	mp.options += rw->isChecked() ? "rw," : "";
	mp.options += users->isChecked() ? "users," : "";
	mp.options += nolock->isChecked() ? "nolock," : "";
	mp.options += intr->isChecked() ? "intr," : "";
	mp.options += soft->isChecked() ? "soft," : "";
	mp.options += udp->isChecked() ? "udp," : "";
	eMountMgr::getInstance()->changeMountPoint(mp.id, mp);
	close(0);
}


/*************************************************
*		TEXT INPUT WINDOW                *
*************************************************/
eTextInput::eTextInput()
{
}

eString eTextInput::showTextInput(eString title, eString helptext, eButton *button)
{
	TextEditWindow wnd(title.c_str(), helptext.c_str());
	if (button)
		wnd.setEditText(button->getText().c_str());
	wnd.show();
	wnd.exec();
	wnd.hide();
	if (button)
		button->setText(wnd.getEditText().c_str());
	return wnd.getEditText();
}

eTextInput::~eTextInput()
{
}
#endif
