#ifndef RC_INPUT_H
#define RC_INPUT_H

#include "input_fake.h"

#define RC_0			KEY_0
#define RC_1			KEY_1
#define RC_2			KEY_2
#define RC_3			KEY_3
#define RC_4			KEY_4
#define RC_5			KEY_5
#define RC_6			KEY_6
#define RC_7			KEY_7
#define RC_8			KEY_8
#define RC_9			KEY_9
#define RC_RIGHT		KEY_RIGHT
#define RC_LEFT			KEY_LEFT
#define RC_UP			KEY_UP
#define RC_DOWN			KEY_DOWN
#define RC_OK			KEY_OK
#define RC_SPKR			KEY_MUTE
#define RC_STANDBY		KEY_POWER
#define RC_GREEN		KEY_GREEN
#define RC_YELLOW		KEY_YELLOW
#define RC_RED			KEY_RED
#define RC_BLUE			KEY_BLUE
#define RC_PLUS			KEY_VOLUMEUP
#define RC_MINUS		KEY_VOLUMEDOWN
#define RC_HELP			KEY_HELP
#define RC_SETUP		KEY_SETUP
#define RC_HOME			KEY_HOME
#define RC_PAGE_DOWN		KEY_PAGEDOWN
#define RC_PAGE_UP		KEY_PAGEUP

/* SAGEM remote controls have the following additional keys */

#ifndef KEY_TOPLEFT
#define KEY_TOPLEFT      0x1a2
#endif

#ifndef KEY_TOPRIGHT
#define KEY_TOPRIGHT     0x1a3
#endif

#ifndef KEY_BOTTOMLEFT
#define KEY_BOTTOMLEFT   0x1a4
#endif

#ifndef KEY_BOTTOMRIGHT
#define KEY_BOTTOMRIGHT  0x1a5
#endif


extern	void			RcGetActCode( void );
extern	int				RcInitialize( int extfd );
extern	void			RcClose( void );

#endif
