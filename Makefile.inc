include $(srcdir)/sim/Makefile.inc
include $(srcdir)/arch/Makefile.inc
include $(srcdir)/cli/Makefile.inc
include $(srcdir)/demo/Makefile.inc
include $(srcdir)/Makefile.cacti.inc

bin_PROGRAMS += mgsim mgsim-dyn
if ENABLE_MEM_SERIAL
bin_PROGRAMS += tinysim tinysim-dyn
endif
dist_man1_MANS += mgsim.1

BASE_CXXFLAGS = $(AM_CXXFLAGS)
MGSIM_CXXFLAGS = $(BASE_CXXFLAGS) $(WARN_CXXFLAGS)
MGSIM_CPPFLAGS = $(AM_CPPFLAGS) -I$(srcdir)
noinst_LIBRARIES += libmgsim.a libmgsim-dyn.a

libmgsim_dyn_a_SOURCES = $(SIM_SOURCES) $(ARCH_SOURCES)
libmgsim_dyn_a_CPPFLAGS = $(SIM_EXTRA_CPPFLAGS) $(ARCH_EXTRA_CPPFLAGS) $(MGSIM_CPPFLAGS)
libmgsim_dyn_a_CXXFLAGS = $(SIM_EXTRA_CXXFLAGS) $(ARCH_EXTRA_CXXFLAGS) $(MGSIM_CXXFLAGS)
libmgsim_a_SOURCES = $(libmgsim_dyn_a_SOURCES)
libmgsim_a_CPPFLAGS = $(libmgsim_dyn_a_CPPFLAGS) -DSTATIC_KERNEL=1
libmgsim_a_CXXFLAGS = $(libmgsim_dyn_a_CXXFLAGS)

mgsim_dyn_SOURCES = $(CLI_SOURCES)
mgsim_dyn_CPPFLAGS = $(CLI_EXTRA_CPPFLAGS) -DMGSIM_CONFIG_PATH=\"$(pkgdatadir)/config.ini\" $(MGSIM_CPPFLAGS)
mgsim_dyn_CXXFLAGS = $(CLI_EXTRA_CXXFLAGS) $(MGSIM_CXXFLAGS)
mgsim_dyn_LDADD = libmgsim-dyn.a

mgsim_SOURCES = $(mgsim_dyn_SOURCES)
mgsim_CPPFLAGS = $(mgsim_dyn_CPPFLAGS) -DSTATIC_KERNEL=1
mgsim_CXXFLAGS = $(mgsim_dyn_CXXFLAGS)
mgsim_LDADD = libmgsim.a

tinysim_dyn_SOURCES = $(DEMO_SOURCES)
tinysim_dyn_CPPFLAGS = $(MGSIM_CPPFLAGS) -DMGSIM_CONFIG_PATH=\"$(pkgdatadir)/config.ini\"
tinysim_dyn_CXXFLAGS = $(MGSIM_CXXFLAGS)
tinysim_dyn_LDADD = $(mgsim_dyn_LDADD)

tinysim_SOURCES = $(tinysim_dyn_SOURCES)
tinysim_CPPFLAGS = $(tinysim_dyn_CPPFLAGS) -DSTATIC_KERNEL=1
tinysim_CXXFLAGS = $(tinysim_dyn_CXXFLAGS)
tinysim_LDADD = $(mgsim_LDADD)

if ENABLE_CACTI
BASE_CXXFLAGS += $(PTHREAD_CFLAGS)
noinst_LIBRARIES += libmgsimcacti.a
nodist_libmgsimcacti_a_SOURCES = $(CACTI_SOURCES)
libmgsimcacti_a_CPPFLAGS = $(MGSIM_CPPFLAGS) $(CACTI_EXTRA_CPPFLAGS)
libmgsimcacti_a_CXXFLAGS = $(BASE_CXXFLAGS) # No warnings
MGSIM_CPPFLAGS += -DENABLE_CACTI=1
MGSIM_CXXFLAGS += $(CACTI_EXTRA_CXXFLAGS)
mgsim_LDADD += libmgsimcacti.a $(PTHREAD_LIBS)
mgsim_dyn_LDADD += libmgsimcacti.a $(PTHREAD_LIBS)
endif

if ENABLE_SDL
MGSIM_CPPFLAGS += -DUSE_SDL=1
MGSIM_CXXFLAGS += $(SDL_CFLAGS)
mgsim_LDADD += $(SDL_LIBS)
mgsim_dyn_LDADD += $(SDL_LIBS)
endif

##
## Pre-processed sources
##
PP_MACROS = sim/macros.p.h
EXTRA_DIST += $(PP_MACROS)
%.h: %.p.h $(PP_MACROS)
	$(AM_V_at)$(MKDIR_P) `dirname $@`
	$(AM_V_GEN) $(builddir)/tools/preproc -I $(srcdir) -I . $< -o $@

##
## Manual page rules
##
mgsim.1: mgsim
	$(AM_V_GEN)$(HELP2MAN) --no-discard-stderr -N --output=$@.tmp ./mgsim
	$(AM_V_at)if ! (test -r $(srcdir)/$@ && cmp $@.tmp $(srcdir)/$@) >/dev/null 2>&1; then mv -f $@.tmp $@; else rm -f $@.tmp; fi

MAINTAINERCLEANFILES += $(dist_man1_MANS)
