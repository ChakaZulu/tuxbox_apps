#include "http_file.h"

eHTTPFile::eHTTPFile(QString path)
{
	file.setName(path);
	QString ext=path.mid(path.findRev('.'));
	mime="text/unknown";
	if ((ext==".html") || (ext==".htm"))
		mime="text/html";
	else if ((ext==".jpeg") || (ext==".jpg"))
		mime="image/jpeg";
	else if (ext==".gif")
		mime="image/gif";
	else if (ext==".css")
		mime="text/css";
	if (file.open(IO_ReadOnly))
		err=200;
	else
		err=404;
	wroteheader=0;
}

int eHTTPFile::getCode()
{
	return err;
}

int eHTTPFile::writeData(eHTTPConnection *conn)
{
	if (err!=200)
		qFatal("ich sagte doch, dass das nicht geht.");
	if (!wroteheader)
	{
		if (!conn->is09)
		{
			QString header="Content-Type: "+mime+"\r\n";
			header+="Content-Length: "+QString().setNum(file.size())+"\r\n";
			header+="\r\n";
			conn->writeBlock((const char*)header.latin1(), header.length());
		}
		wroteheader=1;
	}
	char buff[8192];
	int len=file.readBlock(buff, 8192);
	if (!len)
		return -1;
	conn->writeBlock(buff, len);
	return len;
}

int eHTTPFile::haveData(eHTTPConnection *conn)
{
	return 0;
}

eHTTPFile::~eHTTPFile()
{
	file.close();
}

eHTTPFilePathResolver::eHTTPFilePathResolver()
{
	translate.setAutoDelete(true);
}

eHTTPDataSource *eHTTPFilePathResolver::getDataSource(QString request, QString path, const eHTTPConnection *conn)
{
	if (path.find("../")!=-1)		// evil hax0r
		return new eHTTPError(403);
	if (path[0]!='/')		// prepend '/'
		path.prepend('/');
	if (path[path.length()-1]=='/')
		path+="index.html";
	eHTTPDataSource *data=0;
	for (QListIterator<eHTTPFilePath> i(translate); i.current(); ++i)
	{
		if (i.current()->root==path.left(i.current()->root.length()))
		{
			QString newpath=i.current()->path+path.mid(i.current()->root.length());
			if (newpath.find('?'))
				newpath=newpath.left(newpath.find('?'));
/*			qDebug("translated %s to %s", (const char*)path, (const char*)newpath); */
			data=new eHTTPFile(newpath);
			if ((data->getCode()/100)!=2)
			{
				int code=data->getCode();
				delete data;
				data=new eHTTPError(code);
			}
			break;
		}
	}
	return data;
}

void eHTTPFilePathResolver::addTranslation(QString path, QString root)
{
	if (path[path.length()-1]!='/')
		path+='/';
	if (root[root.length()-1]!='/')
		root+='/';
	translate.append(new eHTTPFilePath(path, root));
}
