#include <lib/gui/emessage.h>

class enigmaVCR : public eMessageBox
{
		static enigmaVCR *instance;
	public:
		static enigmaVCR* getInstance() { return instance; }
		enigmaVCR(eString string, eString caption);
		~enigmaVCR() { instance = 0; }
		int eventHandler(const eWidgetEvent &event);
		void volumeUp();
		void volumeDown();
		void switchBack();
};
