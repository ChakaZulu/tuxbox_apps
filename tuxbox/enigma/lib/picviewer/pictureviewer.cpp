#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/gdi/fb.h>
#include <lib/base/estring.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <lib/picviewer/pictureviewer.h>
#include "fb_display.h"

/* resize.cpp */
extern unsigned char *simple_resize(unsigned char *orgin, int ox, int oy, int dx, int dy);
extern unsigned char *color_average_resize(unsigned char *orgin, int ox, int oy, int dx, int dy);

#ifdef FBV_SUPPORT_GIF
extern int fh_gif_getsize(const char *, int *, int *, int, int);
extern int fh_gif_load(const char *, unsigned char *, int, int);
extern int fh_gif_id(const char *);
#endif
#ifdef FBV_SUPPORT_JPEG
extern int fh_jpeg_getsize(const char *, int *, int *, int, int);
extern int fh_jpeg_load(const char *, unsigned char *, int, int);
extern int fh_jpeg_id(const char *);
#endif
#ifdef FBV_SUPPORT_PNG
extern int fh_png_getsize(const char *, int *, int *, int, int);
extern int fh_png_load(const char *, unsigned char *, int, int);
extern int fh_png_id(const char *);
#endif
#ifdef FBV_SUPPORT_BMP
extern int fh_bmp_getsize(const char *, int *, int *, int, int);
extern int fh_bmp_load(const char *, unsigned char *, int, int);
extern int fh_bmp_id(const char *);
#endif
#ifdef FBV_SUPPORT_CRW
extern int fh_crw_getsize(const char *, int *, int *, int, int);
extern int fh_crw_load(const char *, unsigned char *, int, int);
extern int fh_crw_id(const char *);
#endif

ePictureViewer *ePictureViewer::instance;

ePictureViewer::ePictureViewer(): slideshowTimer(eApp), messages(this, 1)
{
	printf("[PICTUREVIEWER] Constructor...\n");
	if (!instance)
		instance = this;

	fh_root = NULL;
	m_scaling = COLOR;
	m_aspect = 4.0 / 3;
	m_CurrentPic_Name = "";
	m_CurrentPic_Buffer = NULL;
	m_CurrentPic_X = 0;
	m_CurrentPic_Y = 0;
	m_CurrentPic_XPos = 0;
	m_CurrentPic_YPos = 0;
	m_CurrentPic_XPan = 0;
	m_CurrentPic_YPan = 0;
	m_NextPic_Name = "";
	m_NextPic_Buffer = NULL;
	m_NextPic_X = 0;
	m_NextPic_Y = 0;
	m_NextPic_XPos = 0;
	m_NextPic_YPos = 0;
	m_NextPic_XPan = 0;
	m_NextPic_YPan = 0;
	int xs = 0, ys = 0;
	getCurrentRes(&xs, &ys);
	m_startx = 0;
	m_endx = xs - 1;
	m_starty = 0;
	m_endy = ys - 1;

	m_busy_buffer = NULL;

	init_handlers();

	CONNECT(messages.recv_msg, ePictureViewer::gotMessage);
	CONNECT(slideshowTimer.timeout, ePictureViewer::slideshowTimeout);
	run();
	printf("[PICTUREVIEWER] Constructor done.\n");
}

ePictureViewer::~ePictureViewer()
{
	messages.send(Message::quit);
	if (thread_running())
		kill();
	if (instance == this)
		instance = 0;
}

void ePictureViewer::thread()
{
	printf("[PICTUREVIEWER] thread...\n");
	exec();
	printf("[PICTUREVIEWER] thread done.\n");
}

void ePictureViewer::gotMessage(const Message &msg )
{
	switch (msg.type)
	{
		case Message::display:
		{
			printf("[PICTUREVIEWER] display: %s\n", msg.filename);
			struct fb_var_screeninfo *screenInfo = fbClass::getInstance()->getScreenInfo();
			fbClass::getInstance()->SetMode(screenInfo->xres, screenInfo->yres, 16);
			fbClass::getInstance()->PutCMAP();
			ShowImage(std::string(msg.filename), false);
			break;
		}
		case Message::slideshow:
		{
			printf("[PICTUREVIEWER] slideShow: %s\n", msg.filename);
			struct fb_var_screeninfo *screenInfo = fbClass::getInstance()->getScreenInfo();
			fbClass::getInstance()->SetMode(screenInfo->xres, screenInfo->yres, 16);
			fbClass::getInstance()->PutCMAP();
			ShowSlideshow(std::string(msg.filename), false);
			break;
		}
		case Message::zoom:
			printf("[PICTUREVIEWER] zoom\n");
//			zoom
			break;
		case Message::move:
			printf("[PICTUREVIEWER] move\n");
//			move
			break;
		case Message::quit:
			printf("[PICTUREVIEWER] quit\n");
			slideshowTimer.stop();
			Cleanup();
			quit(0);
			break;
		default:
			printf("[PICTUREVIEWER] unhandled thread message");
	}
}

// eAutoInitP0<ePictureViewer> init_ePictureViewer(eAutoInitNumbers::configuration + 1, "Picture Viewer");

void ePictureViewer::add_format(int (*picsize)(const char *, int *, int *, int, int ), int (*picread)(const char *, unsigned char *, int, int), int (*id)(const char*))
{
	CFormathandler *fhn;
	fhn = (CFormathandler *) malloc(sizeof(CFormathandler));
	fhn->get_size = picsize;
	fhn->get_pic = picread;
	fhn->id_pic = id;
	fhn->next = fh_root; 
	fh_root = fhn;
}

void ePictureViewer::init_handlers(void)
{
#ifdef FBV_SUPPORT_GIF
	add_format(fh_gif_getsize, fh_gif_load, fh_gif_id);
#endif
#ifdef FBV_SUPPORT_JPEG
	add_format(fh_jpeg_getsize, fh_jpeg_load, fh_jpeg_id);
#endif
#ifdef FBV_SUPPORT_PNG
	add_format(fh_png_getsize, fh_png_load, fh_png_id);
#endif
#ifdef FBV_SUPPORT_BMP
	add_format(fh_bmp_getsize, fh_bmp_load, fh_bmp_id);
#endif
#ifdef FBV_SUPPORT_CRW
	add_format(fh_crw_getsize, fh_crw_load, fh_crw_id);
#endif
}

ePictureViewer::CFormathandler * ePictureViewer::fh_getsize(const char *name, int *x, int *y, int width_wanted, int height_wanted)
{
	CFormathandler *fh;
	for (fh = fh_root; fh != NULL; fh = fh->next)
	{
		if (fh->id_pic(name))
			if (fh->get_size(name, x, y, width_wanted, height_wanted) == FH_ERROR_OK)
				return(fh);
	}
	return(NULL);
}

bool ePictureViewer::DecodeImage(const std::string& name, bool showBusySign, bool unscaled)
{
	printf("DecodeImage {\n");
	if (name == m_NextPic_Name)
	{
		printf("DecodeImage }\n");
		return true;
	}

	int x, y, xs, ys, imx, imy;
	getCurrentRes(&xs, &ys);

	// Show red block for "next ready" in view state
	if (showBusySign)
		showBusy(m_startx + 3, m_starty + 3, 10, 0xff, 0, 0);

	CFormathandler *fh;
	if (unscaled)
		fh = fh_getsize(name.c_str(), &x, &y, INT_MAX, INT_MAX);
	else
		fh = fh_getsize(name.c_str(), &x, &y, m_endx - m_startx, m_endy - m_starty);
	if (fh)
	{
		if (m_NextPic_Buffer != NULL)
		{
			free(m_NextPic_Buffer);
		}
		m_NextPic_Buffer = (unsigned char *) malloc(x * y * 3);
		if (m_NextPic_Buffer == NULL)
		{
			printf("Error: malloc\n");
			return false;
		}

		printf("---Decoding start(%d/%d)\n", x, y);
		if (fh->get_pic(name.c_str(), m_NextPic_Buffer, x, y) == FH_ERROR_OK)
		{
			printf("---Decoding done\n");
			if ((x > (m_endx - m_startx) || y > (m_endy - m_starty)) && m_scaling != NONE && !unscaled)
			{
				double aspect_ratio_correction = m_aspect / ((double)xs / ys);
				if ((aspect_ratio_correction * y * (m_endx - m_startx) / x) <= (m_endy - m_starty))
				{
					imx = (m_endx - m_startx);
					imy = (int)(aspect_ratio_correction * y * (m_endx - m_startx) / x);
				}
				else
				{
					imx = (int)((1.0 / aspect_ratio_correction) * x * (m_endy - m_starty) / y);
					imy = m_endy - m_starty;
				}
				if (m_scaling == SIMPLE)
					m_NextPic_Buffer = simple_resize(m_NextPic_Buffer, x, y, imx, imy);
				else
					m_NextPic_Buffer = color_average_resize(m_NextPic_Buffer, x, y, imx, imy);
				x = imx; y = imy;
			}
			m_NextPic_X = x;
			m_NextPic_Y = y;
			if (x < (m_endx - m_startx))
				m_NextPic_XPos = (m_endx - m_startx - x) / 2 + m_startx;
			else
				m_NextPic_XPos = m_startx;
			if (y < (m_endy - m_starty))
				m_NextPic_YPos = (m_endy - m_starty-y) / 2 + m_starty;
			else
				m_NextPic_YPos = m_starty;
			if (x > (m_endx - m_startx))
				m_NextPic_XPan = (x - (m_endx - m_startx)) / 2;
			else
				m_NextPic_XPan = 0;
			if (y > (m_endy - m_starty))
				m_NextPic_YPan = (y - (m_endy - m_starty)) / 2;
			else
				m_NextPic_YPan = 0;
		}
		else
		{
			printf("Unable to read file !\n");
			free(m_NextPic_Buffer);
			m_NextPic_Buffer = (unsigned char *) malloc(3);
			if (m_NextPic_Buffer == NULL)
			{
				printf("Error: malloc\n");
				return false;
			}
			memset(m_NextPic_Buffer, 0 , 3);
			m_NextPic_X = 1;
			m_NextPic_Y = 1;
			m_NextPic_XPos = 0;
			m_NextPic_YPos = 0;
			m_NextPic_XPan = 0;
			m_NextPic_YPan = 0;
		}
	}
	else
	{
		printf("Unable to read file or format not recognized!\n");
		if (m_NextPic_Buffer != NULL)
		{
			free(m_NextPic_Buffer);
		}
		m_NextPic_Buffer = (unsigned char *) malloc(3);
		if (m_NextPic_Buffer == NULL)
		{
			printf("Error: malloc\n");
			return false;
		}
		memset(m_NextPic_Buffer, 0 , 3);
		m_NextPic_X = 1;
		m_NextPic_Y = 1;
		m_NextPic_XPos = 0;
		m_NextPic_YPos = 0;
		m_NextPic_XPan = 0;
		m_NextPic_YPan = 0;
	}
	m_NextPic_Name = name;
	hideBusy();
	printf("DecodeImage }\n");
	return(m_NextPic_Buffer != NULL);
}

void ePictureViewer::SetVisible(int startx, int endx, int starty, int endy)
{
	m_startx = startx;
	m_endx = endx;
	m_starty = starty;
	m_endy = endy;
}

bool ePictureViewer::ShowImage(const std::string & filename, bool unscaled)
{
	printf("Show Image {\n");
	// Wird eh ueberschrieben ,also schonmal freigeben... (wenig speicher)
	if (m_CurrentPic_Buffer != NULL)
	{
		free(m_CurrentPic_Buffer);
		m_CurrentPic_Buffer = NULL;
	}
	DecodeImage(filename, false, unscaled);
	DisplayNextImage();
	printf("Show Image }\n");
	return true;
}

void ePictureViewer::slideshowTimeout()
{
	printf("[PICTUREVIEWER] slideshowTimeout...\n");
	
	slideshowTimer.start(5000, true);
	++myIt;
	eString tmp = *myIt;
	printf("[PICTUREVIEWER] slideshowTimeout: show %s\n", tmp.c_str());
	ShowImage(*myIt, false);
}

bool ePictureViewer::ShowSlideshow(const std::string& filename, bool unscaled)
{
	printf("Show Slideshow {\n");
	slideshowList.clear();	
	// gen pic list for slideshow
	int pos = filename.find_last_of("/");
	if (pos == -1)
		pos = filename.length() - 1;
	eString directory = filename.substr(0, pos);
	printf("---directory: %s\n", directory.c_str());
	DIR *d = opendir(directory.c_str());
	if (d)
	{
		while (struct dirent *e = readdir(d))
			slideshowList.push_back(eString(e->d_name));
		closedir(d);
	}
	slideshowList.sort();
	myIt = slideshowList.begin();
	slideshowTimer.start(5000, true);
	ShowImage(*myIt, unscaled);
	printf("Show Slideshow }\n");
	return true;
}

bool ePictureViewer::DisplayNextImage()
{
	printf("DisplayNextImage {\n");
	if (m_CurrentPic_Buffer != NULL)
	{
		free(m_CurrentPic_Buffer);
		m_CurrentPic_Buffer = NULL;
	}
	if (m_NextPic_Buffer != NULL)
		fb_display(m_NextPic_Buffer, m_NextPic_X, m_NextPic_Y, m_NextPic_XPan, m_NextPic_YPan, m_NextPic_XPos, m_NextPic_YPos);
	printf("---DisplayNextImage fb_disp done\n");
	m_CurrentPic_Buffer = m_NextPic_Buffer;
	m_NextPic_Buffer = NULL;
	m_CurrentPic_Name = m_NextPic_Name;
	m_NextPic_Name = "";
	m_CurrentPic_X = m_NextPic_X;
	m_CurrentPic_Y = m_NextPic_Y;
	m_CurrentPic_XPos = m_NextPic_XPos;
	m_CurrentPic_YPos = m_NextPic_YPos;
	m_CurrentPic_XPan = m_NextPic_XPan;
	m_CurrentPic_YPan = m_NextPic_YPan;
	printf("DisplayNextImage }\n");
	return true;
}

void ePictureViewer::Zoom(float factor)
{
	printf("Zoom %f {\n",factor);
	showBusy(m_startx + 3, m_starty + 3, 10, 0xff, 0xff, 0);

	int oldx = m_CurrentPic_X;
	int oldy = m_CurrentPic_Y;
	unsigned char *oldBuf = m_CurrentPic_Buffer;
	m_CurrentPic_X = (int)(factor * m_CurrentPic_X);
	m_CurrentPic_Y = (int)(factor * m_CurrentPic_Y);

	if (m_scaling == COLOR)
		m_CurrentPic_Buffer = color_average_resize(m_CurrentPic_Buffer, oldx, oldy, m_CurrentPic_X, m_CurrentPic_Y);
	else
		m_CurrentPic_Buffer = simple_resize(m_CurrentPic_Buffer, oldx, oldy, m_CurrentPic_X, m_CurrentPic_Y);

	if (m_CurrentPic_Buffer == oldBuf)
	{
		// resize failed
		hideBusy();
		return;
	}

	if (m_CurrentPic_X < (m_endx - m_startx))
		m_CurrentPic_XPos = (m_endx - m_startx - m_CurrentPic_X) / 2 + m_startx;
	else
		m_CurrentPic_XPos = m_startx;
	if (m_CurrentPic_Y < (m_endy - m_starty))
		m_CurrentPic_YPos = (m_endy - m_starty - m_CurrentPic_Y) / 2 + m_starty;
	else
		m_CurrentPic_YPos = m_starty;
	if (m_CurrentPic_X > (m_endx - m_startx))
		m_CurrentPic_XPan = (m_CurrentPic_X - (m_endx - m_startx)) / 2;
	else
		m_CurrentPic_XPan = 0;
	if (m_CurrentPic_Y > (m_endy - m_starty))
		m_CurrentPic_YPan = (m_CurrentPic_Y - (m_endy - m_starty)) / 2;
	else
		m_CurrentPic_YPan = 0;
	fb_display(m_CurrentPic_Buffer, m_CurrentPic_X, m_CurrentPic_Y, m_CurrentPic_XPan, m_CurrentPic_YPan, m_CurrentPic_XPos, m_CurrentPic_YPos);
	printf("Zoom }\n");
}

void ePictureViewer::Move(int dx, int dy)
{
	printf("Move %d %d {\n", dx, dy);
	showBusy(m_startx + 3, m_starty + 3, 10, 0, 0xff, 0);

	int xs, ys;
	getCurrentRes(&xs, &ys);
	m_CurrentPic_XPan += dx;
	if (m_CurrentPic_XPan + xs >= m_CurrentPic_X)
		m_CurrentPic_XPan = m_CurrentPic_X - xs - 1;
	if (m_CurrentPic_XPan < 0)
		m_CurrentPic_XPan = 0;

	m_CurrentPic_YPan += dy;
	if (m_CurrentPic_YPan + ys >= m_CurrentPic_Y)
		m_CurrentPic_YPan = m_CurrentPic_Y - ys - 1;
	if(m_CurrentPic_YPan < 0)
		m_CurrentPic_YPan = 0;

	if (m_CurrentPic_X < (m_endx - m_startx))
		m_CurrentPic_XPos = (m_endx - m_startx - m_CurrentPic_X) / 2 + m_startx;
	else
		m_CurrentPic_XPos = m_startx;
	if (m_CurrentPic_Y < (m_endy - m_starty))
		m_CurrentPic_YPos = (m_endy - m_starty - m_CurrentPic_Y) / 2 + m_starty;
	else
		m_CurrentPic_YPos = m_starty;
//	dbout("Display x(%d) y(%d) xpan(%d) ypan(%d) xpos(%d) ypos(%d)\n",m_CurrentPic_X, m_CurrentPic_Y,
//	m_CurrentPic_XPan, m_CurrentPic_YPan, m_CurrentPic_XPos, m_CurrentPic_YPos);

	fb_display(m_CurrentPic_Buffer, m_CurrentPic_X, m_CurrentPic_Y, m_CurrentPic_XPan, m_CurrentPic_YPan, m_CurrentPic_XPos, m_CurrentPic_YPos);
	printf("Move }\n");
}

void ePictureViewer::showBusy(int sx, int sy, int width, char r, char g, char b)
{
	printf("Show Busy{\n");

	unsigned char rgb_buffer[3];
	unsigned char* fb_buffer;
	unsigned char* busy_buffer_wrk;
	int cpp;
	struct fb_var_screeninfo *var;
	var = fbClass::getInstance()->getScreenInfo();

	rgb_buffer[0] = r;
	rgb_buffer[1] = g;
	rgb_buffer[2] = b;

	fb_buffer = (unsigned char *) convertRGB2FB(rgb_buffer, 1, var->bits_per_pixel, &cpp);
	if (fb_buffer == NULL)
	{
		printf("Error: malloc\n");
		return;
	}
	if (m_busy_buffer != NULL)
	{
		free(m_busy_buffer);
		m_busy_buffer = NULL;
	}
	m_busy_buffer = (unsigned char *) malloc(width * width * cpp);
	if (m_busy_buffer == NULL)
	{
		printf("Error: malloc\n");
		return;
	}
	busy_buffer_wrk = m_busy_buffer;
	unsigned char * fb = fbClass::getInstance()->lfb;
	unsigned int stride = fbClass::getInstance()->Stride();

	for (int y = sy ; y < sy + width; y++)
	{
		for(int x = sx ; x < sx + width; x++)
		{
			memcpy(busy_buffer_wrk, fb + y * stride + x * cpp, cpp);
			busy_buffer_wrk += cpp;
			memcpy(fb + y * stride + x * cpp, fb_buffer, cpp);
		}
	}
	m_busy_x = sx;
	m_busy_y = sy;
	m_busy_width = width;
	m_busy_cpp = cpp;
	free(fb_buffer);
	printf("Show Busy}\n");
}

void ePictureViewer::hideBusy()
{
	printf("Hide Busy {\n");
	if (m_busy_buffer != NULL)
	{
		unsigned char * fb = fbClass::getInstance()->lfb;
		unsigned int stride = fbClass::getInstance()->Stride();
		unsigned char* busy_buffer_wrk = m_busy_buffer;

		for (int y = m_busy_y; y < m_busy_y + m_busy_width; y++)
		{
			for (int x = m_busy_x; x < m_busy_x + m_busy_width; x++)
			{
				memcpy(fb + y * stride + x * m_busy_cpp, busy_buffer_wrk, m_busy_cpp);
				busy_buffer_wrk += m_busy_cpp;
			}
		}
		free(m_busy_buffer);
		m_busy_buffer = NULL;
	}
	printf("Hide Busy}\n");
}

void ePictureViewer::Cleanup()
{
	if (m_busy_buffer != NULL)
	{
		free(m_busy_buffer);
		m_busy_buffer = NULL;
	}
	if (m_NextPic_Buffer != NULL)
	{
		free(m_NextPic_Buffer);
		m_NextPic_Buffer = NULL;
	}
	if (m_CurrentPic_Buffer != NULL)
	{
		free(m_CurrentPic_Buffer);
		m_CurrentPic_Buffer = NULL;
	}
}

void ePictureViewer::displayImage(eString filename)
{
	messages.send(Message(Message::display, filename.c_str()));
}

void ePictureViewer::displaySlideshow(eString filename)
{
	messages.send(Message(Message::slideshow, filename.c_str()));
}
