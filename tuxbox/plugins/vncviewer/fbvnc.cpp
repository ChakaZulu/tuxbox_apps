#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef PLUGIN
#include <configfile.h>
#include <plugin.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <linux/input.h>
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
#else
#include <osis/ofbis.h>
#endif

#include <X11/keysym.h>

extern "C" {
#include "vncviewer.h"
#include "fbgl.h"
#include "overlay.h"
#include "icons.h"
#include "keyboard.h"
#include "list.h"
}

fbvnc_framebuffer_t global_framebuffer;
#ifdef PLUGIN
int fb_fd;
int rc_fd;
char terminate;
#endif

static bool quit_requested = 0;

struct fb_calibration {
	int a;
	int b;
	int c;
	int d;
	int e;
	int f;
	int s;
} tscal;
static bool need_calibrate = 0;

#define TS_TYPE_IPAQ 1
#define TS_TYPE_ZAURUS 2
#define TS_TYPE_PS2 3
#define TS_TYPE_C700 4

static int ts_type = TS_TYPE_ZAURUS;
bool landscape_is_native = 0;

void fbvnc_close(void);

void
cleanup_and_exit(char *msg, int ret) {
	if (ret==EXIT_SYSERROR) {
		if (msg) perror(msg); else perror("error");
	} else {
		if(msg) fprintf(stderr, "%s\n", msg);
	}
	fbvnc_close();
#ifdef PLUGIN
	terminate=1;
#else
	exit(ret);
#endif
}

void signal_quit(int sig) {
	quit_requested = 1;
}
extern "C"
{
void *
xmalloc(size_t s) {
	void *r;
	r=malloc(s);
	if (!r) cleanup_and_exit("out of memory", 1);
	return r;
}
}

void
get_fbinfo() {
	struct fb_fix_screeninfo finf;
	struct fb_var_screeninfo vinf;
#ifndef PLUGIN
	FB *f = global_framebuffer.p_framebuf;
#define FFB f->fb	
#else
#define FFB fb_fd
#endif
	if (ioctl(FFB, FBIOGET_FSCREENINFO, &finf))
		cleanup_and_exit("fscreeninfo", EXIT_SYSERROR);

	if (ioctl(FFB, FBIOGET_VSCREENINFO, &vinf))
		cleanup_and_exit("vscreeninfo", EXIT_SYSERROR);
	
	if (vinf.bits_per_pixel != 8*sizeof(Pixel)) {
		printf("bpp %d 8*sizeof(Pixel)=%d\n",vinf.bits_per_pixel, 8*sizeof(Pixel));
		//cleanup_and_exit("data type 'Pixel' size mismatch", EXIT_ERROR);
	}
	myFormat.bitsPerPixel = 8*sizeof(Pixel);
	myFormat.depth = vinf.bits_per_pixel;
	myFormat.bigEndian = 0;
	myFormat.trueColour = 1;
	myFormat.redShift = vinf.red.offset;
	myFormat.redMax = (1<<vinf.red.length)-1;
	myFormat.greenShift = vinf.green.offset;
	myFormat.greenMax = (1<<vinf.green.length)-1;
	myFormat.blueShift = vinf.blue.offset;
	myFormat.blueMax = (1<<vinf.blue.length)-1;
	//printf("RGB %d/%d %d/%d %d/%d\n", myFormat.redMax, myFormat.redShift, myFormat.greenMax, myFormat.greenShift, myFormat.blueMax, myFormat.blueShift);
	global_framebuffer.p_xsize = vinf.xres;
	global_framebuffer.p_ysize = vinf.yres;
#ifndef PLUGIN
	global_framebuffer.p_buf = f->sbuf;
	global_framebuffer.kb_fd = f->tty;
#else
	global_framebuffer.kb_fd = -1;
#endif
}

#define FBVNC_CALIBRATION_FILE "/etc/fbvnc-calibration.conf"

void
ts_get_calibration() {
	FILE *f;

	f = fopen(FBVNC_CALIBRATION_FILE, "r");
	if (!f) {
		need_calibrate = 1;
	} else {
		fscanf(f, "%d %d %d %d %d %d",
			&tscal.a, &tscal.b, &tscal.c,
			&tscal.d, &tscal.e, &tscal.f);
		fclose(f);
	}
}

void
ts_save_calibration() {
	FILE *f;
	f = fopen(FBVNC_CALIBRATION_FILE, "w");
	if (!f) {
		perror(FBVNC_CALIBRATION_FILE);
		return;
	}
	fprintf(f, "%d %d %d %d %d %d",
		tscal.a, tscal.b, tscal.c,
		tscal.d, tscal.e, tscal.f);
	fclose(f);
}

void
init_pointer() {
	int ts_fd = -1;

        if (!strcmp(hwType, "zaurus")) {
                ts_type = TS_TYPE_ZAURUS;
                landscape_is_native=0;
        } else if (!strcmp(hwType, "ipaq")) {
                ts_type = TS_TYPE_IPAQ;
                landscape_is_native=0;
        } else if (!strcmp(hwType, "ps2de")) {
                ts_type = TS_TYPE_PS2;
                landscape_is_native=1;
        } else if (!strcmp(hwType, "ps2us") || !strcmp(hwType, "ps2")) {
                ts_type = TS_TYPE_PS2;
                landscape_is_native=1;
        } else if (!strcmp(hwType, "c700") || !strcmp(hwType, "C700")) {
                ts_type = TS_TYPE_C700;
                landscape_is_native=0;
        } else {
                cleanup_and_exit("unknown hardware type", EXIT_ERROR);
        }

	switch(ts_type) {
	case TS_TYPE_ZAURUS:
	case TS_TYPE_C700:
		ts_fd = open("/dev/ts", O_RDONLY);
		if (ts_fd<0)
			cleanup_and_exit("Can't open /dev/ts", EXIT_SYSERROR);
		break;
	case TS_TYPE_IPAQ:
		ts_fd = open("/dev/h3600_tsraw", O_RDONLY);
		if (ts_fd<0)
			cleanup_and_exit("Can't open /dev/h3600_tsraw", EXIT_SYSERROR);
		break;
	}

	if (ts_fd >= 0) {
		global_framebuffer.ts_fd = ts_fd;
		ts_get_calibration();
	}
}

static void
hwtype_readproc()
{
	FILE *dinfo;
	char product[100];

	dinfo=fopen("/proc/deviceinfo/product", "r");
	if (!dinfo) return;

	fscanf(dinfo, "%99s", product);
	if (debug) fprintf(stderr, "hwdetect: product='%s'\n", product);
	if (!strcmp(product, "SL-C700")
	 || !strcmp(product, "SL-C750")
	 || !strcmp(product, "SL-C760")
	) {
		hwType = "c700";
		if (debug) fprintf(stderr, "hwdetect: C700 detected\n");
	} else if (!strcmp(product, "SL-5000D")
		|| !strcmp(product, "SL-5500")
	       ) {
		hwType = "zaurus";
		if (debug) fprintf(stderr, "hwdetect: Zaurus 5x00 detected\n");
	}
}

static void
hwtype_guess()
{
	if (access("/dev/h3600_tsraw", F_OK) >= 0) {
		hwType = "ipaq";
		if (debug) fprintf(stderr, "hwdetect: guessing iPAQ\n");
	} else if (access("/dev/ts", F_OK) >= 0) {
		hwType = "zaurus";
		if (debug) fprintf(stderr, "hwdetect: guessing Zaurus 5x00\n");
	} else {
		hwType = "ps2us";
		if (debug) fprintf(stderr, "hwdetect: defaulting to ps2us\n");
	}
}

static void
hwtype_autodetect()
{
	if (!hwType) hwtype_readproc();
	if (!hwType) hwtype_guess();
	if (!hwType) cleanup_and_exit("can't determine hardware type, use '-hw' switch", EXIT_ERROR);
}


void
fbvnc_init() {
	int v_xsize = si.framebufferWidth;
	int v_ysize = si.framebufferHeight;

	global_framebuffer.ts_fd = -1;
	global_framebuffer.kb_fd = -1;
	global_framebuffer.p_framebuf = 0;
	global_framebuffer.num_read_fds = 0;
	global_framebuffer.num_write_fds = 0;

	global_framebuffer.overlays = list_new();
	global_framebuffer.hide_overlays = 0;

	global_framebuffer.ts_x = 0;
	global_framebuffer.ts_y = 0;

	global_framebuffer.v_xsize = v_xsize;
	global_framebuffer.v_ysize = v_ysize;
	global_framebuffer.v_x0 = 0;
	global_framebuffer.v_y0 = 0;
	global_framebuffer.v_scale = 1;
	global_framebuffer.v_bpp = sizeof(Pixel);
	global_framebuffer.v_buf = (Pixel*) xmalloc(v_xsize * v_ysize * sizeof(Pixel));
#ifndef PLUGIN
	signal(SIGINT, signal_quit);
	signal(SIGTERM, signal_quit);
	signal(SIGQUIT, signal_quit);
#endif
	hwtype_autodetect();

	init_keyboard();
	init_pointer();

	/* start in portrait mode by default ? */
	global_framebuffer.p_landscape = landscape_is_native;

#ifdef PLUGIN
	struct fb_var_screeninfo vinf;
	struct fb_fix_screeninfo finf;
	off_t offset = 0;
	global_framebuffer.framebuf_fds = fb_fd;

	if (ioctl(global_framebuffer.framebuf_fds,FBIOGET_VSCREENINFO, &vinf) == -1 )
	{
		cleanup_and_exit("Get variable screen settings failed", EXIT_ERROR);
	}
	if (ioctl(global_framebuffer.framebuf_fds,FBIOGET_FSCREENINFO, &finf) == -1 )
	{
		cleanup_and_exit("Get fixed screen settings failed", EXIT_ERROR);
	}
	vinf.bits_per_pixel = 16;
	if (ioctl(global_framebuffer.framebuf_fds,FBIOPUT_VSCREENINFO, &vinf) == -1 )
	{
		cleanup_and_exit("Put variable screen settings failed", EXIT_ERROR);
	}
	/* Map fb into memory */
	global_framebuffer.smem_len = finf.smem_len;
	if ( (global_framebuffer.p_buf=(unsigned short *)mmap((void *)0,finf.smem_len, 
																			PROT_READ | PROT_WRITE,
																			MAP_SHARED, fb_fd, offset)) == (void *)-1 )
	{
		cleanup_and_exit("fb mmap failed", EXIT_ERROR);
	}

#elif 1
	global_framebuffer.p_framebuf = FBopen(0, FB_OPEN_NEW_VC);
#else
	global_framebuffer.p_framebuf = FBopen(0, FB_KEEP_CURRENT_VC);
#endif
#ifndef PLUGIN
	if (!global_framebuffer.p_framebuf)
		cleanup_and_exit("FBopen", EXIT_ERROR);
#endif	
	get_fbinfo();
}

void
fbvnc_close() {
#ifdef PLUGIN
	/* Unmap framebuffer from memory */

	if ( munmap ( (void *)global_framebuffer.p_buf, global_framebuffer.smem_len ) == -1 ) 
	{
		printf("FBclose: munmap failed");
	}
	struct fb_var_screeninfo vinf;
	if (ioctl(global_framebuffer.framebuf_fds,FBIOGET_VSCREENINFO, &vinf) == -1 )
	{
		printf("Get variable screen settings failed\n");
	}
	vinf.bits_per_pixel = 8;
	if (ioctl(global_framebuffer.framebuf_fds,FBIOPUT_VSCREENINFO, &vinf) == -1 )
	{
		printf("Put variable screen settings failed\n");
	}
#else
	if (global_framebuffer.p_framebuf) {
		FBclose(global_framebuffer.p_framebuf);
		global_framebuffer.p_framebuf = 0;
	}

	if (global_framebuffer.ts_fd != -1) {
		close(global_framebuffer.ts_fd);
		global_framebuffer.ts_fd = -1;
	}

	if (global_framebuffer.kb_fd != -1) {
		close(global_framebuffer.kb_fd);
		global_framebuffer.kb_fd = -1;
	}
#endif
	if (global_framebuffer.v_buf) {
		free(global_framebuffer.v_buf);
	}
}

#define SQR(x) ((x)*(x))

#define TS_SAMPLES 5

int
ts_filter(fbvnc_event_t *ev, int tx, int ty, int pressed) {
	static int curr_sample = 0;
	static int num_samples = 0;
	static int ox[TS_SAMPLES], oy[TS_SAMPLES];

	if (!pressed) {
		num_samples = 0;
		curr_sample = 0;
		ev->x = global_framebuffer.ts_x;
		ev->y = global_framebuffer.ts_y;
		return pressed;
	}

	ox[curr_sample] = tx;
	oy[curr_sample] = ty;

	++ num_samples;

	if (num_samples < TS_SAMPLES) {
		/* ignore it - too little data */
		ev->evtype = FBVNC_EVENT_NULL;
		pressed = 0;
	} else {
		int x=0, y=0;
		int i;
		int worst_i = 0, worst_dist=0;

		/* ignore the sample farthest from mouse pos */
		for (i=0; i < TS_SAMPLES; i++) {
			int mx = global_framebuffer.ts_x;
			int my = global_framebuffer.ts_y;
			int d;

			d = SQR(ox[i]-mx)+SQR(oy[i]-my);
			if (d > worst_dist) {
				worst_i = i;
				worst_dist = d;
			}
		}

		for (i=0; i < TS_SAMPLES; i++) {
			if (i == worst_i) continue;
			x += ox[i];
			y += oy[i];
		}
		ev->x = x/(TS_SAMPLES-1);
		ev->y = y/(TS_SAMPLES-1);
	}

	++ curr_sample;
	if (curr_sample >= TS_SAMPLES) curr_sample = 0;

	return pressed;
}

void
bad_ts_read(int expected, int read)
{
	char err[100];
	sprintf(err, "read touchscreen: expected %d bytes, got %d (bad '-hw' type?)\n",
		expected, read);
	cleanup_and_exit(err, EXIT_ERROR);
}

struct sched {
	int tv_sec;
	int tv_usec;
	int event;
};

int
cmp_sc(struct sched *A, struct sched *B)
{
	if (A->tv_sec < B->tv_sec) return -1;
	if (A->tv_sec > B->tv_sec) return  1;
	if (A->tv_usec < B->tv_usec) return -1;
	if (A->tv_usec > B->tv_usec) return  1;
	return 0;
}

#if 0
int
cmp_sc_p(void *a, void *b)
{
	struct sched *A = ((List*)a)->val;
	struct sched *B = ((List*)b)->val;

	return cmp_sc(A, B);
}
#endif

void
schedule_add(List *s, int ms_delta, int event)
{
	struct timeval tv;
	struct sched sc;
	List *p;

	gettimeofday(&tv, 0);
	sc.tv_sec = tv.tv_sec;
	sc.tv_usec = tv.tv_usec;
	sc.event = event;

	sc.tv_usec += (ms_delta%1000)*1000;
	while (sc.tv_usec > 1000000) {
		sc.tv_usec -= 1000000;
		++ sc.tv_sec;
	}
	sc.tv_sec += ms_delta / 1000;

	/* insert into sorted list */
	for (p=s; p->next; p=p->next) {
		if (cmp_sc(&sc, (struct sched*)p->next->val) < 0) {
			list_insert_copy(p, &sc, sizeof sc);
			break;
		}
	}
	if (!p->next) {
		list_append_copy(s, &sc, sizeof sc);
	}
}

void
schedule_delete(List *s, int event)
{
	List *p;

	for (p=s->next; p; p=p->next) {
		if (p->val) {
			struct sched *n = (struct sched*)p->val;
			if (n->event == event) break;
		}
	}

	if (p) list_remove(s, p);
}

int
schedule_get_next(List *s, struct timeval *tv)
{
	List *p = s->next;
	struct sched *nv;

	if (!p) return 0;

	nv = (struct sched *)p->val;
	gettimeofday(tv, 0);
	tv->tv_sec = nv->tv_sec - tv->tv_sec;
	tv->tv_usec = nv->tv_usec - tv->tv_usec;

	while (tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		-- tv->tv_sec;
	}

	while (tv->tv_usec > 1000000) {
		tv->tv_usec -= 1000000;
		++ tv->tv_sec;
	}

	if (tv->tv_sec < 0) {
		tv->tv_sec = 0;
		tv->tv_usec = 0;
	}
	return nv->event;
}

void
fd_copy(fd_set *out, fd_set *in, int n)
{
	int i;
	FD_ZERO(out);
	
	for (i=0; i<n; i++) {
		if (FD_ISSET(i, in)) FD_SET(i, out);
	}
}

#define FD_SET_u(fd, set) do {			\
		FD_SET((fd), (set));		\
		if ((fd) > max) max=(fd);	\
	} while(0)

#define RetEvent(e) do { ev->evtype = (e); return (e); } while(0)

enum fbvnc_event 
fbvnc_get_event (fbvnc_event_t *ev, List *sched) {
	fd_set rfds, wfds, save_rfds;
	int i,ret;
	enum fbvnc_event retval;
	int max;
	struct timeval timeout;
	struct timeval tzero = {0, 0};
	bool got_data_immediately;
	static fbvnc_event_t nextev = {0,0,0,0,0,0,0};
	static int next_mb = -1, countevt=0;
	static unsigned short lastcode=0;
	static struct timeval evttime;
#ifdef INPUT_PS2MOUSE
	extern int msefd; /* set in ofbis library */
#endif
	IMPORT_FRAMEBUFFER_VARS

	if(nextev.evtype!=0)
	{
		ev->dx = nextev.dx;
		ev->dy = nextev.dy;
		ev->x = nextev.x;
		ev->y = nextev.y;
		ev->evtype = nextev.evtype;
		ev->fd = nextev.fd;
		ev->key = nextev.key;
		if(next_mb!=-1)
			mouse_button=next_mb;
		nextev.evtype=0;
		nextev.dx=0;
		nextev.dy=0;
		nextev.x=-0;
		nextev.y=0;
		nextev.fd=0;
		nextev.key=0;
		next_mb=-1;
		RetEvent((fbvnc_event)ev->evtype);
	}
	if (sched) {
		int s_event;

		s_event = schedule_get_next(sched, &timeout);

		if (s_event) {
			ev->evtype = s_event;
		} else {
			ev->evtype = FBVNC_EVENT_TIMEOUT;
		}
	} else {
		ev->evtype = FBVNC_EVENT_TIMEOUT;
	}
	if (ev->evtype == FBVNC_EVENT_TIMEOUT) {
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
	}
	
	max = 0;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);

#ifdef INPUT_TS
	FD_SET_u(ts_fd, &rfds);
#endif
#ifdef INPUT_PS2MOUSE
	FD_SET_u(msefd, &rfds);
#endif
	if(kb_fd!=-1)
		FD_SET_u(kb_fd, &rfds);
#ifdef PLUGIN
	FD_SET_u(rc_fd, &rfds);
#endif

	/* quick check on input devices without waiting */
	fd_copy(&save_rfds, &rfds, max+1); /* save for later */
	ret = select(max+1, &rfds, 0, 0, &tzero);

	if (ret) {
		got_data_immediately = 1;
	} else {
		got_data_immediately = 0;

		/* try again on all fds, wait this time */
		fd_copy(&rfds, &save_rfds, max+1); /* restore */

		for(i=0; i<num_read_fds; i++) {
			if (read_fd[i] < 0) continue;
			FD_SET_u(read_fd[i], &rfds);
		}

		for(i=0; i<num_write_fds; i++) {
			if (write_fd[i] < 0) continue;
			FD_SET_u(write_fd[i], &wfds);
		}
		ret = select(max+1, &rfds, &wfds, 0, &timeout);
	}

	if (!ret) {
		if (ev->evtype != FBVNC_EVENT_TIMEOUT) {
			schedule_delete(sched, ev->evtype);
		}
		return ((fbvnc_event)ev->evtype);
	}

	if (quit_requested) {
		cleanup_and_exit("Interrupt.", EXIT_OK);
	}

	ev->x = ts_x;
	ev->y = ts_y;
	ev->dx = 0;
	ev->dy = 0;

	if(kb_fd!=-1) {
		if (FD_ISSET(kb_fd, &rfds)) {
			unsigned char k;
			int r;
			r=read(kb_fd, &k, sizeof k);
			if (r!=sizeof k) cleanup_and_exit("read kb", EXIT_SYSERROR);

			if (debug) {
				/* debug keyboard */
				fprintf(stderr, "key=%d (0x%02x)\n", k, k);
			}

			ev->key = k&0x7f;
			RetEvent((k&0x80) ? FBVNC_EVENT_BTN_UP : FBVNC_EVENT_BTN_DOWN);
		}
	}

#ifdef INPUT_TS
	if (FD_ISSET(ts_fd, &rfds)) {
		int t_x=0, t_y=0, pressed=0;
		int r;
		int evtype;
		int raw_x, raw_y;

		if (ts_type == TS_TYPE_IPAQ) {
			struct h3600_ts_event {
				unsigned short pressure;
				unsigned short x;
				unsigned short y;
				unsigned short pad;
			} ts;

			r=read(ts_fd, &ts, sizeof ts);
			if (r!=sizeof ts) bad_ts_read(sizeof ts, r);

			t_x = ts.x;
			t_y = ts.y;
			pressed = (ts.pressure!= 0);
		} else if (ts_type == TS_TYPE_ZAURUS) {
			struct zaurus_ts_event {
				int x;
				int y;
				int pressure;
				long long millisecs;
			} ts;

			r=read(ts_fd, &ts, sizeof ts);
			if (r!=sizeof ts) bad_ts_read(sizeof ts, r);

			t_x = ts.x;
			t_y = ts.y;
			pressed = (ts.pressure!= 0);
		} else if (ts_type == TS_TYPE_C700) {
			struct zaurus_ts_event {
				short pressure;
				short x;
				short y;
				short millisecs;
			} ts;

			r=read(ts_fd, &ts, sizeof ts);
			if (r!=sizeof ts) bad_ts_read(sizeof ts, r);

			t_x = ts.x;
			t_y = ts.y;
			pressed = (ts.pressure!= 0);
		} else {
			cleanup_and_exit("unknown touchscreen type", EXIT_ERROR);
		}

		raw_x = t_x;
		raw_y = t_y;

		t_x = ( tscal.a * raw_x + tscal.b * raw_y + tscal.c ) >>16;
		t_y = ( tscal.d * raw_x + tscal.e * raw_y + tscal.f ) >>16;

		if (debug) {
			fprintf(stderr, "ts: rx=%d ry=%d x=%d y=%d p=%d\n",
				raw_x, raw_y, t_x, t_y, pressed);
		}

		/* filter touchscreen events */
		pressed = ts_filter(ev, t_x, t_y, pressed);
		
		if (!need_calibrate) {
			/* clamp the coordinates - the driver returns 
			 * coordinates outside the tochpad area */

			if (ev->x >= global_framebuffer.p_xsize)
				ev->x = global_framebuffer.p_xsize-1;
			if (ev->y >= global_framebuffer.p_ysize)
				ev->y = global_framebuffer.p_ysize-1;
			if (ev->x < 0 || ev->x > 60000) ev->x = 0;
			if (ev->y < 0 || ev->y > 60000) ev->y = 0;
		}

		ev->dx = ev->x - ts_x;
		ev->dy = ev->y - ts_y;

		if (pressed) {
			evtype = ts_pressed
			       ? FBVNC_EVENT_TS_MOVE
			       : FBVNC_EVENT_TS_DOWN;
			global_framebuffer.ts_pressed = 1;
		} else {
			evtype = ts_pressed
			       ? FBVNC_EVENT_TS_UP
			       : FBVNC_EVENT_TIMEOUT;
			global_framebuffer.ts_pressed = 0;
		}

		if (evtype == FBVNC_EVENT_TS_MOVE && got_data_immediately) {
			/* not enough time passed since last move -
			 * aggregate the deltas and check again later.
			 */
			return 0;
		}

		if (pressed) {
			global_framebuffer.ts_x = ev->x;
			global_framebuffer.ts_y = ev->y;
		}
		RetEvent(evtype);
	} 
#endif
#ifdef INPUT_PS2MOUSE
	if (FD_ISSET(msefd, &rfds)) {
		static int readpos = 0;
		static char buf[3];
		static int mouse_x = -1, mouse_y = -1;
		static int edx = 0, edy = 0;
		int r;
		int buttons, dx, dy;

		if (mouse_x < 0) mouse_x = global_framebuffer.p_xsize / 2;
		if (mouse_y < 0) mouse_y = global_framebuffer.p_ysize / 2;

		/* fprintf(stderr, "read mouse readpos=%d buf=[%d,%d,%d]\n", readpos, buf[0], buf[1], buf[2]); */
		r=read(msefd, buf + readpos, 3-readpos);
		if (r<=0) cleanup_and_exit("bad mouse read", EXIT_ERROR);
		readpos += r;

		if (readpos<3) RetEvent(FBVNC_EVENT_NULL);

		readpos=0;
		buttons = buf[0] & 7;
		
		/* swap buttons #2 and #3 (bit values 2,4) */
		if (buttons) buttons = (buttons&1) | ((buttons&2)<<1) | ((buttons&4)>>1);
		
		dx = (signed char)buf[1];
		dy =-(signed char)buf[2];

		/* fprintf(stderr, "buttons=%d dx=%d dy=%d\n", buttons, dx, dy); */

		mouse_x += dx;
		mouse_y += dy;

		ev->x = mouse_x;
		ev->y = mouse_y;
		ev->dx = edx += dx;
		ev->dy = edy += dy;

		if (mouse_x < 0) mouse_x = 0;
		if (mouse_y < 0) mouse_y = 0;

		if (mouse_x >= global_framebuffer.p_xsize) mouse_x = global_framebuffer.p_xsize-1;
		if (mouse_y >= global_framebuffer.p_ysize) mouse_y = global_framebuffer.p_ysize-1;

		if (buttons & ~mouse_button) {
			mouse_button = buttons;
			edx = edy = 0;
			RetEvent(FBVNC_EVENT_TS_DOWN);
		}
		if (~buttons & mouse_button) {
			mouse_button = buttons;
			edx = edy = 0;
			RetEvent(FBVNC_EVENT_TS_UP);
		}

		if (got_data_immediately) {
			/* not enough time passed since last move -
			 * aggregate the deltas and check again later.
			 */
			return 0;
		}
		edx = edy = 0;
		RetEvent(FBVNC_EVENT_TS_MOVE);
	}
#endif
#ifdef PLUGIN
	if (FD_ISSET(rc_fd, &rfds)) 
	{
		struct input_event iev;
		int count;
		static int rc_x = -1, rc_y=-1, rc_dx=0, rc_dy=0, step=5;
		static char rc_pan=0;
		if (rc_x < 0) rc_x = global_framebuffer.p_xsize / 2;
		if (rc_y < 0) rc_y = global_framebuffer.p_ysize / 2;

		count = read(rc_fd, &iev, sizeof(struct input_event));
		if ((count == sizeof(struct input_event)) && ((iev.value == 1)||(iev.value == 2)))
		{
			retval=FBVNC_EVENT_NULL;
			
			// count events for speedup
			if(lastcode==iev.code)
			{
				struct timeval now;
				gettimeofday(&now, NULL);
				if((now.tv_sec == evttime.tv_sec
					 && (now.tv_usec - evttime.tv_usec) < 150000) ||
					((now.tv_sec-1) == evttime.tv_sec
					 && ((now.tv_usec+1000000) -evttime.tv_usec) < 150000))
				{
					countevt++;
				}
				else
					countevt=0;
			}
			else
				countevt=0;
			lastcode = iev.code;
			gettimeofday(&evttime, NULL);
			
			if(iev.code == KEY_TOPLEFT || iev.code == KEY_TOPRIGHT || iev.code == KEY_UP ||
				iev.code == KEY_BOTTOMLEFT || iev.code == KEY_BOTTOMRIGHT || iev.code == KEY_DOWN ||
				iev.code == KEY_LEFT || iev.code == KEY_RIGHT)
			{
				if(rc_pan)
					step=30;
				else
				{
					if(countevt>20)
						step=80;
					else if(countevt>15)
						step=40;
					else if(countevt>10)
						step=20;
					else if(countevt >5)
						step=10;
					else
						step=5;
				}
			}
			else
			{
				if(countevt>0)
				{
					// for othe rkeys reject because of prelling
					iev.code=0;
				}
			}
			// codes
			if (iev.code == KEY_HOME)
				retval =	FBVNC_EVENT_QUIT;
			if (iev.code == KEY_TOPLEFT || iev.code == KEY_TOPRIGHT || iev.code == KEY_UP)
			{
				if(!rc_pan)
					rc_y -= step;
				else
					rc_dy = step;
				if(rc_y <0) 
					rc_y =0;
				retval=FBVNC_EVENT_TS_MOVE;
			}
			if (iev.code == KEY_BOTTOMLEFT || iev.code == KEY_BOTTOMRIGHT || iev.code == KEY_DOWN)
			{
				if(!rc_pan)
					rc_y += step;
				else
					rc_dy =-step;
				if (rc_y >= global_framebuffer.p_ysize) 
					rc_y = global_framebuffer.p_ysize-1;
				retval=FBVNC_EVENT_TS_MOVE;
			}
			if (iev.code == KEY_TOPLEFT || iev.code == KEY_BOTTOMLEFT || iev.code == KEY_LEFT)
			{
				if(!rc_pan)
					rc_x -= step;
				else
					rc_dx =step;
				if(rc_x <0) 
					rc_x =0;
				retval=FBVNC_EVENT_TS_MOVE;
			}
			if (iev.code == KEY_TOPRIGHT || iev.code == KEY_BOTTOMRIGHT || iev.code == KEY_RIGHT)
			{
				if(!rc_pan)
					rc_x += step;
				else
					rc_dx =-step;
				if (rc_x >= global_framebuffer.p_xsize) 
					rc_x = global_framebuffer.p_xsize-1;
				retval=FBVNC_EVENT_TS_MOVE;
			}
			if (iev.code == KEY_HELP)
			{
				if(mouse_button > 0)
				{
					mouse_button=0;
					retval=FBVNC_EVENT_TS_UP;
				}
				else
				{
					mouse_button=1;
					retval=FBVNC_EVENT_TS_DOWN;
				}
			}
			if (iev.code == KEY_MUTE)
			{
				ev->key = hbtn.pan;
				if(rc_pan)
					retval=FBVNC_EVENT_BTN_UP;
				else
					retval=FBVNC_EVENT_BTN_DOWN;
				rc_pan=!rc_pan;
			}
			if(iev.code == KEY_OK)
			{
				nextev.x =rc_x;
				nextev.y =rc_y;  
				nextev.evtype = FBVNC_EVENT_TS_UP;
				next_mb=0;
				mouse_button=1;
				retval=FBVNC_EVENT_TS_DOWN;
			}
			if(iev.code == KEY_2)
			{
				nextev.x =rc_x;
				nextev.y =rc_y;  
				nextev.evtype = FBVNC_EVENT_TS_UP;
				next_mb=0;
				mouse_button=2;
				retval=FBVNC_EVENT_TS_DOWN;
			}
			if(iev.code == KEY_3)
			{
				nextev.x =rc_x;
				nextev.y =rc_y;  
				nextev.evtype = FBVNC_EVENT_TS_UP;
				next_mb=0;
				mouse_button=4;
				retval=FBVNC_EVENT_TS_DOWN;
			}

			// action
			ev->x =rc_x;
			ev->y =rc_y;
			ev->dx=rc_dx;
			ev->dy=rc_dy;
			
			if(retval==FBVNC_EVENT_TS_MOVE)
				rc_dx = rc_dy = 0;
			//printf("x:%d/y:%d (%d) dx:%d/dy:%d [%d]\n",ev->x,ev->y,mouse_button,ev->dx,ev->dy,countevt);
			RetEvent(retval);
		}
		else
			RetEvent(FBVNC_EVENT_NULL);
	}
#endif
	for (i=0; i<num_read_fds; i++) {
		int fd = read_fd[i];
		if (fd<0) continue;
		if (FD_ISSET(fd, &rfds)) {
			ev->fd = fd;
			RetEvent(FBVNC_EVENT_DATA_READABLE);
		}
	}
	for (i=0; i<num_write_fds; i++) {
		int fd = write_fd[i];
		if (fd<0) continue;
		if (FD_ISSET(fd, &wfds)) {
			ev->fd = fd;
			RetEvent(FBVNC_EVENT_DATA_WRITABLE);
		}
	}
	cleanup_and_exit("unhandled event", EXIT_ERROR);
	return (fbvnc_event)0;
}

void 
scaledPointerEvent(int xp, int yp, int button) {
	IMPORT_FRAMEBUFFER_VARS

	/* fprintf(stderr, "mouse click, button=%d\n", button); */
	if (p_landscape) {
		SendPointerEvent(xp*v_scale + v_x0, yp*v_scale + v_y0, button);
	} else {
		SendPointerEvent((yp          )*v_scale + v_x0,
		                 (p_xsize-xp-1)*v_scale + v_y0,
				 button);
	}
}

bool img_saved = 0;
List *sched;

#define FDNUM_VNC 0
#define FDNUM_PNM 1

void
save_viewport(struct viewport *dst) {
	IMPORT_FRAMEBUFFER_VARS

	dst->v_xsize = v_xsize;
	dst->v_ysize = v_ysize;
	dst->v_buf = v_buf;
	dst->v_scale = v_scale;
	dst->p_landscape = p_landscape;
}

void
activate_viewport(struct viewport *src) {
	global_framebuffer.v_xsize = src->v_xsize;
	global_framebuffer.v_ysize = src->v_ysize;
	global_framebuffer.v_buf = src->v_buf;
	global_framebuffer.v_scale = src->v_scale;
	global_framebuffer.p_landscape = src->p_landscape;
}

static struct viewport v_img;

void
open_pnm_fifo(int *fdp)
{
	printf("open_pnm\n");
	int pnmfd;
	pnmfd = open(pnmFifo, O_RDONLY | O_NDELAY);
	if (pnmfd < 0) {
		perror("can't open pnmfifo");
	}
	*fdp = pnmfd;
}

void
load_pnm_image(int *fdp)
{
	printf("load_pnm\n");
	static FILE *f = 0;
	int fd;

	if (!f) {
		fd = *fdp;
		fcntl(fd, F_SETFL, 0); /* clear non-blocking mode */
		f = fdopen(fd, "rb");
		if (!f) return;

		v_img.v_scale = 1;
		v_img.p_landscape = 0;
		v_img.v_x0 = 0;
		v_img.v_y0 = 0;
		v_img.v_bpp = 16;
	}

	if (img_read(&v_img, f)) {
		img_saved = 1;
	} else {
		fclose(f);
		f=0;
		open_pnm_fifo(fdp);
	}
}

void
show_pnm_image() {
	printf("show_pnm\n");
	struct viewport v_save;
	static int kx0=0, ky0=0;

	img_saved = 0;
	save_viewport(&v_save);
	activate_viewport(&v_img);
	vp_pan_virt(kx0, ky0);

	while(1) {
		fbvnc_event_t ev;

		fbvnc_get_event(&ev, 0);
		if (ev.evtype == FBVNC_EVENT_TS_MOVE) {
			vp_pan(-ev.dx, -ev.dy);
		} else if (ev.evtype == FBVNC_EVENT_BTN_DOWN) {
			int key;
			key = key_map(ev.key);

			if (key==XK_q || key==XK_Q || key==XK_Escape) {
				break;
			} else if (key==XK_k) {
				kx0 = global_framebuffer.v_x0;
				ky0 = global_framebuffer.v_y0;
			} else if (key==XK_space || key==XK_Return) {
				save_viewport(&v_img);
				break; /* next image */
			} else if (key==XK_z) {
				save_viewport(&v_img);
				img_saved=1;
				break;
			} else {
				fn_action = 1;
				key_special_action(key);
				fn_action = 0;
			}
		}
	}
	if (!img_saved) free(v_img.v_buf);

	fn_action = 0;
	activate_viewport(&v_save);
	vp_pan(0, 0);
}

void
do_calibration() {
	int xsize = global_framebuffer.p_xsize;
	int ysize = global_framebuffer.p_ysize;
	int px[2], py[2];
	int gx[2], gy[2];
	int scale = 1<<16;
	int i;
	struct fb_calibration old_cal;

	px[0] = xsize / 10;
	py[0] = xsize / 10;

	px[1] = xsize - px[0] - 1;
	py[1] = ysize - py[0] - 1;

	need_calibrate = 1;
	old_cal = tscal;

	tscal.a = scale;
	tscal.b = 0;
	tscal.c = 0;
	tscal.d = 0;
	tscal.e = scale;
	tscal.f = 0;

	for (i=0; i<2; i++) {
		draw_border(0, 0, xsize, ysize, 0, 0, xsize, ysize);
		draw_pixmap(ico_calibration_help, 120, 0, 80, 240);
		draw_pixmap(ico_crosshair, px[i]-7, py[i]-7, 15, 15);

		while(1) {
			fbvnc_event_t ev;

			fbvnc_get_event(&ev, 0);
		
			if (ev.evtype == FBVNC_EVENT_BTN_DOWN) {
				/* abort calibration on button press */
				tscal = old_cal;
				return;
			}
			if (ev.evtype != FBVNC_EVENT_TS_UP) continue;

			/*
			fprintf(stderr, "cal: x[%d]=%d y[%d]=%d\n", i, ev.x, i, ev.y);
			*/
			gx[i] = ev.x;
			gy[i] = ev.y;
			break;
		}
	}

#ifdef INPUT_TS
	if (ts_type == TS_TYPE_IPAQ || ts_type == TS_TYPE_C700) {
		tscal.a = scale * (px[1]-px[0]) / (gx[1]-gx[0]);
		tscal.b = 0;
		tscal.c = scale * px[1] - tscal.a * gx[1];
		
		tscal.d = 0;
		tscal.e = scale * (py[1]-py[0]) / (gy[1]-gy[0]);
		tscal.f = scale * py[1] - tscal.e * gy[1];
	} else if (ts_type == TS_TYPE_ZAURUS) {
		/* flipped 90 degrees */
		tscal.a = 0;
		tscal.b = scale * (px[1]-px[0]) / (gy[1]-gy[0]);
		tscal.c = scale * px[1] - tscal.b * gy[1];
		
		tscal.d = scale * (py[1]-py[0]) / (gx[1]-gx[0]);
		tscal.e = 0;
		tscal.f = scale * py[1] - tscal.d * gx[1];
	} else {
		cleanup_and_exit("unknown touchscreen type", EXIT_ERROR);
	}
#endif

	ts_save_calibration();
	need_calibrate = 0;
}
#ifdef PLUGIN
extern "C" {
void plugin_exec(PluginParam *par) {
#else
int main(int argc, char **argv) {
#endif
	fbvnc_overlay_t *active_overlay = 0;
	int overlay_toggle = 0;
	int panning = 0;
	int mouse_x = 0, mouse_y = 0;
	int readfd[2]; /* pnmfd and/or rfbsock */
	int key_pending = 0;
	static int pnmfd = -1;
#ifdef PLUGIN
	int lcd, sx, ex, sy, ey;
	fb_fd = lcd = rc_fd = sx = ex = sy = ey = -1;
	for(; par; par = par->next)
	{
		if(!strcmp(par->id, P_ID_FBUFFER))
		{
			fb_fd = atoi(par->val);
		}
		else if(!strcmp(par->id, P_ID_LCD))
		{
			lcd = atoi(par->val);
		}
		else if(!strcmp(par->id, P_ID_RCINPUT))
		{
			rc_fd = atoi(par->val);
		}
		else if(!strcmp(par->id, P_ID_OFF_X))
		{
			sx = atoi(par->val);
		}
		else if(!strcmp(par->id, P_ID_END_X))
		{
			ex = atoi(par->val);
		}
		else if(!strcmp(par->id, P_ID_OFF_Y))
		{
			sy = atoi(par->val);
		}
		else if(!strcmp(par->id, P_ID_END_Y))
		{
			ey = atoi(par->val);
		}
	}
	if(fb_fd == -1 || lcd == -1 || rc_fd == -1 || sx == -1 || ex == -1 || sy == -1 || ey == -1)
	{
		printf("FBVNC <Invalid Param(s)>\n");
		return;
	}
	CConfigFile config('\t');
	config.loadConfig(CONFIGDIR "/vnc.conf");
	strncpy(hostname, config.getString("server","vnc").c_str(), 254);
   port=config.getInt32("port",5900);
	strcpy(passwdString,config.getString("passwd","").substr(0,8).c_str());
	if(strlen(passwdString) == 0)
	{
      passwdFile = CONFIGDIR "/vncpasswd";
	}
#endif
	sched = list_new();

#ifdef PLUGIN
	terminate=0;
	//useBGR233 = True;
	//forceOwnCmap = True;
	requestedDepth = 16;
	forceTruecolour = True;
	hwType = "ps2de";
#else
	processArgs(argc, argv);
#endif

	if (!ConnectToRFBServer(hostname, port)) 
	{
		printf("Cannot connect\n");
		return;
	}

	if (!InitialiseRFBConnection(rfbsock)) 
	{
		printf("Cannot initialize\n");
		return;
	}
	fbvnc_init();
	overlays_init();

	if (ts_type != TS_TYPE_IPAQ) {
		/* hide virt keyboard on machines with real keyboard */
		toggle_keyboard();
	}

	if (need_calibrate) do_calibration();

	if (!SetFormatAndEncodings()) 
		cleanup_and_exit("encodings", EXIT_ERROR);

	if (!SendFramebufferUpdateRequest(0, 0, si.framebufferWidth,
			si.framebufferHeight, False)) 
		cleanup_and_exit("update request", EXIT_ERROR);

	global_framebuffer.num_read_fds = 2;
	global_framebuffer.num_write_fds = 0;
	global_framebuffer.read_fd = readfd;
	readfd[FDNUM_VNC] = rfbsock;
	readfd[FDNUM_PNM]=-1;

	if (pnmFifo) {
		open_pnm_fifo(&pnmfd);
	}

	schedule_add(sched, 1000, FBVNC_EVENT_TICK_SECOND);

	while(!terminate) {
		fbvnc_event_t ev;

		if (sendUpdateRequest) {
			int msWait;
			struct timeval tv;
			gettimeofday(&tv, 0);

			msWait = (updateRequestPeriodms +
				((updateRequestTime.tv_sec - tv.tv_sec) *1000) +
				((updateRequestTime.tv_usec-tv.tv_usec) /1000));

			if (msWait > 3000) {
				/* bug workaround - max wait time to guard
				 * against time jump */
				msWait = 1000;
			}

			if (msWait <= 0) {
				if (!SendIncrementalFramebufferUpdateRequest())
					cleanup_and_exit("inc update", EXIT_ERROR);
			} else {
				schedule_add(sched, msWait, FBVNC_EVENT_SEND_UPDATE_REQUEST);
			}

		}

		if (img_saved) {
			readfd[FDNUM_PNM] = -1;
		} else if (pnmFifo) {
			readfd[FDNUM_PNM] = pnmfd;
		}
		fbvnc_get_event(&ev, sched);

		//printf("Event %X\n",ev.evtype);
		switch(ev.evtype) {
		case FBVNC_EVENT_NULL:
			break;
		case FBVNC_EVENT_QUIT:
			cleanup_and_exit("QUIT", EXIT_OK);
			break;
		case FBVNC_EVENT_TICK_SECOND:
			check_overlays(&ev);
			schedule_delete(sched, FBVNC_EVENT_TICK_SECOND);
			schedule_add(sched, 1000, FBVNC_EVENT_TICK_SECOND);
			break;
		case FBVNC_EVENT_SEND_UPDATE_REQUEST:
			if (!SendIncrementalFramebufferUpdateRequest())
				cleanup_and_exit("inc update", EXIT_ERROR);
			break;
		case FBVNC_EVENT_DATA_READABLE:
			if (ev.fd < 0) break;

			if (ev.fd == readfd[FDNUM_VNC]) {
				if (!HandleRFBServerMessage())
					cleanup_and_exit("rfb msg", EXIT_ERROR);
			} else if (pnmFifo && ev.fd == pnmfd) {
				load_pnm_image(&pnmfd);
				if (img_saved) show_pnm_image();
			} else {
				fprintf(stderr, "Got data on filehandle %d?!",
					ev.fd);
				cleanup_and_exit("bad data", EXIT_ERROR);
			}
			break;
			case FBVNC_EVENT_TS_DOWN:
			pan_toggle_count = 0;
			key_pending = 0;
			active_overlay = check_overlays(&ev);
			if (active_overlay)
				break;
			if (panning) {
				panning = 2;
				overlay_toggle=0;
				break;
			}
			mouse_x = ev.x;
			mouse_y = ev.y;
			if (mouse_multibutton_mode==2) {
				mouse_button = 1;
				/* workaround: Zaurus hardware can't register
				 * [menu] and [mail] simultaneously w/ ts
				 * so use mouse1 instead for right click
				 */
				if (btn_state[hbtn.mouse1]) mouse_button = 4;
				if (btn_state[hbtn.mouse2]) mouse_button = 2;
				if (btn_state[hbtn.mouse3]) mouse_button = 4;
				scaledPointerEvent(mouse_x, mouse_y, mouse_button);
			} else {
				scaledPointerEvent(mouse_x, mouse_y, mouse_button);
			}
			break;
		case FBVNC_EVENT_TS_MOVE:
			key_pending = 0;
			if (active_overlay) {
				overlay_event(&ev, active_overlay, 0);
				break;
			}
#if defined(INPUT_PS2MOUSE) | defined(PLUGIN)
			if (panning) {
				panning=2;
				overlay_toggle=0;
				vp_pan(-ev.dx, -ev.dy);
				break;
			}
#endif
			if (panning) {
				vp_pan(-ev.dx, -ev.dy);
				break;
			} 
			mouse_x = ev.x;
			mouse_y = ev.y;
			scaledPointerEvent(mouse_x, mouse_y, mouse_button);
			break;
		case FBVNC_EVENT_TS_UP:
			key_pending = 0;
			if (active_overlay) {
				overlay_event(&ev, active_overlay, 0);
				active_overlay = 0;
				break;
			}
			if (panning) {
				break;
			}
			mouse_x = ev.x;
			mouse_y = ev.y;
			if (mouse_multibutton_mode != 1) {
				scaledPointerEvent(mouse_x, mouse_y, 0);
			}
			break;
		case FBVNC_EVENT_KEYREPEAT:
			if (kbdRate && rep_key) {
				schedule_add(sched, 1000 / kbdRate, FBVNC_EVENT_KEYREPEAT);
				SendKeyEvent(rep_key, 1);
			}

			break;
		case FBVNC_EVENT_BTN_DOWN:
			if (btn_state[ev.key]) {
				static bool warned=0;
				if (!warned) {
					fprintf(stderr, "got hardware keyrepeat?!\n");
					warned=1;
				}
				rep_key = 0;
			}

			schedule_delete(sched, FBVNC_EVENT_KEYREPEAT);
			btn_state[ev.key] = 1;
			
			if (active_overlay) {
				overlay_event(&ev, active_overlay, 0);
				break;
			}
			if (ev.key==hbtn.mouse1) {
				if (btn_state[hbtn.altgr] || btn_state[hbtn.action]) {
					set_mouse_state((mouse_multibutton_mode+1) % 3);
					break;
				}
			}
			if (mouse_multibutton_mode==1) {
				if (ev.key==hbtn.mouse1) {
					mouse_button = 1;
				} else if (ev.key==hbtn.mouse2) {
					mouse_button = 2;
				} else if (ev.key==hbtn.mouse3) {
					mouse_button = 4;
				}
				scaledPointerEvent(mouse_x, mouse_y, mouse_button);
				break;
			}
			if (mouse_multibutton_mode==2) {
				if (ev.key==hbtn.mouse1 || ev.key==hbtn.mouse2 || ev.key==hbtn.mouse3) {
					mouse_button = 1;
					/* don't generate event yet */
					key_pending = ev.key;
					break;
				}
			}
			if (ev.key==hbtn.pan) {
				vp_hide_overlays();
				overlay_toggle=1;
				panning = 1;
				break;
			}

			if (key_pending) key_press(key_pending);
			key_press(ev.key);
			if (key_pending) key_release(key_pending);
			break;
		case FBVNC_EVENT_BTN_UP:
			btn_state[ev.key] = 0;
			schedule_delete(sched, FBVNC_EVENT_KEYREPEAT);
			rep_key = 0;
			if (active_overlay) {
				overlay_event(&ev, active_overlay, 0);
				break;
			}
			if (mouse_multibutton_mode==1 && (
				ev.key==hbtn.mouse1 ||
				ev.key==hbtn.mouse2 ||
				ev.key==hbtn.mouse3 )
			) {
				mouse_button = 0;
				scaledPointerEvent(mouse_x, mouse_y, 0);
			} else if (mouse_multibutton_mode==2 && (ev.key==hbtn.mouse1 || ev.key==hbtn.mouse2 || ev.key==hbtn.mouse3)) {
				mouse_button = 1;
				if (key_pending) {
					/* not used as modifier - send key events */
					key_press(ev.key);
					key_release(ev.key);
					key_pending = 0;
					schedule_delete(sched, FBVNC_EVENT_KEYREPEAT);
				}
			} else if (ev.key==hbtn.pan) {
#ifdef INPUT_PS2MOUSE
				if (panning == 1 && overlay_toggle) {
#else
				if (panning == 1) {
#endif
					/* no mouse move events, toggle overlay */
					if (global_framebuffer.hide_overlays > 1) {
						/* was hidden, restore it */
						vp_restore_overlays();
						vp_restore_overlays();
					} else {
						/* keep overlay hidden */
					}
					++ pan_toggle_count;
					if (pan_toggle_count > 7) {
						pan_toggle_count = 0;
						do_calibration();
						vp_pan(0, 0);
					}
				} else {
					vp_restore_overlays();
				}
				panning = 0;
			} else if (ev.key==hbtn.action) {
				/* nothing to do ?! */
			} else {
				key_release(ev.key);
			}
			break;
		}
	}
	return;
#ifdef PLUGIN
}
#endif
}

