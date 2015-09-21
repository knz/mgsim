#include <sys_config.h>

#if defined(TARGET_MTALPHA)
#include "ISA.mtalpha.cpp"
#elif defined(TARGET_MTSPARC)
#include "ISA.mtsparc.cpp"
#elif defined(TARGET_MIPS32) || defined(TARGET_MIPS32EL)
#include "ISA.mips.cpp"
#elif defined(TARGET_OR1K)
#include "ISA.or1k.cpp"
#elif defined(TARGET_RISCV64)
#include "ISA.rv64.cpp"
#endif
