/*
 * flashimage.hpp
 *
 * Copyright (C) 2002 Bastian Blank <waldi@tuxbox.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: flashimage.hpp,v 1.1 2002/05/28 18:51:27 waldi Exp $
 */

#ifndef __LIBFLASHIMAGE__LIBFLASHIMAGE_HPP
#define __LIBFLASHIMAGE__LIBFLASHIMAGE_HPP

#define INLINE

#include <istream>
#include <map>
#include <string>

#include <libflashimage/flashimagefs.hpp>

namespace FlashImage
{
  class FlashImage
  {
    public:
      FlashImage ( FlashImageFS & );

      std::string get_control_field ( std::string );

      int verify_cert ();
      int verify_image ();

    protected:
      std::map < std::string, std::string > parse_control ( std::istream & stream );

      FlashImageFS & fs;
      std::map < std::string, std::string > control;
  };
}

#ifdef INLINE
#include <libflashimage/flashimage.ipp>
#endif

#endif
