#ifndef __component__
#define __component__


class CComponent
{
	protected:
		bool		bHasFocus;
		bool		bIsActivated;
		CComponent*	cNextFocusableElement;

		//statische Funktion um das letzte Element das Fokus hatte zu ermitteln
		static CComponent* setGlobalCurrentFocusElement( CComponent* ); 

	public:
		CComponent();
	
		//Funktionen zum setzen des Fokus
		void setFocus(bool focus = true);
		void setNextFocusableElement(CComponent*);
		void setNextFocus();

		//Abfrage des Fokus
		bool hasFocus();

		//überschreibbare Fokus-Events
		virtual void onGainFocus();
		virtual void onLostFocus();

		//Aktivieren des Elements
		void setActivated(bool active=true);
		bool isActivated();

		//überschreibbare Aktivierungs-Events
		virtual void onGetActivated();
		virtual void onGetDeActivated();
};


#endif


