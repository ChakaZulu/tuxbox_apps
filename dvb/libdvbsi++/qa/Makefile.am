CXXTEST = cxxtestgen.pl
CXXTEST_FLAGS = --error-printer
SUFFIXES = .cxxtest

AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = test_si_descriptors

test_si_descriptors_SOURCES = test_si_descriptors.cxxtest
test_si_descriptors_LDADD = $(top_srcdir)/src/.libs/libdvbsi++.a

.cxxtest.cpp:
	@echo "$(CXXTEST) $(CXXTEST_FLAGS) $< -o $@"; \
	$(CXXTEST) $(CXXTEST_FLAGS) $< -o $@;
