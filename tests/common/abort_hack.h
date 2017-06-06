#include <svp/abort.h>

#ifndef ABORT_HACK

#define ABORT_HACK

__attribute__((always_inline))
void abort(void) {
    svp_abort();
}

#endif
