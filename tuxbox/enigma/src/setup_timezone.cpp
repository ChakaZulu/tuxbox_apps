#include <setup_timezone.h>

#include <lib/gui/slider.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/elabel.h>
#include <lib/gui/combobox.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/enumber.h>
#include <lib/gui/eskin.h>
#include <lib/gui/actions.h>
#include <lib/system/econfig.h>
#include <enigma_main.h>

eZapTimeZoneSetup::eZapTimeZoneSetup() : eWindow(0)
{
	setText(_("Time Zone Setup"));
	cmove(ePoint(110, 146));
	cresize(eSize(530, 270));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	ltimeZone=new eLabel(this);
	ltimeZone->move(ePoint(20, 20));
	ltimeZone->resize(eSize(220, fd+4));
	ltimeZone->setText(_("Time Zone:"));
		
	timeZone=new eComboBox(this, 8, ltimeZone);
	timeZone->move(ePoint(20, 60));
	timeZone->resize(eSize(clientrect.width()-40, 35));
	timeZone->setHelpText(_("select your time zone (ok)"));
	timeZone->loadDeco();
	
	errLoadTimeZone = loadTimeZones();
	
	int cuseDst;
	if ( eConfig::getInstance()->getKey("/elitedvb/useDst", cuseDst) )
		cuseDst=1;

	useDst=new eCheckbox(this);
	useDst->setName("useDst");
	useDst->setText(_("use automatically daylight saving time"));
	useDst->move(ePoint(20, 110));
	useDst->resize(eSize(clientrect.width()-40, 35));
	useDst->setHelpText(_("Automatically adjust clock for daylight saving changes"));
	useDst->setCheck(cuseDst);

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(20, clientrect.height()-100));
	ok->resize(eSize(220, 40));
	ok->setHelpText(_("save changes and return"));
	ok->loadDeco();
	CONNECT(ok->selected, eZapTimeZoneSetup::okPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-50 ) );
	statusbar->resize( eSize( clientrect.width(), 50) );
	statusbar->loadDeco();

	setHelpID(90);
}

struct delString
{
	delString()
	{
	}

	bool operator()(eListBoxEntryText &e)
	{
		delete (eString*)(e.getKey());
		return false;
	}
};

eZapTimeZoneSetup::~eZapTimeZoneSetup()
{
	timeZone->forEachEntry(delString());
}

void eZapTimeZoneSetup::okPressed()
{
	if (!errLoadTimeZone)
	{
		// save current selected time zone
		if ( eConfig::getInstance()->setKey("/elitedvb/timezone", ((eString*) timeZone->getCurrent()->getKey())->c_str()))
		{
			eConfig::getInstance()->delKey("/elitedvb/timezone");
			eDebug("Write timezone with error %i", eConfig::getInstance()->setKey("/elitedvb/timezone", ((eString*) timeZone->getCurrent()->getKey())->c_str()));
		}
		if ( eConfig::getInstance()->setKey("/elitedvb/useDst", useDst->isChecked()))
		{
			eConfig::getInstance()->delKey("/elitedvb/timezone");
			eDebug("Write timezone with error %i", eConfig::getInstance()->setKey("/elitedvb/useDst", useDst->isChecked()));
		}
		eConfig::getInstance()->flush();
		setTimeZone();                         
		/* emit */ eDVB::getInstance()->timeUpdated();
	}
	close(0);
}

void eZapTimeZoneSetup::setTimeZone()
{
	const char *ctimeZone;
	ctimeZone = cmdTimeZones();
	if (ctimeZone!="")
		setenv("TZ", ctimeZone, 1);
	eDebug("setenv TZ=%s", ctimeZone);
}

int eZapTimeZoneSetup::loadTimeZones()
{
	XMLTreeParser parser("ISO-8859-1");
	int done=0;
	const char *filename="/etc/timezone.xml";

	FILE *in=fopen(filename, "rt");
	if (!in)
	{
		eWarning("unable to open %s", filename);
		return -1;
	}
	do
	{
		char buf[2048];
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser.Parse(buf, len, done))
		{
			eDebug("parse error: %s at line %d",
				parser.ErrorString(parser.GetErrorCode()),
				parser.GetCurrentLineNumber());
			fclose(in);
			return -1;
		}
	} while (!done);
	fclose(in);

	XMLTreeNode *root=parser.RootNode();
	if (!root)
		return -1;

	char *ctimeZone;
	if ( eConfig::getInstance()->getKey("/elitedvb/timezone", ctimeZone) )
		ctimeZone=0;

	eListBoxEntryText *cur=0;
	for (XMLTreeNode *node = root->GetChild(); node; node = node->GetNext())
		if (!strcmp(node->GetType(), "zone"))
		{
			const char *name=node->GetAttributeValue("name");
			if (!name)
			{
				eFatal("error in a file timezone.xml, no name timezone");
				return -1;
			}
			eListBoxEntryText *tz=new eListBoxEntryText( *timeZone, name, (void*) new eString(name) );
			if ( ctimeZone && !strcmp(ctimeZone, name) )
			{
				cur=tz;
			}
		} else
			eFatal("error in a file timezone.xml, unknown timezone");	

	if ( timeZone->setCurrent(cur) == eComboBox::E_INVALID_ENTRY )
		timeZone->setCurrent(27);  // GMT+1

	free(ctimeZone);

	return 0;
}

const char *eZapTimeZoneSetup::cmdTimeZones()
{
	XMLTreeParser parser("ISO-8859-1");
	int done=0;
	const char *filename="/etc/timezone.xml";

	FILE *in=fopen(filename, "rt");
	if (!in)
	{
		eWarning("unable to open %s", filename);
		return "";
	}
	do
	{
		char buf[2048];
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser.Parse(buf, len, done))
		{
			eDebug("parse error: %s at line %d",
				parser.ErrorString(parser.GetErrorCode()),
				parser.GetCurrentLineNumber());
			fclose(in);
			return "";
		}
	} while (!done);
	fclose(in);

	XMLTreeNode *root=parser.RootNode();
	if (!root)
		return "";

	char *ctimeZone;
	if ( eConfig::getInstance()->getKey("/elitedvb/timezone", ctimeZone) )
		ctimeZone=0;

	int cuseDst;
	if ( eConfig::getInstance()->getKey("/elitedvb/useDst", cuseDst) )
		cuseDst=1;

	for (XMLTreeNode *node = root->GetChild(); node; node = node->GetNext())
		if (!strcmp(node->GetType(), "zone"))
		{
			const char *name=node->GetAttributeValue("name"),
					*zone=node->GetAttributeValue("zone"),
					*dst=node->GetAttributeValue("dst");
			if (!zone)
			{
				eFatal("error in a file timezone.xml, no name timezone");
				return "";
			}
			if ( ctimeZone && !strcmp(ctimeZone, name) )
			{
				if (cuseDst)
					return eString().sprintf("%s%s",zone,dst).c_str();
				else
					return eString().sprintf("%s",zone).c_str();
			}
		} 
		else
			eFatal("error in a file timezone.xml, unknown timezone");
	free(ctimeZone);
	
	return "";
}
