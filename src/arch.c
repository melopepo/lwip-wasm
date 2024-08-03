#include <emscripten/emscripten.h>
#include <stdint.h>

#include "arch/sys_arch.h"

// Time.
static double start_time = -1;

uint32_t sys_now(void) {
    double now = emscripten_get_now();
    if (start_time == -1) {
        start_time = now;
    }
    return (uint32_t)(now - start_time);
}

// No-op, as we use NO_SYS mode.
// There is no need for critical sections, because:
// - We don't have interrupts.
// - We don't use multiple threads.˝
sys_prot_t sys_arch_protect(void) { return NULL; }
void sys_arch_unprotect(sys_prot_t pval) { (void)(pval); }
