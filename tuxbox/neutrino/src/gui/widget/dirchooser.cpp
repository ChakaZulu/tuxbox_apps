/*  
	$Id: dirchooser.cpp,v 1.1 2007/01/24 02:05:30 guenther Exp $
	
	Neutrino-GUI  -   DBoxII-Project
	
	Copyright (C) 2006 Guenther
	Homepage: http://www.tuxbox.org/
	
	
	License: GPL
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/widget/dirchooser.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <system/fsmounter.h>
//#include <system/ping.h>
extern "C" int pingthost ( const char *hostname, int t );
#include <gui/filebrowser.h>

#include <sys/vfs.h> // for statfs
#include <dirent.h>
#include <sys/stat.h>



/**********************************************************************/
/* 			CDirChooser												  */
/**********************************************************************/
/* Use this class to allow the user to select any path with the filebrowser
   You could allow any path or just subdirs from up to two paths. 
   If a path is not valid a empty string is returned.*/


/**********************************************************************/
CDirChooser::CDirChooser(std::string* path, char* allowed_path0,char* allowed_path1)
/* path: user selected path (finished with button OK)
   allowed_path0: if not NULL, subdirs of this path are valid only ( otherwise path ="")
   allowed_path1: second valid path.*/
/**********************************************************************/
{
	dirPath = path;
	if(MAX_ALLOWED_PATHS > 0)
	{
		allowedPath[0] = allowed_path0;
	}
	if(MAX_ALLOWED_PATHS > 1)
	{
		allowedPath[1] = allowed_path1;
	}
}

/**********************************************************************/
int CDirChooser::exec(CMenuTarget* parent, const std::string & actionKey)
/* start dir chooser */
/**********************************************************************/
{
	int result = menu_return::RETURN_REPAINT;
   	new_path_selected = false;
	
    if(parent != NULL)
    {
    	parent->hide();
    }
    	
    CFileBrowser browser;
    browser.Dir_Mode = true;
    
    bool allowed = false;
    bool test_allowed = false;
    if (browser.exec(dirPath->c_str()))
    {
        *dirPath = browser.getSelectedFile()->Name;
        
        for(int i = 0;i < MAX_ALLOWED_PATHS; i++)
        {
        	if(allowedPath[i] != NULL)
        	{
        		test_allowed = true; // there is at least one allowed path, so check it
	        	if(dirPath->compare(0,strlen(allowedPath[i]),allowedPath[i]) == 0)
	        	{
	        		allowed = true;
	        		break;
	        	}
        	}
        }
        if(test_allowed == true && allowed == false)
        {
            *dirPath = "";   // We clear the  string if the selected folder is not at allowed
        }
   		new_path_selected = true;
    }
    return result;
}


/**********************************************************************/
/* 			CRecDirChooser	                                          */
/**********************************************************************/
/* Use this class to allow the user to select a recording directory defined in 
   g_settings.recording_dir or any other dir with the filebrowser.
   The free space is printed for any directory. 
   For nfs directory, a server check (ping) is made and the mount status is determined. 
   (g_settings.network_nfs_*). */


/**********************************************************************/
CRecDirChooser::CRecDirChooser(const neutrino_locale_t Name, const std::string & Icon, int * chosenNfsIndex, char * chosenLocalDir, const char * const selectedLocalDir, const int mwidth, const int mheight)
	: CMenuWidget(Name, Icon,mwidth,mheight), index(chosenNfsIndex), localDir(chosenLocalDir)
/**********************************************************************/
{
	localDirString = NULL;
	initMenu();
}

/**********************************************************************/
CRecDirChooser::CRecDirChooser(const neutrino_locale_t Name, const std::string & Icon, int * chosenNfsIndex, std::string * chosenLocalDir, const char * const selectedLocalDir, const int mwidth, const int mheight)
	: CMenuWidget(Name, Icon,mwidth,mheight), index(chosenNfsIndex), localDirString(chosenLocalDir)
/**********************************************************************/
{
	localDir = NULL;
	initMenu();
}


/**********************************************************************/
void CRecDirChooser::initMenu(void)
/**********************************************************************/
{
	char indexStr[10];
	//************************************************/
	addItem(GenericMenuSeparator);
	for(int i=0 ; i < MAX_RECORDING_DIR ; i++)
	{
		if(!g_settings.recording_dir[i].empty())
		{
			bool get_size = true;
			dirOptionText[i]="";
			nfsIndex[i] = getNFSIDOfDir(g_settings.recording_dir[i].c_str());
			//printf("dir %d = nfs: %d\n",i,nfsIndex[i]);
			if( nfsIndex[i] != -1)
			{ 
				int retvalue = pingthost(g_settings.network_nfs_ip[nfsIndex[i]].c_str(),60); // send ping and wait 60ms
				if (retvalue == 0)//LOCALE_PING_UNREACHABLE
				{
					dirOptionText[i] = g_Locale->getText(LOCALE_RECDIRCHOOSER_SERVER_DOWN); 
					get_size = false;
				}
				else if (retvalue == 1)//LOCALE_PING_OK
				{
					// check if we can get more dir informations
					if(	CFSMounter::isMounted (g_settings.network_nfs_local_dir[nfsIndex[i]]) == 0)
					{
						dirOptionText[i] = g_Locale->getText(LOCALE_RECDIRCHOOSER_NOT_MOUNTED); 
						get_size = false;
					}
				}
			}
			if(get_size)
			{
				// in any other case we can show the free space
				int drivefree;
				drivefree = getFreeDiscSpaceGB(g_settings.recording_dir[i].c_str());
				char tmp[20];
				snprintf(tmp, 19,g_Locale->getText(LOCALE_RECDIRCHOOSER_FREE),drivefree);
				tmp[19]=0;
				dirOptionText[i]=tmp;
			}
			snprintf(indexStr,10,"MID:%d",i);
			addItem(new CMenuForwarderNonLocalized(	g_settings.recording_dir[i].c_str(), true, dirOptionText[i], this, indexStr));
		}
	}
	addItem(GenericMenuSeparatorLine);
	addItem(new CMenuForwarder(LOCALE_RECDIRCHOOSER_USER_DIR, true, NULL, this, "dirChooser"));
	addItem(GenericMenuSeparator);
}

/**********************************************************************/
int CRecDirChooser::exec(CMenuTarget* parent, const std::string & actionKey)
/**********************************************************************/
{
	int result = menu_return::RETURN_EXIT;
	const char * key = actionKey.c_str();
	
	//printf(" CRecDirChooser::exec %s\n",key);
	if (strncmp(key, "MID:",4) == 0)
	{
		int id = -1;
		selectedDir = -1;
		sscanf(&key[4],"%d",&id);
		
		//printf("rec dir: %d\n",id);
		if( id > -1 && id < MAX_RECORDING_DIR)
		{
			dir = g_settings.recording_dir[id];
			selectedDir = id;
			if (index)
			{
				*index = nfsIndex[id];
			}
		}
		hide();
		result =  menu_return::RETURN_EXIT;
	} 
	else if (strcmp(key, "dirChooser") == 0)
	{
		selectedDir = -1;
		if(g_settings.recording_dir[0] != "")
		{
			dir = g_settings.recording_dir[0];
		}
		else
		{
			dir = "/mnt/";
		}
		CDirChooser dirChooser(&dir, "/hdd","/mnt/");
		dirChooser.exec(NULL,"");
		if(dirChooser.new_path_selected == false)
		{
			dir = "";
		}
		if(dir == "" )
		{
			result = menu_return::RETURN_REPAINT;
		}
	}
	else
	{
		dir = "";
		result = CMenuWidget::exec(parent, actionKey);
		if(dir != "")
		{
			if (localDir) 
			{
				strcpy(localDir,dir.c_str());
			}
			if (localDirString)
			{ 
				*localDirString = dir;
			}
		}
	}
	return (result);
}


/**********************************************************************/
/* At least some useful gadgets ;)									  */
/**********************************************************************/

/**********************************************************************/
int getNFSIDOfDir(const char * dir)
/* tries to find the path in any of the nfs definitons and returns the array number
   return: 0-200 if path is found in the nfs defintion g_settings.network_nfs_ the array number is returned
           -1    if not found in g_settings.network_nfs_*/
/**********************************************************************/
{
	int result = -1;
	bool loop = true;
	for (int i = 0; i < NETWORK_NFS_NR_OF_ENTRIES && loop; i++)
	{
		if (g_settings.network_nfs_local_dir[i][0] != 0 &&
		    strstr(dir,g_settings.network_nfs_local_dir[i]) == dir)
		{
			result = i;
			loop = false;
		}
	}
	return (result);
}

/**********************************************************************/
unsigned int getFreeDiscSpaceGB(const char * dir)
/* returns free space of dir */
/**********************************************************************/
{
	unsigned int drivefree = 0;
	struct statfs s;
	if (statfs(dir, &s) >= 0 )
	{
		drivefree = (s.f_bfree * s.f_bsize)>>30;
	}
	
	return drivefree;
}

/**********************************************************************/
int getFirstFreeRecDirNr(int min_free_gb)
/* tries to find a path from g_settings.recording_dir with at least min_free_gb GB 
   return: array number of g_settings.recording_dir if successful
           -1 if not successful */
/**********************************************************************/
{
	int result = -1;
	bool loop = true;
	for(int i = 0 ; i < MAX_RECORDING_DIR && loop ; i++)
	{
		const char* dir = g_settings.recording_dir[i].c_str();
		int nfs_id = getNFSIDOfDir(dir);
		printf("Dir: %s:", g_settings.recording_dir[i].c_str());
		if(	nfs_id != -1 )
		{
			printf("NFS%d", nfs_id);
			int retvalue = pingthost(g_settings.network_nfs_ip[nfs_id].c_str(),60); //  ping for 60 ms
			if (retvalue == 0) //LOCALE_PING_UNREACHABLE
			{
				printf(",Server down ");
			}
			else if (retvalue == 1) //LOCALE_PING_OK
			{
				if(CFSMounter::isMounted (g_settings.network_nfs_local_dir[nfs_id]) == 0)
				{
					CFSMounter::MountRes mres =	CFSMounter::mount(g_settings.network_nfs_ip[nfs_id].c_str(),
												g_settings.network_nfs_dir[nfs_id],
												g_settings.network_nfs_local_dir[nfs_id],
												(CFSMounter::FSType) g_settings.network_nfs_type[nfs_id],
												g_settings.network_nfs_username[nfs_id],
												g_settings.network_nfs_password[nfs_id],
												g_settings.network_nfs_mount_options1[nfs_id],
												g_settings.network_nfs_mount_options2[nfs_id]);
					if (mres != CFSMounter::MRES_OK)
					{
						printf(",mount failed");
						loop = false; // we cannot mount - why? we leave here since other mounts will fail as well
					}
					else
					{
						printf(",mount OK");
					}
				}
				else
				{
					printf(",mounted");
				}
			}
		}
		if(loop) // if we should still process the loop
		{
			int free = getFreeDiscSpaceGB(dir);
			printf(",free %dGB\n",free);
			if (free > min_free_gb)
			{
				result = i; // we found a dir, nothing else we can do here 
				loop = false; 
			}
		}
	}
	return (result); 
}


