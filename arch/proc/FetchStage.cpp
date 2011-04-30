#include "Processor.h"
#include "sim/config.h"
#include "sim/breakpoints.h"
#include <cassert>
using namespace std;

namespace Simulator
{

void Processor::Pipeline::FetchStage::Clear(TID tid)
{
    if (m_output.tid == tid)
    {
        m_switched = true;
    }
}

Processor::Pipeline::PipeAction Processor::Pipeline::FetchStage::OnCycle()
{
    MemAddr pc = m_pc;
    if (m_switched)
    {
        // We need to switch to a new thread

        // Get the thread on the front of the active queue
        TID tid = m_allocator.PopActiveThread();
        if (tid == INVALID_TID)
        {
            // Nothing to do....
            return PIPE_IDLE;
        }
        
        // Read its information from the Family and Thread Tables
        Thread& thread = m_threadTable[tid];
        Family& family = m_familyTable[thread.family];

        pc = thread.pc;
        if (!family.legacy && pc % m_controlBlockSize == 0)
        {
            // We need to check for breakpoints on the control
            // word here.
            GetKernel()->GetBreakPoints().Check(BreakPoints::EXEC, pc, *this);

            // Skip the control word
            pc += sizeof(Instruction);
        }

        // Read the cache line for this PC
        size_t offset = (size_t)(pc % m_icache.GetLineSize());   // Offset within the cacheline
        if (!m_icache.Read(thread.cid, pc - offset, m_buffer, m_icache.GetLineSize()))
        {
            return PIPE_STALL;
        }

        COMMIT
        {
            m_output.tid       = tid;
            m_output.fid       = thread.family;
            m_output.legacy    = family.legacy;
            m_output.placeSize = family.placeSize;

            for (RegType i = 0; i < NUM_REG_TYPES; ++i)
            {
                m_output.regs.types[i].family = family.regs[i];
                m_output.regs.types[i].thread = thread.regs[i];
            }
            
            // Mark the thread as running
            thread.state = TST_RUNNING;
        }
    }

    COMMIT
    {
        // Read the instruction and control bits
        const size_t offset   = (size_t)(pc % m_icache.GetLineSize());                // Offset within the cacheline
        const size_t iInstr   = offset / sizeof(Instruction);                         // Offset in instructions
        const size_t iControl = (offset & -m_controlBlockSize) / sizeof(Instruction); // Align offset down to control block size
    
        const Instruction* instrs = (const Instruction*)m_buffer;
        const Instruction control = (!m_output.legacy) ? UnserializeInstruction(&instrs[iControl]) >> (2 * (iInstr - iControl)) : 0;
        const MemAddr     next_pc = pc + sizeof(Instruction);

        // Fill output latch structure
        m_output.kill         = ((control & 2) != 0);
        const bool wantSwitch = ((control & 1) != 0);
        const bool mustSwitch = m_output.kill || (next_pc % m_icache.GetLineSize() == 0);
        const bool lastThread = m_allocator.m_activeThreads.Empty() || m_allocator.m_activeThreads.Singular();
        m_output.swch         = mustSwitch || (wantSwitch && !lastThread);
        m_output.pc           = pc;
        m_output.pc_dbg       = pc;
        m_output.instr        = UnserializeInstruction(&instrs[iInstr]);

        // Check for breakpoints
        GetKernel()->GetBreakPoints().Check(BreakPoints::EXEC, pc, *this);

        // Update the PC and switched state
        m_pc       = next_pc;
        m_switched = m_output.swch;
    }
        
    return PIPE_CONTINUE;
}

Processor::Pipeline::FetchStage::FetchStage(Pipeline& parent, Clock& clock, FetchDecodeLatch& output, Allocator& alloc, FamilyTable& familyTable, ThreadTable& threadTable, ICache& icache, Config& config)
  : Stage("fetch", parent, clock),
    m_output(output),
    m_allocator(alloc),
    m_familyTable(familyTable),
    m_threadTable(threadTable),
    m_icache(icache),
    m_controlBlockSize(config.getValue<size_t>("ControlBlockSize")),
    m_switched(true)
{
    if ((m_controlBlockSize & ~(m_controlBlockSize - 1)) != m_controlBlockSize)
    {
        throw InvalidArgumentException("Control block size is not a power of two");
    }

    if (m_controlBlockSize > (sizeof(Instruction)*sizeof(Instruction)*8/2))
    {
        throw InvalidArgumentException("Control block size causes control word to be larger than an instruction");
    }

    if (m_controlBlockSize > m_icache.GetLineSize())
    {
        throw InvalidArgumentException("Control block size is larger than the cache line size");
    }

    m_buffer = new char[m_icache.GetLineSize()];
}

Processor::Pipeline::FetchStage::~FetchStage()
{
    delete[] m_buffer;
}

}
