#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <config.h>

#include <core/gui/eskin.h>
#include <core/gui/ewidget.h>
#include <core/gdi/gfbdc.h>
#include <core/gdi/glcddc.h>
#include <core/gdi/epng.h>
#include <core/base/eerror.h>
#include <core/gdi/font.h>
#include <core/base/eptrlist.h>

std::map< eString,tWidgetCreator > eSkin::widget_creator;

eSkin *eSkin::active;

eNamedColor *eSkin::searchColor(const eString &name)
{
	for (std::list<eNamedColor>::iterator i(colors.begin()); i != colors.end(); ++i)
	{
		if (!i->name.compare(name))
			return &*i;
	}
	return 0;
}

void eSkin::clear()
{
}

void eSkin::addWidgetCreator(const eString &name, tWidgetCreator creator)
{
	widget_creator[name] = creator; // add this tWidgetCreator to map... if exist.. overwrite
}

void eSkin::removeWidgetCreator(const eString &name, tWidgetCreator creator)
{
	widget_creator.erase(name);
}

int eSkin::parseColor(const eString &name, const char* color, gRGB &col)
{
	if (color[0]=='#')
	{
		unsigned long vcol=0;
		if (sscanf(color+1, "%lx", &vcol)!=1)
		{
			eDebug("invalid color named \"%s\" (value: %s)", name.c_str(), color+1);
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
			eDebug("invalid color named \"%s\" (alias to: \"%s\")", name.c_str(), color);
			return -1;
		}
		col=n->value;
	}
	return 0;
}

int eSkin::parseColors(XMLTreeNode *xcolors)
{
	XMLTreeNode *node;
	
	std::list<eNamedColor>::iterator newcolors=colors.end();
	
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

		eNamedColor col;
		col.name=name;

		const char *size=node->GetAttributeValue("size");

		if (size)
			col.size=atoi(size);
		else
			col.size=0;
		
		if (!col.size)
			col.size=1;
		
		if ((col.size>1) && (!end))
		{
			eDebug("no end specified in \"%s\" but is gradient", name);
			continue;
		}

		if (parseColor(name, color, col.value))
			continue;

		if (end && parseColor(name, end, col.end))
			continue;

		colors.push_back(col);
		if (newcolors == colors.end())
			--newcolors;
	}
	
	for (std::list<eNamedColor>::iterator i(newcolors); i != colors.end(); ++i)
	{
		eNamedColor &col=*i;
		int d;
		for (d=0; d<maxcolors; d+=col.size)
		{
			int s;
			for (s=0; s<col.size; s++)
				if ((d+s>maxcolors) || colorused[d+s])
					break;
			if (s==col.size)
				break;
		}
		if (d==maxcolors)
			continue;
		col.index=gColor(d);
		for (int s=0; s<col.size; s++, d++)
		{
			colorused[d]=1;
			if (s)
			{
				int rdiff=-col.value.r+col.end.r;
				int gdiff=-col.value.g+col.end.g;
				int bdiff=-col.value.b+col.end.b;
				int adiff=-col.value.a+col.end.a;
				rdiff*=s; rdiff/=(col.size-1);
				gdiff*=s; gdiff/=(col.size-1);
				bdiff*=s; bdiff/=(col.size-1);
				adiff*=s; adiff/=(col.size-1);
				palette[d].r=col.value.r+rdiff;
				palette[d].g=col.value.g+gdiff;
				palette[d].b=col.value.b+bdiff;
				palette[d].a=col.value.a+adiff;
			} else
				palette[d]=col.value;
		}
	}
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
			offset=atoi(base.mid(p).c_str());
			base=base.left(p);
		}
		eNamedColor *n=searchColor(base);
		if (!n)
		{
			eDebug("illegal color \"%s\" specified", base.c_str());
			return -1;
		}
		scheme[name] = gColor(n->index+offset);
	}
	return 0;
}

int eSkin::parseImages(XMLTreeNode *inode)
{
	char *abasepath=inode->GetAttributeValue("basepath");
	if (!abasepath)
		abasepath="";
	eString basepath=eString(DATADIR) + eString("/enigma/pictures/") + abasepath;
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
		gPixmap *image=loadPNG(filename.c_str());
		if (!image)
		{
			eDebug("image/img=\"%s\" - %s: file not found", name, filename.c_str());
			continue;
		}
		if (paldummy && !node->GetAttributeValue("nomerge"))
		{
			gPixmapDC mydc(image);
			gPainter p(mydc);
			p.mergePalette(*paldummy);
		}
 		images[name]=image;
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
		values[name]=atoi(value);
	}
}

int eSkin::parseFonts(XMLTreeNode *xfonts)
{
	const char *abasepath=xfonts->GetAttributeValue("basepath");
	eString basepath=abasepath?abasepath:FONTDIR;

	if (basepath.length())
		if (basepath[basepath.length()-1]!='/')
			basepath+="/";

	for (XMLTreeNode *node=xfonts->GetChild(); node; node=node->GetNext())
	{
		if (strcmp(node->GetType(), "font"))
		{
			eDebug("illegal fonts entry %s", node->GetType());
			continue;
		}
		const char *file=node->GetAttributeValue("file");
		if (!file)
		{
			eDebug("fonts entry has no file");
			continue;
		}
		fontRenderClass::getInstance()->AddFont((basepath+eString(file)).c_str());
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
			std::map< eString, tWidgetCreator >::iterator it = widget_creator.find(c->GetType());

			if ( it == widget_creator.end() )
			{
				eDebug("widget class %s does not exist", c->GetType());
				return -ENOENT;
			}
			w = (it->second)(widget);
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
	maxcolors=256;

	palette=new gRGB[maxcolors];
	
	memset(palette, 0, sizeof(gRGB)*maxcolors);
	paldummy=new gImage(eSize(1, 1), 8);
	paldummy->clut=palette;
	paldummy->colors=maxcolors;

	colorused=new int[maxcolors];
	memset(colorused, 0, maxcolors*sizeof(int));

}

eSkin::~eSkin()
{
	if (active==this)
		active=0;

	clear();

	delete colorused;

	for (std::map<eString, gPixmap*>::iterator it(images.begin()); it != images.end(); it++)
		delete it->second;	

	if (paldummy)
		delete paldummy;
}

int eSkin::load(const char *filename)
{
	eDebug("loading skin: %s", filename);
	FILE *in=fopen(filename, "rt");
	if (!in)
		return -1;

	parsers.push_front(new XMLTreeParser("ISO-8859-1"));
	XMLTreeParser &parser=*parsers.front();
	char buf[2048];

	int done;
	do
	{
		unsigned int len=fread(buf, 1, sizeof(buf), in);
		done=len<sizeof(buf);
		if (!parser.Parse(buf, len, done))
		{
			eDebug("parse error: %s at line %d",
				parser.ErrorString(parser.GetErrorCode()),
				parser.GetCurrentLineNumber());
			parsers.pop_front();
			fclose(in);
			return -1;
		}
	} while (!done);
	fclose(in);

	XMLTreeNode *root=parser.RootNode();
	if (!root)
		return -1;
	if (strcmp(root->GetType(), "eskin"))
	{
		eDebug("not an eskin");
		return -1;
	}
	
	XMLTreeNode *node=parser.RootNode();
	
	for (node=node->GetChild(); node; node=node->GetNext())
		if (!strcmp(node->GetType(), "colors"))
			parseColors(node);
		else if (!strcmp(node->GetType(), "colorscheme"))
			parseScheme(node);
		else if (!strcmp(node->GetType(), "images"))
			parseImages(node);
		else if (!strcmp(node->GetType(), "values"))
			parseValues(node);
		else if (!strcmp(node->GetType(), "fonts"))
			parseFonts(node);

	return 0;
}

int eSkin::build(eWidget *widget, const char *name)
{
	for (parserList::iterator i(parsers.begin()); i!=parsers.end(); ++i)
	{
		XMLTreeNode *node=i->RootNode();
		node=node->GetChild();
		while (node)
		{
			if (!strcmp(node->GetType(), "object"))
			{
				const char *oname=node->GetAttributeValue("name");
				if (!std::strcmp(name, oname))
				{
					node=node->GetChild();
					return build(widget, node);
				}
			}
			node=node->GetNext();
		}
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

eSkin *eSkin::getActive()
{
	if (!active)
		eFatal("no active skin");
	return active;
}

void eSkin::makeActive()
{
	active=this;
}

gColor eSkin::queryScheme(const eString& name) const
{
	std::map<eString, gColor>::const_iterator it = scheme.find(name);

	if (it != scheme.end())
		return it->second;

	eDebug("%s does not exist", name.c_str());
	
	return gColor(0);
}

gPixmap *eSkin::queryImage(const eString& name) const
{
	std::map<eString, gPixmap*>::const_iterator it = images.find(name);

	if (it != images.end())
		return it->second;
	
	return 0;
}

int eSkin::queryValue(const eString& name, int d) const
{
	std::map<eString, int>::const_iterator it = values.find(name);

	if (it != values.end())
		return it->second;
	
	return d;
}

gColor eSkin::queryColor(const eString& name)
{
	char *end;

	int numcol=strtol(name.c_str(), &end, 10);

	if (!*end)
		return gColor(numcol);

	eNamedColor *col=searchColor(name);

	if (!col)
	{
		eDebug("requested color %s does not exist", name.c_str());
		return gColor(0);
	}
	else
		return col->index;
}

