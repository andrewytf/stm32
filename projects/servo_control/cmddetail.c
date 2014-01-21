#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "chprintf.h"

#include "cmddetail.h"



void cmd_mem(BaseSequentialStream *out, int _, char *__[]) {
    (void)_;
    (void)__;

    size_t fragments, size;

    fragments = chHeapStatus(NULL, &size);
    chprintf(out, "core free memory : %u bytes\r\n", chCoreStatus());
    chprintf(out, "heap fragments   : %u\r\n", fragments);
    chprintf(out, "heap free total  : %u bytes\r\n", size);
}


void cmd_threads(BaseSequentialStream *out, int _, char *__[]) {
    (void)_;
    (void)__;

    static const char *THREAD_STATES[] = {THD_STATE_NAMES};
    Thread *t;

    chprintf(out, "addr\t\tstack\t\tprio\trefs\tstate\t\ttime\tname\r\n");

    t = chRegFirstThread();
    do {
        chprintf( out
                , "%.8lx\t%.8lx\t%4lu\t%4lu\t%9s\t%lu\t%s\r\n"
                , (uint32_t)t               // address
                , (uint32_t)t->p_ctx.r13    // stack
                , (uint32_t)t->p_prio       // prio
                , (uint32_t)(t->p_refs - 1) // refs
                , THREAD_STATES[t->p_state] // state
                , (uint32_t)t->p_time       // time
                , t->p_name                 // name
                );
        t = chRegNextThread(t);
    } while (t != NULL);
}


void cmd_pwmlims(BaseSequentialStream *out, int argc, char* argv[]) {
    if (argc == 0) {
        chprintf(out, "lo: %i\thi: %i\r\n", pwm_lo, pwm_hi);
        return;
    }

    if (argc != 2) {
        chprintf(out, "usage: pwm [lo hi]\r\n");
        return;
    }

    int lo = atoi(argv[0]);
    int hi = atoi(argv[1]);

    if (lo > hi) {
        chprintf(out, "ERROR: lo limit (%i) is greater than hi limit (%i)\r\n", lo, hi);
        return;
    }

    pwm_lo = lo;
    pwm_hi = hi;
}
