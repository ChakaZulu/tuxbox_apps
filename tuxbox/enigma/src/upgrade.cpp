#include "upgrade.h"
#include <lib/gui/ebutton.h>
#include <unistd.h>
#include <xmltree.h>
#include <lib/gui/statusbar.h>
#include <lib/gui/emessage.h>
#include <lib/gui/eprogress.h>
#include <libmd5sum.h>
#include <lib/dvb/edvb.h>

#define TMP_IMAGE "/var/tmp/root.cramfs"

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

eListBoxEntryImage::eListBoxEntryImage
	(eListBox<eListBoxEntryImage> *listbox, eString name, eString target, eString url, eString version, eString creator, const unsigned char md5[16])
	: eListBoxEntryText((eListBox<eListBoxEntryText> *)listbox, name),
	name(name), target(target), url(url), version(version), creator(creator)
{
	if (md5)
		memcpy(this->md5, md5, 16);
	else
		memset(this->md5, 0, 16);
}

eHTTPDownload::eHTTPDownload(eHTTPConnection *c, const char *filename): eHTTPDataSource(c), filename(filename)
{
	if (c->remote_header.count("Content-Length"))
		total=atoi(c->remote_header["Content-Length"].c_str());
	else
		total=-1;
	received=0;
	fd=::creat(filename, 0777);
	progress(received, total);
}

eHTTPDownload::~eHTTPDownload()
{
	if (fd >= 0)
		::close(fd);
	if ((total != -1) && (total != received))
		::unlink(filename.c_str());
}

void eHTTPDownload::haveData(void *data, int len)
{
	if (len)
	{
		if (fd >= 0)
			::write(fd, data, len);
	}
	received+=len;
	progress(received, total);
}

eHTTPDownloadXML::eHTTPDownloadXML(eHTTPConnection *c, XMLTreeParser &parser): eHTTPDataSource(c), parser(parser)
{
	error=0;
	errorstring="";
}

void eHTTPDownloadXML::haveData(void *data, int len)
{
	if ((!error) && (!parser.Parse((char*)data, len, !data)))
	{
		errorstring.sprintf("XML parse error: %s at line %d",
			parser.ErrorString(parser.GetErrorCode()),
			parser.GetCurrentLineNumber());
		error=1;
	}
}

eUpgrade::eUpgrade()
{
	cmove(ePoint(100, 100));
	cresize(eSize(500, 340));
	setText(_("Software upgrade.."));
	
	status = new eStatusBar(this);
	status->setFlags(eStatusBar::flagOwnerDraw);
	status->move( ePoint(0, clientrect.height()-30) );
	status->resize( eSize( clientrect.width(), 30) );
	status->loadDeco();

	images=new eListBox<eListBoxEntryImage>(this);
	images->move(ePoint(10, 70));
	images->resize(eSize(480, 190));
	images->loadDeco();
	CONNECT(images->selected, eUpgrade::imageSelected);
	
	imagehelp=new eLabel(this);
	imagehelp->move(ePoint(10, 0));
	imagehelp->resize(eSize(460, 30));
	imagehelp->setText(_("Please select software version to upgrade to:"));

	progress=new eProgress(this);
	progress->move(ePoint(10, 265));
	progress->resize(eSize(250, 30));
	progress->hide();
	
	progresstext=new eLabel(this);
	progresstext->move(ePoint(270, 275));
	progresstext->resize(eSize(210, 30));
	progresstext->hide();
	
	abort=new eButton(this);
	abort->move(ePoint(40, 40));
	abort->resize(eSize(150, 50));
	abort->setText(_("abort"));
	abort->loadDeco();
	CONNECT(abort->selected, eUpgrade::abortDownload);
	abort->hide();

	catalog=0;
	eString caturl=getVersionInfo("catalog");
	if (caturl.length())
		loadCatalog(caturl.c_str());
	
	struct stat s;
	if (!stat(TMP_IMAGE, &s))
		new eListBoxEntryImage(images, _("manual upload"), "", "", "", "", 0);
}

void eUpgrade::loadCatalog(const char *url)
{
	current_url=url;
	int error;
	if (catalog)
		delete catalog;
	catalog=new XMLTreeParser("ISO-8859-1");
	http=eHTTPConnection::doRequest(url, &error);
	if (!http)
	{
		catalogTransferDone(error);
	} else
	{
		setStatus(_("downloading catalog..."));
		CONNECT(http->transferDone, eUpgrade::catalogTransferDone);
		CONNECT(http->createDataSource, eUpgrade::createCatalogDataSink);
		http->local_header["User-Agent"]="enigma-upgrade/1.0.0";
		http->start();
	}
}

void eUpgrade::loadImage(const char *url)
{
	images->hide();
	imagehelp->hide();
	current_url=url;
	int error;
	if (http)
		delete http;
	progress->show();
	progresstext->show();
	abort->show();
	http=eHTTPConnection::doRequest(url, &error);
	if (!http)
	{
		imageTransferDone(error);
	} else
	{
		setStatus(_("downloading image..."));
		CONNECT(http->transferDone, eUpgrade::imageTransferDone);
		CONNECT(http->createDataSource, eUpgrade::createImageDataSink);
		http->local_header["User-Agent"]="enigma-upgrade/1.0.0";
		http->start();
	}
}

void eUpgrade::catalogTransferDone(int err)
{
	if ((!err) && http && (http->code == 200) && datacatalog && !datacatalog->error)
	{
		XMLTreeNode *root=catalog->RootNode();
		eString mytarget=eDVB::getInstance()->getInfo("mID").right(1);

		images->beginAtomic();
		for (XMLTreeNode *r=root->GetChild(); r; r=r->GetNext())
		{
			if (strcmp(r->GetType(), "image"))
				continue;
			const char *name=r->GetAttributeValue("name");
			const char *url=r->GetAttributeValue("url");
			const char *version=r->GetAttributeValue("version");
			const char *target=r->GetAttributeValue("target");
			const char *creator=r->GetAttributeValue("creator");
			const char *amd5=r->GetAttributeValue("md5");
			unsigned char md5[16];
			if (!creator)
				creator=_("unknown");
			if (!amd5)
				continue;
			for (int i=0; i<32; i+=2)
			{
				char x[3];
				x[0]=amd5[i];
				if (!x[0])
					break;
				x[1]=amd5[i+1];
				if (!x[1])
					break;
				int v=0;
				if (sscanf(x, "%02x", &v) != 1)
					break;
				md5[i/2]=v;
			}
			if (!(name && url && version && target))
				continue;
			if (!strstr(target, mytarget.c_str()))
				continue;
			new eListBoxEntryImage(images, name, target, url, version, creator, md5);
		}
		setFocus(images);
		images->endAtomic();
		setStatus("Please select version to upgrade or LAME! to abort");
	} else
	{
		if (err || http->code !=200)
			setError(err);
		else if (datacatalog)
		{
			eDebug("data error.");
			eDebug("%s", datacatalog->errorstring.c_str());
			setStatus(datacatalog->errorstring);
		}
	}
	if (catalog)
		delete catalog;
	http=0;
}

void eUpgrade::imageTransferDone(int err)
{
	progress->hide();
	progresstext->hide();
	abort->hide();
	if (err || !http || http->code != 200)
		setError(err);
	else
		flashImage(1);
	http=0;
	images->show();
	imagehelp->show();
}

void eUpgrade::imageSelected(eListBoxEntryImage *img)
{
	if (img)
	{
		if (img->url.length())
		{
			memcpy(expected_md5, img->md5, 16);
			setStatus(img->url);
			loadImage(img->url.c_str());
		} else
			flashImage(0);
	} else
		close(0); // aborted
}

void eUpgrade::setStatus(const eString &string)
{
	status->getLabel().setText(string);
}

void eUpgrade::setError(int err)
{
	eString errmsg;
	switch (err)
	{
	case 0:
		if (http && http->code != 200)
			errmsg="error: server replied " + eString().setNum(http->code) + " " + http->code_descr;
		break;
	case -2:
		errmsg="Can't resolve hostname!";
		break;
	default:
		errmsg.sprintf("unknown error %d", err);
	}
	setStatus(errmsg);
	if (errmsg.length())
	{
		if (current_url.length())
			errmsg+="\n(URL: " + current_url + ")";
		eMessageBox box(errmsg, _("Error!"), eMessageBox::btOK|eMessageBox::iconError);
		box.show();
		box.exec();
		box.hide();
	}
}

eHTTPDataSource *eUpgrade::createCatalogDataSink(eHTTPConnection *conn)
{
	return datacatalog=new eHTTPDownloadXML(conn, *catalog);
}

eHTTPDataSource *eUpgrade::createImageDataSink(eHTTPConnection *conn)
{
	image=new eHTTPDownload(conn, TMP_IMAGE);
	lasttime=0;
	CONNECT(image->progress, eUpgrade::downloadProgress);
	return image;
}

void eUpgrade::downloadProgress(int received, int total)
{
	if ((time(0) == lasttime) && (received != total))
		return;
	lasttime=time(0);
	if (total > 0)
	{
		eString pt;
		int perc=received*100/total;
		pt.sprintf("%d/%d kb (%d%%)", received/1024, total/1024, perc);
		progress->setPerc(perc);
		progresstext->setText(pt);
	} else
	{
		eString pt;
		pt.sprintf("%d kb", received/1024);
		progress->setPerc(0);
		progresstext->setText(pt);
	}
}

void eUpgrade::abortDownload()
{
	if (http)
	{
		delete http;
		http=0;
	}
	progress->hide();
	progresstext->hide();
	abort->hide();
	images->show();
	imagehelp->show();
}

void eUpgrade::flashImage(int checkmd5)
{
	setStatus(_("checking consistency of file..."));
	unsigned char md5[16];
	if (checkmd5 && md5_file (TMP_IMAGE, 1, (unsigned char*) &md5))
	{
		setStatus(_("write error while downloading..."));
		eMessageBox mb(
			_("write error while downloading..."),
			_("Error!"),
			eMessageBox::btOK|eMessageBox::iconError);
		hide();
		mb.show();
		mb.exec();
		mb.hide();
		show();
	} else
	{
		if (checkmd5 && memcmp(md5, expected_md5, 16))
		{
			setStatus(_("Data error. The checksum didn't match."));
			eMessageBox mb(
				_("Data error. The checksum didn't match."),
				_("Error!"),
				eMessageBox::btOK|eMessageBox::iconError);
			hide();
			mb.show();
			mb.exec();
			mb.hide();
			show();
		} else
		{
			setStatus(_("Checksum OK. Ready to upgrade."));
			eMessageBox mb(
				_("Do you really want to upgrade to the new version?"),
				_("Ready to upgrade"),
				eMessageBox::btYes|eMessageBox::btNo|eMessageBox::iconQuestion);
			hide();
			mb.show();
			int res=mb.exec();
			mb.hide();
			if (res == eMessageBox::btYes)
			{
				eMessageBox mb(
					_("Erasing...\nPlease do not switch off box now!"),
					_("upgrade in progress"), eMessageBox::iconInfo);
				mb.show();
				sync();
				int res=system("/bin/eraseall /dev/mtd/0")>>8;
				mb.hide();
				if (!res)
				{
					eMessageBox mb(
						_("Writing software to flash...\nPlease do not switch off box now!"),
						_("upgrade in progress"), eMessageBox::iconInfo);
					mb.show();
					res=system("cat " TMP_IMAGE " > /dev/mtd/0")>>8;
					mb.hide();
					if (!res)
					{
						eMessageBox mb(
							_("upgrade successful!\nrestarting..."),
							_("upgrade ok"),
						eMessageBox::btOK|eMessageBox::iconInfo);
						mb.show();
						mb.exec();
						mb.hide();
						system("/sbin/reboot");
						exit(0);
					}
				}
				if (res)
				{
					eMessageBox mb(
						_("upgrade failed with errorcode UA15"),
						_("upgrade failed"),
						eMessageBox::btOK|eMessageBox::iconError);
					mb.show();
					mb.exec();
					mb.hide();
				}
			} else
				close(0);
		}
	}
}
