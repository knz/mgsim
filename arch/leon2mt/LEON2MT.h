// -*- c++ -*-
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <sim/inspect.h>
#include <sim/unreachable.h>
#include <arch/IOMessageInterface.h>
#include <arch/Memory.h>
#include <arch/BankSelector.h>
#include <arch/FPU.h>
#include <arch/leon2mt/RAUnit.h>
#include <arch/leon2mt/IOMatchUnit.h>
#include <arch/leon2mt/DebugChannel.h>
#include <arch/leon2mt/ActionInterface.h>
#include <arch/leon2mt/AncillaryRegisterFile.h>
#include <arch/leon2mt/PerfCounters.h>
#include <arch/leon2mt/MMUInterface.h>
#include <arch/leon2mt/RegisterFile.h>
#include <arch/leon2mt/FamilyTable.h>
#include <arch/leon2mt/ThreadTable.h>
#include <arch/leon2mt/ICache.h>
#include <arch/leon2mt/DCache.h>
#include <arch/leon2mt/IOInterface.h>
#include <arch/leon2mt/Network.h>
#include <arch/leon2mt/TMU.h>
#include <arch/leon2mt/Pipeline.h>

class Config;

namespace Simulator
{

    class BreakPointManager;

#define GetLEON2MT() (static_cast<LEON2MT&>(GetLEON2MTParent()))

class LEON2MT : public Object
{
public:
    class TMU;

    LEON2MT(const std::string& name, Object& parent, Clock& clock, PID pid, const std::vector<LEON2MT*>& grid, BreakPointManager& bp);
    LEON2MT(const LEON2MT&) = delete;
    LEON2MT& operator=(const LEON2MT&) = delete;
    ~LEON2MT();

public:
    void ConnectMemory(IMemory* memory, IMemoryAdmin *admin);
    void ConnectLink(LEON2MT* prev, LEON2MT* next);
    void ConnectFPU(FPU* fpu);
    void ConnectIO(IOMessageInterface* ioif);

    void Initialize();

    bool Boot(MemAddr addr, bool legacy);

private:
    // Helper to Initialize()
    void InitializeRegisters();
public:
    CycleNo GetCycleNo() const { return m_clock.GetCycleNo(); }
    PID   GetPID()      const { return m_pid; }
    PSize GetGridSize() const { return m_grid.size(); }
    bool  IsIdle()      const;

    float GetRegFileAsyncPortActivity() const {
        return (float)m_registerFile.p_asyncW.GetBusyCycles() / (float)GetCycleNo();
    }

    TSize GetMaxThreadsAllocated() const { return m_threadTable.GetMaxAllocated(); }
    TSize GetTotalThreadsAllocated() { return m_threadTable.GetTotalAllocated(); }
    TSize GetTotalThreadsCreated() { return m_tmu.GetTotalThreadsCreated(); }
    TSize GetThreadTableSize() const { return m_threadTable.GetNumThreads(); }
    float GetThreadTableOccupancy() { return (float)GetTotalThreadsAllocated() / (float)GetThreadTableSize() / (float)GetKernel()->GetCycleNo(); }
    FSize GetMaxFamiliesAllocated() const { return m_familyTable.GetMaxAllocated(); }
    FSize GetTotalFamiliesAllocated() { return m_familyTable.GetTotalAllocated(); }
    FSize GetTotalFamiliesCreated() { return m_tmu.GetTotalFamiliesCreated(); }
    FSize GetFamilyTableSize() const { return m_familyTable.GetNumFamilies(); }
    float GetFamilyTableOccupancy() { return (float)GetTotalFamiliesAllocated() / (float)GetFamilyTableSize() / (float)GetKernel()->GetCycleNo(); }
    BufferSize GetMaxAllocateExQueueSize() { return m_tmu.GetMaxAllocatedEx(); }
    BufferSize GetTotalAllocateExQueueSize() { return m_tmu.GetTotalAllocatedEx(); }
    float GetAverageAllocateExQueueSize() { return (float)GetTotalAllocateExQueueSize() / (float)GetKernel()->GetCycleNo(); }

    unsigned int GetNumSuspendedRegisters() const;

    void WriteASR(leon2mt::ARAddr which, Integer data) {  m_asr_file.WriteRegister(which, data); }
    Integer ReadASR(leon2mt::ARAddr which) const { return m_asr_file.ReadRegister(which); }
    void WriteAPR(leon2mt::ARAddr which, Integer data) {  m_apr_file.WriteRegister(which, data); }
    Integer ReadAPR(leon2mt::ARAddr which) const { return m_apr_file.ReadRegister(which); }



    // Configuration-dependent helpers
    PSize       GetPlaceSize(LFID fid) const { return m_familyTable[fid].placeSize; }
    MemAddr     GetTLSAddress(LFID fid, TID tid) const;
    MemSize     GetTLSSize() const;
    PlaceID     UnpackPlace(Integer id) const;
    Integer     PackPlace(const PlaceID& id) const;
    FID         UnpackFID(Integer id) const;
    Integer     PackFID(const FID& fid) const;
    FCapability GenerateFamilyCapability() const;

    void MapMemory(MemAddr address, MemSize size, ProcessID pid = 0);
    void UnmapMemory(MemAddr address, MemSize size);
    void UnmapMemory(ProcessID pid);
    bool CheckPermissions(MemAddr address, MemSize size, int access) const;

    BreakPointManager& GetBreakPointManager() { return m_bp_manager; }
    leon2mt::Network& GetNetwork() { return m_network; }
    leon2mt::IOInterface* GetIOInterface() { return m_io_if; }
    leon2mt::RegisterFile& GetRegisterFile() { return m_registerFile; }
    leon2mt::ICache& GetICache() { return m_icache; }
    leon2mt::DCache& GetDCache() { return m_dcache; }
    leon2mt::TMU& GetTMU() { return m_tmu; }
    leon2mt::RAUnit& GetRAUnit() { return m_raunit; }
    leon2mt::Pipeline& GetPipeline() { return m_pipeline; }
    leon2mt::IOMatchUnit& GetIOMatchUnit() { return m_mmio; }
    leon2mt::FamilyTable& GetFamilyTable() { return m_familyTable; }
    leon2mt::ThreadTable& GetThreadTable() { return m_threadTable; }
    SymbolTable& GetSymbolTable() { return *m_symtable; }

private:
    Clock&                         m_clock;
    BreakPointManager&             m_bp_manager;
    IMemory*                       m_memory;
    IMemoryAdmin*                  m_memadmin;
    const std::vector<LEON2MT*>&     m_grid;
    FPU*                           m_fpu;
    SymbolTable*                   m_symtable;
    PID                            m_pid;
    // Register initializers
    std::map<RegAddr, std::string> m_reginits;

    // Bit counts for packing and unpacking configuration-dependent values
    struct
    {
        unsigned int pid_bits;  ///< Number of bits for a PID (LEON2MT ID)
        unsigned int fid_bits;  ///< Number of bits for a LFID (Local Family ID)
        unsigned int tid_bits;  ///< Number of bits for a TID (Thread ID)
        unsigned int gw_bits;  ///< Number of bits for a grid width
        unsigned int gh_bits;  ///< Number of bits for a grid height
        unsigned int bw_bits;  ///< Number of bits for a block width
        unsigned int bh_bits;  ///< Number of bits for a block height
    } m_bits;

    // The components on the core
    leon2mt::FamilyTable    m_familyTable;
    leon2mt::ThreadTable    m_threadTable;
    leon2mt::RegisterFile   m_registerFile;
    leon2mt::RAUnit         m_raunit;
    leon2mt::TMU            m_tmu;
    leon2mt::ICache         m_icache;
    leon2mt::DCache         m_dcache;
    leon2mt::Pipeline       m_pipeline;
    leon2mt::Network        m_network;

    // Local MMIO devices
    leon2mt::IOMatchUnit    m_mmio;
    leon2mt::AncillaryRegisterFile m_apr_file;
    leon2mt::AncillaryRegisterFile m_asr_file;
    leon2mt::PerfCounters   m_perfcounters;
    leon2mt::DebugChannel   m_lpout;
    leon2mt::DebugChannel   m_lperr;
    leon2mt::MMUInterface   m_mmu;
    leon2mt::ActionInterface m_action;

    // External I/O interface, optional
    leon2mt::IOInterface    *m_io_if;

    friend class leon2mt::PerfCounters::Helpers;
};

}
#endif
