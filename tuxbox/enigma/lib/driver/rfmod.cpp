#include <lib/driver/rfmod.h>

#define RFMOD_DEV "/dev/rfmod0"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <lib/system/econfig.h>
#include <lib/dvb/edvb.h>

/*
			7			6			5			4			3			2			1			0
CA		1			1			0			0			1			0			1			0		Chip Address
C0	PWC		OSC		ATT	 SFD1  SFD0		TB1		 X5	   X4		
C1	  1   AUX    SO   LOP    PS    X3    X2  SYSL
FL   N5    N4    N3    N2    N1    N0    X1    X0
FM    0  TPEN   N11   N10    N9    N8    N7    N6

PWC				Peak White Clip enable/disable
OSC				UHF-Osci On/Off
ATT				Modulator Output Attenuate (Sound & Video On/Off)
SFD0,1		Sound sub carrier freq control
TB1				Test mode bit
AUX				Aux Sound Input enable/disable
SO				Sound Oscillator On/Off
LOP				Logical Output Port
PS				Picture to Sound Carrier ratio
SYSL			System L enable (AM sound & positive video modulation)
TPEN			Test Pattern enable
X5,.,X0		Test Mode (all set 0)
N0,.,N11	UHF Freq (steps of 250KHz)
*/
#define C0	3
#define C1	2
#define FL	1
#define FM	0

eRFmod *eRFmod::instance=0;

eRFmod::eRFmod()
{
	if (!instance)
		instance=this;

	rfmodfd=open(RFMOD_DEV, O_RDWR);
}

void eRFmod::init()
{
	memcpy(rfmodreg,"\x00\x00\x80\x00 ",4);

	if (eConfig::getInstance()->getKey("/elitedvb/rfmod/sfd", SFD))
		SFD=5500;

	if (eConfig::getInstance()->getKey("/elitedvb/rfmod/ps", PS))
		PS=12;

	if (eConfig::getInstance()->getKey("/elitedvb/rfmod/so", SO))
		SO=0;

	if (eConfig::getInstance()->getKey("/elitedvb/rfmod/aux", AUX))
		AUX=0;

	if (eConfig::getInstance()->getKey("/elitedvb/rfmod/sysl", SYSL))
		SYSL=0;

	if (eConfig::getInstance()->getKey("/elitedvb/rfmod/pwc", PWC))
		PWC=0;

	if (eConfig::getInstance()->getKey("/elitedvb/rfmod/tpen", TPEN))
		TPEN=0;

	if (eConfig::getInstance()->getKey("/elitedvb/rfmod/osc", OSC))
		OSC=1;
		
	if (eConfig::getInstance()->getKey("/elitedvb/rfmod/att", ATT))
		ATT=0;

	if (eConfig::getInstance()->getKey("/elitedvb/rfmod/div", DIV))
		DIV=2080;

	setSFD(SFD);
	setPS(PS);
	setSO(SO);
	setAUX(AUX);
	setSYSL(SYSL);
	setPWC(PWC);
	setTPEN(TPEN);
	setATT(ATT);
	setDivider(DIV);
	setOSC(OSC);
}

eRFmod *eRFmod::getInstance()
{
	return instance;
}

eRFmod::~eRFmod()
{
	eConfig::getInstance()->setKey("/elitedvb/rfmod/sfd", SFD);
	eConfig::getInstance()->setKey("/elitedvb/rfmod/ps", PS);
	eConfig::getInstance()->setKey("/elitedvb/rfmod/so", SO);
	eConfig::getInstance()->setKey("/elitedvb/rfmod/aux", AUX);
	eConfig::getInstance()->setKey("/elitedvb/rfmod/sysl", SYSL);
	eConfig::getInstance()->setKey("/elitedvb/rfmod/pwc", PWC);
	eConfig::getInstance()->setKey("/elitedvb/rfmod/tpen", TPEN);
	eConfig::getInstance()->setKey("/elitedvb/rfmod/osc", OSC);
	eConfig::getInstance()->setKey("/elitedvb/rfmod/att", ATT);
	eConfig::getInstance()->setKey("/elitedvb/rfmod/div", DIV);

	if (instance==this)
		instance=0;

	if (rfmodfd>=0)
		close(rfmodfd);
}

int eRFmod::setSFD(int freq)			//freq in KHz
{
	unsigned char sfd=0;
	
	switch(freq)
	{
		case 4500:
			sfd=0;
			break;
		case 5500:
			sfd=1;
			break;
		case 6000:
			sfd=2;
			break;
		case 6500:
			sfd=3;
			break;
		default:
			eDebug("eRFMOD: unsupported Sound sub carrier Frequency");	
			return -1;
	}		
	
	SFD=freq;
	
	rfmodreg[C0]&=~(0x18);
	rfmodreg[C0]|= (sfd<<3);

	return setRFmod();
}

int eRFmod::setPS(int ratio)
{
	int ps=0;
	switch (ratio)
	{
		case 12:
			ps=0;
			break;
		case 16:
			ps=1;
			break;
		default:
			eDebug("eRFMOD: unsupported Picture to sound ratio");
			return -1;	
	}
	
	PS=ratio;

	rfmodreg[C1]&=~(0x8);
	rfmodreg[C1]|= (ps<<3);
	
	return setRFmod();
}

int eRFmod::setSO(int valSO)
{
	valSO &= 1;
	
	SO=valSO;
	
	rfmodreg[C1]&=~(0x20);
	rfmodreg[C1]|= (valSO<<5);
	
	return setRFmod();
}

int eRFmod::setAUX(int valAUX)
{
	valAUX &= 1;
	
	AUX=valAUX;
	
	rfmodreg[C1]&=~(0x40);
	rfmodreg[C1]|= (valAUX<<6);
	
	return setRFmod();
}

int eRFmod::setSYSL(int valSYSL)
{
	valSYSL &= 1;
	
	SYSL = valSYSL;
	
	rfmodreg[C1]&=~(0x1);
	rfmodreg[C1]|= (valSYSL);
	
	return setRFmod();
}

int eRFmod::setPWC(int valPWC)
{
	valPWC &= 1;
	
	PWC = valPWC;
	
	rfmodreg[C0]&=~(0x80);
	rfmodreg[C0]|= (valPWC<<7);
	
	return setRFmod();
}

int eRFmod::setTPEN(int valTPEN)
{
	valTPEN &= 1;
	
	TPEN = valTPEN;
	
	rfmodreg[FM]&=~(0x40);
	rfmodreg[FM]|= (valTPEN<<6);
	
	return setRFmod();
}

int eRFmod::setOSC(int valOSC)
{
	valOSC &= 1;
	
	OSC = valOSC;
	
	rfmodreg[C0]&=~(0x40);
	rfmodreg[C0]|= (valOSC<<6);
	
	return setRFmod();
}

int eRFmod::setATT(int valATT)
{
	valATT &= 1;
	
	ATT = valATT;
	
	rfmodreg[C0]&=~(0x20);
	rfmodreg[C0]|= (valATT<<5);
	
	return setRFmod();
}

int eRFmod::setDivider(int valDIV)
{
	valDIV &= 0x1FFF;
	
	DIV = valDIV;	
	
	rfmodreg[FL]&=~(0xFC);
	rfmodreg[FL]|= ((DIV & 0x3f)<<2);
	
	rfmodreg[FM]&=~(0x3F);
	rfmodreg[FM]|= ((DIV >> 6) & 0x3F);
	
	return setRFmod();
}

int eRFmod::setRFmod()
{
	eDebug("set RFmod %02x %02x %02x %02x",rfmodreg[0],rfmodreg[1],rfmodreg[2],rfmodreg[3]);

	if(rfmodfd > 0)
		ioctl(rfmodfd,1,&rfmodreg);
	return 0;
}
