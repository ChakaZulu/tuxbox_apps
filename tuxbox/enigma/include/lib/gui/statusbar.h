#ifndef __CORE_GUI_STATUSBAR__
#define __CORE_GUI_STATUSBAR__

#include <core/gui/ewidget.h>
#include <core/gui/elabel.h>

class eStatusBar : public eDecoWidget
{
	int flags;
	eLabel client;
	void update( const eWidget *);
	const eWidget* current;

	void redrawWidget(gPainter *, const eRect &);
	int setProperty(const eString &, const eString &);
	int eventHandler(const eWidgetEvent &event);
	void initialize();
public:
	enum
	{
		flagOwnerDraw = 1,
		flagVCenter = 2
	};
	eStatusBar( eWidget*, const char* deco="eStatusBar" );
	int getFlags() const;
	eLabel &getLabel()	{	return client; }
	void setFlags( int );
};

#endif // __CORE_GUI_STATUSBAR__

