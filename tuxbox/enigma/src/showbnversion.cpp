#include "showbnversion.h"

#include <core/driver/rc.h>
#include <core/dvb/edvb.h>
#include <core/dvb/si.h>
#include <core/gui/elabel.h>

/*
	was hier fehlt: parsen der BAT (batid: 5001) auf 0001:0085. daher wird 0001:0085:0F03 assumed.
	ebenfalls: ContentDescriptor, stream_identifier fuer population-zuordnung.
	
	extrahieren und tag anzeigen sowie decompilieren/simulieren um "Starte ..." zu ermitteln.
*/

class BNDirectory: public eSection
{
	int sectionRead(__u8 *d);
	void sectionFinish(int err);
	eLabel *result;
	eString text;
public:
	BNDirectory(int pid, eString text, eLabel *res);
};

int BNDirectory::sectionRead(__u8 *d)
{
	d+=0x117;
	eString versions=text;
	int first=1;
	while (1)
	{
		int l=*d++;
		if (l>0x10)	// ugly
			break;
		eString dst="", file="", dst2="", version="";
		while (l--)
			dst+=*d++;
		l=*d++;
		while (l--)
			file+=*d++;
		l=*d++;
		while (l--)
			dst2+=*d++;
		d+=12;
		l=*d++;
		while (l--)
			version+=*d++;
		d+=17;
		eDebug("%s/%s/%s/%s", dst.c_str(), file.c_str(), dst2.c_str(), version.c_str());
		if (versions.find(version)==-1)
		{
			if (!first)
				versions+=", ";
			first=0;
			versions+=version;
		}
	}
	result->setText(versions);
	return 1;
}

void BNDirectory::sectionFinish(int err)
{
}

BNDirectory::BNDirectory(int pid, eString text, eLabel *res): eSection(pid, 0x80, -1, -1, 0), result(res), text(text)
{
}

void ShowBNVersion::willShow()
{
	eDVB::getInstance()->switchService(0x0F03, 0x0085, 0x0001, -1);
	text->setText("Tuning in transponder...");
}

void ShowBNVersion::willHide()
{
}

int ShowBNVersion::keyUp(int rc)
{
	switch (rc)
	{
	case eRCInput::RC_OK:
	case eRCInput::RC_HELP:
		close(0);
		return 1;
	default:
		return 0;
	}
}

void ShowBNVersion::eventOccured(int event)
{
	if (event==eDVB::eventServiceSwitched)
	{
		if ((eDVB::getInstance()->service_id==0x0F03) &&
				(eDVB::getInstance()->original_network_id==0x0085))
		{
			text->setText("Tuned in transponder.\nReading version information...");
			PMT *pmt=eDVB::getInstance()->getPMT();
			if (pmt)
			{
				int pid[2];
				int n=0;
				for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end() && n<2; ++i)
					pid[n++]=i->elementary_PID;
				
				if (n>=1)
				{
					bnd[1]=new BNDirectory(pid[0], "pop=1: ", res1);
					bnd[1]->start();
				}
				
				if (n>=2)
				{
					bnd[0]=new BNDirectory(pid[1], "pop=2: ", res2);
					bnd[0]->start();
				}
				pmt->unlock();
			}
		}
	}
	if (event==eDVB::eventServiceFailed)
		text->setText("Tune failed. Please do a channelsearch first.");
}

ShowBNVersion::ShowBNVersion(): eWindow(1)
{
	setText("Show current BN version");
	move(ePoint(150, 150));
	resize(eSize(400, 300));
	
	text=new eLabel(this);
	text->move(ePoint(10, 40));
	text->resize(eSize(380, 60));
	
	res1=new eLabel(this);
	res1->move(ePoint(10, 100));
	res1->resize(eSize(380, 30));
	
	res2=new eLabel(this);
	res2->move(ePoint(10, 130));
	res2->resize(eSize(380, 30));
	
	CONNECT(eDVB::getInstance()->eventOccured, ShowBNVersion::eventOccured);
	bnd[0]=0;
	bnd[1]=0;
}

ShowBNVersion::~ShowBNVersion()
{
	delete bnd[0];
	delete bnd[1];
}
