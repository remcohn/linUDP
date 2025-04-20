#define _GNU_SOURCE
#include <stdint.h>
#include <time.h>
#include <stdatomic.h>

// Global vars
struct Motion {
    _Atomic float pos1;                     // most back position, in mm
    _Atomic float pos2;                     // most front position, in mm
    _Atomic float stroke;                   // stroke length, in mm
    _Atomic float speed;                    // top speed in m/s
    _Atomic float accel;  
    
    _Atomic int32_t actPos;
    _Atomic int32_t demPos;
    _Atomic int32_t current;
    _Atomic uint32_t statusWord;
    _Atomic uint32_t stateVar;
    _Atomic uint32_t warnWord;
    _Atomic uint32_t errorCode;
    _Atomic int64_t nanotime;

    _Atomic int32_t halt;
};

extern struct Motion m;

// linUDP.c


// ticker.c
void* ticker(void* arg);

// gui.c
void* gtk_thread_func(void* arg);

// buttons.c
void *canReceiveThread(void *data);

// misc.c
long ts_to_ns(struct timespec* ts);
int64_t getNanotime(void);

