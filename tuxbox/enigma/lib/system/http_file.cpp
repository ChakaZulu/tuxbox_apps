#include <lib/system/http_file.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string>

eHTTPFile::eHTTPFile(eHTTPConnection *c, int _fd, const char *mime): eHTTPDataSource(c)
{
	fd=_fd;
	c->local_header["Content-Type"]=std::string(mime);
	size=lseek(fd, 0, SEEK_END);
	char asize[10];
	snprintf(asize, 10, "%d", size);
	lseek(fd, 0, SEEK_SET);
	c->local_header["Content-Length"]=std::string(asize);
	connection->code_descr="OK";
	connection->code=200;
}

int eHTTPFile::doWrite(int bytes)
{
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
	if (!size)
		return -1;
	else
		return 1;
}

eHTTPFile::~eHTTPFile()
{
	close(fd);
}

eHTTPFilePathResolver::eHTTPFilePathResolver()
{
	translate.setAutoDelete(true);
}

eHTTPDataSource *eHTTPFilePathResolver::getDataSource(eString request, eString path, eHTTPConnection *conn)
{
	eDebug("request = %s, path = %s", request.c_str(), path.c_str());
	if (path.find("../")!=eString::npos)		// evil hax0r
		return new eHTTPError(conn, 403);
	eDebug("%s", path.c_str());
	if (path[0] != '/')		// prepend '/'
		path.insert(0,"/");
	eDebug("%s", path.c_str());
	if (path[path.length()-1]=='/')
		path+="index.html";
	eDebug("path: %s", path.c_str());
	eHTTPDataSource *data=0;
	for (ePtrList<eHTTPFilePath>::iterator i(translate); i != translate.end(); ++i)
	{
		if (i->root==path.left(i->root.length()))
		{
			eString newpath=i->path+path.mid(i->root.length());
			if (newpath.find('?'))
				newpath=newpath.left(newpath.find('?'));
			eDebug("translated %s to %s", path.c_str(), newpath.c_str());

			int fd=open(newpath.c_str(), O_RDONLY);

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
			
			eString ext=path.mid(path.rfind('.'));
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

void eHTTPFilePathResolver::addTranslation(eString path, eString root)
{
	if (path[path.length()-1]!='/')
		path+='/';
	if (root[root.length()-1]!='/')
		root+='/';
	translate.push_back(new eHTTPFilePath(path, root));
}
