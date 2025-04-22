#define _GNU_SOURCE
#include <stdint.h>
#include <time.h>
#include <stdatomic.h>

#include "linUDP.h"

struct Motion m;

MessageChannel c1250CmdChannel;

void delay(int milliseconds) {
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}

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

