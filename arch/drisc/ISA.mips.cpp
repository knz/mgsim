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
    case M_OP_SPECIAL:
        return IFORMAT_RTYPE;

        // FILL IN
        // ...some code missing...
        // Check the IFORMAT_XXXXX declarations in ISA.mips.h.
        // FILL IN
    }

    return IFORMAT_INVALID;
}

void Pipeline::DecodeStage::DecodeInstruction(const Instruction& instr)
{
    m_output.opcode = (instr >> 26) & 0x3f;
    m_output.format = GetInstrFormat(m_output.opcode);
    m_output.Ra     = INVALID_REG;
    m_output.Rb     = INVALID_REG;
    m_output.Rc     = INVALID_REG;

    RegIndex Ra = (instr >> 21) & 0x1f;
    RegIndex Rb = (instr >> 16) & 0x1f;
    RegIndex Rc = (instr >> 11) & 0x1f;

    switch (m_output.format) {
        case IFORMAT_RTYPE:
            // Example:
            m_output.function = instr & 0x3f;
            m_output.Ra = MAKE_REGADDR(RT_INTEGER, Ra);
            m_output.Rb = MAKE_REGADDR(RT_INTEGER, Rb);

            // FILL IN HERE
            // Some code missing to extract the "shift amount"
            // if you decide to implement shift instructions.

            if (m_output.function != M_ROP_JR /* PERHAPS MORE HERE... */)
            {
                // Some instructions, including JR, do not write back
                // a result to the destination Rc register.  For all
                // other instructions, Rc is the 3rd (destination)
                // operand. You need to extend the condition
                // above on "function" to include the other
                // instructions you decide to implement
                // that do not write a result to Rc.
                m_output.Rc = MAKE_REGADDR(RT_INTEGER, Rc);
            }
            break;

        case IFORMAT_REGIMM:
            m_output.Ra = MAKE_REGADDR(RT_INTEGER, Ra);
            //
            // FILL IN HERE
            // ...some code missing...
            // You need to:
            // - extract the immediate value (2nd operand)
            // - extract the relative displacement address.
            // Beware, the displacement is a signed value! Test
            // your code with negative displacements.

            break;

        case IFORMAT_JTYPE:
            // FILL IN HERE
            // ...some code missing...
            // (You need to extract the target adress)

            if (m_output.opcode == M_OP_JAL)
                // the JAL instruction, as special case, write
                // a result to the (fixed) register 31.
                m_output.Rc = MAKE_REGADDR(RT_INTEGER, 31);

            break;

        case IFORMAT_ITYPE:

            // FILL IN HERE
            // ...some code missing...
            // You need to extract the following:
            // - the 1st Ra operand
            // - the 2nd Rb operand for store instructions
            // - the 16-bit unsigned immediate value
            // - for those instructions using a relative displacement
            //   (BNE, BEQ, BLEZ, BGTZ), convert the immediate to a
            //   relative displacement (which may be signed).
            // FILL IN HERE


            // Finally:
            // All I-type instructions except for conditional branches and
            // stores write back to $rt (Rb):
            switch(m_output.opcode)
            {
            case M_OP_BNE:  case M_OP_BEQ:
            case M_OP_BLEZ: case M_OP_BGTZ:
            case M_OP_SB:   case M_OP_SH:
            case M_OP_SW:
            case M_OP_SWL:
            case M_OP_SWR:
                break;
            default:
                m_output.Rc = MAKE_REGADDR(RT_INTEGER, Rb);
                break;
            }

            break;
    case IFORMAT_INVALID:
        ThrowIllegalInstructionException(*this, m_input.pc, "Unknown instruction format");
   }
}


Pipeline::PipeAction Pipeline::ExecuteStage::ExecuteInstruction()
{
    // Fetch both potential input buffers.
    uint32_t Rav = m_input.Rav.m_integer.get(m_input.Rav.m_size);
    uint32_t Rbv = m_input.Rbv.m_integer.get(m_input.Rbv.m_size);

    switch (m_input.format) {
    case IFORMAT_RTYPE:
        // FILL IN
        // ...some code missing here...
        // FILL IN

        // For the special instructions JR and JALR, you can reuse the
        // following code:
        // - for JALR:
        /*
            return ExecBranchTo(Rav, m_input.pc + sizeof(Instruction), true);
        */
        // - for JR:
        /*
            return ExecBranchTo(Rav, m_input.pc + sizeof(Instruction), false);
        */
        // ... of course you can make your code look better and only compute
        // the "next" pointer in one place.
        break;

    case IFORMAT_REGIMM:
        // FILL IN
        // ...some code missing here...
        // FILL IN

        // Tip: if you implement branch instructions here you can
        // reuse the following code:
        /*
          // Note: displacement must be set in DecodeInstruction() above
          MemAddr next = m_input.pc + sizeof(Instruction);
          MemAddr target = next + m_input.displacement;
          return ExecBranchTo(target, next, XXXX);
          // XXXX: true for "link" instructions, otherwise false
         */
        break;

    case IFORMAT_ITYPE:
        // FILL IN
        // ...some code missing here...

        // There are 4 "families" of I-format instructions:
        // 1. "simple" register-immediate to register instructions
        // 2. conditional branches.
        // 3. load instructions.
        // 4. store instructions.

        // For family #1, you need to implement the code yourself.
        // For family #2, you need to implement the test on the condition,
        // then to actually perform the branch you can reuse the following code:
        /*
        {
            // Note: displacement must be set in DecodeInstruction() above.
            MemAddr next = m_input.pc + sizeof(Instruction);
            MemAddr target = next + m_input.displacement;
            return ExecBranchTo(target, next, false);
        }
        */

        // For family #3 (loads) you can reuse the following code:
        /*
        COMMIT {
            m_output.sign_extend = (m_input.opcode == M_OP_LB || m_input.opcode == M_OP_LH);
            switch(m_input.opcode)
            {
            case M_OP_LB: case M_OP_LBU: m_output.size = 1; break;
            case M_OP_LH: case M_OP_LHU: m_output.size = 2; break;
            case M_OP_LW:                m_output.size = 4; break;
            }
            // Note: The immediate value must be set by DecodeInstruction() above.
            m_output.address = Rav + (int16_t)m_input.immediate;
            m_output.Rcv.m_state = RST_INVALID;
        }
        */

        // For family #4 (stores) you can reuse the following code:
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
            case M_OP_SWL:
                size = address & 3;
                address &= ~(uint32_t)3;
                value >>= (8 * size);
                break;
            case M_OP_SWR:
                size = address & 3;
                address &= ~(uint32_t)3;
                value &= ~(uint32_t)((1 << size) - 1);
                size = 4 - size;
                break;
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

    case IFORMAT_JTYPE:
        // You can implement your own, or reuse the following:
        /*
        if (m_input.opcode == M_OP_J || m_input.opcode == M_OP_JAL)
        {
            // Note: displacement must be set by DecodeInstruction() above.
            MemAddr next = m_input.pc + sizeof(Instruction);
            MemAddr target = (next & 0xf0000000) | (m_input.displacement << 2);

            return ExecBranchTo(target, next, m_input.opcode == M_OP_JAL);
        }
        else
          ThrowIllegalInstructionException(*this, m_input.pc, "Unknown J instruction");
        */
        break;


    case IFORMAT_INVALID:
        ThrowIllegalInstructionException(*this, m_input.pc, "Unknown instruction format");

    }

    return PIPE_CONTINUE;

}

// Function for naming local registers according to a standard ABI
    const vector<string>& GetDefaultLocalRegisterAliases(RegType type)
    {
        static const vector<string> intnames = {
            "at", "v0", "v1", "a0", "a1", "a2", "a3",
            "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
            "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9",
            "k0", "k1", "gp", "sp", "fp", "ra" };
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
