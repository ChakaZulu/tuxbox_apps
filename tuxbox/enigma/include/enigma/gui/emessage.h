#ifndef __emessage_h
#define __emessage_h

#include <core/gui/ewindow.h>

class eLabel;

/**
 * \brief A (modal) messagebox.
 */
class eMessageBox: public eWindow
{
	eLabel *text;
public:
	void pressedOK();
	void pressedCancel();
	void pressedYes();
	void pressedNo();
public:
	enum { btOK=1, btCancel=2, btYes=4, btNo=8, btMax};
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
	eMessageBox(eString string, eString caption, int flags=btOK, int def=btOK);
	~eMessageBox();
};

#endif
