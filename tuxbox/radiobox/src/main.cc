/* Created by Anjuta version 1.2.2 */
/*	This file will not be overwritten */

#include <radiobox.h>
#include <playlist.h>

#include <iostream>

int main()
{
	std::cout << "Initializing radiobox..." << std::endl;
	
	try
	{
		CRadioBox::GetInstance()->Run();
	}
	catch( EPlayList _e )
	{
		std::cout << _e.GetText() << std::endl;
		return 3;
	}
	
	
	return 0;
}
