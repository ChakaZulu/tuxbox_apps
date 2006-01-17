CXXTEST_FLAGS = --error-printer
SUFFIXES = .cxxtest

AM_CPPFLAGS = -I$(top_srcdir)/include

if HAVE_CXXTEST
bin_PROGRAMS = test_si_descriptors
endif

test_si_descriptors_SOURCES = test_si_descriptors.cxxtest
test_si_descriptors_LDADD = $(top_srcdir)/src/.libs/libdvbsi++.a

.cxxtest.cpp:
	@echo "$(CXXTEST) $(CXXTEST_FLAGS) $< -o $@"; \
	$(CXXTEST) $(CXXTEST_FLAGS) $< -o $@;
