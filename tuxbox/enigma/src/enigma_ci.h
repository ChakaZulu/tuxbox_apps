#ifndef DISABLE_CI

#ifndef __ENIGMA_CI_H_
#define __ENIGMA_CI_H_

#include <src/enigma_mmi.h>

class eDVBCI;

class eButton;
class eCheckbox;
class eWindow;
class eStatusBar;

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

#endif // __ENIGMA_CI_H_

#endif // DISABLE_CI
