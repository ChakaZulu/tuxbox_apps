/*
 * x509.hpp: c++ wrapper for openssl x509
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
 * $Id: x509.hpp,v 1.1 2002/03/02 21:53:26 waldi Exp $
 */

#ifndef __LIBCRYPTO__X509_HPP
#define __LIBCRYPTO__X509_HPP

#include <iostream>

#include <evp.hpp>
#include <exception.hpp>

namespace libcrypto
{
  #include <openssl/x509.h>
  #include <openssl/x509v3.h>
  #include <openssl/x509_vfy.h>
}

namespace Crypto
{
  namespace x509
  {
    class extension;
    class name;
    class req;
    class store;

    class cert
    {
      public:
        cert ( libcrypto::X509 * = NULL ) throw ();
        cert ( const cert & );
        ~cert () throw ();
        cert & operator = ( const cert & );

        void new_empty ();

        void read ( std::istream & );
        void write ( std::ostream & ) const;
        void print ( std::ostream & ) const;

        long get_version () const throw ( Crypto::exception::no_item );
        void set_version ( long ) throw ( Crypto::exception::no_item );

        long get_serialNumber () const throw ( Crypto::exception::no_item );
        void set_serialNumber ( long ) throw ( Crypto::exception::no_item );

        name get_issuer_name () const throw ( Crypto::exception::no_item );
        void set_issuer_name ( name & ) throw ( Crypto::exception::no_item );
        name get_subject_name () const throw ( Crypto::exception::no_item );
        void set_subject_name ( name & ) throw ( Crypto::exception::no_item );

        std::string get_notBefore () const throw ( Crypto::exception::no_item );
        void set_notBefore ( const std::string & ) throw ( Crypto::exception::no_item );
        void set_notBefore ( const long ) throw ( Crypto::exception::no_item );
        std::string get_notAfter () const throw ( Crypto::exception::no_item );
        void set_notAfter ( const std::string & ) throw ( Crypto::exception::no_item );
        void set_notAfter ( const long ) throw ( Crypto::exception::no_item );

        Crypto::evp::key::key get_publickey () const throw ( Crypto::exception::no_item );
        void set_publickey ( Crypto::evp::key::key & ) throw ( Crypto::exception::no_item );

        void add_extension ( extension & ) throw ( Crypto::exception::no_item );

        void sign ( Crypto::evp::key::privatekey &, Crypto::evp::md::md & ) throw ( Crypto::exception::no_item );
        int verify ( store & ) throw ( std::bad_alloc, Crypto::exception::no_item );

      protected:
        operator libcrypto::X509 * () throw ( Crypto::exception::no_item );
        static int verify_callback ( int ok, libcrypto::X509_STORE_CTX * ctx ) throw ();

        libcrypto::X509 * _cert;

        friend class ctx;
        friend class store;
    };

    class crl
    {
      public:
        crl () throw ();
        ~crl () throw ();

        void sign ( Crypto::evp::key::privatekey & ) throw ( Crypto::exception::no_item );
        bool verify ( Crypto::evp::key::key & ) throw ( Crypto::exception::no_item );

      protected:
        operator libcrypto::X509_CRL * () throw ( Crypto::exception::no_item );

        libcrypto::X509_CRL * _crl;

        friend class ctx;
    };

    class ctx
    {
      public:
        void set ( cert & ) throw ();
        void set ( cert &, cert & ) throw ();
        void set ( cert &, cert &, req & ) throw ();
        void set ( req & ) throw ();
        void set ( crl & ) throw ();
        void set ( cert &, crl & ) throw ();

      protected:
        operator libcrypto::X509V3_CTX * () throw ();

        libcrypto::X509V3_CTX _ctx;

        friend class extension;
    };

    class extension
    {
      public:
        extension () throw ();
        ~extension () throw ();

        void create ( ctx &, const std::string &, const std::string & );
        void create ( const std::string &, const std::string & );
        void create ( ctx &, int, const std::string & );
        void create ( int, const std::string & );

      protected:
        operator libcrypto::X509_EXTENSION * () throw ( Crypto::exception::no_item );

        libcrypto::X509_EXTENSION * _extension;

        friend class cert;
        friend class crl;
        friend class req;
    };

    class name
    {
      public:
        name ( libcrypto::X509_NAME * = NULL ) throw ();
        name ( const name & );
        ~name () throw ();
        name & operator = ( const name & ) throw ( Crypto::exception::no_item, std::bad_alloc );

        void new_empty () throw ( std::bad_alloc );

        void print ( std::ostream & ) const throw ( Crypto::exception::no_item );

        void add ( const std::string &, const std::string & ) throw ( Crypto::exception::no_item );
        void add ( int, const std::string & ) throw ( Crypto::exception::no_item );
        std::string get ( const std::string & ) throw ( Crypto::exception::no_item );
        std::string get ( int ) throw ( Crypto::exception::no_item );

      protected:
        operator libcrypto::X509_NAME * () throw ( Crypto::exception::no_item );
        int text2nid ( const std::string & ) throw ();

        libcrypto::X509_NAME * _name;

        friend class cert;
    };

    class req
    {
      public:
        req () throw ();
        ~req () throw ();

        void sign ( Crypto::evp::key::privatekey & ) throw ( Crypto::exception::no_item );
        bool verify ( Crypto::evp::key::key & ) throw ( Crypto::exception::no_item );

      protected:
        operator libcrypto::X509_REQ * () throw ( Crypto::exception::no_item );

        libcrypto::X509_REQ * _req;

        friend class ctx;
    };

    class revoked
    {
      public:
        revoked () throw ();
        ~revoked () throw ();

      protected:
        libcrypto::X509_REVOKED * _revoked;
    };

    class store
    {
      public:
        store ( libcrypto::X509_STORE * = NULL ) throw ();
        ~store () throw ();

        void new_empty () throw ( std::bad_alloc );

        void add ( cert & ) throw ( Crypto::exception::no_item );
        void add ( crl & ) throw ( Crypto::exception::no_item );

      protected:
        operator libcrypto::X509_STORE * () throw ( Crypto::exception::no_item );

        libcrypto::X509_STORE * _store;

      private:
        store ( const store & );

        friend class cert;
    };
  };
};

#endif
