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

//
// -- this module is a evil hack
// -- Neutrino lacks a proper OSD class
//


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define SCREEN_X	720
#define SCREEN_Y	572


#include <gui/streaminfo2.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/screen_max.h>
#include <gui/color.h>
#include <gui/widget/icons.h>

#include <daemonc/remotecontrol.h>
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */




#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <linux/dvb/dmx.h>

/*
 * some definition
 */


#define TS_LEN			188
#define TS_SYNC_BYTE		0x47
#define TS_BUF_SIZE		(TS_LEN * 2048)		/* fix dmx buffer size */

#define DMXDEV	"/dev/dvb/adapter0/demux0"
#define DVRDEV	"/dev/dvb/adapter0/dvr0"

#define dprintf(fmt, args...) { if (0) { printf(fmt, ## args); fflush(stdout); } }

/*
  -- Print receive time of Packet

*/

static unsigned long timeval_to_ms(const struct timeval *tv)
{
	return (tv->tv_sec * 1000) + ((tv->tv_usec + 500) / 1000);
}

long delta_time_ms (struct timeval *tv, struct timeval *last_tv)
{
	return timeval_to_ms(tv) - timeval_to_ms(last_tv);
}

class BitrateCalculator
{
	private:

		int 				pid;
		struct pollfd			pfd;
		struct dmx_pes_filter_params	flt;
		int 				dmxfd;
		struct timeval 			tv,last_tv, first_tv;
		unsigned long long		b_total;
		long				b;
		long				packets_bad;
		long				packets_total;
		u_char 	 			buf[TS_BUF_SIZE];

		struct {				// simple struct for storing last average bandwidth
			unsigned long  kb_sec;
			unsigned long  b_sec;
		} last_avg;

	public:
		BitrateCalculator(int inPid);
		unsigned long long calc(void);
		int sync_ts (u_char *buf, int len);
		int ts_error_count (u_char *buf, int len);
		~BitrateCalculator();
};

BitrateCalculator::BitrateCalculator(int inPid)
{
	pid = inPid;
	printf("PID: %u (0x%04x)\n", pid, pid);

	// -- open DVR device for reading
	pfd.events = POLLIN | POLLPRI;
	if((pfd.fd = open(DVRDEV, O_RDONLY|O_NONBLOCK)) < 0){
		printf("error on %s\n", DVRDEV);
		return;
	}

	if ((dmxfd=open(DMXDEV, O_RDWR)) < 0) {
		printf("error on %s\n", DMXDEV);
		close(pfd.fd);
		return;
	}

	ioctl (dmxfd,DMX_SET_BUFFER_SIZE, sizeof(buf));
	flt.pid = pid;
	flt.input = DMX_IN_FRONTEND;
	flt.output = DMX_OUT_TS_TAP;
	flt.pes_type = DMX_PES_OTHER;
	flt.flags = DMX_IMMEDIATE_START;
	if (ioctl(dmxfd, DMX_SET_PES_FILTER, &flt) < 0) {
		printf("error on DMX_SET_PES_FILTER");
		close(pfd.fd);
		close(dmxfd);
		return;
	}

	gettimeofday (&first_tv, NULL);
	last_tv.tv_sec	=  first_tv.tv_sec;
	last_tv.tv_usec	=  first_tv.tv_usec;
	b_total		= 0;
	packets_total	= 0;
 	packets_bad	= 0;
}

BitrateCalculator::~BitrateCalculator(void)
{
	// -- packets stats
	printf("PID: %u (0x%04x)\n", pid, pid);
	printf("   bad/total packets: %ld/%ld (= %1.1Lf%%)\n",
		packets_bad, packets_total,
                (((long double) packets_bad)*100)/packets_total );
	printf("   Avrg: %5lu.%03lu kbit/s\n",
		last_avg.kb_sec, last_avg.b_sec);

	if (ioctl(dmxfd, DMX_STOP) < 0) {
		printf("error at DMX_STOP");
	}
	close(dmxfd);
	close(pfd.fd);
}

unsigned long long BitrateCalculator::calc(void)
{
	int b_len, b_start;
	unsigned long long avgbit_s;
	long  d_tim_ms;
	int   packets;

	// -- we will poll the PID in 2 secs intervall
	int timeout = 100;

	b_len = 0;
	b_start = 0;
	if (poll(&pfd, 1, timeout) > 0) {
		if (pfd.revents & POLLIN) {

			b_len = read(pfd.fd, buf, sizeof(buf));
			gettimeofday (&tv, NULL);
			
			if (b_len >= TS_LEN) {
				b_start = sync_ts (buf, b_len);
			} else {
				b_len = 0;
			}

			b = b_len - b_start;
			if (b == 0) return 0;
			if (b < 0) {
			   printf("error on read");
			   return 0;
			}

			b_total += b;

			packets = b/TS_LEN;
			packets_total += packets;

			// -- average bandwidth
			d_tim_ms = delta_time_ms (&tv, &first_tv);
			if (d_tim_ms <= 0) d_tim_ms = 1;   //  ignore usecs 
		
			avgbit_s = ((b_total * 8000ULL) + ((unsigned long long)d_tim_ms / 2ULL))
				   / (unsigned long long)d_tim_ms;
		
			last_avg.kb_sec = (unsigned long) (avgbit_s / 1000ULL);
			last_avg.b_sec  = (unsigned long) (avgbit_s % 1000ULL);
		
			dprintf("   (Avrg: %5lu.%03lu kbit/s)\n", last_avg.kb_sec, last_avg.b_sec);
	
			// -- bad packet(s) check in buffer
			{
			  int bp;

			  bp = ts_error_count (buf+b_start, b);
			  packets_bad += bp;
			  dprintf(" [bad: %d]\n", bp);
			}

			last_tv.tv_sec  =  tv.tv_sec;
			last_tv.tv_usec =  tv.tv_usec;
		}
	}
	return avgbit_s;
}



//
// -- sync TS stream (if not already done by firmware)
//

int BitrateCalculator::sync_ts (u_char *buf, int len)
{
	int  i;

	// find TS sync byte...
	// SYNC ...[188 len] ...SYNC...
	
	for (i=0; i < len; i++) {
		if (buf[i] == TS_SYNC_BYTE) {
		   if ((i+TS_LEN) < len) {
		      if (buf[i+TS_LEN] != TS_SYNC_BYTE) continue;
		   }
		   break;
		}
	}
	return i;
}




// 
//  count error packets (ts error bit set, if passed thru by firmware)
//  we are checking a synced buffer with 1..n TS packets
//  so, we have to check every TS_LEN the error bit
//  return: error count
//

int BitrateCalculator::ts_error_count (u_char *buf, int len) 
{
	int error_count = 0;

	while (len > 0) {

		// check  = getBits(buf, 0, 8, 1);
		if (*(buf+1) & 0x80) error_count++;

		len -= TS_LEN;
		buf += TS_LEN;

	}
	return error_count;
}








CStreamInfo2::CStreamInfo2()
{
	pig = new CPIG (0);
	frameBuffer = CFrameBuffer::getInstance();


	font_head = SNeutrinoSettings::FONT_TYPE_MENU_TITLE;
	font_info = SNeutrinoSettings::FONT_TYPE_MENU;
	font_small = SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL;

	hheight     = g_Font[font_head]->getHeight();
	iheight     = g_Font[font_info]->getHeight();
	sheight     = g_Font[font_small]->getHeight();

	width  = w_max (710, 5);
	height = h_max (560, 5); 

	max_height = SCREEN_Y-1;
	max_width  = SCREEN_X-1;


	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;

	frameBuffer->paletteSetColor(COL_WHITE,   0x00FFFFFF, 0);
	frameBuffer->paletteSetColor(COL_RED,     0x00FF0000, 0);
	frameBuffer->paletteSetColor(COL_GREEN,   0x0000FF00, 0);
	frameBuffer->paletteSetColor(COL_BLUE,    0x002020FF, 0);
	frameBuffer->paletteSetColor(COL_YELLOW,  0x0000FFFF, 0);
	frameBuffer->paletteSetColor(COL_BLACK,   0x00000000, 0);

	frameBuffer->paletteSet();

	sigBox_pos = 0;
	paint_mode = 0;
	
	brc = 0;
	int mode = g_Zapit->getMode();
	if (!g_Zapit->isRecordModeActive() && mode == 1) { 
		current_apid = -1;		
		actmode = g_Zapit->PlaybackState();
		if (actmode == 0) { //PES Mode aktiv
			CZapitClient::responseGetPIDs allpids;
			g_Zapit->getPIDS(allpids);
			for (unsigned int i = 0; i < allpids.APIDs.size(); i++) {
					if (allpids.APIDs[i].is_ac3) { //Suche Ac3 Pid
						if (i == allpids.PIDs.selected_apid) { //Aktuelle Pid ist ac3 pid
							current_apid = allpids.PIDs.selected_apid; //Speichere aktuelle pid und switche auf Stereo
							g_Zapit->setAudioChannel(0);
							break;
					}
				}
			}	
			g_Zapit->PlaybackSPTS();
		}
		
		if ( g_RemoteControl->current_PIDs.PIDs.vpid != 0 )
			brc = new BitrateCalculator(g_RemoteControl->current_PIDs.PIDs.vpid);
		else if (!g_RemoteControl->current_PIDs.APIDs.empty())
			brc = new BitrateCalculator(g_RemoteControl->current_PIDs.APIDs[0].pid);
	}
}

CStreamInfo2::~CStreamInfo2()
{
	if (!g_Zapit->isRecordModeActive()) {
		if (actmode == 0) {
			g_Zapit->PlaybackPES();
			if (current_apid != -1) {
				g_Zapit->setAudioChannel(current_apid);		
			}
		}
	}
	delete pig;
	if (brc) delete brc;
}

int CStreamInfo2::exec()
{
	int res;
	paint(paint_mode);
	doSignalStrengthLoop ();
	hide();

	res = menu_return::RETURN_REPAINT;
	return res;
}

void CStreamInfo2::paint_bitrate(unsigned long long bitrate) {
	char buf[100];
	int ypos = y+5;
	int xpos = x+10;
	int width  = w_max (710, 5);

	ypos+= hheight;
	ypos += (iheight >>1);
	ypos += iheight;
	ypos += iheight;
	sprintf((char*) buf, "%s: %5llu.%03llu kbit/s", g_Locale->getText(LOCALE_STREAMINFO_BITRATE), bitrate / 1000ULL, bitrate % 1000ULL);
	frameBuffer->paintBoxRel(xpos, ypos-iheight+1, 350, iheight-1, COL_MENUHEAD_PLUS_0);
	g_Font[font_info]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8
}

int CStreamInfo2::doSignalStrengthLoop ()
{
	neutrino_msg_t      msg;
	CZapitClient::responseFESignal s;
	unsigned long long bitrate = 0;

	while (1) {
		neutrino_msg_data_t data;

		unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd_MS(100);
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		// -- read signal from Frontend
		g_Zapit->getFESignal(s);

		signal.sig = s.sig & 0xFFFF;
		signal.snr = s.snr & 0xFFFF;
		signal.ber = (s.ber < 0x3FFFF) ? s.ber : 0x3FFFF;  // max. Limit

		if (brc) { 
			bitrate = brc->calc();
			if (paint_mode == 0) {
				paint_bitrate(bitrate);
			}
		}
		paint_signal_fe(bitrate, signal);

		signal.old_sig = signal.sig;
		signal.old_snr = signal.snr;
		signal.old_ber = signal.ber;

		

		// switch paint mode
		if (msg == CRCInput::RC_red || msg == CRCInput::RC_blue || msg == CRCInput::RC_green || msg == CRCInput::RC_yellow ) {
			hide ();
			paint_mode = ++paint_mode % 2;
			paint (paint_mode);
			continue;
		}

		// -- any key --> abort
		if (msg <= CRCInput::RC_MaxRC) {
			break;
		}

		// -- push other events
		if ( msg >  CRCInput::RC_MaxRC && msg != CRCInput::RC_timeout) {
			CNeutrinoApp::getInstance()->handleMsg( msg, data ); 
		}
	}
	return msg;
}

void CStreamInfo2::hide()
{
	pig->hide();
	frameBuffer->paintBackgroundBoxRel(0,0, max_width,max_height);
}

void CStreamInfo2::paint_pig(int x, int y, int w, int h)
{
  	frameBuffer->paintBoxRel(x,y,w,h, COL_BACKGROUND);
	pig->show (x,y,w,h);
}

void CStreamInfo2::paint_signal_fe_box(int x, int y, int w, int h)
{
	int y1;
	int xd = w/4;

	g_Font[font_info]->RenderString(x, y+iheight, width-10, g_Locale->getText(LOCALE_STREAMINFO_SIGNAL), COL_MENUCONTENT, 0, true);

	sigBox_x = x;
	sigBox_y = y+iheight;
	sigBox_w = w;
	sigBox_h = h-iheight*3;
	frameBuffer->paintBoxRel(sigBox_x,sigBox_y,sigBox_w,sigBox_h, COL_BLACK);

	y1  = y + h;

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_RED, x+2+xd*0 , y1- 20 );
	g_Font[font_small]->RenderString(x+25+xd*0 , y1, 50, "BER", COL_MENUCONTENT, 0, true);

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_BLUE, x+2+xd*1  , y1- 20 );
	g_Font[font_small]->RenderString(x+25+xd*1, y1, 50, "SNR", COL_MENUCONTENT, 0, true);

	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, x+2+xd*2  , y1- 20 );
	g_Font[font_small]->RenderString(x+25+xd*2,y1, 50, "SIG", COL_MENUCONTENT, 0, true);
	
	frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_YELLOW, x+2+xd*3  , y1- 20 );
	g_Font[font_small]->RenderString(x+25+xd*3,y1, 50, "Bitrate", COL_MENUCONTENT, 0, true);

	sig_text_y = y1 - iheight;
	sig_text_ber_x = x+05+xd*0;
	sig_text_snr_x = x+05+xd*1;
	sig_text_sig_x = x+05+xd*2;
	sig_text_rate_x = x+05+xd*3;

	// --  first draw of dummy signal
	// --  init some values
	{
		sigBox_pos = 0;

		signal.old_sig = 1;
		signal.old_snr = 1;
		signal.old_ber = 1;

		struct feSignal s = {0,0,  0,0,   0,0 };
		paint_signal_fe(0, s);
	}
}

void CStreamInfo2::paint_signal_fe(unsigned long long bitrate, struct feSignal  s)
{
	int   x_now = sigBox_pos;
	int   y = sig_text_y;
	int   yd;

	sigBox_pos = (++sigBox_pos) % sigBox_w;

	frameBuffer->paintVLine(sigBox_x+sigBox_pos,sigBox_y,sigBox_y+sigBox_h,COL_WHITE);
	frameBuffer->paintVLine(sigBox_x+x_now,sigBox_y,sigBox_y+sigBox_h+1,COL_BLACK);

	long value = (long) (bitrate / 1000ULL);
	SignalRenderStr (value,sig_text_rate_x,y);
	yd = y_signal_fe (value, 10000, sigBox_h);
	frameBuffer->paintPixel(sigBox_x+x_now,sigBox_y+sigBox_h-yd,COL_YELLOW);
	
	
	if (s.ber != s.old_ber) {
		SignalRenderStr (s.ber,sig_text_ber_x,y);
	}
	yd = y_signal_fe (s.ber, 4000, sigBox_h);
	frameBuffer->paintPixel(sigBox_x+x_now,sigBox_y+sigBox_h-yd,COL_RED);


	if (s.sig != s.old_sig) {
		SignalRenderStr (s.sig,sig_text_sig_x,y);
	}
	yd = y_signal_fe (s.sig, 65000, sigBox_h);
	frameBuffer->paintPixel(sigBox_x+x_now,sigBox_y+sigBox_h-yd,COL_GREEN);


	if (s.snr != s.old_snr) {
		SignalRenderStr (s.snr,sig_text_snr_x,y);
	}
	yd = y_signal_fe (s.snr, 65000, sigBox_h);
	frameBuffer->paintPixel(sigBox_x+x_now,sigBox_y+sigBox_h-yd,COL_BLUE);
}


// -- calc y from max_range and max_y
int CStreamInfo2::y_signal_fe(unsigned long value, unsigned long max_value, int max_y)
{
	long  l;

	if (!max_value) max_value = 1;

	l = ((long) max_y * (long) value ) / (long) max_value;
	if (l > max_y) l = max_y;

	return (int) l;
}

void CStreamInfo2::SignalRenderStr (unsigned long value, int x, int y)
{
	char str[30];

	frameBuffer->paintBoxRel(x, y-iheight+1, 60, iheight-1, COL_MENUHEAD_PLUS_0);
	sprintf(str,"%6lu",value);
	g_Font[font_small]->RenderString(x, y, 60, str, COL_MENUCONTENT, 0, true);
}

void CStreamInfo2::paint(int mode)
{
	const char * head_string;
	int ypos = y+5;
	int xpos = x+10;



	if (paint_mode == 0) {

		// -- tech Infos, PIG, small signal graph

		head_string = g_Locale->getText(LOCALE_STREAMINFO_HEAD);
		CLCD::getInstance()->setMode(CLCD::MODE_MENU_UTF8, head_string);
		// paint backround, title pig, etc.
		frameBuffer->paintBoxRel(0, 0, max_width, max_height, COL_MENUHEAD_PLUS_0);
		g_Font[font_head]->RenderString(xpos, ypos+ hheight+1, width, head_string, COL_MENUHEAD, 0, true); // UTF-8
		ypos+= hheight;

		// paint PIG
		paint_pig( width-240,  y+10 , 240, 190);

		// Info Output
		ypos += (iheight >>1);
		paint_techinfo ( xpos, ypos);

		paint_signal_fe_box ( width-240,  (y + 190 + hheight), 240, 190);

	} else {

		// --  small PIG, small signal graph

		// -- paint backround, title pig, etc.
		frameBuffer->paintBoxRel(0, 0, max_width, max_height, COL_MENUHEAD_PLUS_0);

		// -- paint large signal graph
		paint_signal_fe_box ( x,  y, width, height);
	}

}

void CStreamInfo2::paint_techinfo(int xpos, int ypos)
{
	// Info Output
	FILE* fd = fopen("/proc/bus/bitstream", "rt");
	if (fd==NULL)
	{
		printf("error while opening proc-bitstream\n" );
		return;
	}

	long bitInfo[10];

	char *key,*tmpptr,buf[100], buf2[100];
	long value;
	int pos=0;
	fgets(buf,35,fd);//dummy
	while(!feof(fd))
	{
		if(fgets(buf,35,fd)!=NULL)
		{
			buf[strlen(buf)-1]=0;
			tmpptr=buf;
			key=strsep(&tmpptr,":");
			value=strtoul(tmpptr,NULL,0);
			bitInfo[pos]= value;
			pos++;
		}
	}
	fclose(fd);

	ypos+= iheight;
	sprintf((char*) buf, "%s: %dx%d", g_Locale->getText(LOCALE_STREAMINFO_RESOLUTION), (int)bitInfo[0], (int)bitInfo[1] );
	g_Font[font_info]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8


	ypos += iheight;
//	sprintf((char*) buf, "%s: %d bits/sec", g_Locale->getText(LOCALE_STREAMINFO_BITRATE), (int)bitInfo[4]*50);
//	g_Font[font_info]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8


	ypos += iheight;
	switch (bitInfo[2])
	{
	case 2:
		sprintf((char*) buf, "%s: 4:3", g_Locale->getText(LOCALE_STREAMINFO_ARATIO));
		break;
	case 3:
		sprintf((char*) buf, "%s: 16:9", g_Locale->getText(LOCALE_STREAMINFO_ARATIO));
		break;
	case 4:
		sprintf((char*) buf, "%s: 2.21:1", g_Locale->getText(LOCALE_STREAMINFO_ARATIO));
		break;
	default:
		strncpy(buf, g_Locale->getText(LOCALE_STREAMINFO_ARATIO_UNKNOWN), sizeof(buf));
	}
	g_Font[font_info]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8



	ypos+= iheight;
	switch ( bitInfo[3] )
	{
			case 3:
			sprintf((char*) buf, "%s: 25fps", g_Locale->getText(LOCALE_STREAMINFO_FRAMERATE));
			break;
			case 6:
			sprintf((char*) buf, "%s: 50fps", g_Locale->getText(LOCALE_STREAMINFO_FRAMERATE));
			break;
			default:
			strncpy(buf, g_Locale->getText(LOCALE_STREAMINFO_FRAMERATE_UNKNOWN), sizeof(buf));
	}
	g_Font[font_info]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8



	if (!bitInfo[7]) strncpy(buf, g_Locale->getText(LOCALE_STREAMINFO_AUDIOTYPE_UNKNOWN), sizeof(buf));
	else {
		const char* layernames[4]={"res","III","II","I"};
		const char* sampfreqnames[4]={"44,1k","48k","32k","res"};
		const char* modenames[4]={"stereo","joint_st","dual_ch","single_ch"};

		long header = bitInfo[7];

		unsigned char layer =	(header>>17)&3;
		unsigned char sampfreq = (header>>10)&3;
		unsigned char mode =	(header>> 6)&3;
		unsigned char copy =	(header>> 3)&1;

		sprintf((char*) buf, "%s: %s (%s/%s) %s", g_Locale->getText(LOCALE_STREAMINFO_AUDIOTYPE),
								modenames[mode],
								sampfreqnames[sampfreq],
								layernames[layer],
								copy?"c":"");
	}
	g_Font[font_info]->RenderString(xpos, ypos+ iheight, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8
	ypos+= iheight+ 10;

	CZapitClient::CCurrentServiceInfo si = g_Zapit->getCurrentServiceInfo();

	//onid
	ypos+= iheight;
	sprintf((char*) buf, "%s: 0x%04x", "ONid", si.onid);
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//sid
	ypos+= sheight;
	sprintf((char*) buf, "%s: 0x%04x", "Sid", si.sid);
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//tsid
	ypos+= sheight;
	sprintf((char*) buf, "%s: 0x%04x", "TSid", si.tsid);
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//tsfrequenz
	ypos+= sheight;
	int written = sprintf((char*) buf, "%s: %d.%d MHz", "Freq", si.tsfrequency/1000, si.tsfrequency%1000);
	if (si.polarisation != 2) /* only satellite has polarisation */
		sprintf((char*) buf+written, " (%c)", (si.polarisation == HORIZONTAL) ? 'h' : 'v');
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//pmtpid
	ypos+= sheight;
	sprintf((char*) buf, "%s: 0x%04x", "PMTpid", si.pmtpid);
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8 

	//vpid
	ypos+= sheight;
	if ( g_RemoteControl->current_PIDs.PIDs.vpid == 0 )
		sprintf((char*) buf, "%s: %s", "Vpid", g_Locale->getText(LOCALE_STREAMINFO_NOT_AVAILABLE));
	else
		sprintf((char*) buf, "%s: 0x%04x", "Vpid", g_RemoteControl->current_PIDs.PIDs.vpid );
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//apid	
	ypos+= sheight;
	if (g_RemoteControl->current_PIDs.APIDs.empty())
		sprintf((char*) buf, "%s: %s", "Apid(s)", g_Locale->getText(LOCALE_STREAMINFO_NOT_AVAILABLE));
	else
	{
		sprintf((char*) buf, "%s: ", "Apid(s)" );
		for (unsigned int i= 0; i< g_RemoteControl->current_PIDs.APIDs.size(); i++)
		{
			sprintf((char*) buf2, " 0x%04x",  g_RemoteControl->current_PIDs.APIDs[i].pid );

			if (i > 0)
			{
				strcat((char*) buf, ",");
				strcat((char*) buf, buf2+4);
			}
			else
				strcat((char*) buf, buf2);
		}
	}
	g_Font[font_small]->RenderString(xpos, ypos, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8

	//vtxtpid
	if ( g_RemoteControl->current_PIDs.PIDs.vtxtpid == 0 )
        	sprintf((char*) buf, "%s: %s", "VTXTpid", g_Locale->getText(LOCALE_STREAMINFO_NOT_AVAILABLE));
	else
        	sprintf((char*) buf, "%s: 0x%04x", "VTXTpid", g_RemoteControl->current_PIDs.PIDs.vtxtpid );
	g_Font[font_small]->RenderString(xpos, ypos+ iheight, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8
	ypos+= sheight+ 10;
	
	//satellite
	sprintf((char*) buf, "Provider / Sat: %s",CNeutrinoApp::getInstance()->getScanSettings().satOfDiseqc(si.diseqc));
	g_Font[font_info]->RenderString(xpos, ypos+ iheight, width-10, buf, COL_MENUCONTENT, 0, true); // UTF-8
}

int CStreamInfo2Handler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	int res = menu_return::RETURN_EXIT_ALL;
	if (parent)
	{
		parent->hide();
	}
	CStreamInfo2 *e = new CStreamInfo2;
	e->exec(); 
	delete e;
	return res;
}

