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
$Log: pat.h,v $
Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef PAT_H
#define PAT_H

#include <map>

struct pat_entry
{
	int TS;
	int ONID;
	int SID;
	int PMT;
};

class pat
{
	int oldpatTS;
	int ONID;
	std::multimap<int, struct pat_entry> pat_list;
public:	
	bool readPAT();
	int getTS() { return (*pat_list.begin()).second.TS; }
	int getONID() { return ONID; }
	int getPMT(int SID);
};

#endif
