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
bool once = true;

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

    return writeBytesSerialPort(buf, 5);
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

    while (index_send < bufSize + 4) // will this work?
    {
        if (buf[index_buf] == F_FLAG)
        {
            send[index_send] = ESC;
            index_send++;
            bufSize++;
            send[index_send] = REPLACED;
        }
        else
        {
            send[index_send] = buf[index_buf];
        }
        bcc2 = BCC(bcc2, buf[index_buf]);
        index_buf++;
        index_send++;
    }
    send[index_send] = bcc2;
    index_send++;
    send[index_send] = F_FLAG;

    return writeBytesSerialPort(send, index_send + 2);
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
    int tries = cp.nRetransmissions;
    if (file_descriptor < 0)
        return -1;

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
            }
            tries--;
        }
        if (state != STOP)
        {
            printf("Timed out after %i tries!\n", cp.nRetransmissions);
            return -1;
        }
        else
            return 0;
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
        sendMessage(A_RC, C_UA);
    }
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // F  A  C  BCC1 D1 ... Dn BCC2 F
    if (once == true)
    {
        stat_start_timer();
    }
    once = false;

    int state = 0;
    int tries = cp.nRetransmissions;

    unsigned char byte = 0x00;

    // send i-ns and receive rr-nr
    (void)signal(SIGALRM, alarmHandler);

    alarmEnabled = false;

    while (state != STOP && tries != 0)
    {

        if (alarmEnabled == false)
        {
            sendData(buf, bufSize);
            alarm(cp.timeout);
            alarmEnabled = true;
        }

        while (alarmEnabled == true && state != STOP)
        {
            if (readByteSerialPort(&byte) > 0)
                state_machine_control_packet(&state, byte, frame_nr);

            if (state == RESEND)
            {
                stat_add_bad_frame();
                stat_set_bits_received(bufSize + 4);

                printf("Error. Resending...\n");
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
        printf("Timed out after %d tries!\n", cp.nRetransmissions);
        return -1;
    }
    else
    {
        stat_add_good_frame();
        stat_set_bits_received(bufSize + 5);

        double t_total = stat_get_t_total();
        double t_frame = (double)bufSize / (double)cp.baudRate;
        double frame_efficiency = t_frame / t_total;
        stat_add_total_efficiency(frame_efficiency);

        frame_ns = (frame_ns == 0 ? 1 : 0);
        frame_nr = (frame_nr == 0 ? 1 : 0);
    }
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    if (once == true)
    {
        stat_start_timer();
    }
    once = false;

    int i = 0;
    int state = 0;
    unsigned char byte = 0x00;

    unsigned char bcc2 = 0x00;

    bool bcc_checked = false;

    while (state != STOP)
    {
        if (readByteSerialPort(&byte) > 0)
        {
            if (state == BCC2_CHECK)
            {
                packet[i - 1] = '\0';
                bcc_checked = bcc2 == 0;
                if (bcc_checked == false)
                {
                    stat_add_bad_frame();
                    stat_set_bits_received(i + 4);
                    sendMessage(A_TX, (frame_ns == 0 ? C_REJ0 : C_REJ1));
                    state = START;
                    printf("Error!\n");
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
                }
            }
            if (state == DATA_STUFFED)
            {
                if (byte == REPLACED)
                {
                    packet[i] = F_FLAG;
                    i++;
                    bcc2 = BCC(bcc2, F_FLAG);
                }
                else if (byte == ESC)
                {
                    packet[i] = ESC;
                    i++;
                    bcc2 = BCC(bcc2, ESC);
                }
                else
                {
                    packet[i] = ESC;
                    i++;
                    bcc2 = BCC(bcc2, ESC);
                    packet[i] = byte;
                    i++;
                    bcc2 = BCC(bcc2, byte);
                }
            }

            if (state == A_RCV)
            {
                if ((frame_ns == 0 && byte == 0x80) || (frame_ns == 1 && byte == 0x00))
                {
                    frame_ns = (frame_ns == 0 ? 1 : 0);
                    frame_nr = (frame_nr == 0 ? 1 : 0);
                    sendMessage(A_TX, (frame_ns == 0 ? C_RR1 : C_RR0));
                    return -1;
                }
            }

            state_machine_writes(&state, byte, bcc_checked, frame_ns);
        }
    }

    sendMessage(A_TX, (frame_ns == 0 ? C_RR1 : C_RR0));
    stat_set_bits_received(i + 4);
    stat_add_good_frame();
    frame_ns = (frame_ns == 0 ? 1 : 0);
    frame_nr = (frame_nr == 0 ? 1 : 0);
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{

    double time_taken = stat_get_time();
    double fer = stat_get_fer();
    double bad_frames = stat_get_bad_frames();
    double good_frames = stat_get_good_frames();
    double baud_rate = stat_get_bitrate(time_taken);
    double efficiency = baud_rate / cp.baudRate;
    double a = ((1 / efficiency) - 1) / 2;

    (void)signal(SIGALRM, alarmHandler);
    alarmEnabled = false;

    int state = 0;
    unsigned char byte = 0x00;
    if (file_descriptor < 0)
        return -1;

    if (cp.role == LlTx)
    {
        while (cp.nRetransmissions != 0 && state != STOP)
        {

            if (alarmEnabled == false)
            {
                sendMessage(A_TX, C_DISC);
                alarm(1);
                alarmEnabled = true;
            }

            while (alarmEnabled == true && state != STOP)
            {
                if (readByteSerialPort(&byte) > 0)
                    state_machine_close(&state, byte);
            }
            cp.nRetransmissions--;
            alarmEnabled = false;
        }
        if (state != STOP)
            printf("Timed out after %d tries!\n", cp.nRetransmissions);
        else
            sendMessage(A_RC, C_UA);
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
        state = 0;

        sendMessage(A_TX, C_DISC);
        while (state != STOP)
        {
            if (readByteSerialPort(&byte) > 0)
                state_machine_sendSET(byte, &state, true);
        }
    }

    printf("Closed!\n");

    if (showStatistics)
    {
        printf("Statistics: \n");
        printf("|- Transfer time: %f\n", time_taken);
        printf("|- Bits sent: %f\n", baud_rate * time_taken);
        printf("|- Measured Baud-rate: %f\n", baud_rate);
        printf("|- Measured Baud-rate: %d\n", cp.baudRate);
        printf("|- FER: %f\n", fer);
        printf("|- Max frame Size: %d\n", MAX_PAYLOAD_SIZE);
        printf("|- Nº bad frames: %f\n", bad_frames);
        printf("|- Nº good frames: %f\n", good_frames);
        printf("|- Efficiency: %f\n", efficiency);
        printf("|- a: %f\n", a);
    }

    int clstat = closeSerialPort();
    return clstat;
}
