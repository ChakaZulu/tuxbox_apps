/*
 * x509.cpp: c++ wrapper for openssl x509
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
 * $Id: x509.cpp,v 1.1 2002/03/02 21:53:26 waldi Exp $
 */

#include <main.hpp>

#include <x509.hpp>

#include <bio.hpp>
#include <lib.hpp>

namespace libcrypto
{
  #include <openssl/pem.h>
}

Crypto::x509::cert::cert ( libcrypto::X509 * _cert = NULL ) throw ()
: _cert ( _cert )
{ }

Crypto::x509::cert::cert ( const cert & orig )
{
  _cert = libcrypto::X509_dup ( orig._cert );
}

Crypto::x509::cert::~cert () throw ()
{
  if ( _cert )
    libcrypto::X509_free ( _cert );
}

Crypto::x509::cert & Crypto::x509::cert::operator = ( const cert & orig )
{
  if ( this != &orig )
  {
    if ( _cert )
      libcrypto::X509_free ( _cert );
    if ( ! orig._cert )
      throw Crypto::exception::no_item ( "Crypto::x509::cert" );

    _cert = libcrypto::X509_dup ( orig._cert );

    if ( ! _cert )
      throw std::bad_alloc ();
  }

  return * this;
}

void Crypto::x509::cert::new_empty ()
{
  if ( _cert )
    libcrypto::X509_free ( _cert );

  _cert = libcrypto::X509_new ();
}

void Crypto::x509::cert::read ( std::istream & stream )
{
  if ( _cert )
  {
    libcrypto::X509_free ( _cert );
    _cert = NULL;
  }

  Crypto::bio::istream bio ( stream );
  _cert = libcrypto::PEM_read_bio_X509 ( bio, NULL, NULL, NULL );

  if ( ! _cert )
    throw Crypto::exception::undefined_libcrypto_error ();
}

void Crypto::x509::cert::write ( std::ostream & stream ) const
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  Crypto::bio::ostream bio ( stream );
  libcrypto::PEM_write_bio_X509 ( bio, _cert );
}

void Crypto::x509::cert::print ( std::ostream & stream ) const
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  Crypto::bio::ostream bio ( stream );
  libcrypto::X509_print ( bio, _cert );
}

long Crypto::x509::cert::get_version () const throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  return libcrypto::X509_get_version ( _cert );
}

void Crypto::x509::cert::set_version ( long version ) throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  libcrypto::X509_set_version ( _cert, version );
}

long Crypto::x509::cert::get_serialNumber () const throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  return libcrypto::ASN1_INTEGER_get ( libcrypto::X509_get_serialNumber ( _cert ) );
}

void Crypto::x509::cert::set_serialNumber ( long serial ) throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  libcrypto::ASN1_INTEGER_set ( libcrypto::X509_get_serialNumber ( _cert ), serial );
}

Crypto::x509::name Crypto::x509::cert::get_issuer_name () const throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  return name ( libcrypto::X509_NAME_dup ( libcrypto::X509_get_issuer_name ( _cert ) ) );
}

void Crypto::x509::cert::set_issuer_name ( Crypto::x509::name & name ) throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  X509_set_issuer_name ( _cert, name );
}

Crypto::x509::name Crypto::x509::cert::get_subject_name () const throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  return name ( X509_NAME_dup ( X509_get_subject_name ( _cert ) ) );
}

void Crypto::x509::cert::set_subject_name ( Crypto::x509::name & name ) throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  X509_set_subject_name ( _cert, name );
}

//std::string Crypto::x509::cert::get_notBefore () const

//void Crypto::x509::cert::set_notBefore ( const std::string & )

void Crypto::x509::cert::set_notBefore ( const long time ) throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  libcrypto::X509_gmtime_adj ( _cert -> cert_info -> validity -> notBefore, time );
}

//std::string Crypto::x509::cert::get_notAfter () const

//void Crypto::x509::cert::set_notAfter ( const std::string & )

void Crypto::x509::cert::set_notAfter ( const long time ) throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  libcrypto::X509_gmtime_adj ( _cert -> cert_info -> validity -> notAfter, time );
}

Crypto::evp::key::key Crypto::x509::cert::get_publickey () const throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  return Crypto::evp::key::key ( libcrypto::X509_get_pubkey ( _cert ) );
}

void Crypto::x509::cert::set_publickey ( Crypto::evp::key::key & key ) throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  libcrypto::X509_set_pubkey ( _cert, key );
}

void Crypto::x509::cert::add_extension ( extension & extension ) throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  libcrypto::X509_add_ext ( _cert, extension, 0 );
}

void Crypto::x509::cert::sign ( Crypto::evp::key::privatekey & key, Crypto::evp::md::md & md ) throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  libcrypto::X509_sign ( _cert, key, md );
}
  
int Crypto::x509::cert::verify ( store & store ) throw ( std::bad_alloc, Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  libcrypto::X509_STORE * _store = store;
  libcrypto::X509_STORE_CTX * ctx = libcrypto::X509_STORE_CTX_new();

  if ( ! ctx )
    throw std::bad_alloc ();
  
  _store -> verify_cb = verify_callback;
  libcrypto::X509_STORE_CTX_init ( ctx, _store, _cert, NULL );
  int ret = libcrypto::X509_verify_cert ( ctx );
  libcrypto::X509_STORE_CTX_free ( ctx );

  if ( ret == 1 )
    return 1;
  if ( ret == -1 )
    throw Crypto::exception::undefined_libcrypto_error ();
  else
    return -ctx -> error;
}

Crypto::x509::cert::operator libcrypto::X509 * () throw ( Crypto::exception::no_item )
{
  if ( ! _cert )
    throw Crypto::exception::no_item ( "Crypto::x509::cert" );

  return _cert;
}

int Crypto::x509::cert::verify_callback ( int ok, libcrypto::X509_STORE_CTX * ctx ) throw ()
{
  char buf[256];

  if (!ok)
  {
    libcrypto::X509_NAME_oneline( libcrypto::X509_get_subject_name ( ctx -> current_cert ), buf, 256 );
    std::cerr << buf << std::endl;
    std::cerr << "error " << ctx -> error << " at " << ctx -> error_depth
      << " depth lookup: " << libcrypto::X509_verify_cert_error_string ( ctx -> error ) << std::endl;
    if ( ctx -> error == X509_V_ERR_CERT_HAS_EXPIRED ) ok = 1;
    if ( ctx -> error == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) ok = 1;
    if ( ctx -> error == X509_V_ERR_INVALID_CA ) ok = 1;
    if ( ctx -> error == X509_V_ERR_PATH_LENGTH_EXCEEDED ) ok = 1;
    if ( ctx -> error == X509_V_ERR_INVALID_PURPOSE ) ok = 1;
  }

  return ok;
}

Crypto::x509::crl::operator libcrypto::X509_CRL * () throw ( Crypto::exception::no_item )
{
  if ( ! _crl )
    throw Crypto::exception::no_item ( "Crypto::x509::crl" );

  return _crl;
}

void Crypto::x509::ctx::set ( cert & issuer ) throw ()
{
  libcrypto::X509V3_set_ctx ( &_ctx, issuer, NULL, NULL, NULL, 0 );
}

void Crypto::x509::ctx::set ( cert & issuer, cert & subject ) throw ()
{
  libcrypto::X509V3_set_ctx ( &_ctx, issuer, subject, NULL, NULL, 0 );
}

void Crypto::x509::ctx::set ( cert & issuer, cert & subject, req & req ) throw ()
{
  libcrypto::X509V3_set_ctx ( &_ctx, issuer, subject, req, NULL, 0 );
}

void Crypto::x509::ctx::set ( req & req ) throw ()
{
  libcrypto::X509V3_set_ctx ( &_ctx, NULL, NULL, req, NULL, 0 );
}

void Crypto::x509::ctx::set ( crl & crl ) throw ()
{
  libcrypto::X509V3_set_ctx ( &_ctx, NULL, NULL, NULL, crl, 0 );
}

void Crypto::x509::ctx::set ( cert & issuer, crl & crl ) throw ()
{
  libcrypto::X509V3_set_ctx ( &_ctx, issuer, NULL, NULL, crl, 0 );
}

Crypto::x509::ctx::operator libcrypto::X509V3_CTX * () throw ()
{
  return &_ctx;
}

Crypto::x509::extension::extension () throw ()
: _extension ( NULL )
{ }

Crypto::x509::extension::~extension () throw ()
{
  if ( _extension )
    libcrypto::X509_EXTENSION_free ( _extension );
}

void Crypto::x509::extension::create ( ctx & ctx, const std::string & name, const std::string & value )
{
  if ( _extension )
    libcrypto::X509_EXTENSION_free ( _extension );

  _extension = libcrypto::X509V3_EXT_conf ( NULL, ctx, ( char * ) name.c_str (), ( char * ) value.c_str () );

  if ( ! _extension )
    throw Crypto::exception::undefined_libcrypto_error ();
}

void Crypto::x509::extension::create ( const std::string & name, const std::string & value )
{
  if ( _extension )
    libcrypto::X509_EXTENSION_free ( _extension );

  _extension = libcrypto::X509V3_EXT_conf ( NULL, NULL, ( char * ) name.c_str (), ( char * ) value.c_str () );

  if ( ! _extension )
    throw Crypto::exception::undefined_libcrypto_error ();
}

void Crypto::x509::extension::create ( ctx & ctx, int nid, const std::string & value )
{
  if ( _extension )
    libcrypto::X509_EXTENSION_free ( _extension );

  _extension = libcrypto::X509V3_EXT_conf_nid ( NULL, ctx, nid, ( char * ) value.c_str () );

  if ( ! _extension )
    throw Crypto::exception::undefined_libcrypto_error ();
}

void Crypto::x509::extension::create ( int nid, const std::string & value )
{
  if ( _extension )
    libcrypto::X509_EXTENSION_free ( _extension );

  _extension = libcrypto::X509V3_EXT_conf_nid ( NULL, NULL, nid, ( char * ) value.c_str () );

  if ( ! _extension )
    throw Crypto::exception::undefined_libcrypto_error ();
}

Crypto::x509::extension::operator libcrypto::X509_EXTENSION * () throw ( Crypto::exception::no_item )
{
  if ( ! _extension )
    throw Crypto::exception::no_item ( "Crypto::x509::extension" );

  return _extension;
}

Crypto::x509::name::name ( libcrypto::X509_NAME * _name ) throw ()
: _name ( _name )
{ }

Crypto::x509::name::name ( const name & orig )
{
  _name = libcrypto::X509_NAME_dup ( orig._name );
}

Crypto::x509::name::~name () throw ()
{
  if ( _name )
    libcrypto::X509_NAME_free ( _name );
}

Crypto::x509::name & Crypto::x509::name::operator = ( const name & orig ) throw ( Crypto::exception::no_item, std::bad_alloc )
{
  if ( this != &orig )
  {
    if ( _name )
      libcrypto::X509_NAME_free ( _name );
    if ( ! orig._name )
      throw Crypto::exception::no_item ( "Crypto::x509::name" );

    _name = libcrypto::X509_NAME_dup ( orig._name );

    if ( ! _name )
    {
      Crypto::lib::get_error ();
      throw std::bad_alloc ();
    }
  }

  return * this;
}

void Crypto::x509::name::new_empty () throw ( std::bad_alloc )
{
  if ( _name )
    libcrypto::X509_NAME_free ( _name );

  _name = libcrypto::X509_NAME_new ();
}

void Crypto::x509::name::print ( std::ostream & stream ) const throw ( Crypto::exception::no_item )
{
  if ( ! _name )
    throw Crypto::exception::no_item ( "Crypto::x509::name" );

  char * buf = libcrypto::X509_NAME_oneline ( _name, NULL, 0 );
  stream << buf;
  free ( buf );
}

void Crypto::x509::name::add ( const std::string & text, const std::string & entry ) throw ( Crypto::exception::no_item )
{
  add ( text2nid ( text ), entry );
}

void Crypto::x509::name::add ( int nid, const std::string & entry ) throw ( Crypto::exception::no_item )
{
  if ( ! _name )
    throw Crypto::exception::no_item ( "Crypto::x509::name" );

  libcrypto::X509_NAME_add_entry_by_NID ( _name, nid, MBSTRING_ASC, ( unsigned char * ) entry.c_str (), -1, -1, 0 );
}

std::string Crypto::x509::name::get ( const std::string & text ) throw ( Crypto::exception::no_item )
{
  return get ( text2nid ( text ) );
}

std::string Crypto::x509::name::get ( int nid ) throw ( Crypto::exception::no_item )
{
  if ( ! _name )
    throw Crypto::exception::no_item ( "Crypto::x509::name" );

  char * buf = new char[256];
  libcrypto::X509_NAME_get_text_by_NID ( _name, nid, buf, 256 );
  std::string ret ( buf );
  delete [] buf;
  return ret;
}

Crypto::x509::name::operator libcrypto::X509_NAME * () throw ( Crypto::exception::no_item )
{
  if ( ! _name )
    throw Crypto::exception::no_item ( "Crypto::x509::name" );

  return _name;
}

int Crypto::x509::name::text2nid ( const std::string & text ) throw ()
{
  if ( text == "CN" )
    return NID_commonName;
  else if ( text == "C" )
    return NID_countryName;
  else if ( text == "L" )
    return NID_localityName;
  else if ( text == "ST" )
    return NID_stateOrProvinceName;
  else if ( text == "O" )
    return NID_organizationName;
  else if ( text == "OU" )
    return NID_organizationalUnitName;
  else if ( text == "Email" )
    return NID_pkcs9_emailAddress;
  throw;
}

Crypto::x509::req::operator libcrypto::X509_REQ * () throw ( Crypto::exception::no_item )
{
  if ( ! _req )
    throw Crypto::exception::no_item ( "Crypto::x509::req" );

  return _req;
}

Crypto::x509::store::store ( libcrypto::X509_STORE * _store ) throw ()
: _store ( _store )
{ }

Crypto::x509::store::~store () throw ()
{
  if ( _store )
    libcrypto::X509_STORE_free ( _store );
}

void Crypto::x509::store::new_empty () throw ( std::bad_alloc )
{
  if ( _store )
    libcrypto::X509_STORE_free ( _store );

  _store = libcrypto::X509_STORE_new ();
}

void Crypto::x509::store::add ( cert & cert ) throw ( Crypto::exception::no_item )
{
  if ( ! _store )
    throw Crypto::exception::no_item ( "Crypto::x509::store" );

  libcrypto::X509_STORE_add_cert ( _store, cert );
}

Crypto::x509::store::operator libcrypto::X509_STORE * () throw ( Crypto::exception::no_item )
{
  if ( ! _store )
    throw Crypto::exception::no_item ( "Crypto::x509::store" );

  return _store;
}


