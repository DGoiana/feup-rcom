// Link layer protocol implementation

#include "../include/link_layer.h"
#include "../include/serial_port.h"
#include "constants.h"
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int alarmEnabled = false;
int alarmCount = 0;
LinkLayer cp;
int file_descriptor = -1;

void alarmHandler(int signal)
{
    alarmEnabled = true;
    alarmCount++;
}

int sendMessage(int fd, int A, int C)
{
    unsigned char buf[5] = {0};

    buf[0] = F_FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = A ^ C;
    buf[4] = F_FLAG;
    int bytes = write(fd, buf, 5);
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{   
    cp = connectionParameters;
    file_descriptor = openSerialPort(connectionParameters.serialPort,
                            connectionParameters.baudRate);
    int state = 0;
    unsigned char byte = 0x00;
    if (file_descriptor < 0)
        return -1;

    if (connectionParameters.role == LlTx)
    {
        (void)signal(SIGALRM,alarmHandler);
        while(connectionParameters.nRetransmissions != 0 && state != STOP) {

            sendMessage(file_descriptor,A_TX,C_SET);
            printf("sent C_SET\n");
            alarmEnabled = false;
            alarm(connectionParameters.timeout);

            
            while(alarmEnabled == false && state != STOP) {
                if (read(file_descriptor,&byte,1) > 0) {
                    switch (state)
                    {
                        case START:
                            if (byte == F_FLAG) {
                                state =  FLAG_RCV;
                                break;
                            }
                            else {
                                state =  state;
                                break;
                            }
                        case FLAG_RCV:
                            if (byte == F_FLAG) {
                                state =  state;
                                break;
                            }
                            else if (byte == A_RC) {
                                state =  A_RCV;
                                break;
                            }
                            else {
                                state =  START;
                                break;
                            }
                        case A_RCV:
                            if (byte == C_UA) {
                                state =  C_RCV;
                                break;
                            }
                            else if (byte == F_FLAG) {
                                state =  FLAG_RCV;
                                break;
                            }
                            else {
                                state =  START;
                                break;
                            }
                        case C_RCV:
                            if (byte == (A_RC ^ C_UA)) {
                                state =  BCC_OK;
                                break;
                            }
                            else if (byte == F_FLAG) {
                                state =  FLAG_RCV;
                                break;
                            }
                            else {
                                state =  START;
                                break;
                            }
                        case BCC_OK:
                            if (byte == F_FLAG) {
                                state =  STOP;
                                break;
                            }
                            else {
                                state =  START;
                                break;
                            }
                        default:
                            break;
                    }
                }
            }
            connectionParameters.nRetransmissions--;
        }
        if(state != STOP) { 
            printf("timed out\n");
            return -1;
        } else {
            printf("received C_UA\n");
            printf("opened\n");
            return 0;
        }
    }
    else
    {
        state = 0;
        byte = 0x00;
        while(state != STOP) {
            if (read(file_descriptor,&byte,1) > 0) {
                switch (state)
                {
                    case START:
                        if (byte == F_FLAG) {
                            state =  FLAG_RCV;
                            break;
                        }
                        else {
                            state =  state;
                            break;
                        }
                    case FLAG_RCV:
                        if (byte == F_FLAG) {
                            state =  state;
                            break;
                        }
                        else if (byte == A_TX) {
                            state =  A_RCV;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    case A_RCV:
                        if (byte == C_SET) {
                            state =  C_RCV;
                            break;
                        }
                        else if (byte == F_FLAG) {
                            state =  FLAG_RCV;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    case C_RCV:
                        if (byte == (A_TX ^ C_SET)) {
                            state =  BCC_OK;
                            break;
                        }
                        else if (byte == F_FLAG) {
                            state =  FLAG_RCV;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    case BCC_OK:
                        if (byte == F_FLAG) {
                            state =  STOP;
                            printf("wtf\n");
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    default:
                        break;
                }
            }
        }
        printf("received C_SET\n");
        sendMessage(file_descriptor,A_RC,C_UA);
        printf("sent C_UA\n");
        // printf("NOT sent C_UA");
        printf("opened\n");
    }
    return file_descriptor;
}



////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    int state = 0;
    unsigned char byte = 0x00;
    if (file_descriptor < 0)
        return -1;

    if (cp.role == LlTx)
    {
        (void)signal(SIGALRM,alarmHandler);
        while(cp.nRetransmissions != 0 && state != STOP) {

            sendMessage(file_descriptor,A_TX,C_DISC);
            printf("sent C_DISC\n");
            alarmEnabled = false;
            alarm(cp.timeout);
            
            while(alarmEnabled == false && state != STOP) {
                if (read(file_descriptor,&byte,1) > 0) {
                    printf("var: 0x%02x\n",byte);
                    printf("current state %d\n",state);
                    switch (state)
                    {
                        case START:
                            if (byte == F_FLAG) {
                                state =  FLAG_RCV;
                                break;
                            }
                            else {
                                state =  state;
                                break;
                            }
                        case FLAG_RCV:
                            if (byte == F_FLAG) {
                                state =  state;
                                break;
                            }
                            else if (byte == A_RC) {
                                state =  A_RCV;
                                break;
                            }
                            else {
                                state =  START;
                                break;
                            }
                        case A_RCV:
                            if (byte == C_DISC) {
                                state =  C_RCV;
                                break;
                            }
                            else if (byte == F_FLAG) {
                                state =  FLAG_RCV;
                                break;
                            }
                            else {
                                state =  START;
                                break;
                            }
                        case C_RCV:
                            if (byte == (A_RC ^ C_DISC)) {
                                state =  BCC_OK;
                                break;
                            }
                            else if (byte == F_FLAG) {
                                state =  FLAG_RCV;
                                break;
                            }
                            else {
                                state =  START;
                                break;
                            }
                        case BCC_OK:
                            if (byte == F_FLAG) {
                                state =  STOP;
                                break;
                            }
                            else {
                                state =  START;
                                break;
                            }
                        default:
                            break;
                    }
                }
            }
            cp.nRetransmissions--;
        }
        if(state != STOP) { 
            printf("timed out\n");
        } else {
            printf("received C_DISC\n");
            sendMessage(file_descriptor,A_TX,C_UA);
            printf("sent first SET_UA\n");
            printf("closed\n");
        }
    }
    else
    {
        state = 0;
        byte = 0x00;
        while(state != STOP) {
            if (read(file_descriptor,&byte,1) > 0) {
                switch (state)
                {
                    case START:
                        if (byte == F_FLAG) {
                            state =  FLAG_RCV;
                            break;
                        }
                        else {
                            state =  state;
                            break;
                        }
                    case FLAG_RCV:
                        if (byte == F_FLAG) {
                            state =  state;
                            break;
                        }
                        else if (byte == A_TX) {
                            state =  A_RCV;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    case A_RCV:
                        if (byte == C_DISC) {
                            state =  C_RCV;
                            break;
                        }
                        else if (byte == F_FLAG) {
                            state =  FLAG_RCV;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    case C_RCV:
                        if (byte == (A_TX ^ C_DISC)) {
                            state =  BCC_OK;
                            break;
                        }
                        else if (byte == F_FLAG) {
                            state =  FLAG_RCV;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    case BCC_OK:
                        if (byte == F_FLAG) {
                            state =  STOP;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    default:
                        break;
                }
            }
        }
        printf("received C_DISK\n");
        state = 0;
        sendMessage(file_descriptor,A_RC,C_DISC);
        printf("sent C_DISK\n");
        //printf("NOT sent C_DISC\n");
        while(state != STOP) {
            if (read(file_descriptor,&byte,1) > 0) {
                switch (state)
                {
                    case START:
                        if (byte == F_FLAG) {
                            state =  FLAG_RCV;
                            break;
                        }
                        else {
                            state =  state;
                        }
                    case FLAG_RCV:
                        if (byte == F_FLAG) {
                            state =  state;
                            break;
                        }
                        else if (byte == A_TX) {
                            state =  A_RCV;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    case A_RCV:
                        if (byte == C_UA) {
                            state =  C_RCV;
                            break;
                        }
                        else if (byte == F_FLAG) {
                            state =  FLAG_RCV;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    case C_RCV:
                        if (byte == (A_TX ^ C_UA)) {
                            state =  BCC_OK;
                            break;
                        }
                        else if (byte == F_FLAG) {
                            state =  FLAG_RCV;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    case BCC_OK:
                        if (byte == F_FLAG) {
                            state =  STOP;
                            break;
                        }
                        else {
                            state =  START;
                            break;
                        }
                    default:
                        break;
                }
            }
        }
        printf("received C_UA\n");
        printf("closed\n");
    }
    int clstat = closeSerialPort();
    return clstat;
}
