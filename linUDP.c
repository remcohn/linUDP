#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/resource.h>

#include "linUDP.h"

union linudpCmdNOP {
   char bytes[8];
   struct {
      uint32_t reqBits;
      uint32_t repBits;
   };
};

union linudpCmdRTC {
        char bytes[16];
        struct {
                uint32_t reqBits;
                uint32_t repBits;
                uint8_t rtcCnt;
                uint8_t rtcCmd;
                uint16_t rtcParam1;
                uint16_t rtcParam2;
                uint16_t rtcParam3;
        };
};

/*
union linudpRep {
        char bytes[64];
        struct {
                uint32_t reqBits;
                uint32_t repBits;
                uint16_t statusWord;
                uint16_t stateVar;
                uint32_t actPos;
                uint32_t demPos;
                uint16_t current;
                uint16_t warnWord;
                uint16_t errorCode;
                uint32_t mon1;
                uint32_t mon2;
                uint32_t mon3;
                uint32_t mon4;
                uint16_t rtcStatus;
                uint16_t rtcUPID;
                uint32_t rtcValue;
        };
};
*/

int main() {
   // Ticker interface thread

    pthread_t tickerThread;
    pthread_attr_t attr;
    struct sched_param param;

    setpriority(PRIO_PROCESS, 0, -20);

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    param.sched_priority = 99;  // Priority between 1 (low) and 99 (high)
    pthread_attr_setschedparam(&attr, &param);

    if (pthread_create(&tickerThread, &attr, ticker, NULL)) {
        perror("pthread_create");
        return 1;
    }

   // GUI thread
   pthread_t gtk_thread;

    // Create GUI in a separate thread
    if (pthread_create(&gtk_thread, NULL, gtk_thread_func, NULL) != 0) {
        perror("Failed to create GTK thread");
        return EXIT_FAILURE;
    }

    // Optionally: join (blocks forever in this case)
    pthread_join(tickerThread, NULL);


    return 0;
}

