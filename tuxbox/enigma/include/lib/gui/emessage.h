#ifndef __emessage_h
#define __emessage_h

#include <lib/gui/ewindow.h>

class eLabel;

/**
 * \brief A (modal) messagebox.
 */
class eMessageBox: public eWindow
{
	eLabel *text, *icon;
	eWidget *def;
public:
	void pressedOK();
	void pressedCancel();
	void pressedYes();
	void pressedNo();
	int eventHandler( const eWidgetEvent & );
public:
	enum { btOK=1, btCancel=2, btYes=4, btNo=8, btMax};
	enum { iconInfo=16, iconWarning=32, iconQuestion=64, iconError=128 };
	/**
	 * \brief Creates a messagebox.
	 *
	 * example: 
	 * \code
{
  eMessageBox message("Your documentation sucks!\nPlease consider using Neutrino!", "Documentation");
  message.show();
  message.exec();
  message.hide();
} \endcode
	 * \param string The string displayed inside the messagebox.
	 * \param caption The title of the messagebox.
	 */
	eMessageBox(eString string, eString caption, int flags=btOK, int def=btOK );
};

#endif
