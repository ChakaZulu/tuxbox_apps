#ifndef __emessage_h
#define __emessage_h

#include "ewindow.h"

class eLabel;

/**
 * \brief A (modal) messagebox.
 */
class eMessageBox: public eWindow
{
//	Q_OBJECT
	eLabel *text;
public:// slots:
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
	eMessageBox(QString string, QString caption);
	~eMessageBox();
};

#endif
