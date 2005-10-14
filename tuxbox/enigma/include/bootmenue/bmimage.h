/*
 * $Id: bmimage.h,v 1.1 2005/10/14 18:37:03 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <dirent.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <include/lib/base/estring.h>

using namespace std;

class eImage
{
public:
	eString name;
	eString location;
};

class bmimages: public Object
{
public:
	std::vector<eImage> imageList;
	std::vector<eImage>::iterator it;
	
	bmimages() {};
	~bmimages() 
	{
		imageList.clear();
	};
	
	int load(eString mpoint, bool clearList)
	{
		struct stat s;
		eImage image;
	
		eString dir[2] = {mpoint + "/image/", mpoint + "/fwpro/"};
	
		if (clearList)
			imageList.clear();
			
		image.name = "Flash-Image";
		image.location  = "";
		imageList.push_back(image);

		for (int i = 0; i < 2; i++)
		{
			DIR *d = opendir(dir[i].c_str());
			if (d)
			{
				while (struct dirent *e = readdir(d))
				{
					if (strcmp(e->d_name, ".") && strcmp(e->d_name, ".."))
					{
						eString name = dir[i] + eString(e->d_name);
						stat(name.c_str(), &s);
						if (S_ISDIR(s.st_mode))
						{
							image.location = name;
							image.name = e->d_name;
							ifstream nameFile(eString(name + "/imagename").c_str());
							if (nameFile)
							{
								eString line;
								getline(nameFile, line, '\n');
								nameFile.close();
								if (line)
									image.name = line;
							}
							imageList.push_back(image);
						}
					}
				}
			}
			closedir(d);
		}
		return imageList.size();
	}
	
	void discard(eString location)
	{
		for (it = imageList.begin(); it != imageList.end(); it++)
		{
			if (it->location == location)
			{
				system(eString("rm -rf \"" + location + "\"").c_str());
				break;
			}
		}
	}
	
	void rename(eString from, eString to)
	{
		for (it = imageList.begin(); it != imageList.end(); it++)
		{
			if (it->name == from)
			{
				system(eString("mv \"" + from + "\" \"" + to + "\"").c_str());
				eString name = to.right(to.length() - to.find_last_of("/") - 1);
				if (FILE *f = fopen(eString(to + "/imagename").c_str(), "w"))
				{
					fprintf(f, "%s", name.c_str());
					fclose(f);
				}
			}
		}
	}
	
	void add(eString mpoint, eString imageName)
	{
		eImage image;
		eString imageLocation = mpoint + "/fwpro/" + imageName;
		
		// Todo: unpack and istall image here...
		
		image.name = imageName;
		image.location = imageLocation;
		imageList.push_back(image);
	}
};
