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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

#include "cam.h"

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
	int timeoffset;
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
	void setVersion(std::string ver) { version = ver; }
	std::string getVersion() { return version; }
	void setTimeOffset(int offset) { timeoffset = offset; }
	int getTimeOffset() { return timeoffset; }
};

#endif // ZAP_H
