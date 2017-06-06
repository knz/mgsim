// -*- c++ -*-
#ifndef FAMILYTABLE_H
#define FAMILYTABLE_H

#include <sim/kernel.h>
#include <sim/inspect.h>
#include <sim/sampling.h>
#include <arch/simtypes.h>
#include <vector>
#include "forward.h"

namespace Simulator
{
namespace leon2mt
{

struct Family
{

    struct RegInfo {
      RegIndex     grb;            // Global register base
      RegIndex     goff;           // Global register offset
    };

    PSize        placeSize;      // Number of cores this family wanted to run on.
    PSize        numCores;       // Number of cores this family is actually running on (1 <= numCores <= placeSize).
    FCapability  capability;     // Capability value for security
    MemAddr      pc;             // Initial PC for newly created threads
    MASK         gmask;          // Register partitioning mask
    TID          htg_base;       // Ptr to free threads allocated to this family
    TID          listener;       // listener thread (t.wait)
    RegIndex     grb;            // Global register base
    RegIndex     goff;           // Global register offset
    bool         dF, dR, dT;     // deallocation flags (f.fence)

    BIDX         bx, by, bw, bh; // Block width/height
    GIDX         gx, gy, gw, gh; // Grid width/height
    union {
        Integer  nThreads;       // Number of threads we still need to run (on this core)
    };
    LFID         link;           // The LFID of the matching family on the next CPU (prev during allocate)
    bool         broken;         // Family terminated due to break

    struct
    {
        PID      pid;            // The core that's synchronising
        RegIndex reg;            // The exit code register on the core
        bool     done;           // Whether the family is done or not
    }            sync;           // Synchronisation information

    RegInfo      regs[NUM_REG_TYPES];    // Register information
    FamilyState2 state;          // Family state

};

class FamilyTable : public Object, public Inspect::Interface<Inspect::Read>
{
public:
    FamilyTable(const std::string& name, LEON2MT& parent);

    FSize GetNumFamilies() const { return m_families.size(); }

    Family& operator[](LFID fid)       { return m_families[fid]; }
    const Family& operator[](LFID fid) const { return m_families[fid]; }

    LFID  AllocateFamily(ContextType type);
    void  FreeFamily(LFID fid, ContextType context);

    FSize GetNumFreeFamilies(ContextType type) const;
    FSize GetNumUsedFamilies(ContextType type) const;
    bool  IsEmpty()             const;
    bool  IsExclusive(LFID fid) const { return fid + 1 == m_families.size(); }
    bool  IsExclusiveUsed()     const { return m_free[CONTEXT_EXCLUSIVE] == 0; }

    // Admin functions
    const std::vector<Family>& GetFamilies() const { return m_families; }

    void Cmd_Info(std::ostream& out, const std::vector<std::string>& arguments) const;
    void Cmd_Read(std::ostream& out, const std::vector<std::string>& arguments) const;

    // Stats
    FSize GetTotalAllocated() { UpdateStats(); return m_totalalloc; }
    TSize GetMaxAllocated() const { return m_maxalloc; }

private:
    Object& GetLEON2MTParent() const { return *GetParent(); }
    std::vector<Family> m_families;
    FSize               m_free[NUM_CONTEXT_TYPES];

    // Admin
    DefineSampleVariable(CycleNo, lastcycle);
    DefineSampleVariable(FSize, totalalloc);
    DefineSampleVariable(FSize, maxalloc);
    DefineSampleVariable(FSize, curalloc);

    void UpdateStats();
    void CheckStateSanity() const;
};

}
}

#endif
