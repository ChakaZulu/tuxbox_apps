/*
 * $Id: dvb_j_application_descriptor.cpp,v 1.3 2005/10/29 00:10:16 obi Exp $
 *
 * Copyright (C) 2004-2005 Stéphane Esté-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#include <dvbsi++/dvb_j_application_descriptor.h>

DvbJApplication::DvbJApplication(const uint8_t * const buffer)
{
	parameterLength = buffer[0];
	parameter.assign((char *)&buffer[1], parameterLength);
}

uint8_t DvbJApplication::getParameterLength(void) const
{
	return parameterLength;
}

const std::string &DvbJApplication::getParameter(void) const
{
	return parameter;
}

DvbJApplicationDescriptor::DvbJApplicationDescriptor(const uint8_t * const buffer) : Descriptor(buffer)
{
	for (size_t i = 0; i < descriptorLength; ) {
		DvbJApplication *dvbJApplication = new DvbJApplication(&buffer[i + 2]);
		dvbJApplications.push_back(dvbJApplication);
		i += dvbJApplication->getParameterLength() + 1;
	}
}

DvbJApplicationDescriptor::~DvbJApplicationDescriptor	(void)
{
	for (DvbJApplicationIterator i = dvbJApplications.begin(); i != dvbJApplications.end(); ++i)
		delete *i;
}

const DvbJApplicationList *DvbJApplicationDescriptor::getDvbJApplications(void) const
{
	return &dvbJApplications;
}
