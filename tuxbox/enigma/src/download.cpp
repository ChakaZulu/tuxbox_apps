#include <core/system/download.h>
#include <core/gui/eskin.h>

int eDownloadWindow::eventFilter(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::execBegin:
	{
		int err;
		c=eHTTPConnection::doRequest(url.c_str(), &err);
		if (!c)
		{
			qDebug("doRequest failed");
			close(err);
		} else
		{
			qDebug("ok, let's start");
//			connect(c, SIGNAL(closing()), SLOT(httpDone()));
			CONNECT(c.closing, eDownloadWindow::httpDone);
			c->start();
		}
		break;
	}
	case eWidgetEvent::execDone:
	{
		if (c)
			c->die();
		c=0;
		break;
	}
	}
	return 0;
}

void eDownloadWindow::httpDone()
{
	if (!c)
		return;
	qDebug("http done!, state %d %d", c->localstate, c->remotestate);
	int s=c->localstate==eHTTPConnection::stateDone;
	c=0;

/*	if (s)
		close(0);
	else
		close(1); */
}

eDownloadWindow::eDownloadWindow(const char *url): eWindow(), url(url)
{
	c=0;
	if (eSkin::getActive()->build(this, "eDownloadWindow"))
		qFatal("skin load of \"eDownloadWindow\" failed");
}

eDownloadWindow::~eDownloadWindow()
{
	if (c)
		c->die();
	c=0;
}
