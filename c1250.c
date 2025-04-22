#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "linUDP.h"


void c1250WriteControlWord(uint32_t value) {
    pthread_mutex_lock(&c1250CmdChannel.lock);

    c1250CmdChannel.cmd = 1;   // int32 write controlWord
    c1250CmdChannel.value = value;
    c1250CmdChannel.request_ready = 1;

    pthread_mutex_unlock(&c1250CmdChannel.lock);
    
    pthread_mutex_lock(&c1250CmdChannel.lock);
    while (!c1250CmdChannel.response_ready)  pthread_cond_wait(&c1250CmdChannel.cond, &c1250CmdChannel.lock);

    c1250CmdChannel.response_ready = 0;
    pthread_mutex_unlock(&c1250CmdChannel.lock);  
}

void c1250WriteInt32(uint32_t upid, int32_t value) {
    
    //printf("Queueing command\n");
    
    pthread_mutex_lock(&c1250CmdChannel.lock);

    c1250CmdChannel.upid = upid;
    c1250CmdChannel.cmd = 2;   // int32 write command
    c1250CmdChannel.value = value;
    c1250CmdChannel.request_ready = 1;

    pthread_mutex_unlock(&c1250CmdChannel.lock);
    
    //printf("Waiting for an answer...\n");
    
    pthread_mutex_lock(&c1250CmdChannel.lock);
    while (!c1250CmdChannel.response_ready)  pthread_cond_wait(&c1250CmdChannel.cond, &c1250CmdChannel.lock);

    c1250CmdChannel.response_ready = 0;
    pthread_mutex_unlock(&c1250CmdChannel.lock);  
    
    //printf("command confirmed\n");
}
