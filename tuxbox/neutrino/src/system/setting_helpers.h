#ifndef __setting_helpers__
#define __setting_helpers__

#include "../widget/menue.h"

class CColorSetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(string OptionName);
};

class CAudioSetupNotifier : public CChangeObserver
{
    public:
    	bool changeNotify(string OptionName);
};

class CVideoSetupNotifier : public CChangeObserver
{
    public:
    	bool changeNotify(string OptionName);
};

class CLanguageSetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(string OptionName);
};

class CKeySetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(string OptionName);
};

class CAPIDChangeExec : public CMenuTarget
{
	public:
        int exec(CMenuTarget* parent, string actionKey);
};

class CNVODChangeExec : public CMenuTarget
{
	public:
        int exec(CMenuTarget* parent, string actionKey);
};

void setDefaultGateway(char* ip);
void setNetworkAddress(char* ip, char* netmask, char* broadcast);
void setNameServer(char* ip);

#endif
