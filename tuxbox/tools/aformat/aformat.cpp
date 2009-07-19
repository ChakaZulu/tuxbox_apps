/*
 * $Id: aformat.cpp,v 1.2 2009/07/19 16:27:01 rhabarber1848 Exp $
 *
 * aformat - d-box2 linux project
 *
 * (C) 2009 by SnowHead
 * (C) 2009 by Houdini
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 Options:
 -d	Debugmode: Don't fork, additionally generate debugging messages

 -l	Run also in the abscense of /var/etc/.aformat

 -q     Be quiet.

 Signal handling:

  SIGUSR1:         Toggles debug mode
  SIGUSR2:         Reread configuration file
*/

/**
####################################################################################
#### aformat 1.1
#### Automatische Formatanpassung bei Schummelsendern
####################################################################################

Viele Sender (z.B. "Discovery-Channel" oder "Discovery Geschichte") machen es sich aus
Kostengründen zur Angewohnheit, auch Sendungen, welche ursprünglich im 16:9- oder sogar
Kino-Format aufgenommen wurden, einfach auf 4:3 runterzuskalieren und dann so zu senden.
Das Ergebnis besteht dann bei 16:9-Fernsehern in schwarzen Streifen nicht nur links und
rechts sondern zusätzlich oben und unten. Da ist dann auf dem nutzbaren Bereich eines
Fernsehers nicht mehr viel zu sehen. Auch mögen Plasmafernseher solche Balken im Hinblick
auf den Einbrenneffekt überhaupt nicht.
Deshalb habe ich mal eine Testversion einer entsprechenden Umschaltung gebaut. Sie wertet
die Größe der schwarzen Balken am oberen Bildrand aus und zoomt das Bild entsrechend der
gemachten Einstellungen auf. Das funktioniert allerdings nur bei Fernsehern, welche das WSS-
Signal (wide screen signaling) auswerten können. Das sollten die meisten können.
Für die Basisfunktion ist das Plugin "aformat" mit den Rechten 755 nach /var/plugins/ und
"aformat.conf" nach /var/tuxbox/config/ zu kopieren. Gestartet wird entweder automatisch
oder über das Flexmenü.
Wichtig: Um das Plugin beim Boxenstart automatisch zu starten, ist noch die Zeile
"[ -e /var/etc/.aformat ] && ( sleep 20; aformat ) &" in die start_neutrino vor dem eigent-
lichen Start von Neutrino einzufügen.

Start und Konfiguration über das Flexmenü:

Installation des Flexmenüstarts:
--------------------------------
plrun_aformat.mnu --> nach /var/tuxbox/config/flexinc/
in_plugin_run.mnu_einfuegen --> den Inhalt in /var/tuxbox/config/flexinc/plugin_run.mnu einfügen

Gesteuert wird das Plugin nun übers Menü unter:
Automatikformat starten / beenden

Installation der Menükonfiguration:
-----------------------------------
afops --> nach /var/plugins/ + Dateirechte 755
plconfig_aformat.mnu --> nach /var/tuxbox/config/flexinc
in_plugin_config.mnu_einfuegen --> den Inhalt in /var/tuxbox/config/flexinc/plugin_config.mnu einfügen

*/

/* system headers */
#include <fcntl.h>
#include <csignal>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <linux/version.h>

#include <dbox/saa7126_core.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* tuxbox headers */
#include <configfile.h>
#include <connection/basicserver.h>
#include <zapit/client/zapitclient.h>
#include <zapit/debug.h>

#define SAA7126_DEVICE "/dev/dbox/saa0"

#define P_VERSION "1.2"		// ??? - Barf

#if HAVE_DVB_API_VERSION >= 3
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#include <linux/dvb/avia/avia_gt_capture.h>
#else
#include <../dvb/drivers/media/dvb/avia/avia_gt_capture.h>
#endif
#else
#include <dbox/avia_gt_capture.h>
#endif

/* the configuration file */
CConfigFile config(',', false);

static int stride;
static char msg[]="[aformat] ";
static const char CFG_FILE[] = "/var/tuxbox/config/aformat.conf";
int debug = 0;			// declared in zapit/debug.h

// Variables in configuration file
static int loop;
static int normal;
static int repeat;
static int noswitch;
static int maxblack14;
static int maxblack16;
static int wide14;
static int wide16;
static int exp;
static std::string blacklist("");
static std::string script_filename("");

#define XRES   120
#define YRES   65
#define CYRES   32

static bool is_blacklisted()
{
	char line_buffer[64];
	CZapitClient zapit;
	t_channel_id channel=zapit.getCurrentServiceID();

	if(!blacklist.empty())
	{
		sprintf(line_buffer,"%llx",channel);
		return (blacklist.find(line_buffer, 0) != std::string::npos);
	}
	return 0;
}

int read_wss()
{
	int arg=0;
	int fd;

	if((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0){
		perror("SAA DEVICE: ");
		return -1;
	}

	if ( (ioctl(fd,SAAIOGWSS,&arg) < 0)) {
		perror("IOCTL: ");
		close(fd);
		return -1;
	}
	close(fd);

	if (debug)
	  DBG("[aformat] read wss = %d", arg);
	return arg;
}

void write_wss(int mode, int oldmode)
{
 		DBG("[aformat] writing wss %d", mode);

	if (script_filename != "") {
	
	  char cmd[255];
	  sprintf(cmd, "%s %d %d %d", script_filename.c_str(), mode, oldmode, debug);
	  
	  DBG("Trying to execute %s", cmd);
	  int status = system(cmd); // MAY hang!!
	  DBG("... returning %d", status);
	}
	else 
	  {
		int arg=mode;
		int fd;

		if((fd = open(SAA7126_DEVICE,O_RDWR|O_NONBLOCK)) < 0){
			perror("SAA DEVICE: ");
			return;
		}

		if ( (ioctl(fd,SAAIOSWSS,&arg) < 0)){
			perror("IOCTL: ");
			close(fd);
			return;
		}
		close(fd);
	}
}

const int compute_lookup[] = 
{
	0,
	2,
	8,
	0x1C,
	0x30,
	0x44,
	0x58,
	0x6c,
	0x80,
	0x94,
	0xa8,
	0xbc,
	0xd0,
	0xe4,
	-8,
	-2,
};

inline int compute(int l, int d)
{
	if ((d>=0) && (d<=2))
		return l + compute_lookup[d];
	else if ((d>=3) && (d<=13))
		return compute_lookup[d];
	else if ((d>=14) && (d<=15))
		return l + compute_lookup[d];
	else 
		return 0;
}

int read_frame(int fd, int experimental)
{
	unsigned short buffer[stride*CYRES/2];
	unsigned short ts, go, lum=0;
	int x, y, tz=3, cnt=0, CXRES=stride/6;
	int LXRES=(stride>>1) - CXRES - 4;

	read(fd, buffer, stride * CYRES);

	for (y=1; y<CYRES && tz; y++)
	{
		go=1;
		for (x=CXRES; x<LXRES && go; x++)
		{
			if(ts=buffer[y*stride/2+x])
			{
				if(experimental)
				{
					int val=buffer[y*stride/2+x];
					int dy[2]={val&0xF, (val>>8)&0xF};
					lum=compute(lum, dy[1]);
					if(abs(lum)>0x02)
						go=0;
					lum=compute(lum, dy[0]);
					if(abs(lum)>0x02)
						go=0;
				}
				else
				{
					go=0;
				}
			}
		}
		if(!go)
			tz--;
		if(tz)
			cnt++;
   	}
	return cnt;
}

void load_config(/*int loop, int repeat, int noswitch, int maxblack14, int maxblack16, int wide14, int wide16, int exp, char *blacklist*/)
{
	/* load configuration or set defaults if no configuration file exists */
	if (!config.loadConfig(CFG_FILE))
		printf("%s not found\n", CFG_FILE);

	loop = config.getInt32("LOOP", 3);
	repeat = config.getInt32("REPEAT", 0);
	normal = config.getInt32("NORMAL", 0);
	noswitch = config.getInt32("NOSWITCH", 7);
	maxblack14 = config.getInt32("MAXBLACK14", 24);
	maxblack16 = config.getInt32("MAXBLACK16", 48);
	wide14 = config.getInt32("WIDE14", 1);
	wide16 = config.getInt32("WIDE16", 3);
	exp = config.getInt32("EXPERIMENTAL", 0);
	blacklist = config.getString("BLACKLIST", "");
	script_filename = config.getString("SCRIPT_FILENAME", "");
}

void signal_handler(int signum)
{
	switch (signum) {
	case SIGUSR1:
		debug = !debug;
		printf("\ndebug = %d\n", debug);
		break;
	case SIGUSR2:
		printf("\nRe-reading configuration\n");
		load_config();
		break;
	default:
	  printf("Received signal %d, quitting\n", signum);
	  exit(2);
		break;
	}
}

int main(int argc, char **argv)
{
	int capture, amode=0, tmode, repeat=0, trepeat, fcnt, scnt, /*loop=3, exp=0,*/ run=0/*, maxblack14=24, maxblack16=48, wide14=1, wide16=3, normal=0, noswitch=7*/;
	unsigned long towait=3000000L;
	int opt;

	fprintf(stdout, "Automatisches Bildschirmformat $Id: aformat.cpp,v 1.2 2009/07/19 16:27:01 rhabarber1848 Exp $\n");

	while ((opt = getopt(argc, argv, "dlq")) > 0) {
		switch (opt) {
		case 'd':
			debug = true;
			break;
		case 'l':
			run = true;
			break;
		case 'q':
			/* don't say anything */
			int fd;
			close(STDOUT_FILENO);
			if ((fd = open("/dev/null", O_WRONLY)) != STDOUT_FILENO)
				close(fd);
			close(STDERR_FILENO);
			if ((fd = open("/dev/null", O_WRONLY)) != STDERR_FILENO)
				close(fd);
			break;
		default:
			fprintf(stderr,
				"Usage: %s [-d] [-q] [-l]\n"
				"-d : debug mode\n"
				"-l : run also without /var/etc/.aformat\n"
				"-q : quiet mode\n"
				"\n"
				"Keys in config file " /*CFG_FILE*/ ":\n"
				"LOOP\n"
				"REPEAT\n"
				"NOSWITCH\n"
				"MAXBLACK14\n"
				"MAXBLACK16\n"
				"WIDE14\n"
				"WIDE16\n"
				"NORMAL\n"
				"EXPERIMENTAL\n"
				"BLACKLIST\n"
				"SCRIPT_FILENAME\n"
				"\n", argv[0]);
			return EXIT_FAILURE;
		}
	}

	//signal(SIGHUP, signal_handler);
	//signal(SIGTERM, signal_handler);
	//signal(SIGINT, signal_handler);
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);

	if (!debug) {
		switch (fork()) {
		case -1: /* can't fork */
			ERROR("fork");
			return -1;
		case 0: /* child, process becomes a daemon */
			if (setsid() == -1) {
				ERROR("setsid");
				return -1;
			}
			break;
		default: /* parent returns to calling process */
			return 0;
		}
	}

	load_config();

	towait=1000000L*loop;
	trepeat=repeat;

	while(run || (access("/var/etc/.aformat", 0)!=-1))
	{

		amode=read_wss();

		if(amode!=noswitch)
		{
			if(!is_blacklisted())
			{
				capture=open("/dev/dbox/capture0", O_RDONLY);
				capture_stop(capture);
				capture_set_input_pos(capture, 0, 0);
				capture_set_input_size(capture, 720, 576);
				capture_set_output_size(capture, XRES, YRES*2);
				stride = capture_start(capture);
				usleep(10000);
				fcnt=read_frame(capture,exp);
				capture_stop(capture);
				close(capture);

				capture=open("/dev/dbox/capture0", O_RDONLY);
				capture_stop(capture);
				capture_set_input_pos(capture, 0, 0);
				capture_set_input_size(capture, 720, 576);
				capture_set_output_size(capture, XRES, YRES*2);
				stride = capture_start(capture);
				usleep(10000);
				scnt=read_frame(capture,exp);
				capture_stop(capture);
				close(capture);
				DBG("scnt=%d fcnt=%d", scnt, fcnt);

				if((fcnt!=scnt) || (fcnt==31))
				{
					fcnt=-1;
				}
				else
				{
					fcnt-=2;
					fcnt<<=3;
				}

				if(fcnt>=0)
				{
					tmode=-1;
					if(fcnt>=maxblack16)
					{
						if(amode != 3 && amode != 4 && amode != 5 && amode != 7)
							tmode=wide16;
					}
					else if(fcnt>=maxblack14)
					{
						if(amode != 1 && amode != 2 && amode != 6 && amode != 7)
							tmode=wide14;
					}
					else
						tmode=normal;

					if(tmode>=0 && amode!=tmode)
					{
						if(trepeat == 0)
						{
						  write_wss(tmode, amode);
							trepeat=repeat;
							printf("%sBildformat von Modus %d auf %d umgeschaltet\n",msg,amode,tmode);
						}
						else
							--trepeat;
					}
				}
				else
					trepeat=repeat;
			}
			else
			{
				if(amode!=8)
				  write_wss(8, amode);
				trepeat=repeat;
			}
		}
		else
			trepeat=repeat;
		DBG("Waiting %ld usek", towait);
		usleep(towait);
	}
	printf("%sProgrammende\n", msg);
	exit(0);
}
