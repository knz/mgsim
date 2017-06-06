// -*- c++ -*-
#ifndef ISA_SPARC_H
#define ISA_SPARC_H

#ifndef PIPELINE_H
#error This file should be included in Pipeline.h
#endif

// op1
enum {
    S_OP1_BRANCH = 0,
    S_OP1_CALL   = 1,
    S_OP1_ALU    = 2,
    S_OP1_MEMORY = 3,
};

// op2 (op1 is S_OP1_BRANCH)
enum {
    S_OP2_UNIMPL     = 0,
    S_OP2_BICC       = 2,
    S_OP2_SETHI      = 4,
    S_OP2_FBFCC      = 6,
    S_OP2_CBCCC      = 7,
};

// op3 (op1 is S_OP1_ALU)
enum {
    S_OP3_ADD       = 0x00,
    S_OP3_AND       = 0x01,
    S_OP3_OR        = 0x02,
    S_OP3_XOR       = 0x03,
    S_OP3_SUB       = 0x04,
    S_OP3_ANDN      = 0x05,
    S_OP3_ORN       = 0x06,
    S_OP3_XNOR      = 0x07,
    S_OP3_ADDX      = 0x08,
    S_OP3_UMUL      = 0x0A,
    S_OP3_SMUL      = 0x0B,
    S_OP3_SUBX      = 0x0C,
    S_OP3_UDIV      = 0x0E,
    S_OP3_SDIV      = 0x0F,
    S_OP3_ADDcc     = 0x10,
    S_OP3_ANDcc     = 0x11,
    S_OP3_ORcc      = 0x12,
    S_OP3_XORcc     = 0x13,
    S_OP3_SUBcc     = 0x14,
    S_OP3_ANDNcc    = 0x15,
    S_OP3_ORNcc     = 0x16,
    S_OP3_XNORcc    = 0x17,
    S_OP3_ADDXcc    = 0x18,
    S_OP3_UMULcc    = 0x1A,
    S_OP3_SMULcc    = 0x1B,
    S_OP3_SUBXcc    = 0x1C,
    S_OP3_UDIVcc    = 0x1E,
    S_OP3_SDIVcc    = 0x1F,
    S_OP3_TADDcc    = 0x20,
    S_OP3_TSUBcc    = 0x21,
    S_OP3_TADDccTV  = 0x22,
    S_OP3_TSUBccTV  = 0x23,
    S_OP3_MULScc    = 0x24,
    S_OP3_SLL       = 0x25,
    S_OP3_SRL       = 0x26,
    S_OP3_SRA       = 0x27,
    S_OP3_RDASR     = 0x28,
    S_OP3_RDPSR     = 0x29,
    S_OP3_RDWIM     = 0x2A,
    S_OP3_RDTBR     = 0x2B,
    
    S_OP3_TGETFID   = 0x2C,
    S_OP3_TGETTID   = 0x2D,
    S_OP3_TGETPIX   = 0x2E,
    S_OP3_FGETBLKIDX= 0x2F,
    
    S_OP3_WRASR     = 0x30,
    S_OP3_WRPSR     = 0x31,
    S_OP3_WRWIM     = 0x32,
    S_OP3_WRTBR     = 0x33,
    S_OP3_FPOP1     = 0x34,
    S_OP3_FPOP2     = 0x35,
    S_OP3_CPOP1     = 0x36,
    S_OP3_CPOP2     = 0x37,
    S_OP3_JMPL      = 0x38,
    S_OP3_RETT      = 0x39,
    S_OP3_TICC      = 0x3A,
    S_OP3_FLUSH     = 0x3B,
    S_OP3_SAVE      = 0x3C,
    S_OP3_RESTORE   = 0x3D,


// op3 (op1 is S_OP1_MEMORY)
    S_OP3_LD      = 0x00,
    S_OP3_LDUB    = 0x01,
    S_OP3_LDUH    = 0x02,
    S_OP3_LDD     = 0x03,
    S_OP3_ST      = 0x04,
    S_OP3_STB     = 0x05,
    S_OP3_STH     = 0x06,
    S_OP3_STD     = 0x07,
    
    S_OP3_FALLOC    = 0x08,
    
    S_OP3_LDSB    = 0x09,
    S_OP3_LDSH    = 0x0A,
    S_OP3_LDSTUB  = 0x0D,
    
    S_OP3_FBREAK    = 0x0E,
   
    S_OP3_SWAP    = 0x0F,
    S_OP3_LDA     = 0x10,
    S_OP3_LDUBA   = 0x11,
    S_OP3_LDUHA   = 0x12,
    S_OP3_LDDA    = 0x13,
    S_OP3_STA     = 0x14,
    S_OP3_STBA    = 0x15,
    S_OP3_STHA    = 0x16,
    S_OP3_STDA    = 0x17,
    S_OP3_LDSBA   = 0x19,
    S_OP3_LDSHA   = 0x1A,
    S_OP3_LDSTUBA = 0x1D,
    S_OP3_SWAPA   = 0x1F,
    S_OP3_LDF     = 0x20,
    S_OP3_LDFSR   = 0x21,
    S_OP3_LDDF    = 0x23,
    S_OP3_STF     = 0x24,
    S_OP3_STFSR   = 0x25,
    S_OP3_STDFQ   = 0x26,
    S_OP3_STDF    = 0x27,
    
    S_OP3_RALLOCSRB = 0x28,
    S_OP3_TALLOCHTG = 0x29,
    S_OP3_RWRITE    = 0x2A,
    S_OP3_FSETGSIZE = 0x2B,
    S_OP3_FSETBSIZE = 0x2C,
    S_OP3_FMAPG     = 0x2D,
    S_OP3_FCREATE   = 0x2E,
    S_OP3_TWAIT     = 0x2F,
    
    S_OP3_LDC     = 0x30,
    S_OP3_LDCSR   = 0x31,
    S_OP3_LDDC    = 0x33,
    S_OP3_STC     = 0x34,
    S_OP3_STCSR   = 0x35,
    S_OP3_STDCQ   = 0x36,
    S_OP3_STDC    = 0x37,

    S_OP3_RFREESRB  = 0x38,
    S_OP3_TFREEHTG  = 0x39,
    S_OP3_RREAD     = 0x3A,
    S_OP3_FGETGSIZE = 0x3B,
    S_OP3_FGETBSIZE = 0x3C,
    S_OP3_FMAPHTG   = 0x3D,
    S_OP3_FFENCE    = 0x3E,
    S_OP3_TEND      = 0x3F

};

// opf (op1 is S_OP1_ALU, op3 is S_OP3_FPOP1/2)
enum {
    S_OPF_FMOV    = 0x01,
    S_OPF_FPRINTS = 0x02,
    S_OPF_FPRINTD = 0x03,
    S_OPF_FPRINTQ = 0x04,
    S_OPF_FNEG    = 0x05,
    S_OPF_FABS    = 0x09,
    S_OPF_FSQRTS  = 0x29,
    S_OPF_FSQRTD  = 0x2a,
    S_OPF_FSQRTQ  = 0x2b,
    S_OPF_FADDS   = 0x41,
    S_OPF_FADDD   = 0x42,
    S_OPF_FADDQ   = 0x43,
    S_OPF_FSUBS   = 0x45,
    S_OPF_FSUBD   = 0x46,
    S_OPF_FSUBQ   = 0x47,
    S_OPF_FMULS   = 0x49,
    S_OPF_FMULD   = 0x4a,
    S_OPF_FMULQ   = 0x4b,
    S_OPF_FDIVS   = 0x4d,
    S_OPF_FDIVD   = 0x4e,
    S_OPF_FDIVQ   = 0x4f,
    S_OPF_FCMPS   = 0x51,
    S_OPF_FCMPD   = 0x52,
    S_OPF_FCMPQ   = 0x53,
    S_OPF_FCMPES  = 0x55,
    S_OPF_FCMPED  = 0x56,
    S_OPF_FCMPEQ  = 0x57,
    S_OPF_FSMULD  = 0x69,
    S_OPF_FDMULQ  = 0x6e,
    S_OPF_FITOS   = 0xc4,
    S_OPF_FDTOS   = 0xc6,
    S_OPF_FQTOS   = 0xc7,
    S_OPF_FITOD   = 0xc8,
    S_OPF_FSTOD   = 0xc9,
    S_OPF_FQTOD   = 0xcb,
    S_OPF_FITOQ   = 0xcc,
    S_OPF_FSTOQ   = 0xcd,
    S_OPF_FDTOQ   = 0xce,
    S_OPF_FSTOI   = 0xd1,
    S_OPF_FDTOI   = 0xd2,
    S_OPF_FQTOI   = 0xd3,
};



// LEON2MT State Register flags
static const PSR PSR_CWP   = 0x0000001FUL; // Current Window Pointer
static const PSR PSR_ET    = 0x00000020UL; // Enable Traps
static const PSR PSR_PS    = 0x00000040UL; // Previous Supervisor
static const PSR PSR_S     = 0x00000080UL; // Supervisor
static const PSR PSR_PIL   = 0x00000F00UL; // LEON2MT Interrupt Level
static const PSR PSR_EF    = 0x00001000UL; // Enable Floating Point
static const PSR PSR_EC    = 0x00002000UL; // Enable Coprocessor
static const PSR PSR_ICC   = 0x00F00000UL; // Integer Condition Codes
static const PSR PSR_ICC_C = 0x00100000UL; // ICC: Carry
static const PSR PSR_ICC_V = 0x00200000UL; // ICC: Overflow
static const PSR PSR_ICC_Z = 0x00400000UL; // ICC: Zero
static const PSR PSR_ICC_N = 0x00800000UL; // ICC: Negative
static const PSR PSR_VER   = 0x0F000000UL; // Version
static const PSR PSR_IMPL  = 0xF0000000UL; // Implementation

// Floating-Point State Register flags
static const FSR FSR_CEXC    = 0x0000001FUL; // Current Exception
static const FSR FSR_AEXC    = 0x000003E0UL; // Accrued Exception
static const FSR FSR_FCC     = 0x00000C00UL; // FPU condition codes
static const FSR FSR_FCC_EQ  = 0x00000000UL; // FCC: Equal
static const FSR FSR_FCC_LT  = 0x00000400UL; // FCC: Less-than
static const FSR FSR_FCC_GT  = 0x00000800UL; // FCC: Greater-than
static const FSR FSR_FCC_UO  = 0x00000C00UL; // FCC: Unordered
static const FSR FSR_QNE     = 0x00002000UL; // FQ Not Empty
static const FSR FSR_FTT     = 0x0001C000UL; // Floating-Point Trap Type
static const FSR FSR_VER     = 0x000E0000UL; // Version
static const FSR FSR_NS      = 0x00400000UL; // Nonstandard FP
static const FSR FSR_TEM     = 0x0F800000UL; // Trap Enable Mask
static const FSR FSR_RD      = 0xC0000000UL; // Rounding Direction
static const FSR FSR_RD_NEAR = 0x00000000UL; // RD: Nearest
static const FSR FSR_RD_ZERO = 0x40000000UL; // RD: Zero
static const FSR FSR_RD_PINF = 0x80000000UL; // RD: +infinity
static const FSR FSR_RD_NINF = 0xC0000000UL; // RD: -infinity

// Latch information for Pipeline
struct ArchDecodeReadLatch
{
    uint8_t  op1, op2, op3;
    uint16_t function;
    uint8_t  asi;
    int32_t  displacement;

    // Memory store data source
    RegAddr      Rs;
    bool         RsIsLocal;
    unsigned int RsSize;

    ArchDecodeReadLatch() : op1(0), op2(0), op3(0), function(0), asi(0), displacement(0), Rs(), RsIsLocal(false),  RsSize(0) {}
    virtual ~ArchDecodeReadLatch() {}
};

struct ArchReadExecuteLatch : public ArchDecodeReadLatch
{
    PipeValue Rsv;
    ArchReadExecuteLatch() : ArchDecodeReadLatch(), Rsv() {}
};


#endif
