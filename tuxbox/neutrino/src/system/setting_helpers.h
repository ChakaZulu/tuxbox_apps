#ifndef __setting_helpers__
#define __setting_helpers__

/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include <gui/widget/menue.h>

#include <string>

unsigned long long getcurrenttime();

class CSatDiseqcNotifier : public CChangeObserver
{
	private:
		CMenuItem* satMenu;
		CMenuItem* extMenu;
		CMenuItem* extMotorMenu;
		CMenuItem* repeatMenu;
	protected:
		CSatDiseqcNotifier( ) : CChangeObserver(){};  // prevent calling constructor without data we need
	public:
		CSatDiseqcNotifier( CMenuItem* SatMenu, CMenuItem* ExtMenu, CMenuItem* ExtMotorMenu, CMenuItem* RepeatMenu) : CChangeObserver()
		{ satMenu = SatMenu; extMenu = ExtMenu; extMotorMenu = ExtMotorMenu; repeatMenu = RepeatMenu;};
		bool changeNotify(const neutrino_locale_t, void * Data);
};

class CTP_scanNotifier : public CChangeObserver
{
	private:
		CMenuOptionChooser* toDisable1[3];
		CMenuForwarder* toDisable2[2];
		CMenuOptionStringChooser* toDisable3[1];
		
	public:
		CTP_scanNotifier(CMenuOptionChooser*, CMenuOptionChooser*, CMenuForwarder*, CMenuForwarder*, CMenuOptionStringChooser*);
		bool changeNotify(const neutrino_locale_t, void *);
};

class CScanSettingsSatManNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void *Data);
};

class CDHCPNotifier : public CChangeObserver
{
	private:
		CMenuForwarder* toDisable[5];
	public:
		CDHCPNotifier( CMenuForwarder*, CMenuForwarder*, CMenuForwarder*, CMenuForwarder*, CMenuForwarder*);
		bool changeNotify(const neutrino_locale_t, void * data);
};
class CStreamingNotifier : public CChangeObserver
{
	private:
		CMenuItem* toDisable[11];
	public:
		CStreamingNotifier( CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*);
		bool changeNotify(const neutrino_locale_t, void *);
};

class CRecordingNotifier : public CChangeObserver
{
	private:
		CMenuItem* toDisable[9];
	public:
		CRecordingNotifier(CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*, CMenuItem*);
		bool changeNotify(const neutrino_locale_t OptionName, void*);
};

class CRecordingSafetyNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void *);
};

class CRecordingNotifier2 : public CChangeObserver
{
	private:
		CMenuItem* toDisable[1];
	public:
		CRecordingNotifier2( CMenuItem*);
		bool changeNotify(const neutrino_locale_t, void *);
};

class CMiscNotifier : public CChangeObserver
{
	private:
		CMenuItem* toDisable[1];
	public:
		CMiscNotifier( CMenuItem* );
		bool changeNotify(const neutrino_locale_t, void *);
};

class CLcdNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void *);
};

#ifndef TUXTXT_CFG_STANDALONE
class CTuxtxtCacheNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void *);
};
#endif

class CPauseSectionsdNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void * Data);
};

class CSPTSNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void *);
};
		
class CScanModeSectionsdNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void * );
};

class CTouchFileNotifier : public CChangeObserver
{
	const char * filename;
 public:
	inline CTouchFileNotifier(const char * file_to_modify)
		{
			filename = file_to_modify;
		};
	bool changeNotify(const neutrino_locale_t, void * data);
};

class CColorSetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void *);
};

class CAudioSetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t OptionName, void *);
};

class CAudioSetupNotifier2 : public CChangeObserver
{
	private:
		CMenuItem* toDisable[1];
	public:
		CAudioSetupNotifier2( CMenuItem* );
		bool changeNotify(const neutrino_locale_t, void *);
};

class CKeySetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void *);
};

class CShutdownCountNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void *);
};

class CIPChangeNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void * Data);
};

class CConsoleDestChangeNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void * Data);
};

class CTimingSettingsNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t OptionName, void *);
};

class CFontSizeNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t, void *);
};

class CRecAPIDSettingsNotifier : public CChangeObserver
{
	public:
		bool changeNotify(const neutrino_locale_t OptionName, void*);
};

class CAPIDChangeExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

void showSubchan(const std::string & subChannelName);
class CNVODChangeExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CStreamFeaturesChangeExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CMoviePluginChangeExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

class CUCodeCheckExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

void testNetworkSettings(const char* ip, const char* netmask, const char* broadcast, const char* gateway, const char* nameserver, bool ip_static);
void showCurrentNetworkSettings();


#endif
