/*
 * flashimagearchive.cpp
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
 * $Id: flashimagecramfs.cpp,v 1.2 2002/05/28 18:52:32 waldi Exp $
 */

#include <flashimagecramfs.hpp>

#include <sstream>
#include <stdexcept>

#include <libcrypto++/bio.hpp>
#include <libcrypto++/evp.hpp>
#include <libcrypto++/lib.hpp>
#include <libcrypto++/rand.hpp>

extern "C"
{
  #include <cramfs.h>
  #include <zlib.h>
}

FlashImage::FlashImageCramFS::FlashImageCramFS ( std::iostream & stream )
: stream ( stream )
{
  char * buf = new char[1024];
  unsigned long cramfssize;

  try
  {
    Crypto::lib::init ();

    stream.seekg ( 0 );
    stream.read ( buf, 1024 );

    {
      cramfs_super * super = (cramfs_super *) buf;
      if ( super -> magic != CRAMFS_32 ( CRAMFS_MAGIC ) )
        throw std::runtime_error ( "cramfs: wrong magic" );
      cramfssize = CRAMFS_32 ( super -> size );
    }

    stream.seekg ( 0, std::ios::end );
    size = stream.tellg ();

    if ( size != ( ( ( size - 1 ) | 0xfff ) + 1 ) )
      throw std::runtime_error ( "cramfs: not correct alligned" );
    if ( cramfssize > size - 4096 )
      throw std::runtime_error ( "cramfs: to short for signature" );
  }

  catch ( ... )
  {
    delete buf;
    throw;
  }

  delete buf;
}

void FlashImage::FlashImageCramFS::file ( const std::string & name, std::ostream & out )
{
  char * buf = new char[1024];

  try
  {
    stream.seekg ( 0 );
    stream.read ( buf, 1024 );

    std::istringstream filename ( std::string ( "share/tuxbox/image/" ) + name );
    
    cramfs_super * super = (cramfs_super *) buf;

    unsigned long offset = readdir ( stream, CRAMFS_GET_OFFSET ( & ( super -> root ) ) << 2, CRAMFS_24 ( super -> root.size ), filename );
    decompress ( stream, out, offset );
  }

  catch ( ... )
  {
    delete buf;
    throw;
  }

  delete buf;
}

void FlashImage::FlashImageCramFS::sign ( Crypto::evp::key::privatekey & key, const Crypto::evp::md::md & md )
{
  char * buf = new char[4096];

  try
  {
    stream.seekg ( 0, std::ios::end );
    int i = stream.tellg ();
    i /= 4096;
    i--;
    stream.seekg ( 0 );

    Crypto::evp::sign sign;
    sign.init ( md );

    while ( i )
    {
      stream.read ( buf, 4096 );
      sign.update ( buf, 4096 );
      i--;
    }

    stream.clear ();

    std::string sig ( sign.final ( key ) );

    if ( sig.size () > 4096 - sizeof ( signatureinfo ) )
      throw std::runtime_error ( "cramfs: signature too long" );

    stream.seekg ( -4096, std::ios::end );

    {
      signatureinfo info;
      info.magic = CRAMFS_32 ( magic );
      info.version = CRAMFS_32 ( 1 );
      info.size = CRAMFS_32 ( sig.length () );
      info.reserved = 0;
      stream.write ( ( char * ) &info, sizeof ( signatureinfo ) );
    }

    stream.write ( sig.data (), sig.length () );
  }

  catch ( ... )
  {
    delete buf;
    throw;
  }

  delete buf;
}

int FlashImage::FlashImageCramFS::verify ( Crypto::evp::key::key & key, const Crypto::evp::md::md & md )
{
  char * buf = new char[4096];

  try
  {
    stream.seekg ( 0, std::ios::end );
    unsigned int i = stream.tellg ();
    i /= 4096;
    i--;

    stream.seekg ( -4096, std::ios::end );
    stream.read ( buf, 4096 );

    std::string sig;

    {
      signatureinfo * info = (signatureinfo *) buf;
      unsigned int i;
      if ( info -> magic != CRAMFS_32 ( magic ) )
        throw std::runtime_error ( "cramfs (signatureinfo): wrong magic" );
      i = CRAMFS_32 ( info -> size );
      if ( i > 4096 - sizeof ( signatureinfo ) )
        throw std::runtime_error ( "cramfs (signatureinfo): signature too long" );
      sig = std::string ( ( buf + sizeof ( signatureinfo ) ), i );
    }

    stream.clear ();
    stream.seekg ( 0 );

    Crypto::evp::verify verify;
    verify.init ( md );

    while ( i )
    {
      stream.read ( buf, 4096 );
      verify.update ( buf, 4096 );
      i--;
    }

    stream.clear ();

    delete buf;

    return verify.final ( sig, key );
  }

  catch ( ... )
  {
    delete buf;
    throw;
  }
}

unsigned long FlashImage::FlashImageCramFS::readdir ( std::istream &, unsigned long offset, unsigned long size, std::istream & filename )
{
  char * buf = new char[size];
  unsigned long inodeoffset = 0, nextoffset;

  try
  {
    std::string filenamepart;
    std::getline ( filename, filenamepart, '/' );

    stream.seekg ( offset );
    stream.read ( buf, size );

    while ( inodeoffset < size )
    {
      cramfs_inode * inode;
      char *name;
      int namelen;

      inode = ( cramfs_inode * ) ( buf + inodeoffset );

      namelen = CRAMFS_GET_NAMELEN( inode ) << 2;
      name = (char *) inode + sizeof ( cramfs_inode );
      nextoffset = inodeoffset + sizeof ( cramfs_inode ) + namelen;

      while ( 1 )
      {
        if ( ! namelen )
          throw std::runtime_error ( "find invalid name" );
        if ( name [ namelen - 1 ] )
          break;
        namelen--;
      }

      if ( filenamepart == std::string ( name, namelen ) )
      {
        if ( S_ISDIR ( CRAMFS_16 ( inode -> mode ) ) )
        {
          unsigned int ret = readdir ( stream, CRAMFS_GET_OFFSET ( inode ) << 2, CRAMFS_24 ( inode -> size ), filename );
          delete buf;
          return ret;
        }
        else if ( S_ISREG ( CRAMFS_16 ( inode -> mode ) ) )
          return offset + inodeoffset;
        else
          throw std::runtime_error ( "find unknown thing" );
      }

      inodeoffset = nextoffset;
    }

    throw std::runtime_error ( "find nothing" );
  }

  catch ( ... )
  {
    delete buf;
    throw;
  }
}

void FlashImage::FlashImageCramFS::decompress ( std::istream & stream, std::ostream & out, unsigned int offset )
{
  char * buf_in = new char [ 12288 + sizeof ( cramfs_inode ) ];
  char * buf_out = buf_in + 4096;
  char * buf_inode = buf_in + 12288;
  unsigned long * buf_block = NULL;
  cramfs_inode * inode = ( cramfs_inode * ) ( buf_inode );
  z_stream z;

  try
  {
    stream.seekg ( offset );
    stream.read ( buf_inode, sizeof ( cramfs_inode ) );

    buf_block = new unsigned long [ ( ( CRAMFS_24 ( inode -> size ) + 4095 ) >> 12 ) ];

    stream.seekg ( CRAMFS_GET_OFFSET ( inode ) << 2 );
    stream.read ( ( char * ) buf_block, ( CRAMFS_24 ( inode -> size ) + 4095 ) >> 10 );

    unsigned long curr_block = ( CRAMFS_GET_OFFSET ( inode ) + ( ( ( CRAMFS_24 ( inode -> size ) ) + 4095 ) >> 12 ) ) << 2;
    unsigned int i;
    int total_size = 0;

    z.zalloc = NULL;
    z.zfree = NULL;

    z.next_in = 0;
    z.avail_in = 0;

    if ( inflateInit ( & z ) != Z_OK )
      throw std::runtime_error ( "inflate error" );

    for ( i = 0; i < ( ( CRAMFS_24 ( inode -> size ) + 4095 ) >> 12 ); i++ )
    {
      stream.seekg ( curr_block );
      stream.read ( buf_in, CRAMFS_32 ( buf_block[i] ) - curr_block );

      inflateReset ( & z );

      z.next_in = ( Bytef * ) buf_in;
      z.avail_in = CRAMFS_32 ( buf_block[i] ) - curr_block;

      z.next_out = ( Bytef * ) buf_out;
      z.avail_out = 8192;

      if ( inflate ( & z, Z_FINISH ) != Z_STREAM_END )
        throw std::runtime_error ( "inflate error" );

      out.write ( buf_out, z.total_out );
      total_size += z.total_out;
      curr_block = CRAMFS_32 ( buf_block[i] );
    }

    inflateEnd ( & z );
  }

  catch ( ... )
  {
    inflateEnd ( & z );
    delete buf_in;
    delete buf_block;
  }

  delete buf_in;
  delete buf_block;
}

