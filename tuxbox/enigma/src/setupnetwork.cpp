#ifndef DISABLE_NETWORK

#include <setupnetwork.h>

#include <netinet/in.h>
#include <linux/route.h>

#ifndef DISABLE_NFS
#include <sys/mount.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <lib/gui/multipage.h>
#include <lib/gui/emessage.h>
#include <lib/base/console.h>
#endif

#ifdef USE_IFUPDOWN
// in Makefile.am INCLUDES @NET_CFLAGS@
// in configure.ac TUXBOX_APPS_LIB_PKGCONFIG(NET,tuxbox-net)
#include <sys/types.h>
#include <sys/wait.h>
#include <network_interfaces.h> /* getInetAttributes, setInetAttributes */
#endif

#include <enigma.h>
#include <lib/base/i18n.h>
#include <lib/dvb/edvb.h>
#include <lib/gui/elabel.h>
#include <lib/gui/enumber.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/textinput.h>
#include <lib/gui/combobox.h>
#include <lib/gui/eskin.h>
#include <lib/driver/rc.h>
#include <lib/system/econfig.h>

#ifdef ENABLE_PPPOE

class eTOnlineDialog: public eWindow
{
	eTextInputField *Kennung, *tOnlineNummer, *Mitbenutzer;
	eButton *ok;
	eStatusBar *sbar;
public:
	eTOnlineDialog( eString Login )
	{
		init_eTOnlineDialog(Login);
	}
	void eTOnlineDialog::init_eTOnlineDialog(eString Login)
	{
		setText("T - Online");
		cmove(ePoint(140,140));
		cresize(eSize(450,270));

		eLabel *l = new eLabel(this);
		l->move(ePoint(10,10));
		l->resize(eSize(220,30));
		l->setText("Anschlusskennung:");

		Kennung = new eTextInputField(this,l);
		Kennung->move(ePoint(230,10));
		Kennung->resize(eSize(200,35));
		Kennung->setMaxChars(12);
		Kennung->setUseableChars("1234567890");
		Kennung->loadDeco();
		Kennung->setHelpText("Anschlusskennung eingeben mit OK (12 Stellen)");
		Kennung->setEditHelpText("Anschlusskennung eingeben (0..9, ok)");

		l = new eLabel(this);
		l->move(ePoint(10,60));
		l->resize(eSize(220,30));
		l->setText("T-Online Nummer:");

		tOnlineNummer = new eTextInputField(this,l);
		tOnlineNummer->move(ePoint(230,60));
		tOnlineNummer->resize(eSize(200,35));
		tOnlineNummer->setMaxChars(12);
		tOnlineNummer->setUseableChars("1234567890");
		tOnlineNummer->loadDeco();
		tOnlineNummer->setHelpText("T-Online Nummer eingeben mit OK (12 Stellen)");
		tOnlineNummer->setEditHelpText("T-Online Nummer eingeben (0..9, ok)");

		l = new eLabel(this);
		l->move(ePoint(10,110));
		l->resize(eSize(220,30));
		l->setText("Mitbenutzernummer:");

		Mitbenutzer = new eTextInputField(this,l);
		Mitbenutzer->move(ePoint(230,110));
		Mitbenutzer->resize(eSize(70,35));
		Mitbenutzer->setMaxChars(4);
		Mitbenutzer->setUseableChars("1234567890");
		Mitbenutzer->loadDeco();
		Mitbenutzer->setHelpText("Mitbenutzernummer eingeben mit OK (4 Stellen)");
		Mitbenutzer->setEditHelpText("Mitbenutzernummer eingeben (0..9, ok)");

		ok = new eButton(this);
		ok->move(ePoint(10,160));
		ok->resize(eSize(170,40));
		ok->setShortcut("green");
		ok->setShortcutPixmap("green");
		ok->setText("speichern");
		ok->setHelpText("Daten �bernehmen und Fenster schliessen");
		ok->loadDeco();
		CONNECT(ok->selected, eWidget::accept);

		sbar = new eStatusBar(this);
		sbar->move( ePoint(0, clientrect.height()-50) );
		sbar->resize( eSize( clientrect.width(), 50) );
		sbar->loadDeco();

		if (Login)
		{
			unsigned int pos1 = Login.find("#"),
									pos2 = Login.find("@");
			if ( pos1 != eString::npos && pos2 != eString::npos )
			{
				Kennung->setText(Login.left(12));
				tOnlineNummer->setText(Login.mid(12,12));
				Mitbenutzer->setText(Login.mid(pos1+1,4));
			}
		}
	}
	eString getLogin()
	{
		eString tmp =
			Kennung->getText() +
			tOnlineNummer->getText() + '#' +
			Mitbenutzer->getText() +
			"@t-online.de";
		return tmp;
	}
};

static bool readSecretString( eString &str )
{
	FILE *f = fopen("/etc/ppp/pap-secrets", "r");
	if (!f)
		return false;

	char buf[100];

	size_t readed=
		fread(buf, 1, sizeof(buf), f );
	if ( !readed )
		return false;

	fclose(f);
	str.assign( buf, readed );
	str.removeChars(' ');
	str.removeChars('\t');
	str.removeChars('\n');
	str.removeChars('\"');
	return true;
}

static void writeSecretString( const eString &str )
{
	FILE *f = fopen("/etc/ppp/pap-secrets", "w");
	if (!f)
		eFatal("couldn't create /etc/ppp/pap-secrets");
	eString tmp;
	unsigned int pos =
		str.find('*');
	if ( pos != eString::npos )
	{
		tmp = '\"' + str.left(pos) + "\"\t*\t\"" +
			str.mid(pos+1, str.length()-pos ) + "\"\n";
	}
	fwrite( tmp.c_str(), 1, tmp.length(), f );

	fclose(f);
}

static void helper( char *&source, char *&dest, uint &spos, uint &dpos, const char* option, const char* value )
{
	char *p = strstr( source+spos, option );
	if( !p )
		eDebug("couldn't find '%s' Option", option);
	else
	{
		p+=strlen(option);
		int cnt = p - (source+spos);
		memcpy( dest+dpos, source+spos, cnt );
		dpos += cnt;
		spos += cnt;
		cnt = strlen(value);
		memcpy( dest+dpos, value, cnt );
		dpos+=cnt;
		p = strchr( source+spos, '\n' );
		if ( p )
		{
			spos = p - source;
			++spos;
		}
	}
}

static char *getOptionString( char *buf, const char *option )
{
	char *p = strstr( buf, option );
	if( !p )
		eDebug("couldn't find '%s' Option", option);
	else
		p+=strlen(option);
	return p;
}

static int getRejectFlags()
{
	char buf[8192];
	FILE *f = fopen("/etc/ppp/pppoe.conf", "r" );
	if ( !f )
	{
		eDebug("couldn't open '/etc/ppp/pppoe.conf' for read");
		return 0;
	}
	size_t readed = fread(buf, 1, sizeof(buf), f );
	if ( !readed )
	{
		eDebug("couldn't read '/etc/ppp/pppoe.conf'");
		return 0;
	}
	int flags=0;
	char *p = getOptionString(buf, "REJECT_WWW=");
	if ( p && !strncasecmp(p,"yes",3) )
		flags|=1;
	p = getOptionString(buf, "REJECT_TELNET=");
	if ( p && !strncasecmp(p,"yes",3) )
		flags|=2;
	p = getOptionString(buf, "REJECT_SAMBA=");
	if ( p && !strncasecmp(p,"yes",3) )
		flags|=4;
	p = getOptionString(buf, "REJECT_FTP=");
	if ( p && !strncasecmp(p,"yes",3) )
		flags|=8;
	fclose(f);
	return flags;
}

static void updatePPPConfig( const eString &secrets, int flags )
{
	char sourceA[8192];  // source buffer
	char destA[8192]; // dest buffer
	char *source = sourceA;
	char *dest = destA;

	FILE *f = fopen("/etc/ppp/pppoe.conf", "r" );
	if ( !f )
	{
		eDebug("couldn't open '/etc/ppp/pppoe.conf' for read");
		return;
	}
	size_t readed = fread(source, 1, sizeof(sourceA), f );
	if ( !readed )
	{
		eDebug("couldn't read '/etc/ppp/pppoe.conf'");
		return;
	}

	uint spos = 0;
	uint dpos = 0;
	uint ppos = secrets.find('*');

	if ( ppos != eString::npos )
	{
		eString strUser = '\'' + secrets.left(ppos) + "\'\n";
		helper( source, dest, spos, dpos, "USER=", strUser.c_str() );
	}
	int webifport=80;
	eConfig::getInstance()->getKey("/elitedvb/network/webifport", webifport);
	eString s;
	s.sprintf("%d\n", webifport);
	helper( source, dest, spos, dpos, "ENIGMA_WEB_IF_PORT=", s.c_str() );
	helper( source, dest, spos, dpos, "REJECT_WWW=", flags&1?"yes\n":"no\n" );
	helper( source, dest, spos, dpos, "REJECT_TELNET=", flags&2?"yes\n":"no\n" );
	helper( source, dest, spos, dpos, "REJECT_SAMBA=", flags&4?"yes\n":"no\n" );
	helper( source, dest, spos, dpos, "REJECT_FTP=", flags&8?"yes\n":"no\n" );

	memcpy( dest+dpos, source+spos, readed - spos );
	dpos += readed-spos;

	fclose(f);
	f = fopen("/etc/ppp/pppoe.conf", "w");
	if ( !f )
	{
		eDebug("couldn't open '/etc/ppp/pppoe.conf' for write");
		return;
	}
	unsigned int written;
	if ( (written = fwrite( dest, 1, dpos, f )) != dpos )
		eDebug("couldn't write correct count of bytes...\n%d bytes written %d should be written", written, dpos );
	fclose(f);
}

#endif

static void getNameserver( __u32 &ip )
{
	char buf[256];
	__u32 tmp=UINT_MAX;

	FILE *file=fopen("/etc/resolv.conf","r");
	if (!file)
		return;

	int tmp1,tmp2,tmp3,tmp4;

	while (fgets(buf,sizeof(buf),file))
	{
		if (sscanf(buf,"nameserver %d.%d.%d.%d", &tmp1, &tmp2, &tmp3, &tmp4) == 4)
		{
			ip=tmp=tmp1<<24|tmp2<<16|tmp3<<8|tmp4;
//			sprintf(ip, "0x%02x%02x%02x%02x", tmp1, tmp2, tmp3, tmp4);
			break;
		}
	}
	fclose(file);
	if ( tmp == UINT_MAX )
		eDebug("parse resolv.conf failed");
}

static void getDefaultGateway(__u32 &ip)
{
	char iface[9];
	unsigned int dest=0, gw=UINT_MAX;
	char buf[256];

	FILE *file=fopen("/proc/net/route","r");
	if (!file)
		return;
	fgets(buf,sizeof(buf),file);
	while(fgets(buf,sizeof(buf),file))
	{
		if (sscanf(buf,"%8s %x %x", iface, &dest, &gw) == 3)
		{
			if (!dest)
			{
				ip = gw;
				break;
			}
		}
	}
	if (gw == UINT_MAX)
		eDebug("get route failed");
	fclose(file);
}

static void getIP( char *dev, __u32 &ip, __u32 &mask)
{
	int fd;
	struct ifreq req;
	struct sockaddr_in *saddr;
	unsigned char *addr;

	fd=socket(AF_INET,SOCK_DGRAM,0);
	if ( !fd )
		return;

	memset(&req, 0, sizeof(req));
	strcpy(req.ifr_name,dev);
	saddr = (struct sockaddr_in *) &req.ifr_addr;
	addr = (unsigned char*) &saddr->sin_addr.s_addr;

	if( ::ioctl(fd,SIOCGIFADDR,&req) < 0 )
		eDebug("SIOCGIFADDR failed(%m)");
	else
		ip = addr[0]<<24|addr[1]<<16|addr[2]<<8|addr[3];	

	if( ::ioctl(fd,SIOCGIFNETMASK,&req) < 0 )
		eDebug("SIOCGIFNETMASK failed(%m)");
	else
		mask = addr[0]<<24|addr[1]<<16|addr[2]<<8|addr[3];

	close(fd);
}

eZapNetworkSetup::eZapNetworkSetup()
	:eWindow(0)
{
	init_eZapNetworkSetup();
}

void eZapNetworkSetup::init_eZapNetworkSetup()
{
	__u32 sip=ntohl(0x0a000061),
				snetmask=ntohl(0xFF000000),
				sdns=ntohl(0x7f000001),
				sgateway=ntohl(0x7f000001);

	int de[4];
	int sdosetup=0;
	int connectionType=0;
	int webifport=80;
	int useDHCP=0;

#ifdef USE_IFUPDOWN
	bool automatic_start;
	std::string Ip, Netmask, Broadcast, Gateway;
	useDHCP=!getInetAttributes("eth0", automatic_start, Ip, Netmask, Broadcast, Gateway);
	sdosetup=automatic_start;
#else
	eConfig::getInstance()->getKey("/elitedvb/network/usedhcp", useDHCP);
	eConfig::getInstance()->getKey("/elitedvb/network/dosetup", sdosetup);
#endif
	if (useDHCP)
	{
		getIP("eth0", sip, snetmask);
		getDefaultGateway(sgateway);
		getNameserver(sdns);
	}
	else
	{
#ifdef USE_IFUPDOWN
		int tmp[4];
		if ( sscanf(Ip.c_str(), "%d.%d.%d.%d", tmp, tmp+1, tmp+2, tmp+3) == 4 )
			sip = tmp[0]<<24 | tmp[1] << 16 | tmp[2] << 8 | tmp[3];
		else
			eDebug("couldn't parse ip(%s)", Ip.length()?Ip.c_str():"");

		if ( sscanf(Netmask.c_str(), "%d.%d.%d.%d", tmp, tmp+1, tmp+2, tmp+3) == 4 )
			snetmask = tmp[0]<<24 | tmp[1] << 16 | tmp[2] << 8 | tmp[3];
		else
			eDebug("couldn't parse netmask(%s)", Netmask.length()?Netmask.c_str():"");

		if ( sscanf(Gateway.c_str(), "%d.%d.%d.%d", tmp, tmp+1, tmp+2, tmp+3) == 4 )
			sgateway = tmp[0]<<24 | tmp[1] << 16 | tmp[2] << 8 | tmp[3];
		else
			eDebug("couldn't parse gateway(%s)", Gateway.length()?Gateway.c_str():"");

		getNameserver(sdns);  // read always from /etc/resolv.conf
#else
		eConfig::getInstance()->getKey("/elitedvb/network/ip", sip);
		eConfig::getInstance()->getKey("/elitedvb/network/netmask", snetmask);
		eConfig::getInstance()->getKey("/elitedvb/network/gateway", sgateway);
		eConfig::getInstance()->getKey("/elitedvb/network/dns", sdns);
#endif
	}
	eConfig::getInstance()->getKey("/elitedvb/network/connectionType", connectionType);
	eConfig::getInstance()->getKey("/elitedvb/network/webifport", webifport);

	eNumber::unpack(sip, de);
	ip=CreateSkinnedNumberWithLabel("ip",0, 4, 0, 255, 3, de, 0, "lip", !useDHCP);
	ip->setFlags(eNumber::flagDrawPoints);
	CONNECT(ip->selected, eZapNetworkSetup::fieldSelected);

	dhcp = CreateSkinnedCheckbox("dhcp", useDHCP);
	CONNECT(dhcp->checked, eZapNetworkSetup::dhcpStateChanged);

	eNumber::unpack(snetmask, de);
	netmask=CreateSkinnedNumberWithLabel("netmask",0, 4, 0, 255, 3, de, 0, "lnetmask", !useDHCP);
	netmask->setFlags(eNumber::flagDrawPoints);
	CONNECT(netmask->selected, eZapNetworkSetup::fieldSelected);

	eListBoxEntryText *sel=0;
	combo_type=CreateSkinnedComboBoxWithLabel("combo_type", 3, "lcombo_type");
#ifdef ENABLE_PPPOE
	if ( !connectionType )
#endif
	{
		sel = new eListBoxEntryText( *combo_type, _("LAN"), (void*)0, 0, _("communicate to Local Area Network"));
#ifdef ENABLE_PPPOE
		new eListBoxEntryText( *combo_type, _("WAN(PPPoE)"), (void*)1, 0, _("communicate to the Internet via DSL"));
#endif
	}
#ifdef ENABLE_PPPOE
	else
	{
		new eListBoxEntryText( *combo_type, _("LAN"), (void*)0, 0, _("communicate to Local Area Network"));
		sel = new eListBoxEntryText( *combo_type, _("WAN(PPPoE)"), (void*)1, 0, _("communicate to the Internet via DSL"));
	}
	CONNECT(combo_type->selchanged, eZapNetworkSetup::typeChanged);
	tdsl = new eButton(this);
	tdsl->move(ePoint(340,90));
	tdsl->resize(eSize(100, fd+10));
	tdsl->setText("T-DSL");
	tdsl->loadDeco();
	tdsl->hide();
	tdsl->setHelpText(_("T-Online User press ok here"));
	CONNECT( tdsl->selected, eZapNetworkSetup::tdslPressed );
#endif

	eNumber::unpack(sdns, de);
	dns=CreateSkinnedNumberWithLabel("dns",0, 4, 0, 255, 3, de, 0, "lNameserver", !useDHCP);
	dns->setFlags(eNumber::flagDrawPoints);
	CONNECT(dns->selected, eZapNetworkSetup::fieldSelected);

#ifdef ENABLE_PPPOE
	lLogin=new eLabel(this);
	lLogin->setText(_("Login:"));
	lLogin->move(ePoint(20, 130));
	lLogin->resize(eSize(140, fd+4));
	lLogin->hide();

	char *strLogin=0;
	eConfig::getInstance()->getKey("/elitedvb/network/login", strLogin);
	login=new eTextInputField(this,lLogin);
	login->move(ePoint(160, 130));
	login->resize(eSize(280, fd+10));
	login->setMaxChars(100);
	login->loadDeco();
	login->setHelpText(_("press ok to edit your provider login name"));
	if ( strLogin )
		login->setText(strLogin);
	login->hide();
	CONNECT(login->selected, eZapNetworkSetup::loginSelected );
#endif

	eNumber::unpack(sgateway, de);
	gateway=CreateSkinnedNumberWithLabel("gateway",0, 4, 0, 255, 3, de, 0, "lgateway", !useDHCP);
	gateway->setFlags(eNumber::flagDrawPoints);
	CONNECT(gateway->selected, eZapNetworkSetup::fieldSelected);

#ifdef ENABLE_PPPOE
	lPassword=new eLabel(this);
	lPassword->setText(_("Password:"));
	lPassword->move(ePoint(20, 170));
	lPassword->resize(eSize(150, fd+4));

	password=new eTextInputField(this,lPassword);
	password->move(ePoint(160, 170));
	password->resize(eSize(280, fd+10));
	password->setMaxChars(30);
	password->loadDeco();
	password->setHelpText(_("press ok to edit your provider password"));
	password->hide();
	CONNECT(password->selected, eZapNetworkSetup::passwordSelected);
#endif

	dosetup=CreateSkinnedCheckbox("dosetup", sdosetup);

	port=CreateSkinnedNumberWithLabel("port",webifport, 1, 0, 65536, 5, 0, 0, "lport");
	port->setFlags(eNumber::flagDrawPoints);
	CONNECT(port->selected, eZapNetworkSetup::fieldSelected);

#ifdef ENABLE_PPPOE
	int flags = getRejectFlags();
	rejectWWW=new eCheckbox(this, flags&1, 1);
	rejectWWW->setText("WWW");
	rejectWWW->move(ePoint(20,255));
	rejectWWW->resize(eSize(90, fd+4));
	eString t = _("reject incoming connections on port 80");
	unsigned int pos = t.find("80");
	if (pos != eString::npos )
	{
		t.erase(pos,2);
		t.insert(pos,eString().sprintf("%d", webifport));
	}
	rejectWWW->setHelpText(t);
	rejectWWW->hide();

	rejectTelnet=new eCheckbox(this, flags&2, 1);
	rejectTelnet->setText("Telnet");
	rejectTelnet->move(ePoint(130,255));
	rejectTelnet->resize(eSize(90, fd+4));
	rejectTelnet->setHelpText(_("reject incoming connections on port 23"));
	rejectTelnet->hide();

	rejectSamba=new eCheckbox(this, flags&4, 1);
	rejectSamba->setText("Samba");
	rejectSamba->move(ePoint(240,255));
	rejectSamba->resize(eSize(100, fd+4));
	rejectSamba->setHelpText(_("reject incoming connections on ports 137,138,139"));
	rejectSamba->hide();

	rejectFTP=new eCheckbox(this, flags&8, 1);
	rejectFTP->setText("FTP");
	rejectFTP->move(ePoint(360,255));
	rejectFTP->resize(eSize(70, fd+4));
	rejectFTP->setHelpText(_("reject incoming connections on ports 21"));
	rejectFTP->hide();
#endif

	CONNECT(CreateSkinnedButton("ok")->selected, eZapNetworkSetup::okPressed);

#ifndef DISABLE_NFS
	nfs = CreateSkinnedButton("nfs");
	CONNECT(nfs->selected, eZapNetworkSetup::nfsPressed );
#else
	CreateSkinnedButton("nfs")->hide();
#endif

	BuildSkin("eZapNetworkSetup");
	setHelpID(82);

	combo_type->setCurrent(sel,true);

#ifdef ENABLE_PPPOE
	if ( readSecretString( secrets ) && secrets )
	{
		unsigned int pos = secrets.find("*");
		if ( pos != eString::npos )
		{
			login->setText( secrets.left(pos) );
			password->setText("******");
		}
	}
#endif
}

eZapNetworkSetup::~eZapNetworkSetup()
{
}

void eZapNetworkSetup::fieldSelected(int *number)
{
	focusNext(eWidget::focusDirNext);
}

void eZapNetworkSetup::dhcpStateChanged(int state)
{
	ip->setActive(!state, dhcp);
	netmask->setActive(!state, combo_type);
	gateway->setActive(!state, dosetup);
	dns->setActive(!state, gateway);
}

void eZapNetworkSetup::okPressed()
{
	int eIP[4], eMask[4], eDNS[4], eGateway[4];
	for (int i=0; i<4; i++)
	{
		eIP[i]=ip->getNumber(i);
		eMask[i]=netmask->getNumber(i);
		eGateway[i]=gateway->getNumber(i);
		eDNS[i]=dns->getNumber(i);
	}

	int useDHCP=dhcp->isChecked();
	int sdosetup=dosetup->isChecked();
	int type = (int) combo_type->getCurrent()->getKey();

	int oldport=80;
	eConfig::getInstance()->getKey("/elitedvb/network/webifport", oldport);

	eConfig::getInstance()->setKey("/elitedvb/network/webifport", port->getNumber());
	eConfig::getInstance()->setKey("/elitedvb/network/connectionType", type );

#ifdef USE_IFUPDOWN
	bool automatic_start = sdosetup;
	if (useDHCP)
	{
		addLoopbackDevice("lo", true);
		setDhcpAttributes("eth0", automatic_start);
	}
	else
	{
		eString Ip, Netmask, Broadcast, Gateway;
		Ip.sprintf("%d.%d.%d.%d", eIP[0], eIP[1], eIP[2], eIP[3]);
		Netmask.sprintf("%d.%d.%d.%d", eMask[0], eMask[1], eMask[2], eMask[3]);
		int bcast = ip->getNumber() | (~netmask->getNumber());
		__u8 tmp[4];
		tmp[0] = ~((__u8)eMask[0]);
		tmp[1] = ~((__u8)eMask[1]);
		tmp[2] = ~((__u8)eMask[2]);
		tmp[3] = ~((__u8)eMask[3]);
		Broadcast.sprintf("%d.%d.%d.%d", eIP[0] | tmp[0], eIP[1] | tmp[1], eIP[2] | tmp[2], eIP[3] | tmp[3]);
		Gateway.sprintf("%d.%d.%d.%d", eGateway[0], eGateway[1], eGateway[2], eGateway[3]);
		addLoopbackDevice("lo", true);
		setStaticAttributes("eth0", automatic_start, Ip, Netmask, Broadcast, Gateway);

		FILE *f=fopen("/etc/resolv.conf", "wt");
		if (!f)
			eDebug("couldn't write resolv.conf");
		else
		{
			fprintf(f, "# Generated by enigma\nnameserver %d.%d.%d.%d\n", eDNS[0], eDNS[1], eDNS[2], eDNS[3]);
			fclose(f);
		}
	}
#else
	eConfig::getInstance()->setKey("/elitedvb/network/dosetup", sdosetup);
	eConfig::getInstance()->setKey("/elitedvb/network/usedhcp", useDHCP);
	if ( !useDHCP )
	{
		__u32 sip, snetmask, sdns, sgateway;
		eNumber::pack(sip, eIP);
		eNumber::pack(snetmask, eMask);
		eNumber::pack(sdns, eDNS);
		eNumber::pack(sgateway, eGateway);
		eConfig::getInstance()->setKey("/elitedvb/network/ip", sip);
		eConfig::getInstance()->setKey("/elitedvb/network/netmask", snetmask);
		eConfig::getInstance()->setKey("/elitedvb/network/dns", sdns);
		eConfig::getInstance()->setKey("/elitedvb/network/gateway", sgateway);
	}
#endif
	eConfig::getInstance()->flush();

#ifdef ENABLE_PPPOE
	if (type)
	{
		int flags=0;
		if ( rejectWWW->isChecked() )
			flags |= 1;
		if ( rejectTelnet->isChecked() )
			flags |= 2;
		if ( rejectSamba->isChecked() )
			flags |= 4;
		if ( rejectFTP->isChecked() )
			flags |= 8;
		writeSecretString( secrets );
		updatePPPConfig( secrets,flags );
	}
#endif

#ifdef USE_IFUPDOWN
	system("ifdown eth0");
	system("killall -9 udhcpc");
	if ( automatic_start )
	{
		pid_t pid;
		switch(pid=fork())
		{
			case -1:
				eDebug("error fork ifup (%m)");
				break;
			case 0:
				for (unsigned int i=3; i < 90; ++i )
					::close(i);
				system("ifup eth0");
				_exit(0);
				break;
			default:
				waitpid(pid,0,0);
				break;
		}
	}
#endif
	// when USE_IFUPDOWN is defined this only do try to mount the automounts
	eDVB::getInstance()->configureNetwork();

	if ( oldport != port->getNumber() )
		eZap::getInstance()->reconfigureHTTPServer();

	close(0);
}

#ifdef ENABLE_PPPOE
void eZapNetworkSetup::typeChanged(eListBoxEntryText *le)
{
	if ( le )
	{
		if ( le->getKey() )
		{
			rejectWWW->show();
			rejectFTP->show();
			rejectTelnet->show();
			rejectSamba->show();
			tdsl->show();
			lNameserver->hide();
			lGateway->hide();
			dns->hide();
			gateway->hide();
			lLogin->show();
			lPassword->show();
			login->show();
			password->show();
		}
		else
		{
			rejectWWW->hide();
			rejectFTP->hide();
			rejectTelnet->hide();
			rejectSamba->hide();
			tdsl->hide();
			lLogin->hide();
			lPassword->hide();
			login->hide();
			password->hide();
			lNameserver->show();
			lGateway->show();
			dns->show();
			gateway->show();
		}
	}
}

void eZapNetworkSetup::loginSelected()
{
	if ( !login->inEditMode() )
	{
		unsigned int pos =
			secrets.find("*");
		eString pw;
		if ( pos )
			pw = secrets.mid(pos+1, secrets.length() - pos - 1 );
		secrets = login->getText()+'*'+pw;
	}
}

void eZapNetworkSetup::passwordSelected()
{
	if ( password->inEditMode() )
	{
		unsigned int pos = secrets.find("*");
		if ( pos != eString::npos )
			password->setText( secrets.mid( pos+1, secrets.length() - pos - 1 ) );
	}
	else
	{
		secrets=login->getText()+'*'+password->getText();
		password->setText("******");
	}
}

void eZapNetworkSetup::tdslPressed()
{
	hide();
	eTOnlineDialog dlg(login->getText());
	dlg.show();
	if ( !dlg.exec() )
	{
		login->setText(dlg.getLogin());
		unsigned int pos =
			secrets.find("*");
		eString pw;
		if ( pos )
			pw = secrets.mid(pos+1, secrets.length() - pos - 1 );
		secrets = login->getText()+'*'+pw;
	}
	dlg.hide();
	show();
	setFocus(tdsl);
}
#endif // ENABLE_PPPOE

#ifndef DISABLE_NFS
void eZapNetworkSetup::nfsPressed()
{
	hide();
	eNFSSetup dlg;
	dlg.show();
	dlg.exec();
	dlg.hide();
	show();
	setFocus(nfs);
}
#endif

#ifndef DISABLE_NFS

#define MAX_NFS_ENTRIES 8

static void errorMessage(const eString message, int type=0)
{
	int flags;
	if(type==1)
		flags = eMessageBox::iconInfo|eMessageBox::btOK;
	else	
		flags = eMessageBox::iconWarning|eMessageBox::btOK;
	eMessageBox::ShowBox(message, _("Info"), flags);
}

eNFSSetup::eNFSSetup()
	:eWindow(0), timeout(eApp), mountContainer(0)
{
	init_eNFSSetup();
}

void eNFSSetup::init_eNFSSetup()
{
	bool have_cifs=false;
	bool have_smbfs=false;
	FILE *f=fopen("/proc/filesystems", "rt");
	if (f)
	{
		while (1)
		{
			char buffer[128];
			if (!fgets(buffer, 128, f))
				break;
			if ( strstr(buffer, "cifs") )
				have_cifs=true;
			if ( strstr(buffer, "smbfs") )
				have_smbfs=true;
		}
		fclose(f);
	}

	CONNECT(timeout.timeout, eNFSSetup::mountTimeout);
	__u32 sip=ntohl(0x0a000061);
    
	int de[4];

	lip = CreateSkinnedLabel("lip");

	eNumber::unpack(sip, de);
	ip=CreateSkinnedNumber("ip",0, 4, 0, 255, 3, de, 0, lip);
	ip->setFlags(eNumber::flagDrawPoints);
	CONNECT(ip->selected, eNFSSetup::fieldSelected);

	combo_fstype=CreateSkinnedComboBox("combo_fstype", 2);
	new eListBoxEntryText( *combo_fstype, "NFS", (void*)0, 0, "Network File System");
	if (have_cifs)
		new eListBoxEntryText( *combo_fstype, "CIFS", (void*)1, 0, "Common Internet File System");
	if (have_smbfs)
		new eListBoxEntryText( *combo_fstype, "SMBFS", (void*)2, 0, _("Samba File System(to mount share from another Dreambox)"));
	combo_fstype->setCurrent(0,true);
	CONNECT(combo_fstype->selchanged, eNFSSetup::fstypeChanged);

	lsdir = CreateSkinnedLabel("lsdir");

	sdir = CreateSkinnedTextInputField("sdir",0);
	sdir->setUseableChars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/");

	lldir = CreateSkinnedLabel("lldir");

	ldir = CreateSkinnedTextInputField("ldir",0);
	ldir->setUseableChars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.,:|!?/");

	loptions = CreateSkinnedLabel("lcombo_options");

	combo_options=CreateSkinnedComboBox("combo_options", 3, loptions);
	new eListBoxEntryText( *combo_options, "", (void*)0, 0);
	new eListBoxEntryText( *combo_options, "ro", (void*)1, 0);
	new eListBoxEntryText( *combo_options, "rw", (void*)2, 0);
	new eListBoxEntryText( *combo_options, "ro,nolock", (void*)3, 0);
	new eListBoxEntryText( *combo_options, "rw,nolock", (void*)4, 0);
	new eListBoxEntryText( *combo_options, "ro,soft", (void*)5, 0);
	new eListBoxEntryText( *combo_options, "rw,soft", (void*)6, 0);	
	new eListBoxEntryText( *combo_options, "ro,soft,nolock", (void*)7, 0);
	new eListBoxEntryText( *combo_options, "rw,soft,nolock", (void*)8, 0);	
	new eListBoxEntryText( *combo_options, "ro,udp,nolock", (void*)9, 0);
	new eListBoxEntryText( *combo_options, "rw,udp,nolock", (void*)10, 0);
	new eListBoxEntryText( *combo_options, "ro,soft,udp", (void*)11, 0);
	new eListBoxEntryText( *combo_options, "rw,soft,udp", (void*)12, 0);
	new eListBoxEntryText( *combo_options, "ro,soft,udp,nolock", (void*)13, 0);
	new eListBoxEntryText( *combo_options, "rw,soft,udp,nolock", (void*)14, 0);
	combo_options->setCurrent(0,true);

	lextras = CreateSkinnedLabel("lextraoptions");

	extraoptions=CreateSkinnedTextInputField("extraoptions",0);
	extraoptions->setMaxChars(100);

	luser = CreateSkinnedLabel("luser");

	user=CreateSkinnedTextInputField("user",0);
	user->setMaxChars(100);

	lpass = CreateSkinnedLabel("lpass");

	pass=CreateSkinnedTextInputField("pass",0);
	pass->setMaxChars(100);

	doamount=CreateSkinnedCheckbox("doamount");

	//buttons
	prev = CreateSkinnedButton("prev");
	CONNECT(prev->selected, eNFSSetup::prevPressed);

	CONNECT(CreateSkinnedButton("umount")->selected, eNFSSetup::umountPressed);

	CONNECT(CreateSkinnedButton("mount")->selected, eNFSSetup::mountPressed);

	CONNECT(CreateSkinnedButton("ok")->selected, eNFSSetup::okPressed);

	next = CreateSkinnedButton("next");
	CONNECT(next->selected, eNFSSetup::nextPressed);

	BuildSkin("eNFSSetup");

	cur_entry=0;
	headline.sprintf("Mount Manager(%d/%d)",cur_entry + 1, MAX_NFS_ENTRIES);
	setText(headline);

	load_config();
}

void eNFSSetup::changeWidgets(int fstype)
{
	int y=5;

	lip->move(ePoint(10,y));
	ip->move(ePoint(120, y));
	combo_fstype->move(ePoint(340,y));
	y += 36;

	lsdir->move(ePoint(10, y));
	sdir->move(ePoint(120, y));
	y += 36;

	lldir->move(ePoint(10, y));
	ldir->move(ePoint(120, y));
	y += 36;

	luser->hide();
	user->hide();
	lpass->hide();
	pass->hide();
	loptions->hide();
	combo_options->hide();
	lextras->hide();
	extraoptions->hide();
	doamount->hide();

	switch( fstype )
	{
		case 0:
		case 1:  // CIFS
			loptions->move(ePoint(10, y));
			combo_options->move(ePoint(120,y));
			y += 36;
			lextras->move(ePoint(10, y));
			extraoptions->move(ePoint(120, y));
			y += 36;
			loptions->show();
			lextras->show();
			combo_options->show();
			extraoptions->show();
			sdir->setHelpText(_("enter the name of the share without trailing slash"));
			break;
		case 2:  // SMBFS
			sdir->setHelpText(_("enter the name of the share (//DreamBOX/harddisk)"));
			break;
	}
	if ( fstype )
	{
		luser->move(ePoint(10, y));
		user->move(ePoint(120, y));
		y += 36;

		lpass->move(ePoint(10, y));
		pass->move(ePoint(120, y));
		y += 36;

		luser->show();
		lpass->show();
		user->show();
		pass->show();
	}
	doamount->move(ePoint(120, y));
	doamount->show();
}
    
void eNFSSetup::fstypeChanged(eListBoxEntryText *le)
{
	if ( le )
		changeWidgets( (int)le->getKey() );
}
    
void eNFSSetup::load_config()
{
	__u32 sip=ntohl(0x0a000061);
	char *ctmp  = 0;
	int itmp = 0;
	int de[4],i;

	cmd.sprintf("/elitedvb/network/nfs%d/",cur_entry);

	eConfig::getInstance()->getKey((cmd+"ip").c_str(), sip);
	eNumber::unpack(sip, de);
	for(i=0;i<4;i++)
		ip->setNumber(i, de[i]);

	eConfig::getInstance()->getKey((cmd+"fstype").c_str(), itmp);

	if (combo_fstype->setCurrent(itmp, true) != eComboBox::OK || !itmp )
		changeWidgets(0);

	if (!eConfig::getInstance()->getKey((cmd+"sdir").c_str(), ctmp))
	{
		sdir->setText(ctmp);
		free(ctmp);
	}

	if (!eConfig::getInstance()->getKey((cmd+"ldir").c_str(), ctmp))
	{
		ldir->setText(ctmp);
		free(ctmp);
	}

	if(!ldir->getText())
		ldir->setText("/mnt");

	itmp=0;
	eConfig::getInstance()->getKey((cmd+"options").c_str(), itmp);
	combo_options->setCurrent(itmp, true);

	if (!eConfig::getInstance()->getKey((cmd+"extraoptions").c_str(), ctmp))
	{
		extraoptions->setText(ctmp);
		free(ctmp);
	}
	else if (!itmp)
		extraoptions->setText("nolock,rsize=8192,wsize=8192");

	if (!eConfig::getInstance()->getKey((cmd+"username").c_str(), ctmp))
	{
		user->setText(ctmp);
		free(ctmp);
	}

	if (!eConfig::getInstance()->getKey((cmd+"password").c_str(), ctmp))
	{
		pass->setText(ctmp);
		free(ctmp);
	}

	itmp=0;
	eConfig::getInstance()->getKey((cmd+"automount").c_str(), itmp);
	doamount->setCheck(itmp);
}
    
void eNFSSetup::prevPressed()
{
	cur_entry = ((cur_entry + MAX_NFS_ENTRIES - 1) % MAX_NFS_ENTRIES);
	load_config();
	headline.sprintf("Mount Manager(%d/%d)",cur_entry + 1, MAX_NFS_ENTRIES);
	setText(headline);
	setFocus(prev);
}
    
void eNFSSetup::nextPressed()
{
	cur_entry = ((cur_entry + 1) % MAX_NFS_ENTRIES);
	load_config();
	headline.sprintf("Mount Manager(%d/%d)",cur_entry + 1, MAX_NFS_ENTRIES);
	setText(headline);
	setFocus(next);
}
    
void eNFSSetup::okPressed()
{
	int IP[4];	
	__u32 sip;
	for (int i=0; i<4; i++)
		IP[i]=ip->getNumber(i);
	eNumber::pack(sip, IP);
	
	if(sdir->getText().length()==0 || ldir->getText().length()==0)
	{
		errorMessage(_("invalid or missing dir or local dir"));
		return;
	}
	else
	{
		cmd.sprintf("/elitedvb/network/nfs%d/",cur_entry);
		eConfig::getInstance()->setKey((cmd+"ip").c_str(), sip);
		eConfig::getInstance()->setKey((cmd+"sdir").c_str(), sdir->getText().c_str());
		eConfig::getInstance()->setKey((cmd+"ldir").c_str(), ldir->getText().c_str());
		eConfig::getInstance()->setKey((cmd+"fstype").c_str(), (int)combo_fstype->getCurrent()->getKey());
		eConfig::getInstance()->setKey((cmd+"options").c_str(), (int)combo_options->getCurrent()->getKey());
		eConfig::getInstance()->setKey((cmd+"extraoptions").c_str(), extraoptions->getText().c_str());
		eConfig::getInstance()->setKey((cmd+"username").c_str(), user->getText().c_str());
		eConfig::getInstance()->setKey((cmd+"password").c_str(), pass->getText().c_str());
		eConfig::getInstance()->setKey((cmd+"automount").c_str(), (int)doamount->isChecked());
	}

	int tmp = (int)combo_fstype->getCurrent()->getKey();
	eString tmp1 = _("NFS/CIFS-Entry stored. Further entry?\n");
	eString tmp2 = _("NFS/CIFS-Setup...");
	tmp1.strReplace("NFS/CIFS", tmp == 2 ? "SMBFS" : tmp == 1 ? "CIFS" : "NFS" );
	tmp2.strReplace("NFS/CIFS", "NFS/CIFS/SMBFS" );
	int res = eMessageBox::ShowBox( tmp1, tmp2,
		eMessageBox::btYes|eMessageBox::btNo, eMessageBox::btNo);

	if (res == eMessageBox::btYes)
	{
		nextPressed();
		setFocus(ip);
	}
}

extern bool ismounted( eString mountpoint );

void eNFSSetup::mountPressed()
{
	if(sdir->getText().length()==0 || ldir->getText().length()==0)
	{
		errorMessage(_("invalid or missing dir or local dir"));
		return;
	}
	else 
	{
		if(ismounted(ldir->getText()))
		{
			eString error=_("NFS/CIFS mount error already mounted");
			int tmp = (int)combo_fstype->getCurrent()->getKey();
			error.strReplace("NFS/CIFS", tmp == 2 ? "SMBFS" : tmp == 1 ? "CIFS" : "NFS" );
			errorMessage(error);
			return;
		}
    
		eString opt;

		int fstype = (int)combo_fstype->getCurrent()->getKey();
		switch(fstype)
		{
			case 0: // NFS
			{
				opt.sprintf("/bin/mount -t nfs %d.%d.%d.%d:/%s",
					ip->getNumber(0),ip->getNumber(1),
					ip->getNumber(2),ip->getNumber(3),
					sdir->getText().c_str());

				if( combo_options->getCurrent()->getKey() && extraoptions->getText() )
					opt+=eString().sprintf(" -o %s,%s",
									combo_options->getCurrent()->getText().c_str(),
									extraoptions->getText().c_str());
				else if( combo_options->getCurrent()->getKey() )
					opt+=eString().sprintf(" -o %s", combo_options->getCurrent()->getText().c_str());
				else if( extraoptions->getText() )
					opt+=eString().sprintf(" -o %s", extraoptions->getText().c_str());
				opt+=' ';
				opt+=ldir->getText().c_str();
				break;
			}
			case 1: // CIFS
			{
				opt.sprintf("/bin/mount -t cifs //bla -o user=%s,pass=%s,unc=//%d.%d.%d.%d/%s",
					user->getText().c_str(), pass->getText().c_str(),
					ip->getNumber(0),ip->getNumber(1),
					ip->getNumber(2),ip->getNumber(3),
					sdir->getText().c_str());
   
				if( combo_options->getCurrent()->getKey() && extraoptions->getText() )
					opt+=eString().sprintf(",%s,%s",
									combo_options->getCurrent()->getText().c_str(),
									extraoptions->getText().c_str());
				else if( combo_options->getCurrent()->getKey() )
					opt+=eString().sprintf(",%s", combo_options->getCurrent()->getText().c_str());
				else if( extraoptions->getText() )
					opt+=eString().sprintf(",%s", extraoptions->getText().c_str());
				opt+=' ';
				opt+=ldir->getText().c_str();
				break;
			}
			case 2: // SMBFS
			{
				opt.sprintf("/bin/smbmount %s %s -U %s -I %d.%d.%d.%d -c \"mount %s\"",
					sdir->getText().c_str(), pass->getText().c_str(), user->getText().c_str(),
					ip->getNumber(0),ip->getNumber(1),
					ip->getNumber(2),ip->getNumber(3),
					ldir->getText().c_str());
				break;
			}
			default:
				errorMessage("not supported network file system");
				return;
		}

		if (!mountContainer)
		{
//			eDebug("%s", opt.c_str() );
			mountContainer = new eConsoleAppContainer(opt.c_str());
			CONNECT(mountContainer->appClosed, eNFSSetup::appClosed);
		}
		if ( fstype == 2 )
		    timeout.start(1500,true);
		else
		    timeout.start(3000,true);
	}
}
    
void eNFSSetup::umountPressed() 
{
	eString error;

	int tmp = (int)combo_fstype->getCurrent()->getKey();
	error.sprintf("%s umount '%s' %s",
		tmp==2?"SMBFS":tmp==1?"CIFS":"NFS",
		ldir->getText().c_str(),
		umount2(ldir->getText().c_str(), MNT_FORCE)?"FAILED!":"OK!");

	errorMessage(error.c_str());
}

void eNFSSetup::mountTimeout()
{
	appClosed(0);
}

void eNFSSetup::appClosed(int)
{
	delete mountContainer;
	mountContainer=0;

	if ( timeout.isActive() )
		timeout.stop();

	int tmp = (int)combo_fstype->getCurrent()->getKey();
	eString error;
	error.sprintf("%s mount '%d.%d.%d.%d/%s %s' %s",
		tmp==2?"SMBFS":tmp==1?"CIFS":"NFS",
		ip->getNumber(0),ip->getNumber(1),
		ip->getNumber(2),ip->getNumber(3),
		sdir->getText().c_str(),
		ldir->getText().c_str(),
		ismounted(ldir->getText())?"OK!":"FAILED!" );
		errorMessage(error);
}

void eNFSSetup::automount()
{
	for(int e=0; e < MAX_NFS_ENTRIES; e++)
	{
		int itmp=0;
		cmd.sprintf("/elitedvb/network/nfs%d/",e);
		eConfig::getInstance()->getKey((cmd+"automount").c_str(), itmp);
		if(itmp)
		{
			load_config();
			mountPressed();
		}
	}
}

int eNFSSetup::eventHandler(const eWidgetEvent &e)
{
	if (e.type == eWidgetEvent::execBegin )
	{
		setFocus(ip);
		return 1;
	}
	return eWindow::eventHandler(e);
}

eNFSSetup::~eNFSSetup()
{
	delete mountContainer;
}

#endif

#endif // DISABLE_NETWORK
