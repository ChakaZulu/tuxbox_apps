/*
 * flashimagefs.hpp
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
 * $Id: flashimagefs.hpp,v 1.1 2002/05/28 18:51:27 waldi Exp $
 */

#ifndef __LIBFLASHIMAGE_LIBFLASHIMAGEFS_HPP
#define __LIBFLASHIMAGE_LIBFLASHIMAGEFS_HPP

#include <ostream>
#include <string>

#include <libcrypto++/evp.hpp>

namespace FlashImage
{
  class FlashImageFS
  {
    public:
      virtual void file ( const std::string &, std::ostream & ) = 0;
      virtual void sign ( Crypto::evp::key::privatekey &, const Crypto::evp::md::md & ) = 0;
      virtual int verify ( Crypto::evp::key::key &, const Crypto::evp::md::md & ) = 0;
  };
}

#endif
