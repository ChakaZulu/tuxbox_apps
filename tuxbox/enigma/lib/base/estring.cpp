#include <lib/base/estring.h>
#include <ctype.h>
#include <limits.h>
#include <lib/system/elock.h>

static eLock lock;

///////////////////////////////////////// eString sprintf /////////////////////////////////////////////////
eString& eString::sprintf(char *fmt, ...)
{
	eLocker locker(lock);
// Implements the normal sprintf method, to use format strings with eString
// The max length of the result string is 1024 char.
	static char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	std::vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	assign(buf);
	return *this;
}

///////////////////////////////////////// eString setNum(uint, uint) ///////////////////////////////////////
eString& eString::setNum(int val, int sys)
{
//	Returns a string that contain the value val as string
//	if sys == 16 than hexadezimal if sys == 10 than decimal
	char buf[12];

	if (sys == 10)
		std::snprintf(buf, 12, "%i", val);
	else if (sys == 16)
		std::snprintf(buf, 12, "%X", val);		
	
	assign(buf);
	return *this;
}

///////////////////////////////////////// eString replaceChars(char, char) /////////////////////////////
eString& eString::removeChars(char fchar)
{
//	Remove all chars that equal to fchar, and returns a reference to itself
	unsigned int index=0;

	while ( ( index = find(fchar, index) ) != npos )
		erase(index, 1);

	return *this;
}

/////////////////////////////////////// eString upper() ////////////////////////////////////////////////
eString& eString::upper()
{
//	convert all lowercase characters to uppercase, and returns a reference to itself
	for (iterator it = begin(); it != end(); it++)
		switch(*it)
		{
			case 'a' ... 'z' :
				*it -= 32;
			break;

			case 'ä' :
				*it = 'Ä';
			break;
			
			case 'ü' :
				*it = 'Ü';
			break;
			
			case 'ö' :
				*it = 'Ö';
			break;
		}

	return *this;
}

eString& eString::strReplace(const char* fstr, const eString& rstr)
{
//	replace all occurrence of fstr with rstr and, and returns a reference to itself
	unsigned int index=0;
	unsigned int fstrlen = strlen(fstr);

	while ( ( index = find(fstr, index) ) != npos )
		replace(index++, fstrlen, rstr);
	
	return *this;
}

int strnicmp(const char *s1, const char *s2, int len)
{
//	makes a case insensitive string compare with len Chars
	while ( *s1 && *s2 && len-- )
		if ( tolower(*s1) != tolower(*s2) )
			return tolower(*s1) < tolower(*s2) ? -1 : 1;
		else
			s1++, s2++;

	return 0;
}

/////////////////////////////////////// eString icompare(const eString&) ////////////////////////////////////////////////
int eString::icompare(const eString& s)
{
//	makes a case insensitive string compare
	std::string::const_iterator p = begin(),
															p2 = s.begin();

	while ( p != end() && p2 != s.end() )
		if ( tolower(*p) != tolower(*p2) )
			return tolower(*p) < tolower(*p2) ? -1 : 1;
		else
			p++, p2++;

	return 0;
}
