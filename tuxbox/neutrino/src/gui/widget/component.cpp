#include "component.h"




CComponent::CComponent()
{
	bHasFocus = false;
	bIsActivated = true;
	cNextFocusableElement = 0;
}

CComponent* setGlobalCurrentFocusElement(CComponent* focusElement)
{
	static CComponent* currentFocusElement = 0;
	CComponent* returnVal = currentFocusElement;
	currentFocusElement = focusElement;
	return returnVal;
}

void CComponent::setFocus(bool focus)
{
	//vorherig aktives Element abfragen
	CComponent* lastFocusElement = setGlobalCurrentFocusElement(this);
	if(lastFocusElement)
	{
		//und diesem den Fokus nehmen
		lastFocusElement->setFocus(false);
	}
	//dieses Element auf Fokus setzen
	bHasFocus = focus;
	if(bHasFocus)
	{
		onGainFocus();
	}
	else
	{
		onLostFocus();
	}
}

bool CComponent::hasFocus()
{
	return bHasFocus;
}

void CComponent::setNextFocusableElement(CComponent* nextFocusableElement)
{
	cNextFocusableElement = nextFocusableElement;
}

void CComponent::setNextFocus()
{
	if(cNextFocusableElement)
	{
		cNextFocusableElement->setFocus();
	}
}

void CComponent::setActivated(bool active)
{
	bIsActivated = active;
	if(active)
	{
		onGetActivated();
	}
	else
	{
		onGetDeActivated();
	}
}

bool CComponent::isActivated()
{
	return bIsActivated;
}

void CComponent::onGainFocus()
{
}

void CComponent::onLostFocus()
{		
}

void CComponent::onGetActivated()
{
}

void CComponent::onGetDeActivated()
{
}





