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
class eCheckbox;

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
	eMMIEnqWindow( const eString& titleBarText, const eString &windowText, int num, bool blind );
	eString getAnswer();
};

class eMMIListWindow : public eListBoxWindow<eListBoxEntryText>
{
	eLabel *title, *subtitle, *bottomText;
	int eventHandler( const eWidgetEvent &e );
public:
	void entrySelected( eListBoxEntryText* e );
	eMMIListWindow(const eString &titleBarText, const eString &title, const eString &subtitle, const eString &bottomText, std::list< std::pair< eString, int> > &entrys );
	int getSelected() { return (int) list.getCurrent()->getKey(); }
};

class enigmaMMI : public eWindow
{
protected:
	eFixedMessagePump<eMMIMsg> mmi_messages;
	eWidget *open;
	Connection conn;
	eLabel *lText;
	eTimer responseTimer, delayTimer, closeTimer;
	const char *scheduledData;
	void closeMMI();
	enum AnswerType { ENQAnswer, MENUAnswer, LISTAnswer };
	enigmaMMI();
public:
	
	bool connected() { return conn.connected(); }
	int eventHandler( const eWidgetEvent &e );
	virtual bool handleMMIMessage(const char *data);
	void gotMMIData(const char* data, int);
	void handleMessage( const eMMIMsg &msg );
	void showWaitForAnswer(int ret);
	void hideWaitForAnswer();
	virtual void beginExec() { }
	virtual void endExec() { }
	virtual void sendAnswer( AnswerType ans, int param, unsigned char *data )=0;
	void haveScheduledData();
};

class enigmaCI: public eWindow
{
	eButton *ok,*reset,*init,*app;
	eButton *reset2,*init2,*app2;
	eCheckbox *twoServices;

	eStatusBar *status;
	eDVBCI *DVBCI;
	eDVBCI *DVBCI2;

private:
	void handleTwoServicesChecked(int);
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

class enigmaCIMMI : public enigmaMMI
{
	eDVBCI *ci;
	static std::map<eDVBCI*,enigmaCIMMI*> exist;
	void beginExec();
	void sendAnswer( AnswerType ans, int param, unsigned char *data );
public:
	static enigmaCIMMI* getInstance( eDVBCI* ci );
	enigmaCIMMI(eDVBCI *ci);
};

#endif

#endif // DISABLE_CI
