#include <streaminfo.h>

#include <stdlib.h>

#include <lib/system/elock.h>
#include <lib/driver/rc.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/servicemp3.h>
#include <lib/gdi/font.h>
#include <lib/gui/multipage.h>
#include <lib/gui/eskin.h>
#include <lib/gui/guiactions.h>
#include <lib/gui/elabel.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/ebutton.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/service.h>
#include <lib/system/info.h>

eStreaminfo *eStreaminfo::instance;

int eStreaminfo::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if ((event.action == &i_cursorActions->ok))
			close(0);
		else if (event.action == &i_cursorActions->right)
		{
			if (!mp.next())
				descr->setText( lb->goNext()->getHelpText() );
		}
		else if (event.action == &i_cursorActions->left)
		{
			if (!mp.prev())
				descr->setText( lb->goPrev()->getHelpText() );
		}
		else if (event.action == &i_cursorActions->up)
			;
		else if (event.action == &i_cursorActions->down)
			;
		else
			break;
		return 1;
	case eWidgetEvent::execBegin:
		takeFocus();
		lb->invalidate();
		break;
	case eWidgetEvent::execDone:
		releaseFocus();
		break;
	default:
		break;
	}
	return eWindow::eventHandler(event);
}

/* http://www.dvb.org/index.php?id=16 */
struct caids_t  caids[]=
	{{0x0100, 0xFF00, "Seca/Mediaguard (Canal Plus)", 0},
	{0x0200, 0xFF00, "CCETT", 0}, 
	{0x0300, 0xFF00, "MSG MediaServices GmbH", 0}, 
	{0x0400, 0xFF00, "Eurodec", 0}, 
	{0x0500, 0xFF00, "Viaccess (France Telecom)", 0}, 
	{0x0600, 0xFF00, "Irdeto", 0}, 
	{0x0700, 0xFF00, "Jerrold/GI/Motorola", 0}, 
	{0x0800, 0xFF00, "Matra Communication", 0}, 
	{0x0900, 0xFF00, "Videoguard (News Datacom)", 0}, 
	{0x0A00, 0xFF00, "Nokia", 0}, 
	{0x0B00, 0xFF00, "Conax (Norwegian Telekom)", 0}, 
	{0x0C00, 0xFF00, "NTL", 0}, 
	{0x0D00, 0xFF00, "Cryptoworks (Philips)", 0}, 
	{0x0E00, 0xFF00, "Power VU (Scientific Atlanta)", 0}, 
	{0x0F00, 0xFF00, "Sony", 0}, 
	{0x1000, 0xFF00, "Tandberg Television", 0}, 
	{0x1100, 0xFF00, "Thomson", 0}, 
	{0x1200, 0xFF00, "TV/Com", 0}, 
	{0x1300, 0xFF00, "HPT - Croatian Post and Telecommunications", 0}, 
	{0x1400, 0xFF00, "HRT - Croatian Radio and Television", 0}, 
	{0x1500, 0xFF00, "IBM", 0}, 
	{0x1600, 0xFF00, "Nera", 0}, 
	{0x1702, 0xFFFF, "Betacrypt (BetaTechnik) (C)", 0}, 
	{0x1708, 0xFFFF, "Betacrypt 2 (BetaTechnik)", 0}, 
	{0x1722, 0xFFFF, "Betacrypt (BetaTechnik) (D)", 0}, 
	{0x1762, 0xFFFF, "Betacrypt (BetaTechnik) (F)", 0}, 
	{0x1700, 0xFF00, "Betacrypt (BetaTechnik)", 0},
	{0x1800, 0xFF00, "Kudelski SA", 0}, 
	{0x1900, 0xFF00, "Titan Information Systems", 0}, 
	{0x2000, 0xFF00, "Telefonica Servicios Audiovisuales", 0}, 
	{0x2100, 0xFF00, "STENTOR (France Telecom, CNES and DGA)", 0}, 
	{0x2200, 0xFF00, "Scopus Network Technologies", 0}, 
	{0x2300, 0xFF00, "BARCO AS", 0}, 
	{0x2400, 0xFF00, "Starguide Digital Networks", 0}, 
	{0x2500, 0xFF00, "Mentor Data Systems, Inc.", 0}, 
	{0x2600, 0xFF00, "European Broadcasting Union", 0}, 
	{0x4700, 0xFF00, "General Instrument", 0}, 
	{0x4800, 0xFF00, "Telemann", 0}, 
	{0x4900, 0xFF00, "Digital TV Industry Alliance of China", 0}, 
	{0x4A00, 0xFFF0, "Tsinghua TongFang", 0}, 
	{0x4A10, 0xFFF0, "Easycas", 0},
	{0x4A20, 0xFFF0, "AlphaCrypt", 0},
	{0x4A30, 0xFFF0, "DVN Holdings", 0},
	{0x4A40, 0xFFF0, "Shanghai Advanced Digital Technology Co. Ltd.", 0},
	{0x4A50, 0xFFF0, "Shenzhen Kingsky Company (China) Ltd.", 0},
	{0x4A60, 0xFFF0, "@Sky", 0},
	{0x4A70, 0xFFFF, "Dream Multimedia TV (DreamCrypt)", 0},
	{0x4A71, 0xFFFF, "Dream Multimedia TV (High security, 4096bit RSA)", 0},
	{0x4A72, 0xFFFF, "Dream Multimedia TV (Consumer, 48bit)", 0},
	{0x4A73, 0xFFFF, "Dream Multimedia TV (non-DVB)", 0},
	{0x4A74, 0xFFFF, "Dream Multimedia TV (export)", 0},
	{0x4A70, 0xFFF0, "Dream Multimedia TV (other)", 0},
	{0x4A80, 0xFFF0, "THALESCrypt", 0},
	{0x4A90, 0xFFF0, "Runcom Technologies", 0},
	{0x4AA0, 0xFFF0, "SIDSA", 0},
	{0x4AB0, 0xFFF0, "Beijing Compunicate Technology Inc.", 0},
	{0x4AC0, 0xFFF0, "Latens Systems Ltd", 0},
	{0x4AD0, 0xFFFE, "XCrypt Inc.", 0},
	{0x4AD2, 0xFFFE, "Beijing Digital Video Technology Co., Ltd.", 0},
	{0x0000, 0x0000, "other/unknown", 0}};

 unsigned int caids_cnt = sizeof(caids)/sizeof(struct caids_t);

eString eStreaminfo::getCAName(int casysid, int always)
{
	for (unsigned int i=0; i< caids_cnt; ++i)
		if ((casysid & caids[i].mask) == caids[i].value)
		{
			if ((caids[i].flag) && !always)
				return "";
			caids[i].flag=1;
			if (always)
				return caids[i].description;
			eString n;
			for (int a=0; a<4; ++a)
				if (caids[i].mask&(0xF<<((3-a)*4)))
					n+=eString().sprintf("%x", (casysid>>((3-a)*4))&0xF);
				else
					n+="x";
			n+="h ";
			n+=caids[i].description;
			return n;
		}
	return "";
}

#ifndef DISABLE_FILE
class siTags: public eWidget
{
	void willShow();
	eString tagString, descrString;
	bool ok;
	eLabel *tags, *descr;
	void init_siTags(const eService *service);
public:
	siTags(const eService *service, eWidget *parent);
};

static eString getDescription(eString tag)
{
	if (tag == "TALB")
		return _("album");
	else if (tag == "TIT2")
		return _("title");
	else if (tag == "TPE1")
		return _("artist");
	else if (tag == "TCON")
		return _("genre");
	else if (tag == "TDRC")
		return _("year");
	else if (tag == "TCOM")
		return _("composer");
	else if (tag == "TRCK")
		return _("track");
	else if (tag == "TLEN")
		return _("length");
	else
		return tag;
}

siTags::siTags(const eService *service, eWidget *parent)
: eWidget(parent), ok(false)
{
	init_siTags(service);
}
void siTags::init_siTags(const eService *service)
{
	if (!service->id3)
		return;

	tags = new eLabel(this);
	descr = new eLabel(this);
	for (std::map<eString,eString>::const_iterator i(service->id3->getID3Tags().begin());
			i != service->id3->getID3Tags().end(); ++i)
	{
		tagString+=getDescription(i->first)+'\n';
		if ( i->first.find("TLEN") != eString::npos )
		{
			int len = atoi( i->second.c_str() ) / 1000;
			descrString+=eString().sprintf("%d:%02d", len/60, len%60)+'\n';
		}
		else
			descrString+=i->second+'\n';
	}
}

void siTags::willShow()
{
	if ( !ok )
	{
		ok=true;
		tags->move(ePoint(0,0));
		tags->resize( eSize(width()/3, height()) );
		descr->move( ePoint(width()/3, 0) );
		descr->resize( eSize((width()/3)*2, height()) );
		tags->setText(tagString);
		descr->setText(descrString);
	}
	eWidget::willShow();
}
#endif

class siPID: public eWidget
{
	void init_siPID(decoderParameters parms, const eService *service, eWidget *parent);
public:
	siPID(decoderParameters parms, const eService *service, eWidget *parent);
	void redrawWidget();
};

siPID::siPID(decoderParameters parms, const eService *cservice, eWidget *parent)
	:eWidget(parent)
{
	init_siPID(parms, cservice, parent);
}



eString getVidFormat()
{
	eString vformat="n/a";
	FILE *bitstream=0;
	
	if (Decoder::current.vpid!=-1)
		bitstream=fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
		char buffer[100];
		int xres=0, yres=0, aspect=0, framerate=0;
		while (fgets(buffer, 100, bitstream))
		{
			if (!strncmp(buffer, "H_SIZE:  ", 9))
				xres=atoi(buffer+9);
			if (!strncmp(buffer, "V_SIZE:  ", 9))
				yres=atoi(buffer+9);
			if (!strncmp(buffer, "A_RATIO: ", 9))
				aspect=atoi(buffer+9);
			if (!strncmp(buffer, "F_RATE: ", 8))
				framerate=atoi(buffer+8);
		}
		fclose(bitstream);
		vformat.sprintf("%dx%d ", xres, yres);
		switch (aspect)
		{
			case 1:
				vformat+="square"; break;
			case 2:
				vformat+="4:3"; break;
			case 3:
				vformat+="16:9"; break;
			case 4:
				vformat+="20:9"; break;
			default:
				vformat+="Unk"; break;
		}
		switch(framerate)
		{
			case 1:
				vformat+=" 23.976 fps"; break;
			case 2:
				vformat+=" 24 fps"; break;
			case 3:
				vformat+=" 25 fps"; break;
			case 4:
				vformat+=" 29.97 fps"; break;
			case 5:
				vformat+=" 30 fps"; break;
			case 6:
				vformat+=" 50 fps"; break;
			case 7:
				vformat+=" 59.94 fps"; break;
			case 8:
				vformat+=" 80 fps"; break;
			default:
				vformat+="Unk"; break;
		}
	}
	return vformat;
}


	
void siPID::init_siPID(decoderParameters parms, const eService *cservice, eWidget *parent)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return;

	CreateSkinnedLabel("service_name",cservice?(cservice->service_name.c_str()):"--");
	CreateSkinnedLabel("service_provider",((cservice && cservice->dvb)?cservice->dvb->service_provider.c_str():"--"));
	CreateSkinnedLabel("vpid",((Decoder::current.vpid==-1)?eString(_("none")):eString().sprintf("%04xh  (%dd)", Decoder::current.vpid, Decoder::current.vpid)).c_str());
	CreateSkinnedLabel("apid",((Decoder::current.apid==-1)?eString(_("none")):eString().sprintf("%04xh  (%dd)", Decoder::current.apid, Decoder::current.apid)).c_str());
	CreateSkinnedLabel("pcrpid",((Decoder::current.pcrpid==-1)?eString(_("none")):eString().sprintf("%04xh  (%dd)", Decoder::current.pcrpid, Decoder::current.pcrpid)).c_str());
	CreateSkinnedLabel("pmtpid",((Decoder::current.pmtpid==-1)?eString(_("none")):eString().sprintf("%04xh  (%dd)", Decoder::current.pmtpid, Decoder::current.pmtpid)).c_str());
	CreateSkinnedLabel("tpid",((Decoder::current.tpid==-1)?eString(_("none")):eString().sprintf("%04xh  (%dd)", Decoder::current.tpid, Decoder::current.tpid)).c_str());
	CreateSkinnedLabel("vform",getVidFormat().c_str());
	CreateSkinnedLabel("tsid",eString().sprintf("%04xh", sapi->service.getTransportStreamID().get()).c_str());
	CreateSkinnedLabel("onid",eString().sprintf("%04xh", sapi->service.getOriginalNetworkID().get()).c_str());
	CreateSkinnedLabel("sid",eString().sprintf("%04xh", sapi->service.getServiceID().get()).c_str());
	CreateSkinnedLabel("nspace",eString().sprintf("%04xh", sapi->service.getDVBNamespace().get()).c_str());
	BuildSkin("streaminfoPID");
}

void siPID::redrawWidget()
{
}

class siCA: public eWidget
{
	eLabel *availca[2], *usedca[2];
	void init_siCA();
public:
	siCA(eWidget *parent);
};

siCA::siCA(eWidget *parent): eWidget(parent)
{
	init_siCA();
}
void siCA::init_siCA()
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
	if (!sapi)
		return;
	
	int yOffs=0;
	int fs=eSkin::getActive()->queryValue("fontsize", 20);
	availca[0]=new eLabel(this);
	availca[0]->setText(_("supported coding systems:"));
	availca[0]->move(ePoint(10, 0));
	availca[0]->resize(eSize(420, fs+5));
	yOffs+=fs+5;

	eString availcas=_("none");
	clearCA();

	int numsys=0;
	{
		singleLock s(eDVBServiceController::availCALock);
		std::set<int>& availCA = sapi->availableCASystems;
		for (std::set<int>::iterator i(availCA.begin()); i != availCA.end(); ++i)
		{
			if (!numsys)
				availcas="";
			eString caname=eStreaminfo::getInstance()->getCAName(*i, 0);
			if (caname)
			{
				availcas+= caname + "\n";
				if (numsys++>5)
				{
					availcas+="...\n";
					break;
				}
			}
		}
	}

	if (!numsys)
		numsys++;

	availca[1]=new eLabel(this);
	availca[1]->setText(availcas);
	availca[1]->move(ePoint(10, fs*2));
	availca[1]->resize(eSize(420, numsys*fs+fs));
	
	int y=numsys*fs+fs*2+20;

	usedca[0]=new eLabel(this);
	usedca[0]->setText(_("Systems used in service:"));
	usedca[0]->move(ePoint(10, y));
	usedca[0]->resize(eSize(420, fs));

	eString usedcas=_("none");
	
	numsys=0;

	std::set<int>& calist = sapi->usedCASystems;

	for (std::set<int>::iterator i(calist.begin()); i != calist.end(); ++i)
	{
		if (!numsys)
			usedcas="";
		if (numsys++>5)
		{
			usedcas+="...\n";
			break;
		}
		eString caname=eStreaminfo::getInstance()->getCAName(*i, 1);
		usedcas+=eString().sprintf("%04xh:  ", *i) + caname + "\n";
	}
	
	if (!numsys)
		numsys++;

	usedca[1]=new eLabel(this);
	usedca[1]->setText(usedcas);
	usedca[1]->move(ePoint(10, y+50));
	usedca[1]->resize(eSize(420, numsys*fs+fs));
}

eStreaminfo::eStreaminfo(int mode, const eServiceReference &ref, decoderParameters *parms): eWindow(1), statusbar(this)
{
	init_eStreaminfo(mode,ref,parms);
}
void eStreaminfo::init_eStreaminfo(int mode, const eServiceReference &ref, decoderParameters *parms)
{
	if (!instance)
		instance = this;
		
	setText(mode?_("Record mode - read manual"):_("Streaminfo"));
	cmove(ePoint(140, 100));
	cresize(eSize(470, 420));
	eSize s(clientrect.size());
	s.setHeight( s.height() - 40 );
	
	setHelpID(41);

	eService *service=eServiceInterface::getInstance()->addRef(ref);
	
	eWidget *w;
	
	if (ref.type == eServiceReference::idDVB)
	{
		w=new siPID(Decoder::current, service, this);
		w->move(ePoint(0, 0));
		w->resize( s );
		w->hide();
	}
#ifndef DISABLE_FILE
	else if (service && service->id3)
	{
		w=new siTags(service, this);
		w->move(ePoint(0, 0));
		w->resize( s );
		w->hide();
	}
#endif
	else
	{
		w=new eLabel(this);
		w->setText(_("no information is available"));
		((eLabel*)w)->setFlags(eLabel::flagVCenter);
		((eLabel*)w)->setAlign(eTextPara::dirCenter);
		w->move(ePoint(0, 0));
		w->resize( s );
		w->hide();
	}

	mp.addPage(w);

	if ((ref.type == eServiceReference::idDVB) && (!ref.path))
	{
		w=new siCA(this);
		w->move(ePoint(0, 0));
		w->resize( s );
		w->hide();
		mp.addPage(w);

		w=new eWidget(this);
		w->move(ePoint(0, 0));
		w->resize( s );
		w->hide();

		eTransponderWidget *t = 0;
		switch ( eSystemInfo::getInstance()->getFEType() )
		{
			case eSystemInfo::feSatellite:
				t = new eTransponderWidget(w, 0, eTransponderWidget::deliverySatellite);
				break;
			case eSystemInfo::feCable:
				t = new eTransponderWidget(w, 0, eTransponderWidget::deliveryCable);
				break;
			case eSystemInfo::feTerrestrial:
				t = new eTransponderWidget(w, 0, eTransponderWidget::deliveryTerrestrial);
				break;
			default:
				break;
		}
		if (t)
		{
			t->move(ePoint(0, 0));
			t->resize(eSize(clientrect.width(), 200));
			t->load();
			eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();
			if (sapi)
				t->setTransponder(sapi->transponder);
		}

		eWidget *fe=new eFEStatusWidget(w, eFrontend::getInstance());
		fe->move(ePoint(0, 200));
		fe->resize(eSize(clientrect.width(), 100));
		mp.addPage(w);
		
	} else
	{
		for ( int i = 0; i < 2; i++ )
		{
			w=new eLabel(this);
			w->setText(_("no information is available"));
			((eLabel*)w)->setFlags(eLabel::flagVCenter);
			((eLabel*)w)->setAlign(eTextPara::dirCenter);
			w->move(ePoint(0, 0));
			w->resize( s );
			w->hide();
			mp.addPage(w);
		}
	}
	
	mp.first();

	statusbar.loadDeco();
	statusbar.move( ePoint(0, clientrect.height()-40) );
	statusbar.resize( eSize(clientrect.width(), 40) );
	eRect rect = statusbar.getClientRect();

	lb = new eListBox<eListBoxEntryMenu>( &statusbar );
	lb->move( ePoint(rect.width()-50, 2) );
	lb->resize( eSize(45, rect.height()-5) );
	lb->setFlags( eListBoxBase::flagNoPageMovement | eListBoxBase::flagNoUpDownMovement );
	new eListBoxEntryMenu( lb, "1/3", _("Service information (right)"), eTextPara::dirCenter );
	new eListBoxEntryMenu( lb, "2/3", _("Scrambled system information (left, right)"), eTextPara::dirCenter );
	new eListBoxEntryMenu( lb, "3/3", _("Transponder information (left)"), eTextPara::dirCenter );
	descr = new eLabel( &statusbar );
	descr->move( ePoint(0,0) );
	descr->resize( eSize(rect.width() - 50, rect.height()) );
	descr->setText( lb->getCurrent()->getHelpText() );
	descr->setFlags( eLabel::flagVCenter );
	
	if (service)
		eServiceInterface::getInstance()->removeRef(ref);
}

eStreaminfo::~eStreaminfo()
{
}
