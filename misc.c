#define _GNU_SOURCE
#include <stdint.h>
#include <time.h>
#include <stdatomic.h>

atomic_int_least32_t globActPos = 0, globDemPos = 0, globCurrent = 0, globStatusWord = 0, globStateVar = 0, globWarnWord = 0, globErrorCode = 0;
atomic_int_least64_t globNanoTime = 0;

long ts_to_ns(struct timespec* ts) {
    return ts->tv_sec * 1000000000L + ts->tv_nsec;
}

int64_t getNanotime(void) {
    int64_t i;
    struct timespec ntime;
    clock_gettime(CLOCK_REALTIME, &ntime);
    i = ((int64_t)ntime.tv_sec * 1000000000) + (int64_t) ntime.tv_nsec;
    return i;
}

