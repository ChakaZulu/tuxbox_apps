/*
 * capmt.cpp
 *
 * dump every capmt of the current transponder
 *
 * Copyright (C) 2003 by Andreas Oberritter <obi@saftware.de>
 *
 * See COPYING in the top level directory for license details.
 */

#include <dvb/hardware/section_filter.h>
#include <dvbsi++/ca_program_map_section.h>
#include <dvbsi++/program_association_section.h>
#include <dvbsi++/program_map_section.h>

static void hexdump(unsigned char *buf, size_t len)
{
	for (size_t i = 0; i < len; ++i)
		printf("%02x ", buf[i]);
	printf("(%d)\n", len);
}

static void capmt2buf(CaProgramMapSection *capmt)
{
	size_t len;
	unsigned char buf[1024];

	len = capmt->writeToBuffer(buf);

	hexdump(buf, len);
}

static void pmt2capmt(ProgramMapSection *pmt, int first, int last)
{
	CaProgramMapSection capmt(pmt, first + (2 * last), 1);
	capmt2buf(&capmt);
}

static void process_pmt(const uint16_t program_number,
			const uint16_t program_map_pid)
{
	printf("program_number=%04x program_map_pid=%04x\n",
		program_number, program_map_pid);

	SectionFilter<ProgramMapSection> pmt;
	pmt.setTableIdExtension(program_number);
	pmt.filter(program_map_pid);

	while (!pmt.isDone())
		usleep(0);

	if (pmt.isTimedOut()) {
		printf("pmt timed out\n");
		return;
	}

	printf("pmt has %d section(s)\n", pmt.getSections()->size());

	ProgramMapSectionConstIterator pmtci;
	for (pmtci = pmt.getSections()->begin(); pmtci != pmt.getSections()->end(); ++pmtci)
		pmt2capmt(*pmtci, pmtci == pmt.getSections()->begin(), pmtci == pmt.getSections()->end() - 1);
}

int main(void)
{
	SectionFilter<ProgramAssociationSection> pat;
	pat.filter();

	while (!pat.isDone())
		usleep(0);

	if (pat.isTimedOut()) {
		printf("pat timed out\n");
		return 1;
	}

	ProgramAssociationSectionConstIterator patci;
	ProgramAssociationConstIterator paci;
	for (patci = pat.getSections()->begin(); patci != pat.getSections()->end(); ++patci)
		for (paci = (*patci)->getPrograms()->begin(); paci != (*patci)->getPrograms()->end(); ++paci)
			process_pmt((*paci)->getProgramNumber(), (*paci)->getProgramMapPid());

	return 0;
}

