#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>

#include <X11/keysym.h>

#include "vncviewer.h"
#include "fbgl.h"
#include "overlay.h"
#include "icons.h"
#include "list.h"
extern int sx,ex;
void
overlay_destructor(void *p) {
	fbvnc_overlay_t *this = p;

	/* if(this->pixels) free(this->pixels); */
	if(this->data) free(this->data);
}

typedef struct {
	int width;
	int key;
	int keyShifted;
	/* filled in as needed */
	int row;
	int col;
} keymap_t;

#define KEYBOARD_ROWS 5
#define KEYBOARD_COLS 30
#define ROW_HEIGHT 32
#define COL_WIDTH 16

static fbvnc_overlay_t *ov_keyboard;
static fbvnc_overlay_t *ov_mousestate;
static fbvnc_overlay_t *ov_battery;

static keymap_t keytable[KEYBOARD_ROWS][16] = {
{
	{2,	XK_Escape,	XK_Escape	},
	{2,	XK_1,		XK_exclam	},
	{2,	XK_2,		XK_at		},
	{2,	XK_3,		XK_numbersign	},
	{2,	XK_4,		XK_dollar	},
	{2,	XK_5,		XK_percent	},
	{2,	XK_6,		XK_asciicircum	},
	{2,	XK_7,		XK_ampersand	},
	{2,	XK_8,		XK_asterisk	},
	{2,	XK_9,		XK_parenleft	},
	{2,	XK_0,		XK_parenright	},
	{2,	XK_minus,	XK_underscore	},
	{2,	XK_equal,	XK_plus		},
	{2,	XK_backslash,	XK_bar		},
	{2,	XK_grave,	XK_asciitilde	},
	{0,	0,		0		},
},
{
	{3,	XK_Tab,		XK_Tab		},
	{2,	XK_q,		XK_Q		},
	{2,	XK_w,		XK_W		},
	{2,	XK_e,		XK_E		},
	{2,	XK_r,		XK_R		},
	{2,	XK_t,		XK_T		},
	{2,	XK_y,		XK_Y		},
	{2,	XK_u,		XK_U		},
	{2,	XK_i,		XK_I		},
	{2,	XK_o,		XK_O		},
	{2,	XK_p,		XK_P		},
	{2,	XK_bracketleft,	XK_braceleft	},
	{2,	XK_bracketright, XK_braceright	},
	{3,	XK_BackSpace,	XK_BackSpace	},
	{0,	0,		0		},
	{0,	0,		0		},
},
{
	{4,	XK_Control_L,	XK_Control_L	},
	{2,	XK_a,		XK_A		},
	{2,	XK_s,		XK_S		},
	{2,	XK_d,		XK_D		},
	{2,	XK_f,		XK_F		},
	{2,	XK_g,		XK_G		},
	{2,	XK_h,		XK_H		},
	{2,	XK_j,		XK_J		},
	{2,	XK_k,		XK_K		},
	{2,	XK_l,		XK_L		},
	{2,	XK_semicolon,	XK_colon	},
	{2,	XK_apostrophe,	XK_quotedbl	},
	{4,	XK_Return,	XK_Return	},
	{0,	0,		0		},
	{0,	0,		0		},
	{0,	0,		0		},
},
{
	{5,	XK_Shift_L,	XK_Shift_L	},
	{2,	XK_z,		XK_Z		},
	{2,	XK_x,		XK_X		},
	{2,	XK_c,		XK_C		},
	{2,	XK_v,		XK_V		},
	{2,	XK_b,		XK_B		},
	{2,	XK_n,		XK_N		},
	{2,	XK_m,		XK_M		},
	{2,	XK_comma,	XK_less		},
	{2,	XK_period,	XK_greater	},
	{2,	XK_slash,	XK_question	},
	{5,	XK_Shift_L,	XK_Shift_L	},
	{0,	0,		0		},
	{0,	0,		0		},
	{0,	0,		0		},
	{0,	0,		0		},
},
{
	{3,	XK_Hyper_L,	XK_Hyper_L	},
	{3,	XK_Alt_L,	XK_Alt_L	},
	{10,	XK_space,	XK_space	},
	{3,	XK_Alt_R,	XK_Alt_R	},
	{3,	XK_Hyper_L,	XK_Hyper_L	},
	{2,	XK_Left,	XK_Left		},
	{2,	XK_Right,	XK_Right	},
	{2,	XK_Up,		XK_Up		},
	{2,	XK_Down,	XK_Down		},
	{0,	0,		0		},
	{0,	0,		0		},
	{0,	0,		0		},
	{0,	0,		0		},
	{0,	0,		0		},
	{0,	0,		0		},
	{0,	0,		0		},
}};

#define NUM_MOD 5
#define MOD_SHIFT 	1
#define MOD_CTRL 	2
#define MOD_ALT 	3
#define MOD_ALTGR	4
#define MOD_FN		5

static keymap_t *mod_key[NUM_MOD+1];

void
open_emergency_xterm() {
	int screen = port - 5900;
	char cmd[]="xterm -geometry 80x24+0+0 -display :0 &";
	char *pos;

	if (screen<0 || screen>9) {
		fprintf(stderr, "screen invalid (port=%d)\n", port);
	}

	pos = index(cmd, ':');
	if (pos) *(pos+1) = '0'+screen;

	system(cmd);
}

void 
refresh_framerate() {
	int n = 0;
	int end;
	IMPORT_FRAMEBUFFER_VARS

	global_framebuffer.hide_overlays = 1;
	end = time(0) + 10;

	while (time(0) < end) {
		redraw_phys_all();
		n++;
	}
	
	global_framebuffer.hide_overlays = hide_overlays;

	fprintf(stderr, "%d.%d fps\n", n/10, n%10);
}

int
fn_translate(int key) {
	int out = key;

	switch (key) {
	case XK_Left:		out=XK_Home; 		break;
	case XK_Right:		out=XK_End;		break;
	case XK_Up:		out=XK_Prior;		break;
	case XK_Down:		out=XK_Next;		break;
	case XK_BackSpace:	out=XK_Delete;		break;
	case XK_Tab:		out=XK_Insert;		break;
	case XK_Escape:		out=XK_asciitilde;	break;
	case XK_1:		out=XK_F1;		break;
	case XK_2:		out=XK_F2;		break;
	case XK_3:		out=XK_F3;		break;
	case XK_4:		out=XK_F4;		break;
	case XK_5:		out=XK_F5;		break;
	case XK_6:		out=XK_F6;		break;
	case XK_7:		out=XK_F7;		break;
	case XK_8:		out=XK_F8;		break;
	case XK_9:		out=XK_F9;		break;
	case XK_0:		out=XK_F10;		break;
	case XK_minus:		out=XK_F11;		break;
	case XK_equal:		out=XK_F12;		break;

	case XK_a:		out=XK_adiaeresis;	break;
	case XK_o:		out=XK_odiaeresis;	break;
	case XK_u:		out=XK_udiaeresis;	break;
	case XK_A:		out=XK_Adiaeresis;	break;
	case XK_O:		out=XK_Odiaeresis;	break;
	case XK_U:		out=XK_Udiaeresis;	break;
	case XK_s:		out=XK_ssharp;		break;
	case XK_S:		out=XK_section;		break;
	case XK_e:		out=XK_EuroSign;	break;
	case XK_m:		out=XK_mu;		break;
	case XK_grave:		out=XK_degree;		break;

	case XK_bracketleft:	
		ev_zoom_out(0, 0);
		out=0;
		break;

	case XK_bracketright:	
		ev_zoom_in(0, 0);
		out=0;
		break;

	case XK_x:
		open_emergency_xterm();
		out=0;
		break;

	case XK_f:
		refresh_framerate();
		out=0;
		break;

	case XK_L:
		system("fbvnc 127.0.0.1:1");
		cleanup_and_exit("Bye.", EXIT_OK);
		break;

	case XK_Q:
#if 0
		/* Hack: don't quit if viewing localhost - could be
		 * the primary display. */
		if (!strcmp(hostname, "localhost") ||
		    !strcmp(hostname, "127.0.0.1")) break;
#endif
		cleanup_and_exit("Bye.", EXIT_OK);
		break;
	}
	return out;
}

void ov_redraw(fbvnc_overlay_t *ov) {
	IMPORT_FRAMEBUFFER_VARS

		redraw_phys(ov->x, ov->y, ov->w, ov->h);
}

void ov_redraw_part(fbvnc_overlay_t *ov, int x, int y, int w, int h) {
	IMPORT_FRAMEBUFFER_VARS

		redraw_phys(ov->x+x, ov->y+y, w, h);
}

void
ov_invert(fbvnc_overlay_t *ov, int x, int y, int w, int h) {
	int j;
	int o_xsize = ov->w;
	IMPORT_FRAMEBUFFER_VARS

	for (j=0; j<h; j++) {
		Pixel *buf_ov = ov->pixels + (y+j)*o_xsize + x;
		int i;

		for (i=0; i<w; i++) {
			buf_ov[i] ^= ~1;
		}
	}
}

void ov_fill(fbvnc_overlay_t *ov, int x, int y, int w, int h, int c) {
	int j;
	int o_xsize = ov->w;
	IMPORT_FRAMEBUFFER_VARS

	for (j=0; j<h; j++) {
		Pixel *buf_ov = ov->pixels + (y+j)*o_xsize + x;
		int i;

		for (i=0; i<w; i++) {
			buf_ov[i] = c;
		}
	}
}

void
invert_key(keymap_t *km, fbvnc_overlay_t *ov) {
	int x, y, w, h;

	x=km->col*COL_WIDTH;
	y=km->row*ROW_HEIGHT;
	w=km->width*COL_WIDTH;
	h=ROW_HEIGHT;

	ov_invert(ov, x, y, w, h);
	ov_redraw_part(ov, x, y, w, h);
}

void
xor_pixmaps(Pixel *dest, Pixel *a, Pixel *b, int w, int h)
{
	int i;
	for (i=0; i<w*h; i++) {
		*dest++ = (*a++) ^ (*b++);
	}
}

static void
init_virt_keyboard() {
	int row;

	for (row=0; row<KEYBOARD_ROWS; row++) {
		int col=0;
		keymap_t *km;
		for (km = keytable[row]; km->width; km++) {
			km->row = row;
			km->col = col;
			col += km->width;
		}
	}

	xor_pixmaps(ico_keybd_shifted, ico_keybd_shifted, ico_keybd, 240, 80);
}

void
shift_keyboard(fbvnc_overlay_t *ov) {
	xor_pixmaps(ico_keybd, ico_keybd, ico_keybd_shifted, 240, 80);
	ov_redraw_part(ov, 0, 0, 240, 80);
}

int
ev_keybd(fbvnc_event_t *ev, fbvnc_overlay_t *ov) {
	int x, y, row, col, i;
	keymap_t *km;	
	int mod = 0;
	static int key = 0;
	static keymap_t *inverted_key;
	static int mod_now = 0;
	static int mod_locked = 0;

	if (ev->evtype == FBVNC_EVENT_TS_UP) {
		if (key) {
			SendKeyEvent(key, 0);
			invert_key(inverted_key, ov);
			key = 0;
		}
		return 1;
	}
	
	x = ev->x;
	y = ev->y;

	row = y/ROW_HEIGHT;
	col = x/COL_WIDTH;

	if (row<0 || row>=KEYBOARD_ROWS || col<0 || col>=KEYBOARD_COLS) {
		fprintf(stderr, "keyboard: bad vals row=%d col=%d\n", row, col);
		return 1;
	}

	i=0;
	for (km = keytable[row]; km->width; km++) {
		if (col >= i && col < i+km->width) break;
		i += km->width;
	}

	if (!km) return 1;

	key = km->key;

	if (key == XK_Shift_L)   mod=MOD_SHIFT;
	if (key == XK_Control_L) mod=MOD_CTRL;
	if (key == XK_Alt_L)     mod=MOD_ALT;
	if (key == XK_Alt_R)     mod=MOD_ALTGR;
	if (key == XK_Hyper_L)   mod=MOD_FN;

	if (mod) {
		int mm = (1<<mod);
		if (mod_locked & mm) {
			mod_locked &= ~mm;
			SendKeyEvent(key, 0);
			invert_key(mod_key[mod], ov);
			if (mod==MOD_SHIFT) shift_keyboard(ov);
			key = 0;
		} else if (mod_now & mm) {
			mod_now &= ~mm;
			mod_locked |= mm;
			SendKeyEvent(key, 1);
			key = 0;
		} else {
			invert_key(km, ov);
			if (mod==MOD_SHIFT) shift_keyboard(ov);
			mod_key[mod]=km;
			mod_now |= mm;
		}
	} else {
		int m;

		for (m=1; m<=NUM_MOD-1; m++) {
			if (mod_now & (1<<m)) {
				SendKeyEvent(mod_key[m]->key, 1);
			}
		}

		if ((mod_now|mod_locked) & (1<<MOD_SHIFT))
			key = km->keyShifted;
		if ((mod_now|mod_locked) & (1<<MOD_FN))
			key = fn_translate(key);
		if (key) SendKeyEvent(key, 1);

		for (m=1; m<=NUM_MOD-1; m++) {
			if (mod_now & (1<<m)) {
				SendKeyEvent(mod_key[m]->key, 0);
				invert_key(mod_key[m], ov);
				if (m==MOD_SHIFT) shift_keyboard(ov);
			}
		}
		if (mod_now & (1<<MOD_FN)) {
			invert_key(mod_key[MOD_FN], ov);
		}
		
		mod_now = 0;
	}

	if (key) {
		invert_key(km, ov);
		inverted_key = km;
	}

	return 1;
}

int
ev_quit(fbvnc_event_t *ev, fbvnc_overlay_t *ov) {
	cleanup_and_exit("Bye.", EXIT_OK);
	return 1;
}

int
ev_zoom_out(fbvnc_event_t *ev, fbvnc_overlay_t *ov) {
	IMPORT_FRAMEBUFFER_VARS

	if (v_scale==MAX_SCALE) return 1;

	global_framebuffer.v_scale++;
#if 0
	vp_pan(-pv_xsize/4, -pv_ysize/4);
#else
	vp_pan(0, 0);
#endif
	return 1;
}

int
ev_zoom_in(fbvnc_event_t *ev, fbvnc_overlay_t *ov) {
	IMPORT_FRAMEBUFFER_VARS

	if (v_scale==1) return 1;

	global_framebuffer.v_scale--;
#if 0
	vp_pan(pv_xsize/4, pv_ysize/4);
#else
	vp_pan(0, 0);
#endif
	return 1;
}

void 
set_scale(int s) {
	if (s<1 || s>4) return;

	global_framebuffer.v_scale = s;
#if 0
	vp_pan(pv_xsize/4, pv_ysize/4);
#else
	vp_pan(0, 0);
#endif
}

int mouse_button;
static bool light=1;

void
set_light(bool on) {
	if (on) {
		system("bl 1 1 0");
		light = 1;
	} else {
		system("bl 1 0 0");
		light = 0;
	}
	
}

void
toggle_light() {
	if (light) {
		set_light(0);
	} else {
		set_light(1);
	}
}

int
ev_lamp(fbvnc_event_t *ev, fbvnc_overlay_t *ov) {
	toggle_light();
	return 1;
}

int
ev_zoom(fbvnc_event_t *ev, fbvnc_overlay_t *ov) {
	int row = ev->x / 10;

	set_scale(row);

	return 1;
}

#define VOLUME_WIDTH 25

#define USE_MIXER_DIRECTLY
#ifdef USE_MIXER_DIRECTLY
	#include <unistd.h>
	#include <sys/ioctl.h>
	#include <fcntl.h>
	#include <linux/soundcard.h>
#endif

int
ev_volume(fbvnc_event_t *ev, fbvnc_overlay_t *ov) {
	int percent = ev->x * 100 / VOLUME_WIDTH;
	char cmd[99];
	int i;

	if (percent<0 || percent>100) return 1;

#ifdef USE_MIXER_DIRECTLY
	{
		/* fail silently */
		int play_fd, devmask_play;
		play_fd=open("/dev/mixer1", O_RDONLY);
	  	ioctl(play_fd, SOUND_MIXER_READ_DEVMASK, &devmask_play);
		if(play_fd >= 0) {
			ioctl(0, MIXER_WRITE(play_fd), &percent);

		}

	}
#else
	sprintf(cmd, "aumix -v%d", percent);
	system(cmd);
#endif

	for (i=2; i<VOLUME_WIDTH-2; i++) {
		int dy;

		dy = i*6/VOLUME_WIDTH;
		if (!dy) continue;

		if (i < ev->x) {
			ov_fill(ov, i, 7-dy, 1, dy, ICO_FG);
		} else if (i == ev->x) {
			ov_fill(ov, i, 7-dy, 1, dy, ICO_BORDER);
		} else {
			if (dy>1) ov_fill(ov, i, 7-dy, 1, 1, ICO_BORDER);
			if (dy>2) ov_fill(ov, i, 8-dy, 1, dy-2, ICO_TRANS);
			ov_fill(ov, i, 6, 1, 1, ICO_BORDER);
		}
	}
	ov_redraw(ov);

	return 1;
}

void
toggle_keyboard() {
	IMPORT_FRAMEBUFFER_VARS

	ov_keyboard->visible ^= 1;

	vp_pan(0,0);
}

int
ev_kbd_sel(fbvnc_event_t *ev, fbvnc_overlay_t *ov) {
	toggle_keyboard();
	return 1;
}

int
get_battery_percent()
{
	FILE *f;
	char buf[100];
	int c_space=0;
	int c_percent=0;
	int i;
	int percent;
 
	f=fopen("/proc/apm", "r");
	if (!f) return 0;

	fgets(buf, sizeof buf, f);
	fclose(f);

	if (!strlen(buf)) return 0;
	
	for (i=strlen(buf)-1; i>=0; --i) {
		if (buf[i]=='%') {
			c_percent=i;
			buf[i]=' ';
		} else if (c_percent && buf[i]==' ') {
			c_space=i;
			break;
		}
	}
	
	percent = atoi(buf+c_space+1);

	return percent;
}

#define BATTERY_WIDTH 19
#define LOW_BATTERY_PERCENT 10

void
set_battery_status(int percent) {
	static int shown_percent = 0;
	static bool blink_state_off = 0;
	int fx;
	IMPORT_FRAMEBUFFER_VARS

	if (percent <= 10) {
		blink_state_off ^= 1;
		if (blink_state_off) {
			ov_fill(ov_battery, 2, 2, BATTERY_WIDTH-6, 5, 63488 /*red*/);
			ov_redraw(ov_battery);
			return;
		}
	} else if (percent == shown_percent) return; /* nothing to do */

	fx = (BATTERY_WIDTH-6)*percent/100;

	ov_fill(ov_battery, 2,    2, fx, 5, ICO_FG);
	ov_fill(ov_battery, 2+fx, 2, BATTERY_WIDTH-6-fx, 5, ICO_BORDER);

	ov_redraw(ov_battery);
	shown_percent = percent;
}

int
ev_battery(fbvnc_event_t *ev, fbvnc_overlay_t *ov) {
	set_battery_status(get_battery_percent());
#if 1
	return 0; /* give others a chance to handle the timer */
#else
	return 1;
#endif
}

int
ev_quickpan(fbvnc_event_t *ev, fbvnc_overlay_t *ov) {
	int x0, y0;
	IMPORT_FRAMEBUFFER_VARS

	if (ev->evtype == FBVNC_EVENT_TS_DOWN) vp_hide_overlays();

	x0 = ev->x * v_xsize / ov->w - pv_xsize*v_scale/2;
	y0 = ev->y * v_ysize / ov->h - pv_ysize*v_scale/2;

	if (x0 != v_x0 || y0 != v_y0) vp_pan_virt(x0, y0);

	if (ev->evtype == FBVNC_EVENT_TS_UP) vp_restore_overlays();
	return 1;
}

fbvnc_overlay_t *
add_overlay(int x0l, int y0l, int x0p, int y0p, 
	int w, int h, Pixel *icon,
	int events_wanted, fbvnc_ev_callback_t func, void *data)
{
	List *p;
	fbvnc_overlay_t *ov;
	IMPORT_FRAMEBUFFER_VARS
	
	p = list_insert_obj(
		overlays,
		sizeof *ov, 
		overlay_destructor
	);
	ov = p->val;

	ov->x0l = x0l;
	ov->y0l = y0l;
	ov->x0p = x0p;
	ov->y0p = y0p;

	ov->x = ov->x0l;
	ov->y = ov->y0l;

	ov->h = h;
	ov->w = w;
	ov->events_wanted = events_wanted;
	
	ov->pixels = icon;
	ov->data = data;

	ov->callback = func;

	ov->visible = 1;

	return ov;
}

void draw_pixmap(Pixel *pix, int x, int y, int w, int h) {
	int j;
	IMPORT_FRAMEBUFFER_VARS
	
	for (j=0; j<h; j++) {
		Pixel *src = pix + j*w;
		Pixel *dst = p_buf + p_xsize*(y + j + p_yoff) + x + p_xoff;
		int i;
		
		for (i=0; i<w; i++) {
			Pixel c = *src;

			if (c != ICO_TRANS) *dst = c;
			dst++;
			src++;
		}
	}
}

void draw_overlay_part(fbvnc_overlay_t *ov, int x, int y, int w, int h) {
	int j;
	IMPORT_FRAMEBUFFER_VARS

	if (hide_overlays) return;
	if (! ov->visible) return;

	for (j=0; j<h; j++) {
		Pixel *src = ov->pixels + (j+y)*ov->w + x;
		Pixel *dst = p_buf + p_xsize*(ov->y + j + y + p_yoff)
		+ ov->x + x + p_xoff;
		int i;
		
		for (i=0; i<w; i++) {
			Pixel c = *src;
			
			if (c != ICO_TRANS) *dst = c;
			dst++;
			src++;
		}
	}
}

void draw_overlay(fbvnc_overlay_t *ov) {
	draw_overlay_part(ov, 0, 0, ov->w, ov->h);
}

void
redraw_all_overlays() {
	List *p;
	
	if (global_framebuffer.hide_overlays)
		return;
	for (p=global_framebuffer.overlays->next; p; p = p->next) {
		draw_overlay((fbvnc_overlay_t *)p->val);
	}
}

#define MIN(a,b) ((a<b) ? (a) : (b))
#define MAX(a,b) ((a>b) ? (a) : (b))

void
redraw_overlays(int xp, int yp, int wp, int hp) {
	List *p;
	IMPORT_FRAMEBUFFER_VARS

	if (global_framebuffer.hide_overlays)
		return;
	for (p=global_framebuffer.overlays->next; p; p = p->next) {
		fbvnc_overlay_t *ov = p->val;

		/* clip to redraw changed regions only */
		int x0, y0, x1, y1, w, h;
		x0 = MAX(xp, ov->x);
		y0 = MAX(yp, ov->y);
		x1 = MIN(xp+wp, ov->x+ov->w);
		y1 = MIN(yp+hp, ov->y+ov->h);
		
		w = x1-x0;
		h = y1-y0;
		
		if (h>0 && w>0) {
			draw_overlay_part(ov, x0 - ov->x, y0 - ov->y, w, h);
		}
	}
}

bool
overlay_event(fbvnc_event_t *ev, fbvnc_overlay_t *ov, bool check_hit) {
	fbvnc_event_t new_ev;
	int x, y;
	IMPORT_FRAMEBUFFER_VARS
	
	if (! (ov->events_wanted & ev->evtype)) {
		return 0;
	}
	if (! ov->visible) return 0;

	x = ev->x;
	y = ev->y;

	if (check_hit && (
			x < ov->x || x >= ov->x + ov->w ||
			y < ov->y || y >= ov->y + ov->h
		)
	) {
	  	return 0;
	}

	x -= ov->x;
	y -= ov->y;
	if (x<0) x=0;
	if (y<0) y=0;
	if (x>ov->w) x=ov->w;
	if (y>ov->h) y=ov->h;

	new_ev = *ev;
	new_ev.x = x;
	new_ev.y = y;

	return ov->callback(&new_ev, ov);
}

static bool
is_mouse_event(int type)
{
	if (type==FBVNC_EVENT_TS_DOWN || type==FBVNC_EVENT_TS_MOVE || type==FBVNC_EVENT_TS_UP) {
		return 1;
	} else {
		return 0;
	}
}

fbvnc_overlay_t *
check_overlays(fbvnc_event_t *ev) {
	List *p;
	IMPORT_FRAMEBUFFER_VARS

	if (hide_overlays)
		return 0;

	for (p=overlays->next; p; p = p->next) {
		fbvnc_overlay_t *ov = p->val;

		if (overlay_event(ev, ov, is_mouse_event(ev->evtype))) return ov;
	}
	return 0;
}

void
vp_hide_overlays() {
	if (! global_framebuffer.hide_overlays++)
		redraw_phys_all();
}

void
vp_restore_overlays() {
	IMPORT_FRAMEBUFFER_VARS

	if (hide_overlays == 0) {
		fprintf(stderr, "Warning: restore_overlays called with ref=0\n");
		return;
	}
	if (! --global_framebuffer.hide_overlays)
		redraw_phys_all();
}

void
overlays_init() {
	IMPORT_FRAMEBUFFER_VARS

/*	add_overlay(
		pv_xsize-45, 0,
		pv_ysize-45, 0,
		10, 10,
		ico_lamp, FBVNC_EVENT_TS_DOWN, ev_lamp, 0);
	
	mouse_multibutton_mode = 0;
	mouse_button = 0;
	ov_mousestate = add_overlay(
		pv_xsize-63, 0,
		pv_ysize-63, 0,
		12, 9,
		ico_mouse, FBVNC_EVENT_TS_DOWN, ev_mouse, 0);

	add_overlay(
		pv_xsize-81, 0,
		pv_ysize-81, 0,
		15, 9,
		ico_kbdselect, FBVNC_EVENT_TS_DOWN, ev_kbd_sel, 0);

	add_overlay(
		pv_xsize-143, 0,
		pv_ysize-143, 0,
		50, 9,
		ico_zoom, FBVNC_EVENT_TS_DOWN, ev_zoom, 0);
	
	add_overlay(
		pv_xsize-173, 0,
		pv_ysize-173, 0,
		25, 9,
		ico_volume, FBVNC_EVENT_TS_DOWN | FBVNC_EVENT_TS_MOVE | FBVNC_EVENT_TS_UP, ev_volume, 0);

	ov_battery = add_overlay(
		pv_xsize-200, 0,
		pv_ysize-200, 0,
		19, 9,
		ico_battery, FBVNC_EVENT_TS_DOWN | FBVNC_EVENT_TICK_SECOND, ev_battery, 0);
	*/
	/* is physical display smaller than logical one? */
	/*if (global_framebuffer.pv_xsize * global_framebuffer.pv_ysize < global_framebuffer.v_xsize * global_framebuffer.v_ysize) {
		add_overlay(
			0, 0,
			0, 0,
			16, 12,
			ico_pan, FBVNC_EVENT_TS_DOWN | FBVNC_EVENT_TS_MOVE | FBVNC_EVENT_TS_UP, ev_quickpan, 0);
	}*/

	init_virt_keyboard();
	ov_keyboard = add_overlay(
		0 + (ex-sx - 480)/2, pv_ysize-160,
		0, pv_xsize-160,
		480, 160, 
		ico_keybd, 
		FBVNC_EVENT_TS_DOWN | FBVNC_EVENT_TS_UP, ev_keybd, 0);
	
}

