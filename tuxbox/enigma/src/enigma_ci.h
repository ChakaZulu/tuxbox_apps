#ifndef DISABLE_CI

#ifndef __enigmaci_h
#define __enigmaci_h

#include <lib/gui/listbox.h>
#include <lib/base/message.h>

class eNumber;
class eButton;
class eDVBCI;
class eTextInputField;
class eWindow;
class eStatusBar;

struct eMMIMsg
{
	char *data;
	int len;
	eMMIMsg()
	{
	}
	eMMIMsg(char* data, int len)
		: data(data), len(len)
	{
	}
};

class eMMIEnqWindow : public eWindow
{
	eLabel *title;
	eNumber *input;
	int num;
	int eventHandler( const eWidgetEvent &e );
	void okPressed(int*);
public:
	eMMIEnqWindow( eString windowText, int num, bool blind );
	eString getAnswer();
};

class eMMIListWindow : public eListBoxWindow<eListBoxEntryText>
{
	eLabel *title, *subtitle, *bottomText;
	int eventHandler( const eWidgetEvent &e );
public:
	void entrySelected( eListBoxEntryText* e );
	eMMIListWindow(eString title, eString subtitle, eString bottomText, std::list< std::pair< eString, int> > &entrys );
	int getSelected() { return (int) list.getCurrent()->getKey(); }
};

class enigmaMMI : public eWindow
{
	eDVBCI *ci;
	eFixedMessagePump<eMMIMsg> mmi_messages;
public:
	Connection conn;
	eLabel *lText;
	eTimer responseTimer;
	enigmaMMI(eDVBCI *ci);
	int eventHandler( const eWidgetEvent &e );
	void handleMMIMessage(const char *data);
	void gotMMIData(const char* data, int);
	void handleMessage( const eMMIMsg &msg );
	void showWaitForCIAnswer(int ret);
	void hideWaitForCIAnswer();
	~enigmaMMI();
};

class enigmaCI: public eWindow
{
	eButton *ok,*reset,*init,*app;
	eButton *reset2,*init2,*app2;

	eStatusBar *status;
	eDVBCI *DVBCI;
	eDVBCI *DVBCI2;

private:
	void okPressed();
	void resetPressed();
	void initPressed();
	void appPressed();
	void reset2Pressed();
	void init2Pressed();
	void app2Pressed();
	void updateCIinfo(const char*);
	void updateCI2info(const char*);
public:
	enigmaCI();
	~enigmaCI();
};

#endif

#endif // DISABLE_CI
