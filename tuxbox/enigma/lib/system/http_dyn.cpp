#include "http_dyn.h"

eHTTPDyn::eHTTPDyn(QString result): result(result)
{
	errcode=200;
	if (!result)
		errcode=500;
}

eHTTPDyn::~eHTTPDyn()
{
}

int eHTTPDyn::getCode()
{
	return errcode;
}

int eHTTPDyn::writeData(eHTTPConnection *conn)
{
	conn->writeBlock(result, result.length());
	return -1;
}

int eHTTPDyn::haveData(eHTTPConnection *conn)
{
	return 0;
}


eHTTPDynPathResolver::eHTTPDynPathResolver()
{
	dyn.setAutoDelete(true);
}

void eHTTPDynPathResolver::addDyn(QString request, QString path, QString (*function)(QString, QString, QString, const eHTTPConnection*))
{
	dyn.append(new eHTTPDynEntry(request, path, function));
}

eHTTPDataSource *eHTTPDynPathResolver::getDataSource(QString request, QString path, const eHTTPConnection *conn)
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
			eHTTPDyn *r=new eHTTPDyn(i.current()->function(request, path, opt, conn));
			if ((r->getCode()/100)!=2)
			{
				delete r;
				return new eHTTPError(500);
			}
			return r;
		}
	return 0;
}
