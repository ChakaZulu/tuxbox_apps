#include "http_dyn.h"

eHTTPDyn::eHTTPDyn(eHTTPConnection *c, QString result): eHTTPDataSource(c), result(result)
{
	wptr=0;
	char buffer[10];
	snprintf(buffer, 10, "%d", size=result.length());
	c->local_header["Content-Length"]=buffer;
	c->code=200;
	c->code_descr="OK";
}

eHTTPDyn::~eHTTPDyn()
{
}

int eHTTPDyn::doWrite(int hm)
{
	int tw=size-wptr;
	if (tw>hm)
		tw=hm;
	if (tw<=0)
		return -1;
	connection->writeBlock(((const char*)result.latin1())+wptr, tw);
	wptr+=tw;
	return tw;
}

eHTTPDynPathResolver::eHTTPDynPathResolver()
{
	dyn.setAutoDelete(true);
}

void eHTTPDynPathResolver::addDyn(QString request, QString path, QString (*function)(QString, QString, QString, eHTTPConnection*))
{
	dyn.append(new eHTTPDynEntry(request, path, function));
}

eHTTPDataSource *eHTTPDynPathResolver::getDataSource(QString request, QString path, eHTTPConnection *conn)
{
	QString p, opt;
	if (path.find('?')!=-1)
	{
		p=path.left(path.find('?'));
		opt=path.mid(path.find('?')+1);
	}	else
	{
		p=path;
		opt="";
	}
	for (QListIterator<eHTTPDynEntry> i(dyn); i.current(); ++i)
		if ((i.current()->path==p) && (i.current()->request==request))
		{
			QString s=i.current()->function(request, path, opt, conn);
			if (s)
				return new eHTTPDyn(conn, s);
			return new eHTTPError(conn, 500);
		}
	return 0;
}
