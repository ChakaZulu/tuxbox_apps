#include <stdlib.h>
#include "streaminfo.h"
#include "rc.h"
#include "dvb.h"
#include "edvb.h"
#include "elabel.h"
#include "decoder.h"
#include "multipage.h"
#include "eskin.h"

int eStreaminfo::keyUp(int code)
{
	switch (code)
	{
	case eRCInput::RC_OK:
	case eRCInput::RC_HELP:
		close(0);
		return 1;
	default:
		return 0;
	}
}

int eStreaminfo::keyDown(int code)
{
	switch (code)
	{
	case eRCInput::RC_RIGHT:
		mp.next();
		return 1;
	case eRCInput::RC_LEFT:
		mp.prev();
		return 1;
	default:
		return 0;
	}
}

static eString getCAName(int casysid)
{
	eString system, subsystem="";
	switch (casysid&0xFF00)
	{
	case 0x0100: system="Seca/Mediaguard (Canal+)"; break;
	case 0x0200: system="CCETT"; break;
	case 0x0300: system="MSG"; break;
	case 0x0400: system="Eurodec"; break;
	case 0x0500: system="Viaccess (France Telekom)"; break;
	case 0x0600: system="Irdeto"; break;
	case 0x0700: system="Jerrold/GI/Motorola"; break;
	case 0x0800: system="Matra"; break;
	case 0x0900: system="Videoguard (NDS)"; break;
	case 0x0A00: system="Nokia"; break;
	case 0x0B00: system="Conax (Norwegian Telekom)"; break;
	case 0x0C00: system="NTL"; break;
	case 0x0D00: system="Cryptoworks (Philips)"; break;
	case 0x0E00: system="Power VU (Scientific Atlanta)"; break;
	case 0x0F00: system="Sony"; break;
	case 0x1000: system="Tandberg Television"; break;
	case 0x1100: system="Thomson"; break;
	case 0x1200: system="TV/Com"; break;
	case 0x1300: system="HPT"; break;
	case 0x1400: system="HRT"; break;
	case 0x1500: system="IBM"; break;
	case 0x1600: system="Nera"; break;
	case 0x1700: system="Betacrypt (Beta Technik)";
		switch (casysid)
		{
		case 0x1702: subsystem=" (C)"; break;
		case 0x1722: subsystem=" (D)"; break;
		case 0x1762: subsystem=" (F)"; break;
		}
		break;
	case 0x1800: system="Kudelski SA"; break;
	case 0x1900: system="Titan Information Systems"; break;
	case 0x2000: system="Telefonica"; break;
	case 0x2100: system="STENTOR"; break;
	case 0x2200: system="Tadiran Scopus"; break;
	case 0x2300: system="BARCO AS"; break;
	case 0x2400: system="Starguide Digital Networks"; break;
	case 0x2500: system="Mentor Data Systems, Inc."; break;
	case 0x2600: system="EBU"; break;
	case 0x4700: system="General Instruments"; break;
	case 0x4800: system="Telemann"; break;
	case 0x4900: system="Digital TV Industry Alliance of China"; break;
	case 0x4A00: system="Tsinghua TongFang"; break;
	default:     system="??"; break;
	}
	return system+subsystem;
}

class siPID: public eWidget
{
	eLabel *service_name[2], *service_provider[2], *apid[2], *vpid[2], *pcrpid[2], *tpid[2], *vform[2], *tsid[2], *onid[2], *sid[2];
public:
	siPID(decoderParameters parms, eWidget *parent);
	void redrawWidget();
};

siPID::siPID(decoderParameters parms, eWidget *parent): eWidget(parent)
{
	eService *cservice=eDVB::getInstance()->service;
	int yOffs=10;
	int fs=eSkin::getActive()->queryValue("fontsize", 20);
	
	service_name[0]=new eLabel(this);
	service_name[0]->setText("Name:");
	service_name[0]->move(ePoint(10, yOffs));
	service_name[0]->resize(eSize(140, fs+5));

	service_name[1]=new eLabel(this);
	service_name[1]->setText(cservice?(cservice->service_name.c_str()):"--");
	service_name[1]->setFont(gFont("NimbusSansL-Regular Sans L Regular", fs));
	service_name[1]->move(ePoint(185, yOffs+2));
	service_name[1]->resize(eSize(260, fs+5));
	yOffs+=fs+5;

	service_provider[0]=new eLabel(this);
	service_provider[0]->setText("Provider:");
	service_provider[0]->move(ePoint(10, yOffs));
	service_provider[0]->resize(eSize(140, fs+5));
	
	service_provider[1]=new eLabel(this);
	service_provider[1]->setText(cservice?cservice->service_provider.c_str():"--");
	service_provider[1]->setFont(gFont("NimbusSansL-Regular Sans L Regular", fs));
	service_provider[1]->move(ePoint(185, yOffs+2));
	service_provider[1]->resize(eSize(260, fs+5));
	yOffs+=fs+5;

	vpid[0]=new eLabel(this);
	vpid[0]->setText("Video PID:");
	vpid[0]->move(ePoint(10, yOffs));
	vpid[0]->resize(eSize(140, fs+5));
	
	vpid[1]=new eLabel(this);
	vpid[1]->setFont(gFont("NimbusSansL-Regular Sans L Regular", fs));
	vpid[1]->setText((parms.vpid==-1)?eString("keine"):eString().sprintf("%04xh  (%dd)", parms.vpid, parms.vpid));
	vpid[1]->move(ePoint(185, yOffs+2));
	vpid[1]->resize(eSize(260, fs+5));
	yOffs+=fs+5;

	apid[0]=new eLabel(this);
	apid[0]->setText("Audio PID:");
	apid[0]->move(ePoint(10, yOffs));
	apid[0]->resize(eSize(140, fs+5));
	
	apid[1]=new eLabel(this);
	apid[1]->setText((parms.apid==-1)?eString("keine"):eString().sprintf("%04xh  (%dd)", parms.apid, parms.apid));
	apid[1]->move(ePoint(185, yOffs+2));
	apid[1]->resize(eSize(260, fs+5));
	apid[1]->setFont(gFont("NimbusSansL-Regular Sans L Regular", fs));
	yOffs+=fs+5;

	pcrpid[0]=new eLabel(this);
	pcrpid[0]->setText("PCR PID:");
	pcrpid[0]->move(ePoint(10, yOffs));
	pcrpid[0]->resize(eSize(140, fs+5));
	
	pcrpid[1]=new eLabel(this);
	pcrpid[1]->setText((parms.pcrpid==-1)?eString("keine"):eString().sprintf("%04xh  (%dd)", parms.pcrpid, parms.pcrpid));
	pcrpid[1]->move(ePoint(185, yOffs+2));
	pcrpid[1]->resize(eSize(260, fs+5));
	pcrpid[1]->setFont(gFont("NimbusSansL-Regular Sans L Regular", fs));
	yOffs+=fs+5;

	tpid[0]=new eLabel(this);
	tpid[0]->setText("Teletext PID:");
	tpid[0]->move(ePoint(10, yOffs));
	tpid[0]->resize(eSize(140, fs+5));
	
	tpid[1]=new eLabel(this);
	tpid[1]->setText((parms.tpid==-1)?eString("keine"):eString().sprintf("%04xh  (%dd)", parms.tpid, parms.tpid));
	tpid[1]->move(ePoint(185, yOffs+2));
	tpid[1]->resize(eSize(260, fs+5));
	tpid[1]->setFont(gFont("NimbusSansL-Regular Sans L Regular", fs));
	yOffs+=fs+5;
	
	eString vformat="n/a";
	FILE *bitstream=0;
	
	if (parms.vpid!=-1)
		bitstream=fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
		char buffer[100];
		int xres=0, yres=0, aspect=0;
		while (fgets(buffer, 100, bitstream))
		{
			if (!strncmp(buffer, "H_SIZE:  ", 9))
				xres=atoi(buffer+9);
			if (!strncmp(buffer, "V_SIZE:  ", 9))
				yres=atoi(buffer+9);
			if (!strncmp(buffer, "A_RATIO: ", 9))
				aspect=atoi(buffer+9);
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
		}
	}
	
	vform[0]=new eLabel(this);
	vform[0]->setText("Videoformat:");
	vform[0]->move(ePoint(10, yOffs));
	vform[0]->resize(eSize(150, fs+5));
	
	vform[1]=new eLabel(this);
	vform[1]->setText(vformat);
	vform[1]->move(ePoint(185, yOffs));
	vform[1]->resize(eSize(260, fs));
	vform[1]->setFont(gFont("NimbusSansL-Regular Sans L Regular", fs));
	yOffs+=fs+5;

	tsid[0]=new eLabel(this);
	tsid[0]->setText("Transport Stream ID:");
	tsid[0]->move(ePoint(10, yOffs));
	tsid[0]->resize(eSize(230, fs+5));

	tsid[1]=new eLabel(this);
	tsid[1]->setText(eString().sprintf("%04xh", eDVB::getInstance()->transport_stream_id));
	tsid[1]->move(ePoint(280, yOffs));
	tsid[1]->resize(eSize(130, fs+5));
	tsid[1]->setFont(gFont("NimbusSansL-Regular Sans L Regular", fs));
	yOffs+=fs+5;

	onid[0]=new eLabel(this);
	onid[0]->setText("Original Network ID:");
	onid[0]->move(ePoint(10, yOffs));
	onid[0]->resize(eSize(210, fs+5));

	onid[1]=new eLabel(this);
	onid[1]->setText(eString().sprintf("%04xh", eDVB::getInstance()->original_network_id));
	onid[1]->move(ePoint(280, yOffs));
	onid[1]->resize(eSize(130, fs+5));
	onid[1]->setFont(gFont("NimbusSansL-Regular Sans L Regular", fs));
	yOffs+=fs+5;

	sid[0]=new eLabel(this);
	sid[0]->setText("Service ID:");
	sid[0]->move(ePoint(10, yOffs));
	sid[0]->resize(eSize(185, fs+5));

	sid[1]=new eLabel(this);
	sid[1]->setText(eString().sprintf("%04xh", eDVB::getInstance()->service_id));
	sid[1]->move(ePoint(280, yOffs));
	sid[1]->resize(eSize(130, fs+5));
	sid[1]->setFont(gFont("NimbusSansL-Regular Sans L Regular", fs));
}

void siPID::redrawWidget()
{
}

class siCA: public eWidget
{
	eLabel *availca[2], *usedca[2];
public:
	siCA(eWidget *parent);
	void redrawWidget();
};

siCA::siCA(eWidget *parent): eWidget(parent)
{
	int yOffs=0;
	int fs=eSkin::getActive()->queryValue("fontsize", 20);
	availca[0]=new eLabel(this);
	availca[0]->setText("Unterstützte Verschlüsselungssyst.:");
	availca[0]->move(ePoint(10, 0));
	availca[0]->resize(eSize(420, fs+5));
	yOffs+=fs+5;

	eString availcas="keine";
	
	int numsys=0;
	std::list<int>& availCA = eDVB::getInstance()->availableCASystems;
	for (std::list<int>::iterator i(availCA.begin()); i != availCA.end(); ++i)
	{
		if (!numsys)
			availcas="";
		if (numsys++>5)
		{
			availcas+="...\n";
			break;
		}
		availcas+= eString().sprintf( "%04xh:  %s\n", *i, getCAName(*i).c_str());
	}
	if (!numsys)
		numsys++;

	availca[1]=new eLabel(this);
	availca[1]->setText(availcas);
	availca[1]->move(ePoint(10, fs*2));
	availca[1]->resize(eSize(420, numsys*fs+fs));
	
	int y=numsys*fs+fs*2+20;

	usedca[0]=new eLabel(this);
	usedca[0]->setText("davon benutzt:");
	usedca[0]->move(ePoint(10, y));
	usedca[0]->resize(eSize(420, fs));

	eString usedcas="keines";
	
	numsys=0;

	ePtrList<eDVB::CA>& calist = eDVB::getInstance()->calist;

	for (ePtrList<eDVB::CA>::iterator i(calist); i != calist.end(); ++i)
	{
		if (!numsys)
			usedcas="";
		if (numsys++>5)
		{
			usedcas+="...\n";
			break;
		}
		usedcas+=eString().sprintf("%04xh:  ", (*i)->casysid)+getCAName((*i)->casysid) + "\n";
	}
	
	if (!numsys)
		numsys++;

	usedca[1]=new eLabel(this);
	usedca[1]->setText(usedcas);
	usedca[1]->move(ePoint(10, y+50));
	usedca[1]->resize(eSize(420, numsys*fs+fs));
}

void siCA::redrawWidget()
{
}

eStreaminfo::eStreaminfo(int mode, decoderParameters *parms): eWindow(1)
{
	setText(mode?"Record mode - read manual":"Streaminfo");
	cmove(ePoint(100, 80));
	cresize(eSize(450, 450));
	
	eWidget *w=new siPID(parms?*parms:Decoder::parms, this);
	w->move(ePoint(0, 0));
	w->resize(clientrect.size());
	w->hide();

	mp.addPage(w);

	w=new siCA(this);
	w->move(ePoint(0, 0));
	w->resize(clientrect.size());
	w->hide();

	mp.addPage(w);
	mp.first();
}
