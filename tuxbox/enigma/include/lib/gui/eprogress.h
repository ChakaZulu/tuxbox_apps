#ifndef __eprogress_h
#define __eprogress_h

#include <lib/gui/ewidget.h>
#include <lib/gdi/grc.h>

/**
 * \brief A progressbar.
 *
 * Useful for displaying a progress or stuff.
 */
class eProgress: public eWidget
{
protected:
	int perc, border;
	gColor left, right;
public:
	eProgress(eWidget *parent, int takeFocus=0);
	~eProgress();
	
	/**
	 * \brief Sets the value. 
	 *
	 * \param perc The range is \c 0..100
	 */
	void setPerc(int perc);
	void redrawWidget(gPainter *target, const eRect &area);
	
	/**
	 * \brief Sets a property.
	 *
	 * Valid, eProgress specific properties are:
	 * \arg \c leftColor, the color of the left part
	 * \arg \c rightColor, the color of the right part
	 * \arg \c border, the size of the border (in pixels)
	 * \sa eWidget::setProperty
	 */
	int setProperty(const eString &prop, const eString &value);
	void setBorder( int b ) { border = b; }
	void setLeftColor( const gColor& c )	{ left = c; }
	void setRightColor( const gColor& c )	{ right = c; }
};

#endif
