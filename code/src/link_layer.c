// Link layer protocol implementation

#include "../include/link_layer.h"
#include "../include/serial_port.h"
#include "constants.h"
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int alarmEnabled = false;
int alarmCount = 0;
LinkLayer cp;
int file_descriptor = -1;

int frame_ns = 0;
int frame_nr = 1;

int counter = 0;

void state_machine_sendSET(unsigned char byte, int *state, bool tx);
void state_machine_control_packet(int *state, unsigned char byte);
void state_machine_writes(int *state, unsigned char byte, bool bcc2_checked);
void state_machine_close(int *state, unsigned char byte);

void alarmHandler(int signal)
{
    alarmEnabled = true;
    alarmCount++;
}

int sendMessage(int A, int C)
{
    unsigned char buf[5] = {0};

    buf[0] = F_FLAG;
    buf[1] = A;
    buf[2] = C;
    buf[3] = BCC(A, C);
    buf[4] = F_FLAG;

    int bytes = writeBytesSerialPort(buf, 5);
}

int sendData(const unsigned char *buf, int bufSize)
{
    unsigned char send[BUF_SIZE + 1] = {0};
    unsigned char bcc2 = 0x00;

    send[0] = F_FLAG;
    send[1] = A_TX;
    send[2] = frame_ns == 1 ? C_FRAME1 : C_FRAME0;
    send[3] = BCC(send[1], send[2]);

    int index_buf = 0;
    int index_send = 4;

/*     int j = 0;
    while(j < bufSize) {
        printf("buf[i]: 0x%02X\n",buf[j]);
        j++;
    } */

    while (index_send < bufSize + 4) // will this work?
    {
        if(buf[index_buf] == F_FLAG) {
            send[index_send] = 0x7D;
            index_send++;
            send[index_send] = 0x5E;
        } else {
            send[index_send] = buf[index_buf];
        }
        bcc2 = BCC(bcc2, buf[index_buf]);
        index_buf++;
        index_send++;
    }

    index_send = index_send + 5;
    send[index_send] = bcc2;
    index_send++;
    send[index_send] = F_FLAG;

/*     int j = 0;
    while(send[j] != 'A') {
        printf("var: 0x%02x\n",send[j]);
        j++;
    }
    printf("\n"); */

    int frame_0_bytes = writeBytesSerialPort(send, index_send);
}

////////////////////////////////////////////////
// LLOPENbyte
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    cp = connectionParameters;
    file_descriptor = openSerialPort(connectionParameters.serialPort,
                                     connectionParameters.baudRate);
    int state = 0;
    unsigned char byte = 0x00;
    int tries = cp.nRetransmissions;
    if (file_descriptor < 0)
        return -1;

    if (connectionParameters.role == LlTx)
    {
        (void)signal(SIGALRM, alarmHandler);
        while (tries != 0 && state != STOP)
        {

            sendMessage(A_TX, C_SET);
            printf("sent C_SET\n");
            alarmEnabled = false;
            alarm(connectionParameters.timeout);

            while (alarmEnabled == false && state != STOP)
            {
                if (readByteSerialPort(&byte) > 0)
                    state_machine_sendSET(byte, &state, true);
            }
            tries--;
        }
        if (state != STOP)
        {
            printf("timed out\n");
            return -1;
        }
        else
        {
            printf("received C_UA\n");
            printf("opened\n");
            return 0;
        }
    }
    else
    {
        state = 0;
        byte = 0x00;

        while (state != STOP)
        {
            if (readByteSerialPort(&byte) > 0)
                state_machine_sendSET(byte, &state, false);
        }
        printf("received C_SET\n");
        sendMessage(A_RC, C_UA);
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
    // F  A  C  BCC1 D1 ... Dn BCC2 F
    (void)signal(SIGALRM, alarmHandler);

    int state = 0;
    int tries = cp.nRetransmissions;

    unsigned char byte = 0x00;

    // send i-ns and receive rr-nr
    while (state != STOP && tries != 0)
    {
        sendData(buf, bufSize);
        printf("sent I - ns0\n");
        alarmEnabled = false;
        alarm(cp.timeout);

        int i = 0;

        while (alarmEnabled == false && state != STOP)
        {
            if (readByteSerialPort(&byte) > 0)
                state_machine_control_packet(&state, byte);

            if (state == RESEND)
            {
                printf("resending I - ns0 without timeout\n");
                state = START;
                tries = cp.nRetransmissions + 1;
                break;
            }
        }
        tries--;
    }
    if (state != STOP)
    {
        printf("timed out\n");
        return -1;
    }
    else
    {
        printf("received RR - nr1\n");
        frame_ns = (frame_ns == 0 ? 1 : 0);
        printf("changed ns to %d\n", frame_ns);
        frame_nr = (frame_nr == 0 ? 1 : 0);
        printf("changed nr to %d\n", frame_nr);
    }
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    int i = 0;
    int state = 0;
    unsigned char byte = 0x00;

    unsigned char bcc2 = 0x00;
    unsigned char last = 0x00;

    bool bcc_checked = false;
    bool temp_stuffed = false;

    while (state != STOP)
    {
        if (readByteSerialPort(&byte) > 0)
        {
/*             printf("current_byte : 0x%02x\n",byte);
            printf("current_state : %d\n",state); */
            if (state == BCC2_CHECK)
            {
                bcc_checked = bcc2 == last;
                if(bcc_checked == false){

                    sendMessage(A_TX, (frame_ns == 0 ? C_REJ0 : C_REJ1));
                    state = START;
                    printf("sent error message\n");
                    bcc2 = 0x00;
                }
            }
            bcc2 = 0;

            if(state == DATA_STUFFED) {
                temp_stuffed = true;
            }
            if(state == STUFFED_RECEIVED ) {
                packet[i] = F_FLAG;
                i++;
                if(byte != ESC && byte != REPLACED) {
                    packet[i] = byte;
                    i++;
                }
                temp_stuffed = false;
            }
            if (state == BCC_OK && temp_stuffed == false && byte != ESC)
            {
                if (byte != ESC && byte != F_FLAG) {
                    packet[i] = byte;
                    i++;                
                }
                bcc2 = BCC(bcc2, byte);
                last = byte;
            }

            if(state == BCC_OK && temp_stuffed) {
                packet[i] = ESC;
                i++;
                packet[i] = byte;
                i++;
                temp_stuffed = false;
            }

               state_machine_writes(&state, byte, bcc_checked);

        }
    }
    
    packet[i] = '\0';
    printf("received i-ns\n");
    int j = 0;
    printf("received:");
    while(packet[j] != '\0') {
        printf(" 0x%02x ",packet[j]);
        j++;
    }
    printf("\n");
    sendMessage(A_TX, (frame_ns == 0 ? C_RR1 : C_RR0));
    printf("sent rr-nr\n");
    frame_ns = (frame_ns == 0 ? 1 : 0);
    frame_nr = (frame_nr == 0 ? 1 : 0);
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
        (void)signal(SIGALRM, alarmHandler);
        while (cp.nRetransmissions != 0 && state != STOP)
        {

            sendMessage(A_TX, C_DISC);
            printf("sent C_DISC\n");
            alarmEnabled = false;
            alarm(cp.timeout);

            while (alarmEnabled == false && state != STOP)
            {
                if (readByteSerialPort(&byte) > 0)
                    state_machine_close(&state, byte);
            }
            cp.nRetransmissions--;
        }
        if (state != STOP)
        {
            printf("timed out\n");
            return -1;
        }
        else
        {
            printf("received C_DISC\n");
            sendMessage(A_RC, C_UA);
            printf("sent final SET_UA\n");
            printf("closed\n");
        }
    }
    else
    {
        state = 0;
        byte = 0x00;

        while (state != STOP)
        {
            if (readByteSerialPort(&byte) > 0)
                state_machine_close(&state, byte);
        }
        printf("received C_DISK\n");

        state = 0;

        sendMessage(A_TX, C_DISC);
        printf("sent C_DISK\n");
        // printf("NOT sent C_DISC\n");
        while (state != STOP)
        {
            if (readByteSerialPort(&byte) > 0)
            {
                state_machine_sendSET(byte, &state, true);
            }
        }
        printf("received C_UA\n");
        printf("closed\n");
    }
    int clstat = closeSerialPort();
    return clstat;
}

void state_machine_sendSET(unsigned char byte, int *state, bool tx)
{
    switch (*state)
    {
    case START:
        if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = *state;
            break;
        }
    case FLAG_RCV:
        if (byte == F_FLAG)
        {
            *state = *state;
            break;
        }
        else if (byte == (tx ? A_RC : A_TX))
        {
            *state = A_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case A_RCV:
        if (byte == (tx ? C_UA : C_SET))
        {
            *state = C_RCV;
            break;
        }
        else if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case C_RCV:
        if (byte == (tx ? (A_RC ^ C_UA) : (A_TX ^ C_SET)))
        {
            *state = BCC_OK;
            break;
        }
        else if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case BCC_OK:
        if (byte == F_FLAG)
        {
            *state = STOP;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    default:
        break;
    }
}

void state_machine_control_packet(int *state, unsigned char byte)
{
    switch (*state)
    {
    case START:
        if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = *state;
            break;
        }
    case FLAG_RCV:
        if (byte == F_FLAG)
        {
            *state = *state;
            break;
        }
        else if (byte == A_TX)
        {
            *state = A_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case A_RCV:
        if (byte == (frame_nr == 0 ? C_RR0 : C_RR1))
        {
            *state = C_RCV;
            break;
        }
        else if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else if (byte == (frame_nr == 0 ? C_REJ1 : C_REJ0))
        {
            *state = RESEND;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case C_RCV:
        if (byte == (frame_nr == 0 ? BCC(A_TX, C_RR0) : BCC(A_TX, C_RR1)))
        {
            *state = BCC_OK;
            break;
        }
        else if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case BCC_OK:
        if (byte == F_FLAG)
        {
            *state = STOP;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    default:
        break;
    }
}

void state_machine_writes(int *state, unsigned char byte, bool bcc2_checked)
{
    switch (*state)
    {
    case START:
        if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = *state;
            break;
        }
    case FLAG_RCV:
        if (byte == F_FLAG)
        {
            *state = *state;
            break;
        }
        else if (byte == A_TX)
        {
            *state = A_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case A_RCV:
        if (byte == (frame_ns == 0 ? C_FRAME0 : C_FRAME1))
        {
            *state = C_RCV;
            break;
        }
        else if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case C_RCV:
        if (byte == (frame_ns == 0 ? BCC(A_TX, C_FRAME0) : BCC(A_TX, C_FRAME1)))
        {
            *state = BCC_OK;
            break;
        }
        else if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case BCC_OK:
        if (byte == ESC) {
            *state = DATA_STUFFED;
            break;
        } 
        else if (byte != F_FLAG)
        {
            *state = *state;
            break;
        }
        else
        {
            *state = BCC2_CHECK;
            break;
        }
    case DATA_STUFFED:
        if(byte == REPLACED) {
            *state = STUFFED_RECEIVED;
            break;
        } else {
            *state = BCC_OK;
            break;
        }
    case STUFFED_RECEIVED:
        if (byte == ESC) {
            *state == DATA_STUFFED;
            break;
        } else {
            *state = BCC_OK;
            break;
        }
    case BCC2_CHECK:
        if (bcc2_checked)
        {
            *state = STOP;
        }
        else
        {
            *state = START;
        }
    default:
        break;
    }
}

void state_machine_close(int *state, unsigned char byte)
{
    switch (*state)
    {
    case START:
        if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = *state;
            break;
        }
    case FLAG_RCV:
        if (byte == F_FLAG)
        {
            *state = *state;
            break;
        }
        else if (byte == A_TX)
        {
            *state = A_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case A_RCV:
        if (byte == C_DISC)
        {
            *state = C_RCV;
            break;
        }
        else if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case C_RCV:
        if (byte == (A_TX ^ C_DISC))
        {
            *state = BCC_OK;
            break;
        }
        else if (byte == F_FLAG)
        {
            *state = FLAG_RCV;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    case BCC_OK:
        if (byte == F_FLAG)
        {
            *state = STOP;
            break;
        }
        else
        {
            *state = START;
            break;
        }
    default:
        break;
    }
}
