/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: settings.h,v $
Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

#include "cam.h"

#define NOKIA 1
#define PHILIPS 2
#define SAGEM 3

struct setting_s
{
	int timeoffset;
	unsigned int ip;
	bool txtreinsertion;
};

class settings
{
	bool isCable;
	bool isGTX;
	int CAID;
	int EMM;
	int find_emmpid(int ca_system_id);
	int box; // 1= nokia 2=sagem
	int oldTS;
	bool usediseqc;
	cam ca;
	std::string version;
	setting_s setting;
public:	
	settings(cam &c);
	void initme();
	bool boxIsCable();
	bool boxIsSat();
	int getCAID();
	int getTransparentColor();
	int getEMMpid(int TS = -1);
	void setIP(char n1, char n2, char n3, char n4);
	char getIP(char number);
	int getBox() { return box; }
	bool boxIsGTX() { return isGTX; }
	void setDiseqc(bool use) { usediseqc = use; }
	bool useDiseqc() { return usediseqc; }

	void setTXTReinsertion(bool insert) { setting.txtreinsertion = insert; }
	bool getTXTReinsertion() { return setting.txtreinsertion; }

	void setVersion(std::string ver) { version = ver; }
	std::string getVersion() { return version; }
	void setTimeOffset(int offset) { setting.timeoffset = offset; }
	int getTimeOffset() { return setting.timeoffset; }
	void saveSettings();
	void loadSettings();
};

#endif // ZAP_H
