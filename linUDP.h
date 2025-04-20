#define _GNU_SOURCE
#include <stdint.h>
#include <time.h>
#include <stdatomic.h>

// Global vars
extern atomic_int_least32_t globActPos, globDemPos, globCurrent, globStatusWord, globStateVar, globWarnWord, globErrorCode;
extern atomic_int_least64_t globNanoTime;

// linUDP.c


// ticker.c
void* ticker(void* arg);

// gui.c
void* gtk_thread_func(void* arg);

// misc.c
long ts_to_ns(struct timespec* ts);
int64_t getNanotime(void);

