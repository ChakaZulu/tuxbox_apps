#ifndef __CORE_GUI_STATUSBAR__
#define __CORE_GUI_STATUSBAR__

#include <core/gui/decoration.h>
#include <core/gui/ewidget.h>
#include <core/gui/elabel.h>

class eStatusBar : public eWidget
{
	int flags;
	eDecoration deco;
	eLabel client;
	void update( const eWidget *);

	void redrawWidget(gPainter *, const eRect &);
	int setProperty(const eString &, const eString &);
	int eventHandler(const eWidgetEvent &event);
	void loadDeco();
	void redrawBorder(gPainter *, const eRect &);
	void initialize();
public:
	enum
	{
		flagOwnerDraw = 1,
		flagLoadDeco = 2
	};
	eStatusBar( eWidget*, int=0 );
	int getFlags() const;
	void setFlags( int );
};

#endif // __CORE_GUI_STATUSBAR__

