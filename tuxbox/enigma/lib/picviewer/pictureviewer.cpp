#include <config.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/econfig.h>
#include <lib/gdi/fb.h>
#include <lib/base/estring.h>
#include <lib/gui/actions.h>
#include <lib/gui/guiactions.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/gdi/font.h>
#include <lib/picviewer/pictureviewer.h>
#include "fb_display.h"
#include "format_config.h"

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

ePictureViewer::ePictureViewer(const eString &filename)
	:eWidget(0, 1), slideshowTimer(eApp), filename(filename)
{
	eDebug("[PICTUREVIEWER] Constructor...");

	addActionMap(&i_cursorActions->map);

	move(ePoint(70, 50));
	resize(eSize(590, 470));
	eLabel *l = new eLabel(this);
	l->move(ePoint(150, clientrect.height() / 2));
	l->setFont(eSkin::getActive()->queryFont("epg.title"));
	l->resize(eSize(clientrect.width() - 100, 30));
	l->setText(_("Loading picture... please wait."));

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

	m_startx = 20, m_starty = 20, m_endx = 699, m_endy = 555;
	eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/left", m_startx); // left
	eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/top", m_starty); // top
	eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/right", m_endx); // right
	eConfig::getInstance()->getKey("/enigma/plugins/needoffsets/bottom", m_endy); // bottom

	int showbusysign = 1;
	eConfig::getInstance()->getKey("/picviewer/showbusysign", showbusysign);
	showBusySign = (showbusysign == 1);

#if 0
	unsigned int v_pin8 = 0;
	eConfig::getInstance()->getKey("/elitedvb/video/pin8", v_pin8);
	m_aspect = (v_pin8 < 2) ? 4.0/3 : 16.0/9;
#endif

	m_busy_buffer = NULL;

	init_handlers();

	CONNECT(slideshowTimer.timeout, ePictureViewer::slideshowTimeout);
	eDebug("[PICTUREVIEWER] Constructor done.");
}

ePictureViewer::~ePictureViewer()
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
	CFormathandler *tmp=NULL;
	while(fh_root)
	{
		tmp = fh_root;
		fh_root = fh_root->next;
		free(tmp);
	}
}

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

bool ePictureViewer::DecodeImage(const std::string& name, bool unscaled)
{
	eDebug("DecodeImage {");

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
			eDebug("Error: malloc");
			return false;
		}

		eDebug("---Decoding start(%d/%d)", x, y);
		if (fh->get_pic(name.c_str(), m_NextPic_Buffer, x, y) == FH_ERROR_OK)
		{
			eDebug("---Decoding done");
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
			eDebug("Unable to read file !");
			free(m_NextPic_Buffer);
			m_NextPic_Buffer = (unsigned char *) malloc(3);
			if (m_NextPic_Buffer == NULL)
			{
				eDebug("Error: malloc");
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
		eDebug("Unable to read file or format not recognized!");
		if (m_NextPic_Buffer != NULL)
		{
			free(m_NextPic_Buffer);
		}
		m_NextPic_Buffer = (unsigned char *) malloc(3);
		if (m_NextPic_Buffer == NULL)
		{
			eDebug("Error: malloc");
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
	eDebug("DecodeImage }");
	return(m_NextPic_Buffer != NULL);
}

bool ePictureViewer::ShowImage(const std::string & filename, bool unscaled)
{
	eDebug("Show Image {");
	int pos = filename.find_last_of("/");
	if (pos == -1)
		pos = filename.length() - 1;
	eString directory = pos ? filename.substr(0, pos) : "/";
	eDebug("---directory: %s", directory.c_str());
	slideshowList.clear();
	listDirectory(directory, 0);
	for (myIt = slideshowList.begin(); myIt != slideshowList.end(); myIt++)
	{
		eString tmp = *myIt;
		eDebug("[PICTUREVIEWER] comparing: %s:%s", tmp.c_str(), filename.c_str());
		if (tmp == filename)
			break;
	}
	DecodeImage(filename, unscaled);
	struct fb_var_screeninfo *screenInfo = fbClass::getInstance()->getScreenInfo();
	if (screenInfo->bits_per_pixel != 16)
	{
		fbClass::getInstance()->lock();
		fbClass::getInstance()->SetMode(720, 576, 16);
#if HAVE_DVB_API_VERSION == 3
		fbClass::getInstance()->setTransparency(0);
#endif
	}
	DisplayNextImage();
	eDebug("Show Image }");
	return true;
}

void ePictureViewer::slideshowTimeout()
{
	int wrap = 1;
	bool setTimer = true;
	eString tmp = *myIt;
	eDebug("[PICTUREVIEWER] slideshowTimeout: show %s", tmp.c_str());
	ShowImage(tmp, false);
	if (++myIt == slideshowList.end())
	{
		eConfig::getInstance()->getKey("/picviewer/wraparound", wrap);
		if (wrap == 1)
			myIt = slideshowList.begin();
		else
			setTimer = false;
	}
	if (setTimer)
	{
		int timeout = 5;
		eConfig::getInstance()->getKey("/picviewer/slideshowtimeout", timeout);
		slideshowTimer.start(timeout * 1000, true);
	}
}

int ePictureViewer::eventHandler(const eWidgetEvent &evt)
{
	fflush(stdout);
	switch(evt.type)
	{
		case eWidgetEvent::evtAction:
			if (/* evt.action == &i_cursorActions->ok || */
				evt.action == &i_cursorActions->cancel ||
				evt.action == &i_cursorActions->left ||
				evt.action == &i_cursorActions->right )
				close(0);
			else
			if (evt.action == &i_cursorActions->up)
			{
				if (myIt++ == slideshowList.end())
					myIt = slideshowList.begin();
				DecodeImage(*myIt, false);
				DisplayNextImage();
			}
			else
			if (evt.action == &i_cursorActions->down)
			{
				if (myIt == slideshowList.begin())
					myIt = slideshowList.end()--;
				else
					myIt--;
				DecodeImage(*myIt, false);
				DisplayNextImage();
			}
			break;
		case eWidgetEvent::execBegin:
		{
			int mode = 0;
			eConfig::getInstance()->getKey("/picviewer/lastPicViewerStyle", mode);
			if (mode)
				ShowSlideshow(filename, false);
			else
				ShowImage(filename, false);
			break;
		}
		case eWidgetEvent::execDone:
			fbClass::getInstance()->SetMode(720, 576, 8);
			fbClass::getInstance()->PutCMAP();
			fbClass::getInstance()->unlock();
			break;
		default:
			return eWidget::eventHandler(evt);
	}
	return 1;
}

void ePictureViewer::listDirectory(eString directory, int includesubdirs)
{
	eDebug("[PICTUREVIEWER] listDirectory: dir %s", directory.c_str());
	DIR *d = opendir(directory.c_str());
	if (d)
	{
		while (struct dirent *e = readdir(d))
		{
			eString filename = eString(e->d_name);
			eDebug("[PICTUREVIEWER] listDirectory: processing %s", filename.c_str());
			if ((filename != ".") && (filename != ".."))
			{
				struct stat s;
				eString fullFilename = directory + "/" + filename;
				if (lstat(fullFilename.c_str(), &s) < 0)
				{
					eDebug("--- file no good :(");
					continue;
				}

				if (S_ISREG(s.st_mode))
				{
					if (filename.right(4).upper() == ".JPG" ||
					    filename.right(5).upper() == ".JPEG" ||
					    filename.right(4).upper() == ".CRW" ||
					    filename.right(4).upper() == ".PNG" ||
					    filename.right(4).upper() == ".BMP" ||
					    filename.right(4).upper() == ".GIF")
					{
						eDebug("[PICTUREVIEWER] ShowSlideshow: adding %s", filename.c_str());
						eString tmp = directory + "/" + filename;
						slideshowList.push_back(tmp);
					}
				}
				else
				if ((includesubdirs == 1) && (S_ISDIR(s.st_mode) || S_ISLNK(s.st_mode)))
				{
					listDirectory(directory + "/" + filename, includesubdirs);
				}
			}
		}
		closedir(d);
	}
}

bool ePictureViewer::ShowSlideshow(const std::string& filename, bool unscaled)
{
	eDebug("Show Slideshow { %s", filename.c_str());
	int includesubdirs = 1;
	eConfig::getInstance()->getKey("/picviewer/includesubdirs", includesubdirs);

	// gen pic list for slideshow
	int pos = filename.find_last_of("/");
	if (pos == -1)
		pos = filename.length() - 1;
	eString directory = pos ? filename.substr(0, pos) : "/";
	eDebug("---directory: %s", directory.c_str());
	slideshowList.clear();
	listDirectory(directory, includesubdirs);
	if (!slideshowList.empty())
	{
		int sortpictures = 1;
		eConfig::getInstance()->getKey("/picviewer/sortpictures", sortpictures);
		if (sortpictures == 1)
			slideshowList.sort();

		int startwithselectedpic = 1;
		eConfig::getInstance()->getKey("/picviewer/startwithselectedpic", startwithselectedpic);
		if (startwithselectedpic == 1)
		{
			for (myIt = slideshowList.begin(); myIt != slideshowList.end(); myIt++)
			{
				eString tmp = *myIt;
				eDebug("[PICTUREVIEWER] comparing: %s:%s", tmp.c_str(), filename.c_str());
				if (tmp == filename)
					break;
			}
		}
		else
			myIt = slideshowList.begin();

		slideshowTimer.stop();
		slideshowTimeout();
	}
	eDebug("Show Slideshow }");
	return true;
}

bool ePictureViewer::DisplayNextImage()
{
	eDebug("DisplayNextImage {");
	if (m_NextPic_Buffer != NULL)
		fb_display(m_NextPic_Buffer, m_NextPic_X, m_NextPic_Y, m_NextPic_XPan, m_NextPic_YPan, m_NextPic_XPos, m_NextPic_YPos);
	eDebug("DisplayNextImage }");
	return true;
}

void ePictureViewer::Zoom(float factor)
{
	eDebug("Zoom %f {",factor);
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
	eDebug("Zoom }");
}

void ePictureViewer::Move(int dx, int dy)
{
	eDebug("Move %d %d {", dx, dy);
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
	eDebug("Move }");
}

void ePictureViewer::showBusy(int sx, int sy, int width, char r, char g, char b)
{
	eDebug("Show Busy{");

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
		eDebug("Error: malloc");
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
		eDebug("Error: malloc");
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
	eDebug("Show Busy}");
}

void ePictureViewer::hideBusy()
{
	eDebug("Hide Busy {");
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
	eDebug("Hide Busy}");
}
