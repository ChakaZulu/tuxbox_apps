#include <map>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <lib/base/estring.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <lib/dvb/servicestructure.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/dvbservice.h>
#include <lib/dvb/service.h>
#include <enigma_dyn_utils.h>

using namespace std;

extern int smallScreen;

eString getAttribute(eString filename, eString attribute)
{
	eString result = "&nbsp;";

	ifstream infile(filename.c_str());
	if (infile)
	{
		eString buffer;
		while (getline(infile, buffer, '\n'))
		{
			if (buffer.find(attribute + "=") == 0)
			{
				result = getRight(buffer, '=');
				if (result == "")
					result = "&nbsp;";
				break;
			}
		}
	}
	return result;
}

eString readFile(eString filename)
{
	eString result;
	eString line;

	ifstream infile(filename.c_str());
	if (infile)
		while (getline(infile, line, '\n'))
			result += line + "\n";

	return result;
}

eString button(int width, eString buttonText, eString buttonColor, eString buttonRef)
{
	eString ref1, ref2;

	std::stringstream result;
	int height = 22;
	if (smallScreen == 1)
	{
		width = width / 2;
		height = 14;
	}

	if (buttonRef.find("javascript") == eString::npos)
	{
		ref1 = "\"self.location.href='";
		ref2 = "'\"";
	}
	result << "<input name=\"" << buttonText << "\""
		"type=\"button\" style='width: " << width << "px;"
		"height:" << height << "px;";
	if (buttonColor != "")
		result << "background-color: " << buttonColor;
	result << "' value=\"" << buttonText <<
		"\" onclick=" << ref1 << buttonRef <<
		ref2 << ">";
	return result.str();
}

eString getTitle(eString title)
{
	return "<h1>" + title + "</h1>";
}

eString filter_string(eString string)
{
	string.strReplace("\xc2\x86","");
	string.strReplace("\xc2\x87","");
	string.strReplace("\xc2\x8a"," ");
	return string;
}

int getHex(int c)
{
	c = toupper(c);
	c -= '0';
	if (c < 0)
		return -1;
	if (c > 9)
		c -= 'A' - '0' - 10;
	if (c > 0xF)
		return -1;
	return c;
}

eString httpUnescape(const eString &string)
{
	eString result;
	for (unsigned int i = 0; i < string.length(); ++i)
	{
		int c = string[i];
		switch (c)
		{
		case '%':
		{
			int value = '%';
			if ((i + 1) < string.length())
				value = getHex(string[++i]);
			if ((i + 1) < string.length())
			{
				value <<= 4;
				value += getHex(string[++i]);
			}
			result += value;
			break;
		}
		case '+':
			result += ' ';
			break;
		default:
			result += c;
			break;
		}
	}
	return result;
}

eString httpEscape(const eString &string)
{
	eString result;
	for (unsigned int i = 0; i < string.length(); ++i)
	{
		int c = string[i];
		int valid = 0;
		if ((c >= 'a') && (c <= 'z'))
			valid = 1;
		else if ((c >= 'A') && (c <= 'Z'))
			valid = 1;
		else if (c == ':')
			valid = 1;
		else if ((c >= '0') && (c <= '9'))
			valid = 1;

		if (valid)
			result += c;
		else
			result += eString().sprintf("%%%x", c);
	}
	return result;
}

eString ref2string(const eServiceReference &r)
{
	return httpEscape(r.toString());
}

eServiceReference string2ref(const eString &service)
{
	eString str = httpUnescape(service);
	return eServiceReference(str);
}

std::map<eString, eString> getRequestOptions(eString opt, char delimiter)
{
	std::map<eString, eString> result;

	if (opt[0] == '?')
		opt = opt.mid(1);

	while (opt.length())
	{
		unsigned int e = opt.find("=");
		if (e == eString::npos)
			e = opt.length();
		unsigned int a = opt.find(delimiter, e);
		if (a == eString::npos)
			a = opt.length();
		eString n = opt.left(e);

		unsigned int b = opt.find(delimiter, e + 1);
		if (b == eString::npos)
			b = (unsigned)-1;
		eString r = httpUnescape(opt.mid(e + 1, b - e - 1));
		result.insert(std::pair<eString, eString>(n, r));
		opt = opt.mid(a + 1);
	}
	return result;
}
