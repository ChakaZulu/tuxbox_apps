//=============================================================================
// YHTTPD
// Global System Configuration
//=============================================================================

//-----------------------------------------------------------------------------
#ifndef __yconfig_h__
#define __yconfig_h__

// c++
#include <string>
#include <map>
#include <vector>

//-----------------------------------------------------------------------------
// TODO: comment it, if CVS checkin
//#define Y_UPDATE_BETA y
#ifdef Y_UPDATE_BETA
#warning "!!!! THIS IS BETA COMPILING. SWITCH OF (Y_UPDATE_BETA) FOR CVS"
#define Y_CONFIG_USE_WEBLOG y
#define Y_CONFIG_FEATURE_KEEP_ALIVE y
#endif
//-----------------------------------------------------------------------------
// System Choice <configure!> ONE choice
//-----------------------------------------------------------------------------
#ifndef CONFIG_SYSTEM_BY_COMPILER 				// use Compiler directive to set CONFIG_SYSTEM
#define CONFIG_SYSTEM_TUXBOX	y				// Tuxbox dbox project
//#define CONFIG_SYSTEM_AVM	y				// AVM FritzBox
//#define CONFIG_SYSTEM_MSS	y				// Maxtor Shared Storage
//#define CONFIG_SYSTEM_LINUX	y				// Normal Linux PC
//#define CONFIG_SYSTEM_CYGWIN	y				// Windows (Cygwin)
#endif
//-----------------------------------------------------------------------------
// General central Definitions <configure!>
//-----------------------------------------------------------------------------
#define HTTPD_VERSION 		"3.0.1"				// Webserver version  (can be overloaded)
#define YHTTPD_VERSION 		"1.0.3"				// Webserver version  (Version of yhttpd-core!)
#define IADDR_LOCAL 		"127.0.0.1"			// local IP
#define HTTPD_NAME 		"yhttpd"			// Webserver name (can be overloaded)
#define YHTTPD_NAME 		"yhttpd_core"			// Webserver name (Name of yhttpd-core!)
#define AUTH_NAME_MSG		"yhhtpd"			// Name in Authentication Dialogue

//=============================================================================
// Features wanted <configure!>
//=============================================================================
#define Y_CONFIG_FEATURE_CHECK_PORT_AUTORITY y			// System: Port < 1024 need Admin-Privileges-Check
#define Y_CONFIG_HAVE_SENDFILE y				// System: Have *IX SendFile
#define Y_CONFIG_FEATURE_UPLOAD y				// Add Feature: File Upload POST Command
#define Y_CONFIG_USE_HOSTEDWEB y				// Add Feature: Use HOSTED Web
#define Y_CONFIG_USE_YPARSER y					// Add mod: "y-Parsing"
//#define Y_CONFIG_USE_TESTHOOK y				// Add mod: "Test-Hook" (hook example)
#define Y_CONFIG_USE_AUTHHOOK y					// Add mod: "Authentication"
//#define Y_CONFIG_USE_WEBLOG y					// Add mod: "WebLogging"
// TODO build error page function
//#define Y_CONFIG_HAVE_HTTPD_ERRORPAGE y			// Have an HTTPD Error Page
#define Y_CONFIG_FEATURE_SHOW_SERVER_CONFIG y			// Add Feature (in yParser): add /y/server-config
//#define Y_CONFIG_USE_OPEN_SSL y					// Add Feature: use openSSL
#define Y_CONFIG_FEATURE_KEEP_ALIVE y				// Add Feature: Keep-alive //FIXME: does not work correctly now

//-----------------------------------------------------------------------------
// Define/Undefine Features forced by CONFIG_SYSTEM_xxx
// Dependencies
//-----------------------------------------------------------------------------
#ifdef Y_CONFIG_USE_OPEN_SSL
#undef Y_CONFIG_HAVE_SENDFILE					// Sendfile does not work for SSL
#endif

#ifdef CONFIG_SYSTEM_CYGWIN
#undef Y_CONFIG_FEATURE_CHECK_PORT_AUTORITY
#undef Y_CONFIG_HAVE_SENDFILE					// No Sendfile under cygwin
#endif
#ifdef CONFIG_SYSTEM_TUXBOX
#define Y_CONFIG_FEATURE_UPLOAD y
#define Y_CONFIG_USE_YPARSER y
#define Y_CONFIG_USE_AUTHHOOK y
#endif
//=============================================================================
// Configurations for systems/OSs <configure!>
//=============================================================================
//-----------------------------------------------------------------------------
// Configurations for LINUX
//-----------------------------------------------------------------------------
#ifdef CONFIG_SYSTEM_LINUX
#define HTTPD_STANDARD_PORT		8080
#define HTTPD_MAX_CONNECTIONS		3
#define HTTPD_CONFIGDIR 		"/home/y"
#define HTTPD_CONFIGFILE HTTPD_CONFIGDIR "/yhttpd.conf"
#define HTTPD_REQUEST_LOG 		"/tmp/httpd_log"	//TODO: delete every occurence
#define HTTPD_ERRORPAGE			"/Y_ErrorPage.yhtm"

#define AUTHUSER			"test"
#define AUTHPASSWORD			"test1"
#define PRIVATEDOCUMENTROOT		"/home/y/nhttpd-y"
#define PUBLICDOCUMENTROOT		"/var/httpd"
#define HOSTEDDOCUMENTROOT		"/mnt/hosted"

#define SSL_PEMFILE			HTTPD_CONFIGDIR "/server.pem"
#define SSL_CA_FILE			HTTPD_CONFIGDIR "/cacert.pem"

#define UPLOAD_TMP_FILE 		"/tmp/upload.tmp"
#endif
//-----------------------------------------------------------------------------
// Configurations for WINDOWS (Cygwin)
//-----------------------------------------------------------------------------
#ifdef CONFIG_SYSTEM_CYGWIN
#define HTTPD_STANDARD_PORT		80
#define HTTPD_MAX_CONNECTIONS		30
#define HTTPD_CONFIGDIR 		"/usr/local/yweb"
#define HTTPD_CONFIGFILE HTTPD_CONFIGDIR "/yhttpd.conf"
#define HTTPD_REQUEST_LOG 		"/tmp/httpd_log"
#define HTTPD_ERRORPAGE			"/Y_ErrorPage.yhtm"

#define AUTHUSER			"test"
#define AUTHPASSWORD			"test1"
#define PRIVATEDOCUMENTROOT		"/cygdrive/d/Work/y/ws/nhttpd30/web"
#define PUBLICDOCUMENTROOT		"/var/httpd"
#define HOSTEDDOCUMENTROOT		"/mnt/hosted"

#define SSL_PEMFILE			HTTPD_CONFIGDIR "/server.pem"
#define SSL_CA_FILE			HTTPD_CONFIGDIR "/cacert.pem"

#define LOG_FILE			"./yhhtpd.log"
#define LOG_FORMAT			"CLF"

#define UPLOAD_TMP_FILE 		"/tmp/upload.tmp"
#endif
//-----------------------------------------------------------------------------
// Configurations for LINUX (Tuxbox dbox2)
//-----------------------------------------------------------------------------
#ifdef CONFIG_SYSTEM_TUXBOX
#undef HTTPD_NAME
#define HTTPD_NAME 			"nhttpd"
#undef HTTPD_VERSION
#define HTTPD_VERSION 			"3.0.1"
#define HTTPD_STANDARD_PORT		80
#define HTTPD_MAX_CONNECTIONS		10
#define HTTPD_CONFIGDIR 		"/var/tuxbox/config"
#define HTTPD_CONFIGFILE HTTPD_CONFIGDIR "/nhttpd.conf"
#define HTTPD_REQUEST_LOG 		"/tmp/httpd_log"
#define HTTPD_ERRORPAGE			"/Y_ErrorPage.yhtm"

#define AUTHUSER			"root"
#define AUTHPASSWORD			"dbox2"
#define PRIVATEDOCUMENTROOT		"/share/tuxbox/neutrino/httpd-y"
#define PUBLICDOCUMENTROOT		"/var/httpd"
#define NEUTRINO_CONFIGFILE		"/var/tuxbox/config/neutrino.conf"
#define HOSTEDDOCUMENTROOT		"/mnt/hosted"
#define EXTRASDOCUMENTROOT		"/mnt/hosted/extras"
#define EXTRASDOCUMENTURL		"/hosted/extras"
#define ZAPITXMLPATH			"/var/tuxbox/config/zapit"

#define SSL_PEMFILE			HTTPD_CONFIGDIR "/server.pem"
#define SSL_CA_FILE			HTTPD_CONFIGDIR "/cacert.pem"

#define LOG_FILE			"/tmp/yhhtpd.log"
#define LOG_FORMAT			"CLF"

#define UPLOAD_TMP_FILE 		"/tmp/upload.tmp"
#endif
//-----------------------------------------------------------------------------
// Configurations for AVM FritzBox
//-----------------------------------------------------------------------------
#ifdef CONFIG_SYSTEM_AVM
#define HTTPD_STANDARD_PORT		81
#define HTTPD_MAX_CONNECTIONS		3
#define HTTPD_CONFIGDIR 		"/tmp"
#define HTTPD_CONFIGFILE HTTPD_CONFIGDIR "/yhttpd.conf"
#define HTTPD_REQUEST_LOG 		"/tmp/httpd_log"
#define HTTPD_ERRORPAGE			"/Y_ErrorPage.yhtm"

#define AUTHUSER			"root"
#define AUTHPASSWORD			"oxmox"
#define PRIVATEDOCUMENTROOT		"/tmp/web" //FIXME: Test
#define PUBLICDOCUMENTROOT		"/var/httpd"
#define HOSTEDDOCUMENTROOT		"/mnt/hosted"

#define SSL_PEMFILE			HTTPD_CONFIGDIR "/server.pem"
#define SSL_CA_FILE			HTTPD_CONFIGDIR "/cacert.pem"

#define UPLOAD_TMP_FILE 		"/tmp/upload.tmp"
#endif

//-----------------------------------------------------------------------------
// Configurations for Maxtor Shared Storage
//-----------------------------------------------------------------------------
#ifdef CONFIG_SYSTEM_MSS
#define HTTPD_STANDARD_PORT		81
#define HTTPD_MAX_CONNECTIONS		3
#define HTTPD_CONFIGDIR 		"/tmp"
#define HTTPD_CONFIGFILE HTTPD_CONFIGDIR "/yhttpd.conf"
#define HTTPD_REQUEST_LOG 		"/tmp/httpd_log"
#define HTTPD_ERRORPAGE			"/Y_ErrorPage.yhtm"

#define AUTHUSER			"root"
#define AUTHPASSWORD			"oxmox"
#define PRIVATEDOCUMENTROOT		"/shares/mss-hdd/Install/web"
#define PUBLICDOCUMENTROOT		"/var/httpd"
#define HOSTEDDOCUMENTROOT		"/mnt/hosted"

#define SSL_PEMFILE			HTTPD_CONFIGDIR "/server.pem"
#define SSL_CA_FILE			HTTPD_CONFIGDIR "/cacert.pem"

#define UPLOAD_TMP_FILE 		"/tmp/upload.tmp"
#endif


//-----------------------------------------------------------------------------
// Aggregated definitions
//-----------------------------------------------------------------------------
#define WEBSERVERNAME HTTPD_NAME "/" HTTPD_VERSION "(" YHTTPD_NAME "/" YHTTPD_VERSION ")"

#endif // __yconfig_h__
