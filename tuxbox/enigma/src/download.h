#ifndef __download_h
#define __download_h

#include "httpd.h"
#include "ewindow.h"
#include "qobject.h"

class eDownloadWindow: public eWindow
{
//	Q_OBJECT
	eHTTPConnection *c;
	std::string url;
	int eventFilter(const eWidgetEvent &event);
private:// slots:
	void httpDone();
public:
	eDownloadWindow(const char *url);
	~eDownloadWindow();
};

#endif
