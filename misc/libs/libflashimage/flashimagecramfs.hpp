/*
 * flashimagecramfs.hpp
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
 * $Id: flashimagecramfs.hpp,v 1.1 2002/05/28 18:51:27 waldi Exp $
 */

#ifndef __LIBFLASHIMAGE_LIBFLASHIMAGECRAMFS_HPP
#define __LIBFLASHIMAGE_LIBFLASHIMAGECRAMFS_HPP

#include <libflashimage/flashimagefs.hpp>

#include <iostream>
#include <map>
#include <string>

#include <libcrypto++/evp.hpp>

namespace FlashImage
{
  class FlashImageCramFS : public FlashImageFS
  {
    private:
      struct signatureinfo
      {
        unsigned long magic;
        unsigned long version;
        unsigned long size;
        unsigned long reserved;
      };

    public:
      FlashImageCramFS ( std::iostream & );

      void file ( const std::string &, std::ostream & );
      void sign ( Crypto::evp::key::privatekey &, const Crypto::evp::md::md & );
      int verify ( Crypto::evp::key::key &, const Crypto::evp::md::md & );

      static const unsigned long magic = 0x9ad13486;

    private:
      unsigned long readdir ( std::istream &, unsigned long, unsigned long, std::istream & );
      void decompress ( std::istream &, std::ostream &, unsigned int );

      std::iostream & stream;
      unsigned int size;
  };
}

#endif
