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
$Log: teletext.h,v $
Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef TELETEXT_H
#define TELETEXT_H

#include "rc.h"
#include "fbClass.h"

class teletext
{
	fbClass *fb_obj;;
	rc *rc_obj;
public:
	teletext(fbClass *f, rc *r) { fb_obj = f; rc_obj = r; }
	void getTXT(int PID);
	void startReinsertion(int PID);
};

#endif
