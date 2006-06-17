/*
 * Tool for manipulating the AVS Switch settings.
 *
 * $Id: avswitch.cpp,v 1.1 2006/06/17 13:47:46 barf Exp $
 *
 * Copyright (C) 2006 Bengt Martensson <barf@bengt-martensson.de>
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

#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <dbox/avs_core.h>
#include <sys/ioctl.h>

static const char *version = "avswitch version 0.5; $Id: avswitch.cpp,v 1.1 2006/06/17 13:47:46 barf Exp $";

class parameter {
private:
  static const char *device;
  static std::vector<parameter> parameters;
  static int fd;
  static int type;

  std::string name;
  int max;
  int min;
  int get_ioctl;
  std::string get_ioctl_name;
  int set_ioctl;
  std::string set_ioctl_name;

public:
  parameter(std::string name, int min, int max, int get_io_ctl,
	    std::string, int int_set_ioctl, std::string);
  std::string& get_name() { return name; }
  int get_value();
  bool set_value(int value);

  static std::string dump();
  static std::string list_names();
  static bool open_device();
  static bool close_device();
  static std::string vendor();
  static std::string chip();
  static bool nokia() 		{ return type == CXA2092; }
  static bool sagem() 		{ return type == CXA2126; }
  static bool philips() 	{ return type == STV6412; }
  static parameter & get_parameter(std::string name);
  static int get_value(std::string name);
  static bool set_value(std::string name, int value);
  static bool is_found(std::string name);
};

parameter::parameter(std::string name_, int min_, int max_,
		     int get_ioctl_, std::string get_ioctl_name_,
		     int set_ioctl_, std::string set_ioctl_name_)
{
  name = name_;
  min = min_;
  max = max_;
  get_ioctl = get_ioctl_;
  get_ioctl_name = get_ioctl_name_;
  set_ioctl = set_ioctl_;
  set_ioctl_name = set_ioctl_name_;
  parameters.push_back(*this);
};

std::string parameter::dump() {
  std::ostringstream os;
  std::vector<parameter>::iterator p;
  for (p = parameters.begin(); p != parameters.end(); p++) {
    os << p->name << " = " << p->get_value() << "\n";
  }
  return os.str();
}

std::string parameter::list_names() {
  std::string s;
  std::vector<parameter>::iterator p;
  for (p = parameters.begin(); p != parameters.end(); p++) {
    if (s != "")
      s += ", ";
    s += p->name;
  }
  return s;
}

int parameter::get_value() {
  if (fd == -1)
    open_device();

  int i = 0;
  if (ioctl(fd, get_ioctl, &i)< 0) {
    perror(get_ioctl_name.c_str());
    return -1;
  } else
    return i;
}

bool parameter::set_value(int value) {
  if (value < min || value > max) {
    std::cerr << "Attempt to set " << name
	      << " to an invalid value. Valid are " << min << "-" << max
	      << ".\n";
    return false;
  }
  if (fd == -1)
    open_device();

  if (ioctl(fd, set_ioctl, &value)< 0) {
    perror(set_ioctl_name.c_str());
    return false;
  } else
    return true;
}

bool parameter::is_found(std::string name_) {
  std::vector<parameter>::iterator p;
  for (p = parameters.begin(); p != parameters.end(); p++)
    if (p->name == name_)
      return true;

  return false;
}

// Returns nonsense if not found
parameter &parameter::get_parameter(std::string name_) {
  parameter &pr = parameters[0];
  for (unsigned int i = 0; i < parameters.size(); i++)
    if (parameters[i].name == name_)
      pr = parameters[i];

  return pr;
}

int parameter::get_value(std::string name) {
  return is_found(name) ? get_parameter(name).get_value() : -1;
}

bool parameter::set_value(std::string name, int value) {
  return is_found(name) ? get_parameter(name).set_value(value) : false;
}

bool parameter::open_device() {
  fd = open(device, O_RDWR);
  
  if (fd <= 0) {
    perror("open");
    return false;
  }
  if (ioctl(fd, AVSIOGTYPE, &type) < 0) {
    perror("can't get avs-type:");
    close(fd);
    return false;
  }

  switch(type) {
  case CXA2092:
  case CXA2126:
  case STV6412:
    break;
  default:
    std::cerr << "unknown avsswitch type\n";
    close(fd);
    return false;
  }

  return true;
}

bool parameter::close_device() {
  bool success = close(fd) == 0;
  fd = -1;
  return success;
}

std::string parameter::chip() {
  return type == CXA2092 ? "CXA2092" : 
    type == CXA2126 ? "CXA2126" : 
    "STV6412";
};

std::string parameter::vendor() {
  return type == CXA2092 ? "nokia" : 
    type == CXA2126 ? "sagem" : 
    "philips";
};

std::vector<parameter> parameter::parameters;
int parameter::fd = -1;
int parameter::type = -1;
const char *parameter::device = "/dev/dbox/avs0";

void usage(std::string argv0) {
  std::cerr << version << "\n\n";
  std::cerr << "usage:\n\t" << argv0 << " command\n";
  std::cerr << "where command = show, list, scart.conf, scart.conf_ext\n";
  std::cerr << "\nor\n";
  std::cerr << "\tavswitch variable\n";
  std::cerr << "\nor\n";
  std::cerr << "\tavswitch variable value\n";
  std::cerr << "where variable = " << parameter::list_names() << "\n";
}

int main(int argc, char *argv[]) {
  
  parameter v1("v1", 0, 7, AVSIOGVSW1, "AVSIOGVSW1", AVSIOSVSW1, "AVSIOSVSW1");
  parameter a1("a1", 0, 4, AVSIOGASW1, "AVSIOGASW1", AVSIOSASW1, "AVSIOSASW1");
  parameter v2("v2", 0, 7, AVSIOGVSW2, "AVSIOGVSW2", AVSIOSVSW2, "AVSIOSVSW2");
  parameter a2("a2", 0, 3, AVSIOGASW2, "AVSIOGASW2", AVSIOSASW2, "AVSIOSASW2");
  parameter v3("v3", 0, 7, AVSIOGVSW3, "AVSIOGVSW3", AVSIOSVSW3, "AVSIOSVSW3");
  parameter a3("a3", 0, 3, AVSIOGASW3, "AVSIOGASW3", AVSIOSASW3, "AVSIOSASW3");
  parameter fblk("fblk", 0, 3, AVSIOGFBLK, "AVSIOGFBLK",
		 AVSIOSFBLK, "AVSIOSFBLK");
  parameter fnc("fnc", 0, 3, AVSIOGFNC, "AVSIOGFNC", AVSIOSFNC, "AVSIOSFNC");
  parameter zcd("zcd", 0, 1, AVSIOGZCD, "AVSIOGZCD", AVSIOSZCD, "AVSIOSZCD");
  parameter ycm("ycm", 0, 1, AVSIOGYCM, "AVSIOGYCM", AVSIOSYCM, "AVSIOSYCM");
  parameter volume("volume", 0, 63, AVSIOGVOL, "AVSIOGVOL",
		   AVSIOSVOL, "AVSIOSVOL");
  parameter mute("mute", 0, 1, AVSIOGMUTE, "AVSIOGMUTE",
		 AVSIOSMUTE, "AVSIOSMUTE");

  parameter::open_device();

  bool success = false;

  if (argc == 1) {
    usage(argv[0]);
  } else if (std::string(argv[1]) == "scart.conf") {
    std::cout << "#           v1 a1 v2 a2 v3 a3 fblk\n";
    std::cout << parameter::vendor() << "_xxxx: " 
	      << v1.get_value() << "  " << a1.get_value() << "  "
	      << v2.get_value() << "  " << a2.get_value() << "  "
	      << v3.get_value() << "  " << a3.get_value() << "  "
	      << fblk.get_value() << "\n";
    success = true;
  } else if (argc > 1 && std::string(argv[1]) == "scart.conf_ext") {
    std::cout << "#               v1      a1             v2                 a2     v3    a3 fblk\n";
    std::cout << parameter::vendor() << "_xxxx: {" 
	      << v1.get_value() << " " << v1.get_value() << " "
	      << v1.get_value() << " " << v1.get_value() << " "
	      << v1.get_value() << "} " 
	      << a1.get_value()
	      << " {{" << v2.get_value() << " " << v2.get_value() << "} {" 
	      << v2.get_value() << " " << v2.get_value() << "} {" 
	      << v2.get_value() << " " << v2.get_value() << "} {" 
	      << v2.get_value() << " " << v2.get_value() << "} {" 
	      << v2.get_value() << " " << v2.get_value() << "}} " 
	      << a2.get_value() << " {"
	      << v3.get_value() << " " << v3.get_value() << " "
	      << v3.get_value() << " " << v3.get_value() << " "
	      << v3.get_value() << "} " 
	      << a3.get_value() << " "
	      << fblk.get_value() << "\n";
    success = true;
  } else if (std::string(argv[1]) == "list") {
    std::cout << "Chip: " << parameter::chip()
	      << " (" << parameter::vendor() << ")\n";
    std::cout << parameter::dump();
    success = true;
  } else if (std::string(argv[1]) == "s" || std::string(argv[1]) == "show") {
    std::cout << "TV-Scart:  video (v1) = " << v1.get_value()
	      << " \tAudio (a1) = " << a1.get_value() << "\n";
    if (parameter::nokia()) {
      std::cout << "VCR-Scart: video (v2) = " << v2.get_value()
		<< " \tAudio (a3) = " << a3.get_value() << "\n";
      std::cout << "AUX:       video (v3) = " << v3.get_value()
		<< " \tAudio (a2) = " << a3.get_value() << "\n";
    } else {
      std::cout << "VCR-Scart: video (v2) = " << v2.get_value()
		<< " \tAudio (a2) = " << a2.get_value() << "\n";
      std::cout << "AUX:                    "
		<< " \tAudio (a1) = " << a1.get_value() << "\n";
    }
    std::cout << "FBLK: " << fblk.get_value() << "\n";
    std::cout << "FNC:  " << fnc.get_value() << " (= "
	      << (fnc.get_value() == 0 ? "inactive" :
		  fnc.get_value() == 1 ? "16:9" : "4:3") << ")\n";
    std::cout << "avs volume:  " << volume.get_value()
	      << " (0-63; 63 = quiet, 0 = loud)\n";
    std::cout << "avs mute:  " << mute.get_value() << "\n";
    success = true;
  } else if (argc == 2 && parameter::is_found(argv[1])) {
    std::cout << parameter::get_value(argv[1]) << "\n";
    success = true;
  } else if (argc == 3 && parameter::set_value(argv[1], atoi(argv[2]))) {
    success = true;
  } else {
    usage(argv[0]);
  }

  parameter::close_device();
  return success ? 0 : 1;
}
