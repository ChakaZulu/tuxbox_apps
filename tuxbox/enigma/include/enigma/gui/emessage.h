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
	void okPressed();
public:
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
	eMessageBox(eString string, eString caption);
	~eMessageBox();
};

#endif
