/*
 * $Header: /cvs/tuxbox/apps/dvb/zapit/include/zapit/Attic/xmlinterface.h,v 1.2 2002/09/19 10:24:50 thegoodguy Exp $
 */

#include <string>

#include "xml/xmltree.h"


std::string convertForXML(const std::string s);
std::string Utf8_to_Latin1(const std::string s);

XMLTreeParser* parseXmlFile(const std::string filename);
