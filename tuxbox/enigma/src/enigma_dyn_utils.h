#ifndef __enigma_dyn_utils_h
#define __enigma_dyn_utils_h

eString getAttribute(eString filename, eString attribute);
eString readFile(eString filename);
eString button(int width, eString buttonText, eString buttonColor, eString buttonRef);
eString getTitle(eString title);
int getHex(int c);
eString httpUnescape(const eString &string);
eString filter_string(eString string);
eString httpEscape(const eString &string);
std::map<eString, eString> getRequestOptions(eString opt, char delimiter);

extern eString getRight(const eString&, char); // implemented in timer.cpp
extern eString getLeft(const eString&, char);  // implemented in timer.cpp

#endif /* __enigma_dyn_utils_h */
