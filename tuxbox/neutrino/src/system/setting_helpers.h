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

#ifndef __setting_helpers__
#define __setting_helpers__

#include "../widget/menue.h"
#include "libnet.h"
#include "libucodes/libucodes.h"


class CStartNeutrinoDirectNotifier : public CChangeObserver
{
	public:
		bool changeNotify(string OptionName, void*);
};

class CColorSetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(string OptionName, void*);
};

class CAudioSetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(string OptionName, void*);
};

class CVideoSetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(string OptionName, void*);
};

class CLanguageSetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(string OptionName, void*);
};

class CKeySetupNotifier : public CChangeObserver
{
	public:
		bool changeNotify(string OptionName, void*);
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

class CStreamFeaturesChangeExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, string actionKey);
};

class CUCodeCheckExec : public CMenuTarget
{
	public:
		int exec(CMenuTarget* parent, string actionKey);
};

void setDefaultGateway(char* ip);
void setNetworkAddress(char* ip, char* netmask, char* broadcast);
void setNameServer(char* ip);
void testNetworkSettings(char* ip, char* netmask, char* broadcast, char* gateway
, char* nameserver);

#endif
