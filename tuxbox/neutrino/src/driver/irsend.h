/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Sven Traenkle 'Zwen'
	License: GPL

	Aenderungen: $Log: irsend.h,v $
	Aenderungen: Revision 1.1  2002/11/24 19:55:56  Zwen
	Aenderungen: - send ir signals on sleeptimer event (see timer docu)
	Aenderungen:
*/
#ifndef __irsend__
#define __irsend__

#include <string>

using namespace std;

class CIRSend
{
   public:
		CIRSend(string configfile);
		bool Send();
   private:
		string m_configFile;
};
#endif
