#ifndef __pictureviewer_h
#define __pictureviewer_h

#include <lib/base/estring.h>
#include <lib/gui/ewidget.h>

#define FH_ERROR_OK 0
#define FH_ERROR_FILE 1		/* read/access error */
#define FH_ERROR_FORMAT 2	/* file format error */
#define FH_ERROR_MALLOC 3	/* error during malloc */

#define dbout(fmt, args...) {struct timeval tv; gettimeofday(&tv, NULL); \
        printf( "PV[%ld|%02ld] " fmt, (long)tv.tv_sec, (long)tv.tv_usec / 10000, ## args);}

class ePictureViewer: public eWidget
{
	eTimer slideshowTimer;
	std::list<eString> slideshowList;
	std::list<eString>::iterator myIt;
	struct cformathandler 
	{
		struct cformathandler *next;
		int (*get_size)(const char *, int *, int *, int, int);
		int (*get_pic)(const char *, unsigned char *, int, int);
		int (*id_pic)(const char *);
	};
	typedef  struct cformathandler CFormathandler;
	eString filename;
	int eventHandler(const eWidgetEvent &evt);
	void listDirectory(eString, int);
	bool showBusySign;
	bool slideshowPaused;
	bool blockOK;
public:
	ePictureViewer( const eString &filename);
	~ePictureViewer();

	enum ScalingMode
	{
		NONE = 0,
		SIMPLE = 1,
		COLOR = 2
	};

	bool ShowImage(const std::string& filename, bool unscaled = false);
	bool ShowSlideshow(const std::string& filename, bool unscaled = false);
	bool DecodeImage(const std::string& name, bool unscaled = false);
	bool DisplayNextImage();
	void SetScaling(ScalingMode s) {m_scaling = s;}
	void SetAspectRatio(float aspect_ratio) {m_aspect = aspect_ratio;}
	void showBusy(int sx, int sy, int width, char r, char g, char b);
	void hideBusy();
	void Zoom(float factor);
	void Move(int dx, int dy);
	void slideshowTimeout();
private:
	CFormathandler *fh_root;
	ScalingMode m_scaling;
	float m_aspect;
	std::string m_NextPic_Name;
	unsigned char *m_NextPic_Buffer;
	int m_NextPic_X;
	int m_NextPic_Y;
	int m_NextPic_XPos;
	int m_NextPic_YPos;
	int m_NextPic_XPan;
	int m_NextPic_YPan;

	std::string m_CurrentPic_Name;
	unsigned char *m_CurrentPic_Buffer;
	int m_CurrentPic_X;
	int m_CurrentPic_Y;
	int m_CurrentPic_XPos;
	int m_CurrentPic_YPos;
	int m_CurrentPic_XPan;
	int m_CurrentPic_YPan;

	unsigned char *m_busy_buffer;
	int m_busy_x;
	int m_busy_y;
	int m_busy_width;
	int m_busy_cpp;

	int m_startx;
	int m_starty;
	int m_endx;
	int m_endy;
	
	CFormathandler * fh_getsize(const char *name, int *x, int *y, int width_wanted, int height_wanted);
	void init_handlers(void);
	void add_format(int (*picsize)(const char *, int *, int *, int, int), int (*picread)(const char *, unsigned char *, int , int), int (*id)(const char *));
};
#endif
