#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "eskin.h"
#include "ewidget.h"

#include "gfbdc.h"
#include "glcddc.h"
#include "epng.h"
#include <config.h>
#include <eerror.h>


QMap<eString,tWidgetCreator> eSkin::widget_creator;

eSkin *eSkin::active;

eNamedColor *eSkin::searchColor(const char *name) const
{
	for (QListIterator<eNamedColor> i(colors); i.current(); ++i)
		if (!strcmp(i.current()->name, name))
			return i.current();
	return 0;
}

void eSkin::clear()
{
	if (palette)
		delete[] palette;
	if (parser)
		delete parser;
	parser=0;
}

void eSkin::addWidgetCreator(const eString &name, tWidgetCreator creator)
{
	widget_creator.insert(name, creator);
}

void eSkin::removeWidgetCreator(const eString &name, tWidgetCreator creator)
{
	widget_creator.remove(name);
}

int eSkin::parseColor(const char *name, const char *color, gRGB &col)
{
	if (color[0]=='#')
	{
		unsigned long vcol=0;
		if (sscanf(color+1, "%lx", &vcol)!=1)
		{
			eDebug("invalid color named \"%s\" (value: %s)", name, color+1);
			return -1;
		}
		col.r=(vcol>>16)&0xFF;
		col.g=(vcol>>8)&0xFF;
		col.b=vcol&0xFF;
		col.a=(vcol>>24)&0xFF;
	} else
	{
		eNamedColor *n=searchColor(color);
		if (!n)
		{
			eDebug("invalid color named \"%s\" (alias to: \"%s\")", name, color);
			return -1;
		}
		col=n->value;
	}
	return 0;
}

int eSkin::parseColors(XMLTreeNode *xcolors)
{
	XMLTreeNode *node;
	
	if (palette)
		delete[] palette;
	
	int numnamed=0;
	
	for (node=xcolors->GetChild(); node; node=node->GetNext())
	{
		if (strcmp(node->GetType(), "color"))
		{
			eDebug("junk found in colorsection (%s)", node->GetType());
			continue;
		}
		
		const char *name=node->GetAttributeValue("name"), *color=node->GetAttributeValue("color"), *end=node->GetAttributeValue("end");

		if (!color || !name)
		{
			eDebug("no color/name specified");
			continue;
		}

		eNamedColor *col=new eNamedColor;
		col->name=name;

		const char *size=node->GetAttributeValue("size");

		if (size)
			col->size=atoi(size);
		else
			col->size=0;
		
		if (!col->size)
			col->size=1;
		
		if ((col->size>1) && (!end))
		{
			eDebug("no end specified in \"%s\" but is gradient", name);
			delete col;
			continue;
		}

		if (parseColor(name, color, col->value))
		{
			delete col;
			continue;
		}

		if (end && parseColor(name, end, col->end))
		{
			delete col;
			continue;
		}

		colors.append(col);
		numnamed++;
	}
	
	const int maxcolors=numcolors;
	
	int colorused[maxcolors];
	memset(colorused, 0, maxcolors*sizeof(int));
	palette=new gRGB[maxcolors];
	memset(palette, 0, sizeof(gRGB)*maxcolors);
	
	for (QListIterator<eNamedColor> i(colors); i.current(); ++i)
	{
		eNamedColor *col=i.current();
		int d;
		for (d=0; d<maxcolors; d+=col->size)
		{
			int s;
			for (s=0; s<col->size; s++)
				if ((d+s>maxcolors) || colorused[d+s])
					break;
			if (s==col->size)
				break;
		}
		if (d==maxcolors)
			continue;
		col->index=gColor(d);
		for (int s=0; s<col->size; s++, d++)
		{
			colorused[d]=1;
			if (s)
			{
				int rdiff=-col->value.r+col->end.r;
				int gdiff=-col->value.g+col->end.g;
				int bdiff=-col->value.b+col->end.b;
				int adiff=-col->value.a+col->end.a;
				rdiff*=s; rdiff/=(col->size-1);
				gdiff*=s; gdiff/=(col->size-1);
				bdiff*=s; bdiff/=(col->size-1);
				adiff*=s; adiff/=(col->size-1);
				palette[d].r=col->value.r+rdiff;
				palette[d].g=col->value.g+gdiff;
				palette[d].b=col->value.b+bdiff;
				palette[d].a=col->value.a+adiff;
			} else
				palette[d]=col->value;
		}
	}
	if (paldummy)
		delete paldummy;
	paldummy=new gImage(eSize(1, 1), 8);
	paldummy->clut=palette;
	paldummy->colors=maxcolors;
	return 0;
}

int eSkin::parseScheme(XMLTreeNode *xscheme)
{
	XMLTreeNode *node;
	for (node=xscheme->GetChild(); node; node=node->GetNext())
	{
		if (strcmp(node->GetType(), "map"))
		{
			eDebug("illegal scheme entry found: %s", node->GetType());
			return -1;
		}
		char *name=node->GetAttributeValue("name"), *color=node->GetAttributeValue("color");
		if (!name || !color)
		{
			eDebug("no name or color specified in colorscheme");
			return -1;
		}
		eString base=color;
		int offset=0, p;
		if ((p=base.find('+'))!=-1)
		{
			offset=atoi(base.mid(p));
			base=base.left(p);
		}
		eNamedColor *n=searchColor(base);
		if (!n)
		{
			eDebug("illegal color \"%s\" specified", (const char*) base);
			return -1;
		}
		scheme.insert(name, new gColor(n->index+offset));
	}
	return 0;
}

int eSkin::parseImages(XMLTreeNode *inode)
{
	eString basepath= eString(DATADIR) + eString("/enigma/pictures/") + inode->GetAttributeValue("basepath");
	if (!basepath)
		basepath="";
	if (basepath[basepath.length()-1]!='/')
		basepath+="/";

	for (XMLTreeNode *node=inode->GetChild(); node; node=node->GetNext())
	{
		if (strcmp(node->GetType(), "img"))
		{
			eDebug("illegal image entry found: %s", node->GetType());
			continue;
		}
		const char *name=node->GetAttributeValue("name");
		if (!name)
		{
			eDebug("illegal <img> entry: no name");
			continue;
		}
		const char *src=node->GetAttributeValue("src");
		if (!src)
		{
			eDebug("image/img=\"%s\" no src given", name);
			continue;
		}
		eString filename=basepath + eString(src);
		gPixmap *image=loadPNG(filename);
		if (!image)
		{
			eDebug("image/img=\"%s\" - %s: file not found", name, (const char*)filename);
			continue;
		}
		if (paldummy && !node->GetAttributeValue("nomerge"))
		{
			gPixmapDC mydc(image);
			gPainter p(mydc);
			p.mergePalette(*paldummy);
		}
		images.insert(name, image);
	}
	return 0;
}

int eSkin::parseValues(XMLTreeNode *xvalues)
{
	for (XMLTreeNode *node=xvalues->GetChild(); node; node=node->GetNext())
	{
		if (strcmp(node->GetType(), "value"))
		{
			eDebug("illegal values entry %s", node->GetType());
			continue;
		}
		const char *name=node->GetAttributeValue("name");
		if (!name)
		{
			eDebug("values entry has no name");
			continue;
		}
		const char *value=node->GetAttributeValue("value");
		if (!value)
		{
			eDebug("values entry has no value");
			continue;
		}
		values.insert(name, new int(atoi(value)));
	}
}

gDC *eSkin::getDCbyName(const char *name)
{
	gPixmapDC *dc=0;
	if (!strcmp(name, "fb"))
		dc=gFBDC::getInstance();
	else if (!strcmp(name, "lcd"))
		dc=gLCDDC::getInstance();
	return dc;
}

int eSkin::build(eWidget *widget, XMLTreeNode *node)
{
	// eDebug("building a %s", node->GetType());
	// if (widget->getType() != node->GetType())
	//		return -1;
	
	for (XMLAttribute *attrib=node->GetAttributes(); attrib; attrib=attrib->GetNext())
	{
//		eDebug("setting %s := %s", attrib->GetName(), attrib->GetValue());
		if (widget->setProperty(attrib->GetName(), attrib->GetValue()))
		{
			eDebug("failed");
			return -1;
		}
	}
	for (XMLTreeNode *c=node->GetChild(); c; c=c->GetNext())
	{
		eWidget *w=0;
		const char *name=c->GetAttributeValue("name");
		if (name)
			w=widget->search(name);
		if (!w)
		{
			if (!widget_creator.contains(c->GetType()))
			{
				eDebug("widget class %s does not exist", c->GetType());
				return -ENOENT;
			}
			w=widget_creator[c->GetType()](widget);
		}
		if (!w)
		{
			// eDebug("failed.");
			return -EINVAL;
		}
		int err;
		if ((err=build(w, c)))
		{
			return err;
		}
	}
	return 0;
}

eSkin::eSkin()
{
	parser=0;
	palette=0;
	numcolors=256;
	paldummy=0;
	colors.setAutoDelete(true);
	scheme.setAutoDelete(true);
	images.setAutoDelete(true);
}

eSkin::~eSkin()
{
	if (active==this)
		active=0;
	clear();
	if (paldummy)
		delete paldummy;
}

int eSkin::load(const char *filename)
{
	FILE *in=fopen(filename, "rt");
	if (!in)
		return -1;

	parser=new XMLTreeParser("ISO-8859-1");
	char buf[2048];
	
	int done;
	do
	{
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser->Parse(buf, len, done))
		{
			eDebug("parse error: %s at line %d",
				parser->ErrorString(parser->GetErrorCode()),
				parser->GetCurrentLineNumber());
			delete parser;
			parser=0;
			fclose(in);
			return -1;
		}
	} while (!done);
	fclose(in);

	XMLTreeNode *root=parser->RootNode();
	if (!root)
		return -1;
	if (strcmp(root->GetType(), "eskin"))
	{
		eDebug("not an eskin");
		return -1;
	}
	
	XMLTreeNode *node=parser->RootNode();
	
	for (node=node->GetChild(); node; node=node->GetNext())
		if (!strcmp(node->GetType(), "colors"))
			parseColors(node);
		else if (!strcmp(node->GetType(), "colorscheme"))
			parseScheme(node);
		else if (!strcmp(node->GetType(), "images"))
			parseImages(node);
		else if (!strcmp(node->GetType(), "values"))
			parseValues(node);

	return 0;
}

int eSkin::build(eWidget *widget, const char *name)
{
	XMLTreeNode *node=parser->RootNode();
	node=node->GetChild();
	while (node)
	{
		if (!strcmp(node->GetType(), "object"))
		{
			const char *oname=node->GetAttributeValue("name");
			if (!qstrcmp(name, oname))
			{
				node=node->GetChild();
				return build(widget, node);
			}
		}
		node=node->GetNext();
	}
	eDebug("didn't found it");
	return -ENOENT;
}

void eSkin::setPalette(gPixmapDC *pal)
{
	if (palette)
	{
		gPainter p(*pal);
		p.setPalette(palette, 0, 256);
	}
}

gColor eSkin::queryColor(const eString& name) const
{
	char *end;
	int numcol=strtol(name, &end, 10);
	if (!*end)
		return gColor(numcol);
	eNamedColor *col=searchColor(name);
	if (!col)
	{
		eDebug("requested color %s does not exist", (const char*)name);
		return gColor(0);
	} else
		return col->index;
}

eSkin *eSkin::getActive()
{
	if (!active)
		qFatal("no active skin");
	return active;
}

void eSkin::makeActive()
{
	active=this;
}

gColor eSkin::queryScheme(const eString& name) const
{
	eString base=name;
	int offset=0, p;
	if ((p=base.find('+'))!=-1)
	{
		base=name.left(p);
		offset=atoi(name.mid(p));
	}
	gColor *n=scheme.find((const char*)name);
	if (!n)
	{
		eDebug("%s does not exist", (const char*)name);
		return gColor(0);
	} else
		return *n;
}

gPixmap *eSkin::queryImage(const eString& name) const
{
	return images.find((const char*)name);
}

int eSkin::queryValue(const eString& name, int d) const
{
	int *v=values.find((const char*)name);
	if (v)
		return *v;
	else
		return d;
}
