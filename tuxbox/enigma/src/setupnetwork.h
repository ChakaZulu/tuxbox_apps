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

#ifndef DISABLE_NFS
class eNFSSetup: public eWindow
{
        eButton *ok, *mount, *umount, *prev, *next;
	eCheckbox *doamount;
	eComboBox *combo_fstype, *combo_options;
	eLabel *lpass , *luser;
	eNumber *ip;
	eStatusBar *sbar;
	eString cmd,headline;
	eTextInputField *sdir, *ldir, *user, *pass, *extraoptions;
	int cur_entry;
    
	void fieldSelected(int *number) { focusNext(eWidget::focusDirNext); }
	void fstypeChanged(eListBoxEntryText *le);
    
	void load_config();
    
	void okPressed();
	void prevPressed();
	void nextPressed();
	void mountPressed();
	void umountPressed();
	bool ismounted();
public:
	eNFSSetup();
	~eNFSSetup(){};
    
	void automount();
};
#endif

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
	
#ifndef DISABLE_NFS
	eButton *nfs;
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
#ifndef DISABLE_NFS
	void nfsPressed();
#endif
public:
	eZapNetworkSetup();
	~eZapNetworkSetup();
};

#endif

#endif // DISABLE_NETWORK
