#include "rc.h"
#include "showbnversion.h"
#include "edvb.h"
#include "elabel.h"
#include "si.h"
#include "enigma.h"

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
	QString text;
public:
	BNDirectory(int pid, QString text, eLabel *res);
};

int BNDirectory::sectionRead(__u8 *d)
{
	d+=0x117;
	QString versions=text;
	int first=1;
	while (1)
	{
		int l=*d++;
		if (l>0x10)	// ugly
			break;
		QString dst="", file="", dst2="", version="";
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
		qDebug("%s/%s/%s/%s", (const char*)dst, (const char*)file, (const char*)dst2, (const char*)version);
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

BNDirectory::BNDirectory(int pid, QString text, eLabel *res): eSection(pid, 0x80, -1, -1, 0), result(res), text(text)
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

void ShowBNVersion::keyUp(int rc)
{
	switch (rc)
	{
	case eRCInput::RC_OK:
	case eRCInput::RC_HELP:
		close(0);
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
				for (QListIterator<PMTEntry> i(pmt->streams); i.current() && n<2; ++i)
					pid[n++]=i.current()->elementary_PID;
				
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
	move(QPoint(150, 150));
	resize(QSize(400, 300));
	
	text=new eLabel(this);
	text->move(QPoint(10, 40));
	text->resize(QSize(380, 60));
	
	res1=new eLabel(this);
	res1->move(QPoint(10, 100));
	res1->resize(QSize(380, 30));
	
	res2=new eLabel(this);
	res2->move(QPoint(10, 130));
	res2->resize(QSize(380, 30));
	
	connect(eDVB::getInstance(), SIGNAL(eventOccured(int)), SLOT(eventOccured(int)));
	bnd[0]=0;
	bnd[1]=0;
}

ShowBNVersion::~ShowBNVersion()
{
	delete bnd[0];
	delete bnd[1];
}
