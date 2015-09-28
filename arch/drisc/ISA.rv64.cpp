#include "Pipeline.h"
#include "DRISC.h"
#include <arch/symtable.h>
#include <cassert>
#include <sstream>
#include <iomanip>
using namespace std;

namespace Simulator
{
namespace drisc
{

Pipeline::InstrFormat Pipeline::DecodeStage::GetInstrFormat(uint8_t opcode)
{
    // GetInstrFormat must select the format of the instruction
    // depending on the opcode part. The format determines how the
    // instruction is further decoded in DecodeInstruction() below.

    switch (opcode) {

        // FILL IN
        // ...some code missing...
        // Check the IFORMAT_XXXXX declarations in ISA.rv64.h.
        // FILL IN
    default:
        break;
    }

    return IFORMAT_INVALID;
}

void Pipeline::DecodeStage::DecodeInstruction(const Instruction& instr)
{
    m_output.opcode = instr & 0x7f;
    m_output.format = GetInstrFormat(m_output.opcode);
    m_output.Ra     = INVALID_REG;
    m_output.Rb     = INVALID_REG;
    m_output.Rc     = INVALID_REG;

    RegIndex Ra = (instr >> 22) & 0x1f;
    RegIndex Rb = (instr >> 17) & 0x1f;
    RegIndex Rc = (instr >> 27) & 0x1f;


    switch (m_output.format) {
    case IFORMAT_RTYPE:
        // Example:
        m_output.function = (instr >> 7) & 0x3ff;
        m_output.Ra = MAKE_REGADDR(RT_INTEGER, Ra);
        m_output.Rb = MAKE_REGADDR(RT_INTEGER, Rb);
        m_output.Rc = MAKE_REGADDR(RT_INTEGER, Rc);

        break;

    case IFORMAT_JTYPE:
        // FILL IN HERE
        // ...some code missing...

        if (m_output.opcode == 0x6f /* JAL, try to define a constant in the .h file! */)
            // the JAL instruction, as special case, writes
            // a result to the (fixed) register 1.
            m_output.Rc = MAKE_REGADDR(RT_INTEGER, 1);

        break;

    case IFORMAT_ITYPE:

        // FILL IN HERE
        // ...some code missing...
        // FILL IN HERE


        break;

    // perhaps more cases here!

    default:
        ThrowIllegalInstructionException(*this, m_input.pc, "Unknown instruction format");
    }
}



Pipeline::PipeAction Pipeline::ExecuteStage::ExecuteInstruction()
{
    auto& thread = m_threadTable[m_input.tid];

    // Fetch both potential input buffers.
    uint32_t Rav = m_input.Rav.m_integer.get(m_input.Rav.m_size);
    uint32_t Rbv = m_input.Rbv.m_integer.get(m_input.Rbv.m_size);

    /* NB:

       - m_input is the read-execute latch, containing all buffers
       between the read and execute stages. This has the buffers
       from the decode-read latch, and also the additional buffers declared
       in ISA.rv64.h, and also the following from Pipeline.h:

       - PipeValue Rav, Rbv; <- the values read from the register
       file by the read stage

       - m_output is the execute-memory latch, with the following
       "standard" buffers from Pipeline.h:

       - PipeValue Rcv; <- the register value resulting from execute
       - MemAddr address; <- for memory instructions
       - MemSize size; <- for memory instructions

    */


    switch (m_input.format) {
    case IFORMAT_RTYPE:
        // FILL IN
        // ...some code missing here...
        // FILL IN

        break;

    case IFORMAT_BTYPE:
        // FILL IN
        // ...some code missing here...
        // FILL IN

        // Tip: if you implement branch instructions here you can
        // reuse the following code:
        /*
        // Note: displacement must be set in DecodeInstruction() above
        MemAddr next = m_input.pc + sizeof(Instruction);
        MemAddr target = m_input.pc + (m_input.displacement << 1);
        return ExecBranchTo(target, next, XXXX);
        // XXXX: true for "link" instructions, otherwise false
        */

        // Tip: for stores you can reuse the following code:
        /*
        {
            // Note: The immediate value must be set by DecodeInstruction() above.
            MemAddr address = Rav + (int16_t)m_input.immediate;
            uint32_t value = Rbv;
            unsigned size = 0;
            switch(m_input.opcode)
            {
            case M_OP_SB: size = 1; break;
            case M_OP_SH: size = 2; break;
            case M_OP_SW: size = 4; break;
            case M_OP_SD: size = 8; break;
            }
            COMMIT {
                m_output.Rcv.m_state = RST_FULL;
                m_output.Rcv.m_integer = value;
                m_output.address = address;
                m_output.size = size;
            }
        }
        */

        break;

    case IFORMAT_ITYPE:
        // FILL IN
        // ...some code missing here...

        // Tip: for loads you can reuse the following code:
        /*
        COMMIT {
            m_output.sign_extend = (m_input.opcode == M_OP_LB || m_input.opcode == M_OP_LH || m_input.opcode == M_OP_LW);
            switch(m_input.opcode)
            {
            case M_OP_LB: case M_OP_LBU: m_output.size = 1; break;
            case M_OP_LH: case M_OP_LHU: m_output.size = 2; break;
            case M_OP_LW: case M_OP_LWU: m_output.size = 4; break;
            case M_OP_LD:                m_output.size = 8; break;
            }
            // Note: The immediate value must be set by DecodeInstruction() above.
            m_output.address = Rav + (int16_t)m_input.immediate;
            m_output.Rcv.m_state = RST_INVALID;
        }
        */

        // Tip: if you implement JALR here you can
        // reuse the following code:
        /*
          MemAddr target = (Rav + m_input.displacement);
          target = target & ~(uint64_t)1;
          return ExecBranchTo(target, m_input.pc + sizeof(Instruction), true);
        */

        break;

    case IFORMAT_JTYPE:
        // You can implement your own, or reuse the following:
        /*
          if (m_input.opcode == M_OP_J || m_input.opcode == M_OP_JAL)
          {
          // Note: displacement must be set by DecodeInstruction() above.
          MemAddr next = m_input.pc + sizeof(Instruction);
          MemAddr target = m_input.pc + (m_input.displacement << 1);
          return ExecBranchTo(target, next, m_input.opcode == M_OP_JAL);
          }
          else
          ThrowIllegalInstructionException(*this, m_input.pc, "Unknown J instruction");
        */
        break;


    default:
        ThrowIllegalInstructionException(*this, m_input.pc, "Unknown instruction format");

  }

  return PIPE_CONTINUE;

}


Pipeline::PipeAction Pipeline::ExecuteStage::ExecBranchTo(MemAddr target, MemAddr next, bool writeRc)
{
    // This method is called from ExecuteInstruction on branches
    // that are being taken. It actually optimizes the branch if
    // it jumps directly to its next instruction.

    if (writeRc)
    {
	COMMIT {
	    m_output.Rcv.m_state = RST_FULL;
	    m_output.Rcv.m_integer = next;
	}
    }

    if (target != next) {

	DebugFlowWrite("F%u/T%u(%llu) %s branch %s",
		       (unsigned)m_input.fid, (unsigned)m_input.tid, (unsigned long long)m_input.logical_index, m_input.pc_sym,
		       GetDRISC().GetSymbolTable()[target].c_str());

	COMMIT {
	    // Enact the branch.
	    m_output.swch = true;
	    m_output.pc = target;
	}

	// Flush the pipeline from further instructions
	// in the same thread.
	return PIPE_FLUSH;
    }
    return PIPE_CONTINUE;
}



// Function for naming local registers according to a standard ABI
const vector<string>& GetDefaultLocalRegisterAliases(RegType type)
{
    static const vector<string> intnames = {
               "ra",  "sp",  "gp",  "tp",  "t0",  "t1",  "t2",
        "s0",  "s1",  "a0",  "a1",  "a2",  "a3",  "a4",  "a5",
        "a6",  "a7",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
        "s8",  "s9",  "s10", "s11", "t3",  "t4",  "t5",  "t6"
    };
    static const vector<string> fltnames = {
        "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
        "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
        "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
        "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31" };
    if (type == RT_INTEGER)
        return intnames;
    else
        return fltnames;
}

// Function for getting a register's type and index within that type
unsigned char GetRegisterClass(unsigned char addr, const RegsNo& regs, RegClass* rc, RegType /*type*/)
{
    // $0 is zero, otherwise all registers are local.
    if (addr > 0)
    {
        addr--;
        if (addr < regs.locals)
        {
            *rc = RC_LOCAL;
            return addr;
        }
    }
    *rc = RC_RAZ;
    return 0;
}


}
}
