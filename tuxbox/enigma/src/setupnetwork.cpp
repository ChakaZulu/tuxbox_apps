#ifndef DISABLE_NETWORK

#include <setupnetwork.h>

#include <netinet/in.h>

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
		ok->setHelpText("Daten übernehmen und Fenster schliessen");
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

bool readSecretString( eString &str )
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

void writeSecretString( const eString &str )
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

void helper( char *&source, char *&dest, uint &spos, uint &dpos, const char* option, const char* value )
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

char *getOptionString( char *buf, const char *option )
{
	char *p = strstr( buf, option );
	if( !p )
		eDebug("couldn't find '%s' Option", option);
	else
		p+=strlen(option);
	return p;
}

int getRejectFlags()
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
	if ( p && !strnicmp(p,"yes",3) )
		flags|=1;
	p = getOptionString(buf, "REJECT_TELNET=");
	if ( p && !strnicmp(p,"yes",3) )
		flags|=2;
	p = getOptionString(buf, "REJECT_SAMBA=");
	if ( p && !strnicmp(p,"yes",3) )
		flags|=4;
	p = getOptionString(buf, "REJECT_FTP=");
	if ( p && !strnicmp(p,"yes",3) )
		flags|=8;
	fclose(f);
	return flags;
}

void updatePPPConfig( const eString &secrets, int flags )
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

eZapNetworkSetup::eZapNetworkSetup():
	eWindow(0)
{
	setText(_("Communication setup"));
	cmove(ePoint(130, 110));
	cresize(eSize(450, 400));

	__u32 sip=ntohl(0x0a000061), snetmask=ntohl(0xFF000000), sdns=ntohl(0x7f000001), sgateway=ntohl(0x7f000001);
	int de[4];
	int sdosetup=0;
	int fd=eSkin::getActive()->queryValue("fontsize", 20);
	int connectionType=0;
	int webifport=80;

	eConfig::getInstance()->getKey("/elitedvb/network/ip", sip);
	eConfig::getInstance()->getKey("/elitedvb/network/netmask", snetmask);
	eConfig::getInstance()->getKey("/elitedvb/network/dns", sdns);
	eConfig::getInstance()->getKey("/elitedvb/network/gateway", sgateway);
	eConfig::getInstance()->getKey("/elitedvb/network/dosetup", sdosetup);
	eConfig::getInstance()->getKey("/elitedvb/network/connectionType", connectionType);
	eConfig::getInstance()->getKey("/elitedvb/network/webifport", webifport);

	eLabel *l=new eLabel(this);
	l->setText("IP:");
	l->move(ePoint(20, 10));
	l->resize(eSize(150, fd+4));

	eNumber::unpack(sip, de);
	ip=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	ip->move(ePoint(160, 10));
	ip->resize(eSize(200, fd+10));
	ip->setFlags(eNumber::flagDrawPoints);
	ip->setHelpText(_("enter IP Address of the box (0..9, left, right)"));
	ip->loadDeco();
	CONNECT(ip->selected, eZapNetworkSetup::fieldSelected);

	l=new eLabel(this);
	l->setText("Netmask:");
	l->move(ePoint(20, 50));
	l->resize(eSize(150, fd+4));

	eNumber::unpack(snetmask, de);
	netmask=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	netmask->move(ePoint(160, 50));
	netmask->resize(eSize(200, fd+10));
	netmask->setFlags(eNumber::flagDrawPoints);
	netmask->setHelpText(_("enter netmask of your network (0..9, left, right)"));
	netmask->loadDeco();
	CONNECT(netmask->selected, eZapNetworkSetup::fieldSelected);

	l=new eLabel(this);
	l->setText("Type:");
	l->move(ePoint(20, 90));
	l->resize(eSize(140, fd+4));

	eListBoxEntryText *sel=0;
	combo_type=new eComboBox(this, 3, l);
	combo_type->move(ePoint(160,90));
	combo_type->resize(eSize(200, fd+10));
	combo_type->loadDeco();
	combo_type->setHelpText(_("press ok to change connection type"));
	((eZapNetworkSetup*)combo_type)->setProperty("showEntryHelp", "");
	if ( !connectionType )
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
	tdsl->move(ePoint(370,90));
	tdsl->resize(eSize(70, fd+10));
	tdsl->setText("T-DSL");
	tdsl->loadDeco();
	tdsl->hide();
	tdsl->setHelpText(_("T-Online User press ok here"));
	CONNECT( tdsl->selected, eZapNetworkSetup::tdslPressed );
#endif

	lNameserver=new eLabel(this);
	lNameserver->setText("Nameserver:");
	lNameserver->move(ePoint(20, 130));
	lNameserver->resize(eSize(140, fd+4));

	eNumber::unpack(sdns, de);
	dns=new eNumber(this, 4, 0, 255, 3, de, 0, lNameserver);
	dns->move(ePoint(160, 130));
	dns->resize(eSize(200, fd+10));
	dns->setFlags(eNumber::flagDrawPoints);
	dns->setHelpText(_("enter your domain name server (0..9, left, right)"));
	dns->loadDeco();
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

	lGateway=new eLabel(this);
	lGateway->setText("Gateway:");
	lGateway->move(ePoint(20, 170));
	lGateway->resize(eSize(140, fd+4));

	eNumber::unpack(sgateway, de);
	gateway=new eNumber(this, 4, 0, 255, 3, de, 0, l);
	gateway->move(ePoint(160, 170));
	gateway->resize(eSize(200, fd+10));
	gateway->setFlags(eNumber::flagDrawPoints);
	gateway->setHelpText(_("enter your gateways IP Address (0..9, left, right)"));
	gateway->loadDeco();
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

	dosetup=new eCheckbox(this, sdosetup, 1);
	dosetup->setText(_("Configure Network"));
	dosetup->move(ePoint(20, 215));
	dosetup->resize(eSize(fd+4+240, fd+4));
	dosetup->setHelpText(_("enable/disable network config (ok)"));

	l = new eLabel(this);
	l->setText("Port:");
	l->move(ePoint(280+fd+4, 215));
	l->resize(eSize(80, fd+4));

	port=new eNumber(this, 1, 0, 65536, 5, 0, 0, l);
	port->move(ePoint(370, 213));
	port->resize(eSize(70, fd+10));
	port->setFlags(eNumber::flagDrawPoints);
	port->setHelpText(_("enter port of the Web Interface (0..9, left, right)"));
	port->setNumber(webifport);
	port->loadDeco();
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

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(20, 295));
	ok->resize(eSize(220, 40));
	ok->setHelpText(_("save changes and return"));
	ok->loadDeco();
	
	CONNECT(ok->selected, eZapNetworkSetup::okPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-50 ) );
	statusbar->resize( eSize( clientrect.width(), 50) );
	statusbar->loadDeco();
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

void eZapNetworkSetup::okPressed()
{
	int eIP[4], eMask[4], eDNS[4], eGateway[4];
	__u32 sip, snetmask, sdns, sgateway;
	for (int i=0; i<4; i++)
	{
		eIP[i]=ip->getNumber(i);
		eMask[i]=netmask->getNumber(i);
		eGateway[i]=gateway->getNumber(i);
		eDNS[i]=dns->getNumber(i);
	}
	eNumber::pack(sip, eIP);
	eNumber::pack(snetmask, eMask);
	eNumber::pack(sdns, eDNS);
	eNumber::pack(sgateway, eGateway);

	eDebug("IP: %d.%d.%d.%d, Netmask: %d.%d.%d.%d, gateway %d.%d.%d.%d, DNS: %d.%d.%d.%d",
		eIP[0], eIP[1],  eIP[2], eIP[3],
		eMask[0], eMask[1],  eMask[2], eMask[3],
		eGateway[0], eGateway[1], eGateway[2], eGateway[3],
		eDNS[0], eDNS[1],  eDNS[2], eDNS[3]);

	int sdosetup=dosetup->isChecked();
	int type = (int) combo_type->getCurrent()->getKey();

	int oldport=80;
	eConfig::getInstance()->getKey("/elitedvb/network/webifport", oldport);

	eConfig::getInstance()->setKey("/elitedvb/network/ip", sip);
	eConfig::getInstance()->setKey("/elitedvb/network/netmask", snetmask);
	eConfig::getInstance()->setKey("/elitedvb/network/dns", sdns);
	eConfig::getInstance()->setKey("/elitedvb/network/gateway", sgateway);
	eConfig::getInstance()->setKey("/elitedvb/network/dosetup", sdosetup);
	eConfig::getInstance()->setKey("/elitedvb/network/connectionType", type );
	eConfig::getInstance()->setKey("/elitedvb/network/webifport", port->getNumber());
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

	if ( sdosetup )
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


#endif // DISABLE_NETWORK
