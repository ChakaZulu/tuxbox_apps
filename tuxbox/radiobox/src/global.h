#ifndef __global_h__
#define __global_h__

#ifndef RADIOBOX_CPP
  #define RADIOBOX_CPP extern
#endif

#include <settings.h>

RADIOBOX_CPP  CRadioboxSettings	g_settings;

#define DBGINFO "[" << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "]: "


#endif /* __global_h__ */
