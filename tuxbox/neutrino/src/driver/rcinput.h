/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __MOD_rcinput__
#define __MOD_rcinput__

#ifdef HAVE_DREAMBOX_HARDWARE
#define OLD_RC_API
#endif

#include <linux/input.h>
#include <string>
#include <vector>

#ifndef KEY_OK
#define KEY_OK           0x160
#endif

#ifndef KEY_RED
#define KEY_RED          0x18e
#endif

#ifndef KEY_GREEN
#define KEY_GREEN        0x18f
#endif

#ifndef KEY_YELLOW
#define KEY_YELLOW       0x190
#endif

#ifndef KEY_BLUE
#define KEY_BLUE         0x191
#endif

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



typedef uint neutrino_msg_t;
typedef uint neutrino_msg_data_t;

#define NEUTRINO_UDS_NAME "/tmp/neutrino.sock"


class CRCInput
{
	private:
		struct event
		{
			neutrino_msg_t      msg;
			neutrino_msg_data_t data;
		};

		struct timer
		{
			uint			id;
			unsigned long long	interval;
			unsigned long long	times_out;
			bool			correct_time;
		};

		uint               timerid;
		std::vector<timer> timers;

		int 		fd_pipe_high_priority[2];
		int 		fd_pipe_low_priority[2];
#ifdef OLD_RC_API
#define NUMBER_OF_EVENT_DEVICES 1
#else /* OLD_RC_API */
#define NUMBER_OF_EVENT_DEVICES 2
#endif /* OLD_RC_API */
		int         	fd_rc[NUMBER_OF_EVENT_DEVICES];
		int		fd_keyb;
		int		fd_event;

		int		fd_max;

		void open();
		void close();
		int translate(int code);

		void calculateMaxFd(void);

		int checkTimers();

	public:
		//rc-code definitions
		static const neutrino_msg_t RC_MaxRC    = KEY_MAX;    /* /include/linux/input.h: #define KEY_MAX                 0x1ff */
		static const neutrino_msg_t RC_KeyBoard = 0x4000;
		static const neutrino_msg_t RC_Events   = 0x80000000;
		static const neutrino_msg_t RC_Messages = 0x90000000;
		static const neutrino_msg_t RC_WithData = 0xA0000000;
		enum
		{
			RC_0            = KEY_0,            /* /include/linux/input.h: #define KEY_0			11   */
			RC_1            = KEY_1,            /* /include/linux/input.h: #define KEY_1			 2   */
			RC_2            = KEY_2,            /* /include/linux/input.h: #define KEY_2			 3   */
			RC_3            = KEY_3,            /* /include/linux/input.h: #define KEY_3			 4   */
			RC_4            = KEY_4,            /* /include/linux/input.h: #define KEY_4			 5   */
			RC_5            = KEY_5,            /* /include/linux/input.h: #define KEY_5			 6   */
			RC_6            = KEY_6,            /* /include/linux/input.h: #define KEY_6			 7   */
			RC_7            = KEY_7,            /* /include/linux/input.h: #define KEY_7			 8   */
			RC_8            = KEY_8,            /* /include/linux/input.h: #define KEY_8			 9   */
			RC_9            = KEY_9,            /* /include/linux/input.h: #define KEY_9			10   */
			RC_backspace    = KEY_BACKSPACE,    /* /include/linux/input.h: #define KEY_BACKSPACE            14   */
			RC_home         = KEY_HOME,         /* /include/linux/input.h: #define KEY_HOME                102   */
			RC_up           = KEY_UP,           /* /include/linux/input.h: #define KEY_UP                  103   */
			RC_page_up      = KEY_PAGEUP,       /* /include/linux/input.h: #define KEY_PAGEUP              104   */
			RC_left         = KEY_LEFT,         /* /include/linux/input.h: #define KEY_LEFT                105   */
			RC_right        = KEY_RIGHT,        /* /include/linux/input.h: #define KEY_RIGHT               106   */
			RC_down         = KEY_DOWN,         /* /include/linux/input.h: #define KEY_DOWN                108   */
			RC_page_down    = KEY_PAGEDOWN,     /* /include/linux/input.h: #define KEY_PAGEDOWN            109   */
			RC_spkr         = KEY_MUTE,         /* /include/linux/input.h: #define KEY_MUTE                113   */
			RC_minus        = KEY_VOLUMEDOWN,   /* /include/linux/input.h: #define KEY_VOLUMEDOWN          114   */
			RC_plus         = KEY_VOLUMEUP,     /* /include/linux/input.h: #define KEY_VOLUMEUP            115   */
			RC_standby      = KEY_POWER,        /* /include/linux/input.h: #define KEY_POWER               116   */
			RC_help         = KEY_HELP,         /* /include/linux/input.h: #define KEY_HELP                138   */
			RC_setup        = KEY_SETUP,        /* /include/linux/input.h: #define KEY_SETUP               141   */
			RC_ok           = KEY_OK,           /* /include/linux/input.h: #define KEY_OK           0x160        */ /* in patched input.h */
			RC_red          = KEY_RED,          /* /include/linux/input.h: #define KEY_RED          0x18e        */ /* in patched input.h */
			RC_green        = KEY_GREEN,        /* /include/linux/input.h: #define KEY_GREEN        0x18f        */ /* in patched input.h */
			RC_yellow       = KEY_YELLOW,       /* /include/linux/input.h: #define KEY_YELLOW       0x190        */ /* in patched input.h */
			RC_blue         = KEY_BLUE,         /* /include/linux/input.h: #define KEY_BLUE         0x191        */ /* in patched input.h */
			RC_top_left     = KEY_TOPLEFT,      /* /include/linux/input.h: #define KEY_TOPLEFT      0x1a2        */ /* in patched input.h */
			RC_top_right    = KEY_TOPRIGHT,     /* /include/linux/input.h: #define KEY_TOPRIGHT     0x1a3        */ /* in patched input.h */
			RC_bottom_left  = KEY_BOTTOMLEFT,   /* /include/linux/input.h: #define KEY_BOTTOMLEFT   0x1a4        */ /* in patched input.h */
			RC_bottom_right = KEY_BOTTOMRIGHT,  /* /include/linux/input.h: #define KEY_BOTTOMRIGHT  0x1a5        */ /* in patched input.h */
#ifdef HAVE_DREAMBOX_HARDWARE
			// definitions for additional buttons on Dreambox remote
			RC_tv           = KEY_TV,           /* /include/linux/input.h: #define KEY_TV		0x179 */
			RC_radio        = KEY_RADIO,        /* /include/linux/input.h: #define KEY_RADIO	0x181 */
			RC_text         = KEY_TEXT,         /* /include/linux/input.h: #define KEY_TEXT		0x184 */
			RC_audio        = KEY_AUDIO,        /* /include/linux/input.h: #define KEY_AUDIO	0x188 */
			RC_video        = KEY_VIDEO,        /* /include/linux/input.h: #define KEY_VIDEO	0x189 */
			RC_next         = KEY_NEXT,         /* /include/linux/input.h: #define KEY_NEXT		0x197 */
			RC_prev         = KEY_PREVIOUS,     /* /include/linux/input.h: #define KEY_PREVIOUS	0x19c */
#endif
		        //////////////// Keys on the IR Keyboard, not the RC ////////////////
			RC_esc		= KEY_ESC,		/* #define KEY_ESC		 	1  */

			// RC_minus is (mis-)used above above volume, call it RC_hyphen instead
			RC_hyphen	= KEY_MINUS, 		/* #define KEY_MINUS		 	12 */
			RC_equal	= KEY_EQUAL, 		/* #define KEY_EQUAL		 	13 */
			//RC_backspace	= KEY_BACKSPACE, 	/* #define KEY_BACKSPACE		14 */
			RC_tab		= KEY_TAB, 		/* #define KEY_TAB			15 */
			RC_q		= KEY_Q,		/* #define KEY_Q			16 */
			RC_w		= KEY_W,		/* #define KEY_W			17 */
			RC_e		= KEY_E,		/* #define KEY_E			18 */
			RC_r		= KEY_R,		/* #define KEY_R			19 */
			RC_t		= KEY_T,		/* #define KEY_T			20 */
			RC_y		= KEY_Y,		/* #define KEY_Y			21 */
			RC_u		= KEY_U,		/* #define KEY_U			22 */
			RC_i		= KEY_I,		/* #define KEY_I			23 */
			RC_o		= KEY_O,		/* #define KEY_O			24 */
			RC_p		= KEY_P,		/* #define KEY_P			25 */
			RC_leftbrace	= KEY_LEFTBRACE,	/* #define KEY_LEFTBRACE		26 */
			RC_rightbrace	= KEY_RIGHTBRACE,	/* #define KEY_RIGHTBRACE		27 */
			RC_enter	= KEY_ENTER,		/* #define KEY_ENTER			28 */
			RC_leftctrl	= KEY_LEFTCTRL,		/* #define KEY_LEFTCTRL			29 */
			RC_a		= KEY_A,		/* #define KEY_A			30 */
			RC_s		= KEY_S,		/* #define KEY_S			31 */
			RC_d		= KEY_D,		/* #define KEY_D			32 */
			RC_f		= KEY_F,		/* #define KEY_F			33 */
			RC_g		= KEY_G,		/* #define KEY_G			34 */
			RC_h		= KEY_H,		/* #define KEY_H			35 */
			RC_j		= KEY_J,		/* #define KEY_J			36 */
			RC_k		= KEY_K,		/* #define KEY_K			37 */
			RC_l		= KEY_L,		/* #define KEY_L			38 */
			RC_semicolon	= KEY_SEMICOLON,	/* #define KEY_SEMICOLON		39 */
			RC_apostrophe	= KEY_APOSTROPHE,	/* #define KEY_APOSTROPHE		40 */
			RC_grave	= KEY_GRAVE,		/* #define KEY_GRAVE			41 */
			RC_leftshift	= KEY_LEFTSHIFT,	/* #define KEY_LEFTSHIFT		42 */
			RC_backslash	= KEY_BACKSLASH,	/* #define KEY_BACKSLASH		43 */
			RC_z		= KEY_Z,		/* #define KEY_Z			44 */
			RC_x		= KEY_X,		/* #define KEY_X			45 */
			RC_c		= KEY_C,		/* #define KEY_C			46 */
			RC_v		= KEY_V,		/* #define KEY_V			47 */
			RC_b		= KEY_B,		/* #define KEY_B			48 */
			RC_n		= KEY_N,		/* #define KEY_N			49 */
			RC_m		= KEY_M,		/* #define KEY_M			50 */
			RC_comma	= KEY_COMMA, 		/* #define KEY_COMMA		 	51 */
			RC_dot		= KEY_DOT, 		/* #define KEY_DOT			52 */
			RC_slash	= KEY_SLASH, 		/* #define KEY_SLASH		 	53 */
			RC_rightshift	= KEY_RIGHTSHIFT, 	/* #define KEY_RIGHTSHIFT		54 */
			RC_kpasterisk	= KEY_KPASTERISK, 	/* #define KEY_KPASTERISK		55 */
			RC_leftalt	= KEY_LEFTALT, 		/* #define KEY_LEFTALT		 	56 */
			RC_space	= KEY_SPACE, 		/* #define KEY_SPACE		 	57 */
			RC_capslock	= KEY_CAPSLOCK, 	/* #define KEY_CAPSLOCK		 	58 */
			RC_f1		= KEY_F1, 	 	/* #define KEY_F1			59 */
			RC_f2		= KEY_F2, 	 	/* #define KEY_F2			60 */
			RC_f3		= KEY_F3, 	 	/* #define KEY_F3			61 */
			RC_f4		= KEY_F4, 	 	/* #define KEY_F4			62 */
			RC_f5		= KEY_F5, 	 	/* #define KEY_F5			63 */
			RC_f6		= KEY_F6, 	 	/* #define KEY_F6			64 */
			RC_f7		= KEY_F7, 	 	/* #define KEY_F7			65 */
			RC_f8		= KEY_F8, 	 	/* #define KEY_F8			66 */
			RC_f9		= KEY_F9, 	 	/* #define KEY_F9			67 */
			RC_f10		= KEY_F10, 	 	/* #define KEY_F10			68 */
			RC_numlock	= KEY_NUMLOCK, 	 	/* #define KEY_NUMLOCK		 	69 */
			RC_scrolllock	= KEY_SCROLLLOCK, 	/* #define KEY_SCROLLLOCK		70 */
			RC_kp7		= KEY_KP7,		/* #define KEY_KP7			71 */
			RC_kp8		= KEY_KP8,		/* #define KEY_KP8			72 */
			RC_kp9		= KEY_KP9,		/* #define KEY_KP9			73 */
			RC_kpminus	= KEY_KPMINUS,		/* #define KEY_KPMINUS			74 */
			RC_kp4		= KEY_KP4,		/* #define KEY_KP4			75 */
			RC_kp5		= KEY_KP5,		/* #define KEY_KP5			76 */
			RC_kp6		= KEY_KP6,		/* #define KEY_KP6			77 */
			RC_kpplus	= KEY_KPPLUS,		/* #define KEY_KPPLUS			78 */
			RC_kp1		= KEY_KP1,		/* #define KEY_KP1			79 */
			RC_kp2		= KEY_KP2,		/* #define KEY_KP2			80 */
			RC_kp3		= KEY_KP3,		/* #define KEY_KP3			81 */
			RC_kp0		= KEY_KP0,		/* #define KEY_KP0			82 */
			RC_kpdot	= KEY_KPDOT,		/* #define KEY_KPDOT			83 */

			RC_102nd	= KEY_102ND, 	 	/* #define KEY_102ND		 	86 */
			RC_kpenter	= KEY_KPENTER,		/* #define KEY_KPENTER			96 */
			RC_kpslash	= KEY_KPSLASH,		/* #define KEY_KPSLASH			98 */
			RC_sysrq	= KEY_SYSRQ,		/* #define KEY_SYSRQ			99 */

			RC_rightalt	= KEY_RIGHTALT, 	/* #define KEY_RIGHTALT		 	100 */
			RC_end		= KEY_END, 	 	/* #define KEY_END			107 */
			RC_insert	= KEY_INSERT, 	 	/* #define KEY_INSERT		 	110 */
			RC_delete	= KEY_DELETE, 	 	/* #define KEY_DELETE		 	111 */

			RC_pause	= KEY_PAUSE, 	 	/* #define KEY_PAUSE		 	119 */

			RC_leftmeta	= KEY_LEFTMETA,		/* #define KEY_LEFTMETA			125 */
			RC_rightmeta	= KEY_RIGHTMETA,	/* #define KEY_RIGHTMETA		126 */

			RC_btn_left	= BTN_LEFT, 	 	/* #define BTN_LEFT		 	0x110 */
			RC_btn_right	= BTN_RIGHT, 	 	/* #define BTN_RIGHT		 	0x111 */
			//////////////// End of IR Keyboard only keys ////////////////

			RC_timeout	= 0xFFFFFFFF,
			RC_nokey	= 0xFFFFFFFE
		};

		inline int getFileHandle(void) /* used for plugins (i.e. games) only */
		{
			return fd_rc[0];
		}
		void stopInput();
		void restartInput();

		unsigned long long repeat_block;
		unsigned long long repeat_block_generic;
		CRCInput();      //constructor - opens rc-device and starts needed threads
		~CRCInput();     //destructor - closes rc-device


		static bool isNumeric(const neutrino_msg_t key);
		static int getNumericValue(const neutrino_msg_t key);
		static unsigned int convertDigitToKey(const unsigned int digit);
		static int getUnicodeValue(const neutrino_msg_t key);

		static const char * getSpecialKeyName(const unsigned int key);
		static std::string getKeyName(const unsigned int key);

		int addTimer(unsigned long long Interval, bool oneshot= true, bool correct_time= true );
		int addTimer(struct timeval Timeout);
		int addTimer(const time_t *Timeout);

		void killTimer(uint id);

		static long long calcTimeoutEnd_MS(const int timeout_in_milliseconds);
		static long long calcTimeoutEnd(const int timeout_in_seconds);

		void getMsgAbsoluteTimeout(neutrino_msg_t * msg, neutrino_msg_data_t * data, unsigned long long *TimeoutEnd, bool bAllowRepeatLR= false);
		void getMsg(neutrino_msg_t * msg, neutrino_msg_data_t * data, int Timeout, bool bAllowRepeatLR= false);                  //get message, timeout in 1/10 secs :)
		void getMsg_ms(neutrino_msg_t * msg, neutrino_msg_data_t * data, int Timeout, bool bAllowRepeatLR= false);               //get message, timeout in msecs :)
		void getMsg_us(neutrino_msg_t * msg, neutrino_msg_data_t * data, unsigned long long Timeout, bool bAllowRepeatLR= false);//get message, timeout in µsecs :)
		void postMsg(const neutrino_msg_t msg, const neutrino_msg_data_t data, const bool Priority = true);  // push message back into buffer
		void clearRCMsg();

		int messageLoop( bool anyKeyCancels = false, int timeout= -1 );
};


#endif
