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
    switch (opcode) {
        // FILL IN
        // ...some code missing...
        // FILL IN
    }
}

void Pipeline::DecodeStage::DecodeInstruction(const Instruction& instr)
{
    m_output.Ra     = INVALID_REG;
    m_output.Rb     = INVALID_REG;
    m_output.Rc     = INVALID_REG;

    RegIndex Ra = (instr >> 21) & 0x1f;
    RegIndex Rb = (instr >> 16) & 0x1f;
    RegIndex Rc = (instr >> 11) & 0x1f;

    // FILL IN HERE
    // ...some code missing...
    m_output.format = IFORMAT_INVALID; // REPLACE: = GetInstrFormat(...FILL IN HERE...)

    switch (m_output.format) {
        case IFORMAT_RTYPE:
            // FILL IN HERE
            // ...some code missing...

            break;

        case IFORMAT_REGIMM:
            // FILL IN HERE
            // ...some code missing...

            break;

        case IFORMAT_JTYPE:
            // FILL IN HERE
            // ...some code missing...

            break;

        case IFORMAT_ITYPE:
            // FILL IN HERE
            // ...some code missing...
            break;
    }
}



Pipeline::PipeAction Pipeline::ExecuteStage::ExecuteInstruction()
{
    auto& thread = m_threadTable[m_input.tid];

    // Fetch both potential input buffers.
    uint32_t Rav = m_input.Rav.m_integer.get(m_input.Rav.m_size);
    uint32_t Rbv = m_input.Rbv.m_integer.get(m_input.Rbv.m_size);

    switch (m_input.format) {
        case IFORMAT_RTYPE:
            // FILL IN
            // ...some code missing here...
            break;

        case IFORMAT_REGIMM:
            // FILL IN
            // ...some code missing here...
            break;

        case IFORMAT_JTYPE:
            // FILL IN
            // ...some code missing here...
            break;

        case IFORMAT_ITYPE:
            // FILL IN
            // ...some code missing here...
            break;
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
