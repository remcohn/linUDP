#define _GNU_SOURCE
#include <stdint.h>
#include <time.h>
#include <stdatomic.h>
#include <pthread.h>

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

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    int             request_ready;
    int             response_ready;

    int32_t cmd;
    uint32_t upid;
    uint32_t value;
} MessageChannel;

extern MessageChannel c1250CmdChannel;

// linUDP.c


// ticker.c
void* ticker(void* arg);

// gui.c
void* gtk_thread_func(void* arg);

// buttons.c
void *canReceiveThread(void *data);

// c1250.c
void c1250WriteControlWord(uint32_t value);
void c1250WriteInt32(uint32_t upid, int32_t value);

// misc.c
long ts_to_ns(struct timespec* ts);
int64_t getNanotime(void);
void delay(int milliseconds);



#define E1100_RUN_MODE_SELECTION        0x1450
#define E1100_RUN_MODE_SELECTION_MCI            1
#define E1100_RUN_MODE_SELECTION_VAI2POSCON     9
#define E1100_VAI2POSCON_1_POS          0x145A  // sint32       0.0001mm
#define E1100_VAI2POSCON_1_SPEED        0x145B  // sint32       0.000001 m/s
#define E1100_VAI2POSCON_1_ACCEL        0x145C  // sint32       0.00001 m/s2
#define E1100_VAI2POSCON_1_DECEL        0x145D  // sint32       0.00001 m/s2
#define E1100_VAI2POSCON_1_WAIT         0x147D  // uint16       0.1ms                   0 - 5000ms
#define E1100_VAI2POSCON_2_POS          0x145F
#define E1100_VAI2POSCON_2_SPEED        0x1460
#define E1100_VAI2POSCON_2_ACCEL        0x1461
#define E1100_VAI2POSCON_2_DECEL        0x1462
#define E1100_VAI2POSCON_2_WAIT         0x147E
#define E1100_PID_P                                     0x13A2  // uint16       0.1A/mm
#define E1100_PID_D                                     0x13A8  // uint16       0.1A/m/s
#define E1100_MAX_I                                     0x13A6  // sint16       0.001A (1mA)

#define E1100_FACTOR_POS                        10000           // from mm
#define E1100_FACTOR_SPEED                      1000000         // from m/s
#define E1100_FACTOR_ACCEL                      100000          // from m/s2
