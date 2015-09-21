// -*- c++ -*-
#ifndef ISA_RV64_H
#define ISA_RV64_H

#ifndef PIPELINE_H
#error This file should be included in Pipeline.h
#endif

enum InstrFormat {
    IFORMAT_RTYPE,
    IFORMAT_ITYPE,
    IFORMAT_BTYPE,
    IFORMAT_UTYPE,
    IFORMAT_JTYPE,
    IFORMAT_INVALID
};

// FIXME: define more ISA constants here.

// Latch information for Pipeline
struct ArchDecodeReadLatch
{

    InstrFormat format;
    int32_t     displacement; // jump target for J-type instructions
    uint16_t    function; // opcode for R-type instructions
    uint16_t    opcode; // opcode for non-R-type instructions

    // FILL IN
    // Your code here.
    // FILL IN


    ArchDecodeReadLatch() :
        // NB: all latch fields should be initialized here.
        format(IFORMAT_ITYPE)
        {}
    virtual ~ArchDecodeReadLatch() {}
};

struct ArchReadExecuteLatch : public ArchDecodeReadLatch
{
    // FIXME: FILL ADDITIONAL READ-EXECUTE LATCHES HERE (IF NECESSARY)

    /* NB: the buffers in ArchDecodeReadLatch are automatically
       propagated to the read-execute latch. */
};

#endif
