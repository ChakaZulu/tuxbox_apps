#ifndef __LIBSIG_COMP_H
#define __LIBSIG_COMP_H

#include <sigc++/signal_system.h>
#ifdef SIGC_CXX_NAMESPACES
using namespace SigC;
#endif

#define CONNECT(SENDER, EMPFAENGER) SENDER.connect(slot(*this, &EMPFAENGER))

#endif // __LIBSIG_COMP_H
