/*
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
 
#ifndef __transport_protocol_descriptor_h__
#define __transport_protocol_descriptor_h__

#include "descriptor.h"

class OcTransport
{
	protected:
		unsigned remoteConnection			: 1;
		unsigned originalNetworkId			: 16;
		unsigned transportStreamId			: 16;
		unsigned serviceId				: 16;
		unsigned componentTag				: 8;

	public:
		OcTransport(const uint8_t * const buffer);

		uint8_t getRemoteConnection(void) const;
		uint16_t getOriginalNetworkId(void) const;
		uint16_t getTransportStreamId(void) const;
		uint16_t getServiceId(void) const;
		uint8_t getComponentTag(void) const;
};

class Url
{
	protected:
		unsigned urlLength				: 8;
		std::string url;

	public:
		Url(const uint8_t * const buffer);

		uint8_t getLength(void) const;
		const std::string &getUrl(void) const;
};

typedef std::vector<Url *> UrlVector;
typedef UrlVector::iterator UrlIterator;
typedef UrlVector::const_iterator UrlConstIterator;

class IpTransport
{
	protected:
		unsigned remoteConnection			: 1;
		unsigned originalNetworkId			: 16;
		unsigned transportStreamId			: 16;
		unsigned serviceId				: 16;
		unsigned alignmentIndicator			: 1;
		UrlVector urls;

	public:
		IpTransport(const uint8_t * const buffer, size_t length);
		~IpTransport(void);

		uint8_t getRemoteConnection(void) const;
		uint16_t getOriginalNetworkId(void) const;
		uint16_t getTransportStreamId(void) const;
		uint16_t getServiceId(void) const;
		uint8_t getAlignmentIndicator(void) const;
		const UrlVector *getUrls(void) const;
};

class InteractionTransport
{
	protected:
		Url *urlBase;
		UrlVector urlExtensions;

	public:
		InteractionTransport(const uint8_t * const buffer, size_t length);
		~InteractionTransport(void);

		const Url *getUrlBase(void) const;
		const UrlVector *getUrlExtensions(void) const;
};

class TransportProtocolDescriptor : public Descriptor
{
	protected:
		unsigned protocolId				: 16;
		unsigned transportProtocolLabel			: 8;
		OcTransport *ocTransport;
		IpTransport *ipTransport;
		InteractionTransport *interactionTransport;

	public:
		TransportProtocolDescriptor(const uint8_t * const buffer);
		~TransportProtocolDescriptor(void);

		uint16_t getProtocolId(void) const;
		uint8_t getTransportProtocolLabel(void) const;
		const OcTransport *getOcTransport(void) const;
		const IpTransport *getIpTransport(void) const;
		const InteractionTransport *getInteractionTransport(void) const;
};

#endif /* __transport_protocol_descriptor_h__ */
