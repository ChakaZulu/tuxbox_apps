/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2002 Sven Traenkle 'Zwen'
	License: GPL

	Aenderungen: $Log: irsend.h,v $
	Aenderungen: Revision 1.1  2004/02/19 22:29:18  zwen
	Aenderungen: - moved irsend to neutrino libs
	Aenderungen:
	Aenderungen: Revision 1.2  2003/09/19 19:25:27  thegoodguy
	Aenderungen: cleanup
	Aenderungen:
	Aenderungen: Revision 1.1  2002/11/24 19:55:56  Zwen
	Aenderungen: - send ir signals on sleeptimer event (see timer docu)
	Aenderungen:
*/
#ifndef __irsend__
#define __irsend__

#include <string>

class CIRSend
{
   public:
		CIRSend(const char * const configfile);
		bool Send();
   private:
		std::string m_configFile;
};
#endif
