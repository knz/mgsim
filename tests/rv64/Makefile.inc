RV64_CRT = \
    tests/rv64/crt_simple.s

COMMON_RV64_TEST_SOURCES = \
    tests/rv64/simple.s 

EXTRA_DIST += $(COMMON_RV64_TEST_SOURCES) $(RV64_CRT)

# OpenRISC test selection
RV64_TEST_BINS = $(COMMON_RV64_TEST_SOURCES:.s=.rv64-bin)
RV64_CTEST_BINS = $(COMPILED_TEST_SOURCES:.c=.rv64-bin)

#if ENABLE_COMPILED_RISCV64_TESTS
#TEST_BINS += $(RV64_CTEST_BINS)
#endif

if ENABLE_RISCV64_TESTS
TEST_BINS += $(RV64_TEST_BINS)
endif

SUFFIXES += .rv64-o .rv64-bin

.s.rv64-o:
	$(AM_V_at)$(MKDIR_P) `dirname "$@"`
	$(AM_V_GEN)$(AS_RISCV64) -o $@ `test -f "$<" || echo "$(srcdir)"/`$<

.rv64-o.rv64-bin:
	$(AM_V_at)$(MKDIR_P) `dirname "$@"`
	$(AM_V_GEN)$(LD_RISCV64) -e main -Ttext 0x10000 -o $@ `test -f "$<" || echo "$(srcdir)"/`$<

.c.rv64-bin: $(COMMON_SLC_DEPS)
	$(MKDIR_P) `dirname "$@"`
	$(AM_V_GEN)$(SLC) -b rv64_s -o $@ $(srcdir)/tests/rv64/crt_simple.s `test -f "$<" || echo "$(srcdir)"/`$< $(COMMON_SLC_OPTIONS)


