#define _GNU_SOURCE
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <stdatomic.h>

#include "linUDP.h"

#define ENC_LENGTH_STEP 2.5;

struct sockaddr_can addr;
int canSocket;

void *canReceiveThread(void *data) {
    struct ifreq ifr;
    int i; 
    int nbytes;

    struct can_frame frame;
    int halt = 0;

    int updateAccel = 0;

    if ((canSocket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Socket");
        return NULL;
    }

    strcpy(ifr.ifr_name, "can0" );
    ioctl(canSocket, SIOCGIFINDEX, &ifr);

    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(canSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("Bind");
            return 1;
    }

    while (!halt) {
        nbytes = read(canSocket, &frame, sizeof(struct can_frame));
        if (nbytes < 0) {
            printf("read() error from canProcess thread\n");
            return 0;
        }

        if (frame.can_id == 0x1A0 || frame.can_id == 0x2A0 || frame.can_id == 0x3A0 || frame.can_id == 0x4A0) {
            int ch = frame.can_id >> 8;
            int rot = frame.data[0] | (frame.data[1]<<8) | (frame.data[2]<<16) | (frame.data[3]<<24);
            int8_t dir = frame.data[4];
            int8_t btn = frame.data[5];

            if (btn == 0) {
                printf("ESTOP\n");
                c1250WriteControlWord(0x003E); // Not enable
                atomic_store(&m.halt, 1);
                return NULL;
            }

            if (ch == 1 || ch == 2) {
                float pos1 =  atomic_load(&m.pos1);
                float stroke =  atomic_load(&m.stroke);

                if (ch == 1) {
                    if (dir == 1) pos1 += ENC_LENGTH_STEP;
                    if (dir == -1) pos1 -= ENC_LENGTH_STEP;
                    if (pos1 < 0) pos1 = 0;
                    if (pos1 > 290) pos1 = 290;
                    if (pos1 + stroke > 300) stroke = 300 - pos1;   // limit stroke length is start pos is too far
                }

                if (ch == 2) {
                    if (dir == 1) stroke += ENC_LENGTH_STEP;
                    if (dir == -1) stroke -= ENC_LENGTH_STEP;
                    if (stroke < 0) stroke = 0;
                    if (stroke > 300) stroke = 300;
                    if (pos1 + stroke > 300) pos1 = 300 -  stroke;   // limit start pos if stroke length is too long
                }

                float pos2 = pos1 + stroke;
                atomic_store(&m.pos1, pos1);
                atomic_store(&m.pos2, pos2);
                atomic_store(&m.stroke, stroke);
             
                c1250WriteInt32(E1100_VAI2POSCON_1_POS, m.pos1 * E1100_FACTOR_POS);
                c1250WriteInt32(E1100_VAI2POSCON_2_POS, m.pos2 * E1100_FACTOR_POS);
            }

            if (ch == 3) {
                float speed = atomic_load(&m.speed);
                if (dir == 1) speed += 0.025;
                if (dir == -1) speed -= 0.025;
                if (speed < 0.05) speed = 0.05;
                if (speed > 3.0) speed = 3.0;
                atomic_store(&m.speed, speed);
                
                c1250WriteInt32(E1100_VAI2POSCON_1_SPEED, m.speed * E1100_FACTOR_SPEED);
                c1250WriteInt32(E1100_VAI2POSCON_2_SPEED, m.speed * E1100_FACTOR_SPEED);
            }

            if (ch == 4) {
                float accel = atomic_load(&m.accel);
                if (dir == 1) accel += 0.1;
                if (dir == -1) accel  -= 0.1;
                if (accel < 0.2) accel = 0.2;
                if (accel > 25) accel = 25;
                atomic_store(&m.accel, accel);
                
                c1250WriteInt32(E1100_VAI2POSCON_1_ACCEL, m.accel * E1100_FACTOR_ACCEL);
                c1250WriteInt32(E1100_VAI2POSCON_1_DECEL, m.accel * E1100_FACTOR_ACCEL);
                c1250WriteInt32(E1100_VAI2POSCON_2_ACCEL, m.accel * E1100_FACTOR_ACCEL);
                c1250WriteInt32(E1100_VAI2POSCON_2_DECEL, m.accel * E1100_FACTOR_ACCEL);
            }
            printf("pos1: %0.1f stroke: %0.1f pos2: %0.1f speed: %0.3f accel: %0.1f\n",m.pos1,m.stroke, m.pos2, m.speed,m.accel);
        } 
/*
        if (frame.can_id == 0x282) {
                uint16_t cmdWord = frame.data[0] | (frame.data[1] << 8);
                int16_t actPos = frame.data[2] | (frame.data[3] << 8);
                int16_t demI = frame.data[4] | (frame.data[5] << 8);
                int16_t demPos = frame.data[6] | (frame.data[7] << 8);
                m.motReqPos = (float)demPos / 100.0;
                m.motActPos = (float)actPos / 100.0;
                m.motReqCur = (float)demI / 1000.0;
                m.motNewData = 1;

                //printf(": %04X %3.2f %3.2f %3.2f\n", cmdWord, m.motReqPos, m.motActPos, m.motReqCur);
                doMotorLog();
        } else if (frame.can_id == 0x182) {
                m.statusWord = frame.data[0] | (frame.data[1] << 8);
                m.statusVar = frame.data[2] | (frame.data[3] << 8);
                m.loggedErrorCode = frame.data[4] | (frame.data[5] << 8);
                m.warnWord = frame.data[6] | (frame.data[7] << 8);
                doLogStatus(m.statusWord, m.statusVar, m.loggedErrorCode, m.warnWord);

                if (m.warnWord & 0x0002) {      // overload warning
                        if (m.accel > 2) m.accel -= 0.2;
                        updateAccel = 1;
                        printf("Overload warning, access reduced\n");
                }

        } else {

        }
*/

        } 
}
