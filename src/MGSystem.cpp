#include "MGSystem.h"

#ifdef ENABLE_COMA_ZL
# include "coma/ZLCOMA.h"
# include "coma/simlink/th.h"
# include "coma/simlink/linkmgs.h"
#else
# include "SerialMemory.h"
# include "ParallelMemory.h"
# include "BankedMemory.h"
# include "RandomBankedMemory.h"
# include "coma/COMA.h"
#endif

#include "loader.h"

// For Tokenize:
#include "simreadline.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>

using namespace Simulator;
using namespace std;

// WriteConfiguration()

// This function exposes the Microgrid configuration to simulated
// programs through the memory interface. The configuration data
// starts at the address given by local register 2 in the first
// thread created on the system.

// The configuration is composed of data blocks, each block
// composed of 32-bit words. The first word in each block is a
// "block type/size" tag indicating the type and size of the
// current block. 

// A tag set to 0 indicates the end of the configuration data. 
// A non-zero tag is a word with bits 31-16 set to the block type
// and bits 15-0 are set to the block size (number of words to
// next block).
// From each tag, the address of the tag for the next block is (in
// words) A_next = A_cur + size.

// Current block types:

// - architecture type v1, organized as follows:
//     - word 0 after tag: architecture model (1 for simulated microgrid, others TBD)
//     - word 1: ISA type (1 = alpha, 2 = sparc)
//     - word 2: number of FPUs (0 = no FPU)
//     - word 3: memory type (0 = unknown, for other values see enum MEMTYPE in MGSystem.h)

// - timing words v1, organized as follows:
//     - word 0 after tag: core frequency (in MHz)
//     - word 1 after tag: external memory bandwidth (in 10^6bytes/s)

// - cache parameters words v1, organized as follows:
//     - word 0 after tag: cache line size (in bytes)
//     - word 1: L1 I-cache size (in bytes)
//     - word 2: L1 D-cache size (in bytes)
//     - word 3: number of L2 caches 
//     - word 4: L2 cache size per cache (in bytes)

// - concurrency resource words v1, organized as follows:
//     - word 0 after tag: number of family entries per core
//     - word 1: number of thread entries per core
//     - word 2: number of registers in int register file
//     - word 3: number of registers in float register file

// - place layout v1, organized as follows:
//     - word 0 after tag: number of cores (P)
//     - word 1 ... P: (ringID << 16) | (coreID << 0) 
//     (future layout configuration formats may include topology information)

#define CONFTAG_ARCH_V1    1
#define CONFTAG_TIMINGS_V1 2
#define CONFTAG_CACHE_V1   3
#define CONFTAG_CONC_V1    4
#define CONFTAG_LAYOUT_V1  5
#define MAKE_TAG(Type, Size) (uint32_t)(((Type) << 16) | ((Size) & 0xffff))


void MGSystem::FillConfWords(ConfWords& words) const
{
    uint32_t cl_sz = m_config.getInteger<uint32_t>("CacheLineSize", 0);

    // configuration words for architecture type v1

    words << MAKE_TAG(CONFTAG_ARCH_V1, 4)
          << 1 // simulated microgrid
#if TARGET_ARCH == ARCH_ALPHA
          << 1 // alpha
#else
          << 2 // sparc
#endif    
          << m_fpus.size()
          << m_memorytype

    
    // timing words v1

          << MAKE_TAG(CONFTAG_TIMINGS_V1, 2)
          << m_config.getInteger<uint32_t>("CoreFreq", 0)
          << ((m_memorytype == MEMTYPE_COMA_ZL || m_memorytype == MEMTYPE_COMA_ML) ? 
              m_config.getInteger<uint32_t>("DDRMemoryFreq", 0) 
              * m_config.getInteger<uint32_t>("NumRootDirectories", 0)
              * 3 /* DDR3 = triple rate */ 
              * 8 /* 64 bit = 8 bytes per transfer */
              : 0 /* no timing information if memory system is not COMA */)
        
    // cache parameter words v1
    
          << MAKE_TAG(CONFTAG_CACHE_V1, 5)
          << cl_sz
          << (cl_sz 
              * m_config.getInteger<uint32_t>("ICacheAssociativity", 0)
              * m_config.getInteger<uint32_t>("ICacheNumSets", 0))
          << (cl_sz
              * m_config.getInteger<uint32_t>("DCacheAssociativity", 0)
              * m_config.getInteger<uint32_t>("DCacheNumSets", 0));
    if (m_memorytype == MEMTYPE_COMA_ZL || m_memorytype == MEMTYPE_COMA_ML)
        words << (m_procs.size()
                  / m_config.getInteger<uint32_t>("NumProcessorsPerCache", 0)) // FIXME: COMA?
              << (cl_sz
                  * m_config.getInteger<uint32_t>("L2CacheAssociativity", 0)
                  * m_config.getInteger<uint32_t>("L2CacheNumSets", 0));
    else
        words << 0 << 0;

    // concurrency resources v1
    
    words << MAKE_TAG(CONFTAG_CONC_V1, 4)
          << m_config.getInteger<uint32_t>("NumFamilies", 0)
          << m_config.getInteger<uint32_t>("NumThreads", 0)
          << m_config.getInteger<uint32_t>("NumIntRegisters", 0)
          << m_config.getInteger<uint32_t>("NumFltRegisters", 0)

        ;

    // place layout v1

    const vector<PSize>& placeSizes = m_config.getIntegerList<PSize>("NumProcessors");

    words << MAKE_TAG(CONFTAG_LAYOUT_V1, m_procs.size() + 1)
          << m_procs.size();

    // Store the cores, per place
    PSize first = 0;
    for (size_t p = 0; p < placeSizes.size(); ++p)
    {
        PSize placeSize = placeSizes[p];
        for (size_t i = 0; i < placeSize; ++i)
        {
            PSize pid = first + i;
            words << ((p << 16) | (pid << 0));
        }
        first += placeSize;
    }

    // after last block
    words << 0 << 0;

}

MemAddr MGSystem::WriteConfiguration()
{

    ConfWords words;
    FillConfWords(words);

    MemAddr base;
    if (!m_memory->Allocate(words.data.size() * sizeof words.data[0], IMemory::PERM_READ, base))
    {
        throw runtime_error("Unable to allocate memory to store configuration data");
    }
    m_memory->Write(base, &words.data[0], words.data.size() * sizeof words.data[0]);
    return base;
}

uint64_t MGSystem::GetOp() const
{
    uint64_t op = 0;
    for (size_t i = 0; i < m_procs.size(); ++i) {
        op += m_procs[i]->GetPipeline().GetOp();
    }
    return op;
}

uint64_t MGSystem::GetFlop() const
{
    uint64_t flop = 0;
    for (size_t i = 0; i < m_procs.size(); ++i) {
        flop += m_procs[i]->GetPipeline().GetFlop();
    }
    return flop;
}

#define MAXCOUNTS 30

struct my_iomanip_i { };
template<typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>&os, my_iomanip_i _unused)
{
    os << std::left << std::fixed << std::setprecision(2) << std::setw(9);
    return os;
}

struct my_iomanip_f { };
template<typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>&os, my_iomanip_f _unused)
{
    os << std::left << std::scientific << std::setprecision(6) << std::setw(12);
    return os;
}

struct my_iomanip_p { };
template<typename _CharT, typename _Traits>
std::basic_ostream<_CharT, _Traits>& operator<<(std::basic_ostream<_CharT, _Traits>&os, my_iomanip_p _unused)
{
    os << std::left << std::setprecision(1) << std::setw(8);
    return os;
}

void MGSystem::PrintCoreStats(std::ostream& os) const {
    struct my_iomanip_i fi;
    struct my_iomanip_f ff;
    struct my_iomanip_p fp;
    const char sep = ' ';

    const size_t P = m_procs.size();
    enum ct { I, F, PC };
    struct dt { uint64_t i; float f; };
    struct dt c[P][MAXCOUNTS];
    enum ct types[MAXCOUNTS];
    
    size_t i, j;

    // Collect the data 
    for (i = 0; i < P; ++i) {       
        Processor &p = *m_procs[i];
        const Pipeline& pl = p.GetPipeline();

        j = 0;
        types[j] = I; c[i][j++].i = pl.GetOp();
        types[j] = I; c[i][j++].i = pl.GetFlop();

        types[j] = types[j+1] = types[j+2] = types[j+3] = I;
        c[i][j].i = c[i][j+1].i = c[i][j+2].i = c[i][j+3].i = 0;
        pl.CollectMemOpStatistics(c[i][j].i, c[i][j+1].i, c[i][j+2].i, c[i][j+3].i);
        j += 4;

        types[j] = PC; c[i][j++].f = 100. * p.GetRegFileAsyncPortActivity();
        types[j] = I; c[i][j++].i = pl.GetTotalBusyTime();
        types[j] = I; c[i][j++].i = pl.GetNStages();
        types[j] = I; c[i][j++].i = pl.GetStagesRun();
        types[j] = PC; c[i][j++].f = 100. * pl.GetEfficiency();
        types[j] = PC; c[i][j++].f = 100. * (float)pl.GetOp() / (float)m_kernel.GetCycleNo();
        types[j] = I; c[i][j++].i = p.GetLocalFamilyCompletion();
        types[j] = I; c[i][j++].i = p.GetMaxThreadsAllocated();
        types[j] = I; c[i][j++].i = p.GetTotalThreadsAllocated();
        types[j] = I; c[i][j++].i = p.GetThreadTableSize();
        types[j] = PC; c[i][j++].f = 100. * p.GetThreadTableOccupancy();
        types[j] = I; c[i][j++].i = p.GetMaxFamiliesAllocated();
        types[j] = I; c[i][j++].i = p.GetTotalFamiliesAllocated();
        types[j] = I; c[i][j++].i = p.GetFamilyTableSize();
        types[j] = PC; c[i][j++].f = 100. * p.GetFamilyTableOccupancy();
        types[j] = I; c[i][j++].i = p.GetMaxAllocateExQueueSize();
        types[j] = I; c[i][j++].i = p.GetTotalAllocateExQueueSize();
        types[j] = F; c[i][j++].f = p.GetAverageAllocateExQueueSize();

    }

    const size_t NC = j;

    // compute min, max, total, average
    struct dt dmin[NC];
    struct dt dmax[NC];
    struct dt dtotal[NC];
    float davg[NC][2];
    size_t activecores;

    for (j = 0; j < NC; ++j)
    {
        dmin[j].i = std::numeric_limits<uint64_t>::max(); 
        dmin[j].f = std::numeric_limits<float>::max();
        dmax[j].i = 0; dmax[j].f = std::numeric_limits<float>::min();
        dtotal[j].i = 0; dtotal[j].f = 0.;
    }
    
    for (i = 0, activecores = 0; i < P; ++i) {
        if (c[i][0].i == 0) // core inactive; do not count
            continue;

        ++ activecores;
        for (j = 0; j < NC; ++j) {
            dmin[j].i = std::min(dmin[j].i, c[i][j].i);
            dmin[j].f = std::min(dmin[j].f, c[i][j].f);
            dmax[j].i = std::max(dmax[j].i, c[i][j].i);
            dmax[j].f = std::max(dmax[j].f, c[i][j].f);
            dtotal[j].i += c[i][j].i;
            dtotal[j].f += c[i][j].f;
        }
    }

    for (j = 0; j < NC; ++j) {
        davg[j][0] = (float)dtotal[j].i / (float)activecores;
        davg[j][1] = dtotal[j].f / (float)activecores;
    } 
    
    // print the data

    os << "## core statistics:" << endl
       << "# P " << sep
       << fi << "iops" << sep
       << fi << "flops" << sep
       << fi << "lds" << sep
       << fi << "sts" << sep
       << fi << "ibytes" << sep
       << fi << "obytes" << sep
       << fp << "regf_act" << sep
       << fi << "plbusy" << sep
       << fi << "plstgs" << sep
       << fi << "plstgrun" << sep
       << fp << "pl%busy" << sep
       << fp << "pl%eff" << sep
       << fi << "lastend" << sep
       << fi << "ttmax" << sep
       << fi << "ttotal" << sep
       << fi << "ttsize" << sep
       << fp << "tt%occ" << sep
       << fi << "ftmax" << sep
       << fi << "ftotal" << sep
       << fi << "ftsize" << sep
       << fp << "ft%occ" << sep
       << fi << "xqmax" << sep
       << fi << "xqtot" << sep
       << ff << "xqavg" << sep
       << endl;

    os << "# per-core values" << endl;
    for (i = 0; i < P; ++i) {
        if (c[i][0].i == 0)  continue; // unused core
        os << std::setw(4) << i << sep;
        for (j = 0; j < NC; ++j) 
            if (types[j] == I) 
                os << fi << c[i][j].i << sep;
            else if (types[j] == PC)
                os << fp << c[i][j].f << sep;
            else
                os << ff << c[i][j].f << sep;
        os << endl;
    }

/*
    os << "# minimas - all active cores" << endl
       << std::setw(4) << activecores << sep;
    for (j = 0; j < NC; ++j)
        if (types[j] == I)
            os << fi << dmin[j].i << sep;
        else
            os << ff << dmin[j].f << sep;
    os << endl
       << "# maxima - all active cores" << endl
       << std::setw(4) << activecores << sep;
    for (j = 0; j < NC; ++j)
        if (types[j] == I)
            os << fi << dmin[j].i << sep;
        else
            os << ff << dmin[j].f << sep;
    os << endl;
*/
    os << "# cumulative - all active cores" << endl
       << std::setw(4) << activecores << sep;
    for (j = 0; j < NC; ++j)
        if (types[j] == I)
            os << fi << dtotal[j].i << sep;
        else if (types[j] == PC)
            os << fp << dtotal[j].f << sep;
        else
            os << ff << dtotal[j].f << sep;
    os << endl
       << "# average per core = cumulative/" << activecores << endl
       << std::setw(4) << activecores << sep;
    for (j = 0; j < NC; ++j)
        if (types[j] == I)
            os << fi << davg[j][0] << sep;
        else if (types[j] == PC)
            os << fp << davg[j][1] << sep;
        else
            os << ff << davg[j][1] << sep;

    os << endl;

    os << "## descriptions:" << endl
       << "# P: core ID / number of active cores" << endl
       << "# iops: number of instructions executed" << endl
       << "# flops: number of floating-point instructions issued" << endl
       << "# lds: number of load instructions executed" << endl
       << "# sts: number of store instructions executed" << endl
       << "# ibytes: number of bytes loaded from L1 cache into core" << endl
       << "# obytes: number of bytes stored into L1 cache from core" << endl
       << "# regf_act: register file async port activity (= 100. * ncycles_asyncport_busy / ncycles_total)" << endl
       << "# plbusy: number of cycles the pipeline was active" << endl
       << "# plstgs: number of pipeline stages" << endl
       << "# plstgrun: cumulative number of cycles active in all pipeline stages" << endl
       << "# pl%busy: pipeline efficiency while active (= 100. * plstgrun / plstgs / plbusy)" << endl
       << "# pl%eff: pipeline efficiency total (= 100. * iops / ncycles_total)" << endl
       << "# lastend: time of last family termination" << endl
       << "# ttmax: maximum of thread entries simulatenously allocated" << endl
       << "# ttotal: cumulative number of thread entries busy" << endl
       << "# ttsize: thread table size" << endl
       << "# tt%occ: thread table occupancy (= 100. * ttotal / ttsize / ncycles_total)" << endl
       << "# ttmax: maximum of family entries simulatenously allocated" << endl
       << "# ftotal: cumulative number of family entries busy" << endl
       << "# ftsize: family table size" << endl
       << "# ft%occ: family table occupancy (= 100. * ftotal / ftsize / ncycles_total)" << endl
       << "# xqmax: high water mark of the exclusive allocate queue size" << endl
       << "# xqtot: cumulative exclusive allocate queue size" << endl
       << "# xqavg: average size of the exclusive allocate queue (= xqtot / ncycles_total)" << endl;

}

void MGSystem::PrintMemoryStatistics(std::ostream& os) const {
    uint64_t nr = 0, nrb = 0, nw = 0, nwb = 0;

    m_memory->GetMemoryStatistics(nr, nw, nrb, nwb);
    os << nr << "\t# number of load reqs. from the ext. mem. interface" << endl
       << nrb << "\t# number of bytes loaded from the ext. mem. interface" << endl
       << nw << "\t# number of store reqs. to the ext. mem. interface" << endl
       << nwb << "\t# number of bytes stored to the ext. mem. interface" << endl;

}

void MGSystem::PrintState(const vector<string>& arguments) const
{
    typedef map<string, RunState> StateMap;

    StateMap   states;
    streamsize length = 0;

    // This should be all non-idle processes
    for (const Process* process = m_kernel.GetActiveProcesses(); process != NULL; process = process->GetNext())
    {
        const std::string name = process->GetName();
        states[name] = process->GetState();
        length = std::max(length, (streamsize)name.length());
    }

    cout << left << setfill(' ');
    for (StateMap::const_iterator p = states.begin(); p != states.end(); ++p)
    {
        cout << setw(length) << p->first << ": ";
        switch (p->second)
        {
        case STATE_IDLE:     assert(0); break;
        case STATE_ACTIVE:   cout << "active"; break;
        case STATE_DEADLOCK: cout << "stalled"; break;
        case STATE_RUNNING:  cout << "running"; break;
        case STATE_ABORTED:  assert(0); break;
        }
        cout << endl;
    }
    cout << endl;

    int width = (int)log10(m_procs.size()) + 1;
    for (size_t i = 0; i < m_procs.size(); ++i)
    {
        if (!m_procs[i]->IsIdle()) {
            cout << "Processor " << dec << right << setw(width) << i << ": non-empty" << endl;
        }
    }
}

void MGSystem::PrintRegFileAsyncPortActivity(std::ostream& os) const
{
    float avg  = 0;
    float amax = 0.0f;
    float amin = 1.0f;
    for (size_t i = 0; i < m_procs.size(); ++i) {
        float a = m_procs[i]->GetRegFileAsyncPortActivity();
        amax = max(amax, a);
        amin = min(amin, a);
        avg += a;
    }
    avg /= (float)m_procs.size();
    os << avg << "\t# average reg. file async port activity" << endl
       << amin << "\t# min reg. file async port activity" << endl
       << amax << "\t# max reg. file async port activity" << endl;
}


void MGSystem::PrintAllFamilyCompletions(std::ostream& os) const
{
    for (PSize i = 0; i < m_procs.size(); i++) {
        CycleNo last = m_procs[i]->GetLocalFamilyCompletion();
        if (last != 0)
            os << m_procs[i]->GetLocalFamilyCompletion()
               << "\t# cycle counter at last family completion on core " << i
               << endl;
    }
}

void MGSystem::PrintFamilyCompletions(std::ostream& os) const
{
    CycleNo first = numeric_limits<CycleNo>::max();
    CycleNo last  = 0;
    for (size_t i = 0; i < m_procs.size(); ++i) {
        CycleNo cycle = m_procs[i]->GetLocalFamilyCompletion();
        if (cycle != 0)
        {
            first = min(first, cycle);
            last  = max(last,  cycle);
        }
    }
    os << first << "\t# cycle counter at first family completion" << endl
       << last << "\t# cycle counter at last family completion" << endl;
}

void MGSystem::PrintAllStatistics(std::ostream& os) const
{
    os << dec;
    os << GetKernel().GetCycleNo() << "\t# cycle counter" << endl
       << GetOp() << "\t# total executed instructions" << endl
       << GetFlop() << "\t# total issued fp instructions" << endl;
    PrintCoreStats(os);
    os << "## memory statistics:" << endl;
    PrintMemoryStatistics(os);
    // PrintRegFileAsyncPortActivity(os);
    // PrintPipelineIdleTime(os);
    // PrintPipelineEfficiency(os);
    // PrintFamilyCompletions(os);
    // PrintAllFamilyCompletions(os);
}

// Find a component in the system given its path
// Returns NULL when the component is not found
Object* MGSystem::GetComponent(const string& path)
{
    Object* cur = this;
    vector<string> names = Tokenize(path, ".");
    for (vector<string>::iterator p = names.begin(); cur != NULL && p != names.end(); ++p)
    {
        transform(p->begin(), p->end(), p->begin(), ::toupper);

        Object* next = NULL;
        for (unsigned int i = 0; i < cur->GetNumChildren(); ++i)
        {
            Object* child = cur->GetChild(i);
            string name   = child->GetName();
            transform(name.begin(), name.end(), name.begin(), ::toupper);
            if (name == *p)
            {
                next = child;
                break;
            }
        }
        cur = next;
    }
    return cur;
}

// Steps the entire system this many cycles
void MGSystem::Step(CycleNo nCycles)
{
    m_breakpoints.Resume();
    RunState state = GetKernel().Step(nCycles);
    if (state == STATE_IDLE)
    {
        // An idle state might actually be deadlock if there's a suspended thread.
        // So check all cores to see if they're really done.
        for (size_t i = 0; i < m_procs.size(); ++i)
        {
            if (!m_procs[i]->IsIdle())
            {
                state = STATE_DEADLOCK;
                break;
            }
        }
    }

    if (state == STATE_DEADLOCK)
    {
        // See how many processes are in each of the states
        unsigned int num_stalled = 0, num_running = 0;
        
        for (const Process* process = m_kernel.GetActiveProcesses(); process != NULL; process = process->GetNext())
        {
            switch (process->GetState())
            {
            case STATE_DEADLOCK: ++num_stalled; break;
            case STATE_RUNNING:  ++num_running; break;
            default:             assert(false); break;
            }
        }
        
        unsigned int num_regs = 0;
        for (size_t i = 0; i < m_procs.size(); ++i)
        {
            num_regs += m_procs[i]->GetNumSuspendedRegisters();
        }

        ostringstream ss;
        ss << "Deadlock!" << endl
           << "(" << num_stalled << " processes stalled;  " << num_running << " processes running; "
           << num_regs << " registers waited on)";
        throw runtime_error(ss.str());
    }

    if (state == STATE_ABORTED)
    {
        if (m_breakpoints.NewBreaksDetected())
        {
            ostringstream ss;
            m_breakpoints.ReportBreaks(ss);
            throw runtime_error(ss.str());
        }
        else
            // The simulation was aborted, because the user interrupted it.
            throw runtime_error("Interrupted!");
    }
}

string MGSystem::GetSymbol(MemAddr addr) const
{
    return m_symtable[addr];
}

void MGSystem::PrintAllSymbols(ostream& o, const string& pat) const
{
    m_symtable.Write(o, pat);
}

void MGSystem::Disassemble(MemAddr addr, size_t sz) const
{
    ostringstream cmd;

    cmd << m_objdump_cmd << " -d -r --prefix-addresses --show-raw-insn --start-address=" << addr
        << " --stop-address=" << addr + sz << " " << m_program;
    system(cmd.str().c_str());
}

MGSystem::MGSystem(const Config& config, Display& display, const string& program,
                   const string& symtable,
                   const vector<pair<RegAddr, RegValue> >& regs,
                   const vector<pair<RegAddr, string> >& loads,
                   bool quiet, bool doload)
    : Object("system", m_kernel),
      m_breakpoints(m_kernel),
      m_kernel(display, m_symtable, m_breakpoints),
      m_program(program),
      m_config(config)
{
    const vector<PSize> placeSizes = config.getIntegerList<PSize>("NumProcessors");
    const size_t numProcessorsPerFPU = max<size_t>(1, config.getInteger<size_t>("NumProcessorsPerFPU", 1));

    PSize numProcessors = 0;
    PSize curFPUmod = 0;
    PSize numFPUs = 0;
    size_t i;
    for (i = 0; i < placeSizes.size(); ++i) {
        if (curFPUmod != 0)
            std::cerr << "# warning: place " << i << " shares a FPU with the previous place" << std::endl;

        for (size_t j = 0; j < placeSizes[i]; ++j) {            
            ++numProcessors;
            curFPUmod = (curFPUmod + 1) % numProcessorsPerFPU;
            if (curFPUmod == 0) ++numFPUs;
        }
    }
    if (curFPUmod != 0)
    {
        ++numFPUs;
        std::cerr << "# warning: last FPU in place " << i-1 << " is not shared fully" << std::endl;
    }

#ifdef ENABLE_COMA_ZL
    m_objects.resize(numProcessors + numFPUs + 1);
    ZLCOMA* memory = new ZLCOMA("memory", *this, config);
    m_objects.back() = memory;
    m_memory = memory;
    m_memorytype = MEMTYPE_COMA_ZL;
#else
    std::string memory_type = config.getString("MemoryType", "");
    std::transform(memory_type.begin(), memory_type.end(), memory_type.begin(), ::toupper);

    m_objects.resize(numProcessors + numFPUs + 1);
    if (memory_type == "SERIAL") {
        SerialMemory* memory = new SerialMemory("memory", *this, config);
        m_objects.back() = memory;
        m_memory = memory;
        m_memorytype = MEMTYPE_SERIAL;
    } else if (memory_type == "PARALLEL") {
        ParallelMemory* memory = new ParallelMemory("memory", *this, config);
        m_objects.back() = memory;
        m_memory = memory;
        m_memorytype = MEMTYPE_PARALLEL;
    } else if (memory_type == "BANKED") {
        BankedMemory* memory = new BankedMemory("memory", *this, config);
        m_objects.back() = memory;
        m_memory = memory;
        m_memorytype = MEMTYPE_BANKED;
    } else if (memory_type == "RANDOMBANKED") {
        RandomBankedMemory* memory = new RandomBankedMemory("memory", *this, config);
        m_objects.back() = memory;
        m_memory = memory;
        m_memorytype = MEMTYPE_RANDOMBANKED;
    } else if (memory_type == "COMA") {
        COMA* memory = new COMA("memory", *this, config);
        m_objects.back() = memory;
        m_memory = memory;
        m_memorytype = MEMTYPE_COMA_ML;
    } else {
        throw std::runtime_error("Unknown memory type specified in configuration");
    }
#endif

    // Create the FPUs
    m_fpus.resize(numFPUs);
    for (size_t f = 0; f < numFPUs; ++f)
    {
        stringstream name;
        name << "fpu" << f;
        m_fpus[f] = new FPU(name.str(), *this, config, numProcessorsPerFPU);
    }

    // Create processor grid
    m_procs.resize(numProcessors);
    m_places.resize(placeSizes.size());


    PSize first = 0;
    for (size_t p = 0; p < placeSizes.size(); ++p)
    {
        m_places[p] = new PlaceInfo(m_kernel, placeSizes[p]);
        for (size_t i = 0; i < m_places[p]->m_size; ++i)
        {
            PSize pid = (first + i);
            FPU&  fpu = *m_fpus[pid / numProcessorsPerFPU];

            stringstream name;
            name << "cpu" << pid;
            m_procs[pid]   = new Processor(name.str(), *this, pid, i, m_procs, m_procs.size(), 
                                           *m_places[p], *m_memory, fpu, config);
            m_objects[pid] = m_procs[pid];
        }
        first += m_places[p]->m_size;
    }

    // Load the program into memory
    std::pair<MemAddr, bool> progdesc = make_pair(0, false);
    if (doload)
        progdesc = LoadProgram(m_memory, program, quiet);

    // Connect processors in rings
    first = 0;
    for (size_t p = 0; p < placeSizes.size(); ++p)
    {
        PSize placeSize = placeSizes[p];
        for (size_t i = 0; i < placeSize; ++i)
        {
            PSize pid = (first + i);
            LPID prev = (i + placeSize - 1) % placeSize;
            LPID next = (i + 1) % placeSize;
            m_procs[pid]->Initialize(*m_procs[first + prev], *m_procs[first + next], progdesc.first, progdesc.second);
        }
        first += placeSize;
    }

    if (doload && !m_procs.empty())
    {
        // Fill initial registers
        for (size_t i = 0; i < regs.size(); ++i)
        {
            m_procs[0]->WriteRegister(regs[i].first, regs[i].second);
        }

        // Load data files
        for (size_t i = 0; i < loads.size(); ++i)
        {
            RegValue value;
            value.m_state   = RST_FULL;

            pair<MemAddr, size_t> p = LoadDataFile(m_memory, loads[i].second, quiet);
            value.m_integer = p.first;

            m_procs[0]->WriteRegister(loads[i].first, value);
            m_symtable.AddSymbol(p.first, loads[i].second, p.second);
            m_inputfiles.push_back(loads[i].second);
        }

        // Load configuration
        // Store the address in local #2
        RegValue value;
        value.m_state   = RST_FULL;
        value.m_integer = WriteConfiguration();
        m_procs[0]->WriteRegister(MAKE_REGADDR(RT_INTEGER, 2), value);

#if TARGET_ARCH == ARCH_ALPHA
        // The Alpha expects the function address in $27
        value.m_integer = progdesc.first;
        m_procs[0]->WriteRegister(MAKE_REGADDR(RT_INTEGER, 27), value);
#endif
    }

    // Set program debugging per default
    m_kernel.SetDebugMode(Kernel::DEBUG_PROG);

    // Load symbol table
    if (doload && !symtable.empty()) 
    {
        ifstream in(symtable.c_str(), ios::in);
        m_symtable.Read(in);
    }

    // Find objdump command
#if TARGET_ARCH == ARCH_ALPHA
    const char *default_objdump = "mtalpha-linux-gnu-objdump";
    const char *objdump_var = "MTALPHA_OBJDUMP";
#endif
#if TARGET_ARCH == ARCH_SPARC
    const char *default_objdump = "mtsparc-leon-linux-objdump";
    const char *objdump_var = "MTSPARC_OBJDUMP";
#endif
    const char *v = getenv(objdump_var);
    if (!v) v = default_objdump;
    m_objdump_cmd = v;
}

MGSystem::~MGSystem()
{
    for (size_t i = 0; i < m_procs.size(); ++i)
    {
        delete m_procs[i];
    }
    for (size_t i = 0; i < m_places.size(); ++i)
    {
        delete m_places[i];
    }
    for (size_t i = 0; i < m_fpus.size(); ++i)
    {
        delete m_fpus[i];
    }
    delete m_memory;
}
