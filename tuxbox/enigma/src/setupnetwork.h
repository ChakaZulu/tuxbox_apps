#ifndef DISABLE_NETWORK

#ifndef __setupnetwork_h
#define __setupnetwork_h

#include <lib/gui/ewindow.h>
#include <lib/gui/statusbar.h>

class eNumber;
class eButton;
class eCheckbox;
class eComboBox;
class eTextInputField;
class eListBoxEntryText;

class eZapNetworkSetup: public eWindow
{
	eNumber *ip, *netmask, *dns, *gateway, *port;
	eButton *ok, *abort;
	eCheckbox *dosetup;
	eLabel *lNameserver, *lGateway;
	eComboBox *combo_type;
	eStatusBar *statusbar;

#ifdef ENABLE_PPPOE
	eButton *tdsl;
	eCheckbox *rejectTelnet, *rejectWWW, *rejectSamba, *rejectFTP;
	eTextInputField *login, *password;
	eLabel *lLogin, *lPassword;
	eString secrets;
#endif
private:
	void fieldSelected(int *number);
	void okPressed();
	void abortPressed();
#ifdef ENABLE_PPPOE
	void typeChanged(eListBoxEntryText*);
	void passwordSelected();
	void loginSelected();
	void tdslPressed();
#endif
public:
	eZapNetworkSetup();
	~eZapNetworkSetup();
};

#endif

#endif // DISABLE_NETWORK
