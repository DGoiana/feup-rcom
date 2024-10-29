// Link layer protocol implementation

#include "../include/link_layer.h"
#include "../include/serial_port.h"
#include "constants.h"
#include "statistics.h"
#include "state_machines.h"
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


void alarmHandler(int signal)
{
    alarmEnabled = false;
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

    printf("\n");

    while (index_send < bufSize + 4) // will this work?
    {
        if (buf[index_buf] == F_FLAG)
        {
            send[index_send] = 0x7D;
            index_send++;
            bufSize++;
            send[index_send] = 0x5E;
        }
        else
        {
            send[index_send] = buf[index_buf];
        }
        int oldbcc2 = bcc2;
        bcc2 = BCC(bcc2, buf[index_buf]);
        index_buf++;
        index_send++;
    }
    send[index_send] = bcc2;
    index_send++;
    send[index_send] = F_FLAG;

    int frame_0_bytes = writeBytesSerialPort(send, index_send + 2);
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

    stat_start_timer();

    if (connectionParameters.role == LlTx)
    {
        (void)signal(SIGALRM, alarmHandler);
        while (tries != 0 && state != STOP)
        {
            while (alarmEnabled == true && state != STOP)
            {
                if (readByteSerialPort(&byte) > 0)
                    state_machine_sendSET(byte, &state, true);
            }

            if (alarmEnabled == false)
            {
                alarmEnabled = true;
                alarm(connectionParameters.timeout);
                sendMessage(A_TX, C_SET);
                printf("sent C_SET\n");
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

    int state = 0;
    int tries = cp.nRetransmissions;

    unsigned char byte = 0x00;

    // send i-ns and receive rr-nr
    (void)signal(SIGALRM, alarmHandler);

    alarmEnabled = false;

    stat_start_frame_timer();
    while (state != STOP && tries != 0)
    {

        if (alarmEnabled == false)
        {
            sendData(buf, bufSize);
            printf("sent I - ns0\n");
            alarm(cp.timeout);
            alarmEnabled = true;
        }

        while (alarmEnabled == true && state != STOP)
        {
            if (readByteSerialPort(&byte) > 0)
            {
                state_machine_control_packet(&state, byte,frame_nr);
                printf("byte read\n");
            }

            if (state == RESEND)
            {
                stat_add_bad_frame();

                double t_total = stat_get_t_total();
                double t_frame = (double)bufSize/(double)cp.baudRate;
                double frame_efficiency = t_frame / t_total;
                stat_add_total_efficiency(frame_efficiency);

                printf("resending I - ns0 without timeout\n");
                state = START;
                tries = cp.nRetransmissions + 1;
                break;
            }
        }

        tries--;
        alarmEnabled = false;
    }
    if (state != STOP)
    {
        printf("timed out\n");
        return -1;
    }
    else
    {
        stat_set_bits_received(bufSize+5);
        stat_add_good_frame();

        double t_total = stat_get_t_total();
        double t_frame = (double)bufSize/(double)cp.baudRate;
        double frame_efficiency = t_frame / t_total;
        stat_add_total_efficiency(frame_efficiency);

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
            int oldbcc2 = bcc2;

            if (state == BCC2_CHECK)
            {
                packet[i - 1] = '\0';
                bcc_checked = bcc2 == 0;
                if (bcc_checked == false)
                {
                    stat_add_bad_frame();
                    sendMessage(A_TX, (frame_ns == 0 ? C_REJ0 : C_REJ1));
                    state = START;
                    printf("sent error message\n");
                    bcc2 = 0x00;
                    i = 0;
                }
            }
            if (state == BCC_OK)
            {
                if (byte != ESC && byte != F_FLAG)
                {
                    packet[i] = byte;
                    i++;
                    bcc2 = BCC(bcc2, byte);
                    last = byte;
                }
            }
            if (state == DATA_STUFFED)
            {
                if (byte == REPLACED)
                {
                    packet[i] = F_FLAG;
                    i++;
                    bcc2 = BCC(bcc2, F_FLAG);
                    last = F_FLAG;
                }
                else if (byte == ESC)
                {
                    packet[i] = ESC;
                    i++;
                    bcc2 = BCC(bcc2, ESC);
                    last = ESC;
                }
                else
                {
                    packet[i] = ESC;
                    i++;
                    bcc2 = BCC(bcc2, ESC);
                    packet[i] = byte;
                    i++;
                    oldbcc2 = bcc2;
                    bcc2 = BCC(bcc2, byte);
                    last = byte;
                }
            }

            state_machine_writes(&state, byte, bcc_checked,frame_ns);
        }
    }

    printf("received i-ns\n");
    sendMessage(A_TX, (frame_ns == 0 ? C_RR1 : C_RR0));
    stat_set_bits_received(i+5);
    stat_add_good_frame();
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

    if(showStatistics) {
        double time_taken = stat_get_time();
        double fer = stat_get_fer();
        double bad_frames = stat_get_bad_frames();
        double good_frames = stat_get_good_frames();
        double bit_rate = stat_get_bitrate(time_taken); 
        double avg_efficiency = stat_get_average_effiency();
        double avg_a = stat_get_average_a();

        printf("Statistics: \n");
        printf("|- Transfer time: %f\n",time_taken);
        printf("|- Bit-rate: %f\n",bit_rate);
        printf("|- FER: %f\n",fer);
        printf("|- Max frame Size: %ld\n",MAX_PAYLOAD_SIZE);
        printf("|- Nº bad frames: %f\n",bad_frames);
        printf("|- Nº good frames: %f\n",good_frames);
        printf("|- Average Efficiency: %f\n",avg_efficiency);
        printf("|- Average A: %f\n",avg_a);
    }

    int clstat = closeSerialPort();
    return clstat;
}
