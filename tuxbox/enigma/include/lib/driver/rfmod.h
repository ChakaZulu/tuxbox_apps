#ifndef __erfmod_h
#define __erfmod_h

#include <lib/base/ebase.h>

class eRFmod: public Object
{
	static eRFmod *instance;
	
	unsigned char rfmodreg[4];
	int SFD, PS, SO, AUX, SYSL, PWC, TPEN, OSC, ATT, DIV;

	int rfmodfd;

public:
	eRFmod();
	~eRFmod();

	void init();

	static eRFmod *getInstance();

	int setSFD(int freq);
	int setPS(int ratio);
	int setSO(int SO);
	int setAUX(int AUX);
	int setSYSL(int SYSL);
	int setPWC(int PWC);
	int setTPEN(int TPEN);
	int setOSC(int OSC);
	int setATT(int ATT);
	int setDivider(int DIV);
	
	int setRFmod(void);
};
#endif
