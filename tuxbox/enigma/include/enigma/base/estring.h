#ifndef __E_STRING__
#define __E_STRING__

#include <string>
#include <stdarg.h>
#include <stdio.h>
#include "eerror.h"

class eString : public std::string
{
public:
// Constructors
	inline eString()	{}	
	inline eString(const char* p);
	inline eString(const std::string &s);
// methods
	inline eString left(unsigned int len) const;
	inline eString mid(unsigned int index, unsigned int len=0) const;
	inline eString right(unsigned int len) const;
	bool isNull() const;
// operators
	inline operator bool() const;
	inline bool operator!() const;
	inline operator const char*() const;
// methods with implementation in estring.cpp
	eString& eString::sprintf(char *fmt, ...);
	eString& setNum(int val, int sys=10);
	eString& removeChars(const char fchar);
	eString& strReplace(const char* fstr, const eString& rstr);
	eString& upper();
};

/////////////////////////////////////////////// Copy Constructors ////////////////////////////////////////////////
inline eString::eString(const std::string &s)
	:std::string(s)
{
}

inline eString::eString(const char* p)
	:std::string(p)
{
}

///////////////////////////////////////// eString operator bool /////////////////////////////////////////////////
inline eString::operator bool() const
{
// Returns a bool that contains true if the string longer than 0 Character otherwise false;
	return !empty();
}

///////////////////////////////////////// eString operator! ////////////////////////////////////////////////////
inline bool eString::operator!() const
{
// Returns a bool that contains true if the string ist empty otherwise false;
	return empty();
}

///////////////////////////////////////// eString operator! ////////////////////////////////////////////////////
inline eString::operator const char*() const
{
// Makes the eString compatibel with all functions and methods they accept const char*
// i hope this cause no ambiguouses
	return c_str();
}

///////////////////////////////////////// eString left //////////////////////////////////////////////////////////
inline eString eString::left(unsigned int len) const
{
//	Returns a substring that contains the len leftmost characters of the string.
//	The whole string is returned if len exceeds the length of the string.
	return substr(0, len);
}

//////////////////////////////////////// eString mid ////////////////////////////////////////////////////////////
inline eString eString::mid(unsigned int index, unsigned int len) const
{
//	Returns a substring that contains the len characters of this string, starting at position index.
//	Returns a null string if the string is empty or index is out of range. Returns the whole string from index if index+len exceeds the length of the string.
	return len?substr(index, len):substr(index);
}

//////////////////////////////////////// eString right ////////////////////////////////////////////////////////////
inline eString eString::right(unsigned int len) const
{
//	Returns a substring that contains the len rightmost characters of the string.
//	The whole string is returned if len exceeds the length of the string.
	int strlen = length();
	return substr(strlen-len-1, len);
}

inline bool eString::isNull() const
{
//	Returns a bool, that contains true, when the internal char* is null
	return c_str();
}

#endif // __E_STRING__
