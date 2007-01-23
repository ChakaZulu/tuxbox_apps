/*
	Neutrino-GUI  -   DBoxII-Project


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


#ifndef __streaminfo2__
#define __streaminfo2__

#include <gui/widget/menue.h>

#include <driver/framebuffer.h>
#include <driver/pig.h>
#include <driver/BitrateCalculator.h>

class BitrateCalculator;

class CStreamInfo2
{
	private:

		CFrameBuffer	*frameBuffer;
		int x;
		int y;
		int width;
		int height;
		int hheight,iheight,sheight; 	// head/info/small font height

		int  max_height;	// Frambuffer 0.. max
		int  max_width;	

		CPIG *pig;

		// -- color hack using evil knowledge from frambuffer.c
		enum {	COL_WHITE=0x60, COL_RED, COL_GREEN,
			COL_BLUE, COL_YELLOW, COL_BLACK };

		int  paint_mode;

		int  font_head;
		int  font_info;
		int  font_small;

		int   sigBox_x;
		int   sigBox_y;
		int   sigBox_w;
		int   sigBox_h;
		int   sigBox_pos;
		int   sig_text_y;
		int   sig_text_ber_x;
		int   sig_text_sig_x;
		int   sig_text_snr_x;
		int   sig_text_rate_x;
		int   current_apid;
		int   actmode;
		BitrateCalculator *brc;

		struct feSignal {
			unsigned long	ber, old_ber;
			unsigned long	sig, old_sig;
			unsigned long	snr, old_snr;
			// int	has_lock;
			// int	has_signal;
		} signal;


		int  doSignalStrengthLoop();
		void paint(int mode);
		void paint_pig(int x, int y, int w, int h);
		void paint_techinfo(int x, int y);
		void paint_bitrate(unsigned long bitrate);
		void paint_signal_fe_box(int x, int y, int w, int h);
		void paint_signal_fe(unsigned long bitrate, struct feSignal s);
		int  y_signal_fe(unsigned long value, unsigned long max_range, int max_y);
		void SignalRenderStr (unsigned long value, int x, int y);
		void hide();
	public:
		CStreamInfo2();
		 ~CStreamInfo2();
		int exec();

};
class CStreamInfo2Handler : public CMenuTarget
{
	public:
		int exec( CMenuTarget* parent,  const std::string &actionKey);
};


#endif

