#ifndef __eskin_h
#define __eskin_h

#include <xmltree.h>
#include <qstring.h>
#include <qmap.h>
#include <qdict.h>
#include <qlist.h>
#include "grc.h"

class eWidget;
class gPixmap;
typedef eWidget *(*tWidgetCreator)(eWidget *parent);

struct eNamedColor
{
	QString name;
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
	
	static QMap<QString,tWidgetCreator> eSkin::widget_creator;
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

	static void addWidgetCreator(const QString &name, tWidgetCreator creator);
	static void removeWidgetCreator(const QString &name, tWidgetCreator creator);

	int load(const char *filename);
	
	int build(eWidget *widget, const char *name);
	void setPalette(gPixmapDC *pal);

	gColor queryColor(const QString &name) const;
	gColor queryScheme(const QString &name) const;
	gPixmap *queryImage(const QString &name) const;
	int queryValue(const QString &name, int d) const;
	
	void makeActive();
	
	static eSkin *getActive();
};

#endif
