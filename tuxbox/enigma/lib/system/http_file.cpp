#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "http_file.h"

eHTTPFile::eHTTPFile(eHTTPConnection *c, int _fd, const char *mime): eHTTPDataSource(c)
{
	fd=_fd;
	c->local_header["Content-Type"]=mime;
	size=lseek(fd, 0, SEEK_END);
	char asize[10];
	snprintf(asize, 10, "%d", size);
	lseek(fd, 0, SEEK_SET);
	c->local_header["Content-Length"]=asize;
	connection->code_descr="OK";
	connection->code=200;
}

int eHTTPFile::doWrite(int bytes)
{
	qDebug("doWrite(%d)", bytes);
	char buff[bytes];
	if (!size)
		return -1;
	int len=bytes;
	if (len>size)
		len=size;
	len=read(fd, buff, len);
	if (!len)
		return -1;
	size-=connection->writeBlock(buff, len);
	return len;
}

eHTTPFile::~eHTTPFile()
{
	close(fd);
}

eHTTPFilePathResolver::eHTTPFilePathResolver()
{
	translate.setAutoDelete(true);
}

eHTTPDataSource *eHTTPFilePathResolver::getDataSource(QString request, QString path, eHTTPConnection *conn)
{
	if (path.find("../")!=-1)		// evil hax0r
		return new eHTTPError(conn, 403);
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
			qDebug("translated %s to %s", (const char*)path, (const char*)newpath);

			int fd=open(newpath, O_RDONLY);
			if (fd==-1)
			{
				switch (errno)
				{
				case ENOENT:
					data=new eHTTPError(conn, 404);
					break;
				case EACCES:
					data=new eHTTPError(conn, 403);
					break;
				default:
					data=new eHTTPError(conn, 401); // k.a.
					break;
				}
				break;
			}
			
			QString ext=path.mid(path.findRev('.'));
			const char *mime="text/unknown";
			if ((ext==".html") || (ext==".htm"))
				mime="text/html";
			else if ((ext==".jpeg") || (ext==".jpg"))
				mime="image/jpeg";
			else if (ext==".gif")
				mime="image/gif";
			else if (ext==".css")
				mime="text/css";

			data=new eHTTPFile(conn, fd, mime);
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
