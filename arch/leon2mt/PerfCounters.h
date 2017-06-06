// -*- c++ -*-
#ifndef PERFCOUNTERS_H
#define PERFCOUNTERS_H

#include "IOMatchUnit.h"
#include <sim/sampling.h>

namespace Simulator
{
namespace leon2mt
{

class PerfCounters : public leon2mt::MMIOComponent
{
    class Helpers;
    friend class Simulator::LEON2MT;

    std::vector<Integer (*)(LEON2MT&, LFID)> m_counters;
    DefineSampleVariable(uint64_t, nCycleSampleOps); // nr of samplings of the cycle counter by program
    DefineSampleVariable(uint64_t, nOtherSampleOps); // nr of samplings of other counters

    Object& GetLEON2MTParent() const { return *GetParent(); }
public:

    size_t GetSize() const;

    Result Read (MemAddr address, void* data, MemSize size, LFID fid, TID tid, const RegAddr& writeback);
    Result Write(MemAddr address, const void* data, MemSize size, LFID fid, TID tid);

    PerfCounters(LEON2MT& parent);

    ~PerfCounters() {}
};

}
}

#endif
