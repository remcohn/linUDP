#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdatomic.h>
#include <stdint.h>

#include "linUDP.h"

#define PERIOD_NS (1 * 1000 * 1000)  // 1 ms in nanoseconds

#define DRIVE_IP "192.168.100.10" // Replace with your drive's IP address
#define DRIVE_PORT 49360         // LinUDP drive port
#define MASTER_PORT 41136        // LinUDP master port
#define BUFFER_SIZE 1024

int tickMissed = 0;

void* ticker(void* arg) {
    struct timespec next, now;
    int globRtcCnt = 0;

    // ******************************** LinMot C1250 socket
    int sockfd;
    struct sockaddr_in drive_addr;
    char send_buffer[64];
    char recv_buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(drive_addr);

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind to master's LinUDP port
    struct sockaddr_in master_addr;
    memset(&master_addr, 0, sizeof(master_addr));
    master_addr.sin_family = AF_INET;
    master_addr.sin_addr.s_addr = INADDR_ANY;
    master_addr.sin_port = htons(MASTER_PORT);

    if (bind(sockfd, (struct sockaddr *)&master_addr, sizeof(master_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Setup drive address
    memset(&drive_addr, 0, sizeof(drive_addr));
    drive_addr.sin_family = AF_INET;
    drive_addr.sin_port = htons(DRIVE_PORT);
    inet_pton(AF_INET, DRIVE_IP, &drive_addr.sin_addr);

    // set the recv_from() timeout to 2x the loop speed
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = PERIOD_NS * 2;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // ******************************** InfluxDB socket
    int influxSocket;
    struct sockaddr_in influxServer_addr;

    influxSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(influxSocket < 0){
        printf("Error while creating influx socket\n");
        return NULL;
    }
    
    influxServer_addr.sin_family = AF_INET;
    influxServer_addr.sin_port = htons(8089);
    influxServer_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    // ******************************** Lets go!
    printf("Ticker thread ready to tick\n");
    
    clock_gettime(CLOCK_MONOTONIC, &next);
    int c = 0;

    while (1) {
        uint64_t nanotime = getNanotime();
        
        ssize_t recv_bytes;
        // try a recv_from() with MSG_DONTWAIT to make sure there are no waiting packets. there shouldnt be any
        recv_bytes = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr *)&drive_addr, &addr_len);
        if (recv_bytes > 0) {
            perror("recvfrom failed: we didnt expect data!");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // TODO: Check if we have a new ControlWord to include
        
        // TODO: Check if we have a MC Control message to include
        
        // TODO: Check if we have any RealTimeControl messages to include

        // TODO: Assemble TX packet

        *(uint32_t *)&send_buffer[0] = 0x00000000; // reqBits
        *(uint32_t *)&send_buffer[4] = 0x000000FF; // repBits
        
        ssize_t sent_bytes = sendto(sockfd, send_buffer, 8, 0, (struct sockaddr *)&drive_addr, addr_len);
        if (sent_bytes < 0) {
            perror("sendto failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        
        recv_bytes = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&drive_addr, &addr_len);
        if (recv_bytes < 0) {
            perror("recvfrom failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        /*printf("Received %ld bytes from drive:\n", recv_bytes);
        for (int i = 0; i < recv_bytes; i++) {
            printf("0x%02X ", (unsigned char)recv_buffer[i]);
        }
        printf("\n"); */

        int ptr = 8; // first data byte
        uint32_t flags = le32toh(*(uint32_t *)&recv_buffer[4]); // lets be nice and fix endianness

        //printf("flags: %08X\n", flags);

        uint32_t statusWord = 0, stateVar = 0, warnWord = 0, errorCode = 0, current = 0;
        int32_t actPos = 0, demPos = 0;

        if (flags & 0xFF != 0xFF) {
            printf("Invalid return flags: %02X\n", flags);
            return NULL;
        }

        statusWord = le16toh(*(uint32_t *)&recv_buffer[ptr]);  ptr += 2;
        stateVar = le16toh(*(uint32_t *)&recv_buffer[ptr]); ptr += 2;
        actPos = le32toh(*(uint32_t *)&recv_buffer[ptr]); ptr += 4;
        demPos = le32toh(*(uint32_t *)&recv_buffer[ptr]); ptr += 4;
        current = le16toh(*(uint32_t *)&recv_buffer[ptr]); ptr += 2;
        warnWord = le16toh(*(uint32_t *)&recv_buffer[ptr]); ptr += 2;
        errorCode = le16toh(*(uint32_t *)&recv_buffer[ptr]); ptr += 2;

        if (flags & 0x80) {
            ptr += 16;
            //printf("found monitoring data. not decoded yet\n");
        }

        atomic_store(&globStatusWord, statusWord);
        atomic_store(&globStateVar, stateVar);        
        atomic_store(&globActPos, actPos);
        atomic_store(&globDemPos, demPos);
        atomic_store(&globCurrent, current);
        atomic_store(&globWarnWord, warnWord);
        atomic_store(&globErrorCode, errorCode);
        atomic_store(&globNanoTime, nanotime);

        // ******************************** Log data to InfluxDB
        char client_message[1024];
        sprintf(client_message, "log motReqPos=%0.4f,motActPos=%0.4f,mosReqCur=%0.3f %lli", (float) demPos / 10000.0, (float)actPos / 10000.0, (float) current / 1000.0, nanotime);
        if(sendto(influxSocket, client_message, strlen(client_message), 0, (struct sockaddr*)&influxServer_addr, sizeof(influxServer_addr)) < 0) {
            printf("Unable to send message\n");
            return NULL;
        }

        // ******************************** Process RealTimeCommand data
        if (flags & 0x100) {
            uint16_t p1 = le16toh(*(uint32_t *)&recv_buffer[ptr]); ptr += 2;
            uint16_t p2 = le16toh(*(uint32_t *)&recv_buffer[ptr]); ptr += 2;
            uint16_t p3 = le16toh(*(uint32_t *)&recv_buffer[ptr]); ptr += 2;
            uint16_t p4 = le16toh(*(uint32_t *)&recv_buffer[ptr]); ptr += 2;
            int32_t px = (p4<<16) | p3;
            printf("RTC: %04X %04X %d\n", p1, p2, px);
        }

        // ******************************** Do some timing stuff to prepare for the next cycle
        // Mark end of work, Calculate time spent working (active time)
        clock_gettime(CLOCK_MONOTONIC, &now);
        long work_ns = ts_to_ns(&now) - ts_to_ns(&next);
        float cpu_load = (float)work_ns / PERIOD_NS * 100.0f;
        
        if (c++ % 100 == 0) {
            printf("%04X %04X %10.4f %10.4f %4d %04X %04X -- ", statusWord, stateVar, (float)actPos / 10000.0, (float) demPos / 10000.0, current, warnWord, errorCode);
            printf("Load = %.3f%% (%ld ns) (%d)\n", cpu_load, work_ns, tickMissed);
        }

        // Calculate next wake time
        next.tv_nsec += PERIOD_NS;
        if (next.tv_nsec >= 1000000000) {
            next.tv_nsec -= 1000000000;
            next.tv_sec += 1;
        }

        long lateness_ns = ts_to_ns(&now) - ts_to_ns(&next);
        if (lateness_ns > 0) {
            printf("!!! MISSED OUR TARGET by %ld\n", lateness_ns);
            tickMissed++;
            if (tickMissed == 500) return NULL;
            //return NULL;
        }

        // Sleep until next period
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    return NULL;
}


