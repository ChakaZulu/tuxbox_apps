#ifndef __eskin_h
#define __eskin_h

#include <xmltree.h>
#include <estring.h>
#include <qmap.h>
#include <qdict.h>
#include <qlist.h>
#include "grc.h"

class eWidget;
class gPixmap;
typedef eWidget *(*tWidgetCreator)(eWidget *parent);

struct eNamedColor
{
	eString name;
	gRGB value, end;
	int index;
	int size;
};

class eSkin
{
	XMLTreeParser *parser;
	void clear();
	
	int parseColor(const char *name, const char *color, gRGB &col);
	int parseColors(XMLTreeNode *colors);
	int parseScheme(XMLTreeNode *scheme);
	int parseImages(XMLTreeNode *images);
	int parseValues(XMLTreeNode *values);
	
	gDC *getDCbyName(const char *name);
	
	gRGB *palette;
	int numcolors;
	gImage *paldummy;
	
	static QMap<eString,tWidgetCreator> eSkin::widget_creator;
	int build(eWidget *widget, XMLTreeNode *rootwidget);
	
	QList<eNamedColor> colors;
	QDict<gColor> scheme;
	QDict<gPixmap> images;
	
	QDict<int> values;
	eNamedColor *searchColor(const char *name) const;

	static eSkin *active;
public:
	eSkin();
	~eSkin();

	static void addWidgetCreator(const eString &name, tWidgetCreator creator);
	static void removeWidgetCreator(const eString &name, tWidgetCreator creator);

	int load(const char *filename);
	
	int build(eWidget *widget, const char *name);
	void setPalette(gPixmapDC *pal);

	gColor queryColor(const eString &name) const;
	gColor queryScheme(const eString &name) const;
	gPixmap *queryImage(const eString &name) const;
	int queryValue(const eString &name, int d) const;
	
	void makeActive();
	
	static eSkin *getActive();
};

#define ASSIGN(v, t, n) \
  v =(t*)search(n); if (! v ) { qWarning("skin has undefined element: %s", n); v=new t(this); }

#endif
