#include <time.h>
#include "scan.h"
#include "frontend.h"
#include "si.h"
#include "dvb.h"
#include "enigma.h"

#include "elabel.h"
#include "ewindow.h"
#include "font.h"
#include "eprogress.h"
#include "edvb.h"
#include "rc.h"

tsText::tsText(QString sheadline, QString sbody, eWidget *parent): eWidget(parent, 1)
{
	headline=new eLabel(this);
	headline->setText(sheadline);
	headline->setFont(gFont("NimbusSansL-Regular Sans L Regular", 32));
	body=new eLabel(this, RS_WRAP);
	body->setText(sbody);
}

int tsText::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::changedSize:
		headline->move(ePoint(0, 0));
		headline->resize(eSize(size.width(), 40));

		body->move(ePoint(0, 40));
		body->resize(eSize(size.width(), size.height()-40));
		break;
	}
	return 0;
}

void tsText::keyUp(int rc)
{
	switch(rc)
	{
	case eRCInput::RC_OK:
		close(0);
		break;
	case eRCInput::RC_HELP:
		close(2);
		break;
	}
}

void tsText::redrawWidget()
{
}

tsFindInit::tsFindInit(eWidget *parent): eWidget(parent, 1)
{
	headline=new eLabel(this);
	headline->setText("Empfange");
	headline->setFont(gFont("NimbusSansL-Regular Sans L Regular", 32));
	
	body=new eLabel(this);
	
	signalbar=new eProgress(this);

	packets.setAutoDelete(true);
	
/*	connect(&sstimer, SIGNAL(timeout()), SLOT(showSignalStrength()));
	connect(eFrontend::fe(), SIGNAL(tunedIn(eTransponder*,int)), SLOT(tunedIn(eTransponder*,int)));*/
	CONNECT(sstimer.timeout, tsFindInit::showSignalStrength);
	CONNECT(eFrontend::fe()->tunedIn, tsFindInit::tunedIn);
	state=sInactive;
}

void tsFindInit::scanPacket()
{
	if (!packets.current())
	{
		state=sFailed;
		body->setText("Tja, leider konnte kein einziger Transponder gefunden werden. Vielleicht sollte die Antenne angeschlossen werden..."
		"\nIm Ernst: es tut mir wirklich leid, aber da IST NICHTS.");
		return;
	}
	body->setText("scanning " + packets.current()->name);
	packets.current()->possibleTransponders.front()->tune();
}

int tsFindInit::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::execBegin:
	{
		packets.clear();
		eTransponder *t;
		tpPacket *p;
		result=0;
		int f;
		switch (eFrontend::fe()->Type())
		{
		case eFrontend::feCable:
			p=new tpPacket("Kabel", SCAN_SKIP);
			for (f=330000; f<460000; f+=8000)
			{
				t=new eTransponder(-1, f/8000); t->setCable(f, 6900000);
				p->possibleTransponders.push_back(t);
			}
			t=new eTransponder(-1, -2); t->setCable(330000, 6875000); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -3); t->setCable(460000, 6900000); p->possibleTransponders.push_back(t);
			packets.append(p);
			break;
		case eFrontend::feSatellite:
			p=new tpPacket("Astra 19.2E", SCAN_SKIP);
			t=new eTransponder(0x0009, 0x0085); t->setSatellite(12422000, 27500000, eFrontend::polHor, 3, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(0x0454, 0x0001); t->setSatellite(12551500, 22000000, eFrontend::polVert, 4, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(0x0441, 0x0001); t->setSatellite(12187500, 27500000, eFrontend::polHor, 3, 0); p->possibleTransponders.push_back(t);
			packets.append	(p);

			p=new tpPacket("Hotbird 13.0E", SCAN_ONIT);
			t=new eTransponder(-1, -1); t->setSatellite(12692000, 27500000, eFrontend::polHor, 3, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -2); t->setSatellite(12539000, 27500000, eFrontend::polHor, 3, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -3); t->setSatellite(11746000, 27500000, eFrontend::polHor, 3, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -4); t->setSatellite(12168500, 27500000, eFrontend::polHor, 3, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -5); t->setSatellite(12034000, 27500000, eFrontend::polVert, 3, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -6); t->setSatellite(11919000, 27500000, eFrontend::polVert, 2, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -7); t->setSatellite(11804000, 27500000, eFrontend::polVert, 2, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -8); t->setSatellite(12169000, 27500000, eFrontend::polHor, 3, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -9); t->setSatellite(12539000, 27500000, eFrontend::polHor, 3, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -10); t->setSatellite(12111000, 27500000, eFrontend::polVert, 3, 0); p->possibleTransponders.push_back(t);
			packets.append(p);
	
/*			p=new tpPacket("Astra 24.2E", SCAN_ONIT);
			t=new eTransponder(-1, -1); t->setSatellite(11913000, 27500000, eFrontend::polHor, 3, 0); p->possibleTransponders.push_back(t);
			packets.push_back(p); */
	
			p=new tpPacket("Astra 2 28E", 0);
			t=new eTransponder(-1, -2); t->setSatellite(11954000, 27500000, eFrontend::polHor, 2, 0); p->possibleTransponders.push_back(t);
			t=new eTransponder(-1, -3); t->setSatellite(12051000, 27500000, eFrontend::polVert, 2, 0); p->possibleTransponders.push_back(t);
			packets.append(p);
	
			p=new tpPacket("andere...", SCAN_ONIT);
			t=new eTransponder(-1, -1); t->setSatellite(11042000,  4340000, eFrontend::polHor, 4, 0); p->possibleTransponders.push_back(t); // eut w2 16E
			t=new eTransponder(-1, -2); t->setSatellite(11010000,  2928000, eFrontend::polHor, 4, 0); p->possibleTransponders.push_back(t); // eut w1 10E
			t=new eTransponder(-1, -3); t->setSatellite(11386000, 27500000, eFrontend::polHor, 4, 0); p->possibleTransponders.push_back(t); // eut w3 7E
	
			t=new eTransponder(-1, -4); t->setSatellite(12245000, 27500000, eFrontend::polVert, 5, 0); p->possibleTransponders.push_back(t); // sirius 5E
			t=new eTransponder(-1, -5); t->setSatellite(10974000,  9000000, eFrontend::polVert, 5, 0); p->possibleTransponders.push_back(t); // thor 1w
			t=new eTransponder(-1, -6); t->setSatellite(12245000, 27500000, eFrontend::polVert, 5, 0); p->possibleTransponders.push_back(t); // sirius 5E
			packets.append(p);
			break;
	
		default:
			body->setText("Da geht gerade was gaanz merkwürdiges vor! Komischerweise entspricht dein DVB-Equipment absolut keiner Norm, und daher "
				"kann das schöne EliteDVB sowas merkwürdiges nicht unterstützen! DVB-T? Oder was ist das?");
			break;
		}
		packets.first();
		scanPacket();
		sstimer.start(1000);
		state=sSearching;
		break;
	}
	case eWidgetEvent::changedSize:
		headline->move(ePoint(10, 0));
		headline->resize(eSize(size.width()-20, 40));

		body->move(ePoint(10, 40));
		body->resize(eSize(size.width()-20, 100));
	
		signalbar->move(ePoint(10, 150));
		signalbar->resize(eSize(size.width()-20, 10));
		break;
	case eWidgetEvent::execDone:
		state=sInactive;
		sstimer.stop();
		break;
	}
	return 0;
}

void tsFindInit::tunedIn(eTransponder *trans, int error)
{
	if (!packets.current())
		return;

  static std::list<eTransponder*>::iterator current = packets.current()->possibleTransponders.begin();

	if (trans != *current)
		return;

	if (!error)
	{
		state=sFound;
		body->setFlags(RS_WRAP);
		body->setText("Es wurde ein Transponder gefunden, die Suche kann nun beginnen.");
		result=packets.take();
	}
	else
	{
		eTransponder *n = (++current != packets.current()->possibleTransponders.end()?*current:0);

		if (!n)
		{
			packets.next();
			current = packets.current()->possibleTransponders.begin();
			scanPacket();
		}	
		else
			n->tune();
	}
}

void tsFindInit::showSignalStrength()
{
	int sig=eFrontend::fe()->SNR();
	sig+=50000;
	if (sig<0)
		sig=0;
	sig/=20000;
	sig=100-sig;
	if (sig<0)
		sig=0;
	signalbar->setPerc(sig);
}

void tsFindInit::keyUp(int rc)
{
	switch(rc)
	{
	case eRCInput::RC_OK:
		if (state==sFound)
			close(0);
		if (state==sFailed)
			close(1);
		break;
	case eRCInput::RC_HELP:
		close(2);
		break;
	}
}

void tsFindInit::redrawWidget()
{
}


void tsDoScan::stateChanged(int newstate)
{
}

void tsDoScan::eventOccured(int event)
{
	switch (event)
	{
	case eDVB::eventScanNext:
		updateStats();
		break;
	case eDVB::eventScanCompleted:
	{
		qDebug("SCAN HAS COMPLETED");
		close(0);
	}
	}
}

void tsDoScan::updateETA()
{
	time_t diff=time(0)-start;
	int transponders, scanned, services;
	eDVB::getInstance()->getTransponders()->updateStats(transponders, scanned, services);

	time_t expected;
	if (scanned>4)
		expected=diff*transponders/scanned;
	else
		expected=fexp;
	
	fexp=(fexp*7+expected)/8;
	
	time_t left=fexp-diff;
	if (left<0)
		left=0;
		
	eta->setText(QString().sprintf("Left: %d Minutes and %02d Seconds", (int)left/60, (int)left%60));
}

tsDoScan::tsDoScan(tsFindInit *init, eWidget *parent): eWidget(parent, 1), init(init)
{
	transp_found=new eLabel(this);
	transp_found->move(ePoint(10, 0));
	transp_found->resize(eSize(440, 30));

	transp_scanned=new eLabel(this);
	transp_scanned->move(ePoint(10, 30));
	transp_scanned->resize(eSize(440, 30));

	known_services=new eLabel(this);
	known_services->move(ePoint(10, 60));
	known_services->resize(eSize(440, 30));

	bar=new eProgress(this);
	bar->move(ePoint(10, 90));
	bar->resize(eSize(440, 20));

	eta=new eLabel(this);
	eta->move(ePoint(10, 120));
	eta->resize(eSize(440, 30));

/*	connect(eDVB::getInstance(), SIGNAL(stateChanged(int)), SLOT(stateChanged(int)));
	connect(eDVB::getInstance(), SIGNAL(eventOccured(int)), SLOT(eventOccured(int)));
	connect(&etatimer, SIGNAL(timeout()), SLOT(updateETA()));*/
	CONNECT(eDVB::getInstance()->stateChanged, tsDoScan::stateChanged);
	CONNECT(eDVB::getInstance()->eventOccured, tsDoScan::eventOccured);
	CONNECT(etatimer.timeout, tsDoScan::updateETA);
}

int tsDoScan::eventFilter(const eWidgetEvent &event)
{
	switch(event.type)
	{
	case eWidgetEvent::execBegin:
		start=time(0);
		eDVB::getInstance()->startScan(init->result->possibleTransponders, init->result->scanflags);
		delete init->result;
		fexp=3*60;
		etatimer.start(1000);

		break;
	case eWidgetEvent::execDone:
		etatimer.stop();
		break;
	}
	return 0;
}

void tsDoScan::updateStats()
{
	int transponders, scanned, services;
	eDVB::getInstance()->getTransponders()->updateStats(transponders, scanned, services);
	transp_found->setText(QString().sprintf("%d\ttransponders found", transponders));
	transp_scanned->setText(QString().sprintf("%d\ttransponders scanned", scanned));
	known_services->setText(QString().sprintf("%d\tknown services", services));
	if (transponders)
		bar->setPerc(scanned*100/transponders);
}

void tsDoScan::redrawWidget()
{
}


TransponderScan::TransponderScan()
{
	window=new eWindow(0);
	window->setText("Transponder Scan");
	window->move(ePoint(100, 100));
	window->resize(eSize(460, 300));
	
	progress=new eProgress(window);
	progress->move(ePoint(60, window->getClientSize().height()-25));
	progress->resize(eSize(window->getClientSize().width()-70, 10));

	progress_text=new eLabel(window);
	progress_text->move(ePoint(0, window->getClientSize().height()-30));
	progress_text->resize(eSize(50, 30));

	eWidget *s=new tsText("Kanalsuche", "Willkommen bei der Kanalsuche.\n"
			"OK zum fortfahren, ? zum Abbruch", window);
	s->move(ePoint(0, 0));
	s->resize(eSize(
			window->getClientSize().width(), window->getClientSize().height()-30));
	s->hide();
	mp.addPage(s);

	s=new tsFindInit(window);
	s->move(ePoint(0, 0));
	s->resize(eSize(window->getClientSize().width(), window->getClientSize().height()-30));
	s->hide();
	mp.addPage(s);

	s=new tsDoScan((tsFindInit*)s, window);
	s->move(ePoint(0, 0));
	s->resize(eSize(window->getClientSize().width(), window->getClientSize().height()-30));
	s->hide();
	mp.addPage(s);
	s=new tsText("ÜBERSTANDEN!",
		"Herzlichen Glückwunsch!\n"
		"Sie haben es heil überstanden!\n\n"
		"Es erwartet sie nun eine schier unglaubliche\nProgrammnvielfalt! "
		"(wenn denn beim Scan alles geklappt hat. Wenn nicht: Don't worry, "
		"be happy und: Auf ein neues, wenn es wieder heisst:\nStarte V1.6!)"
		, window);
	s->move(ePoint(0, 0));
	s->resize(eSize(
			window->getClientSize().width(), window->getClientSize().height()-30));
	s->hide();
	mp.addPage(s);
}

TransponderScan::~TransponderScan()
{
	delete window;
}

int TransponderScan::exec()
{
	int ret;
	window->show();
	mp.first();

	ret=0;
	do
	{
		int n=mp.at();
		int total=mp.count();
		progress_text->setText(QString().sprintf("%d/%d", n+1, total));
		if (total<2)
			total=2;
		total--;
		progress->setPerc(n*100/total);
		int res=mp.current()->exec();
		if (res==1)
		{
			mp.prev();
			continue;
		}
		
		if (res==2)
		{
			ret=-1;
			break;
		}
	} while (!mp.next());
	mp.current()->hide();
	window->hide();
	return ret;
}
