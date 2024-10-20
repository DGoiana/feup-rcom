#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include "constants.h"
#include "../include/link_layer.h"
#include "transmitter.h"
#include "state_machines.h"
#include "serial_port.h"

int alarmEnabled = false;
int alarmCount = 0;

int createSET(unsigned char *set)
{
    set[0] = F_FLAG;
    set[1] = A_TX;
    set[2] = C_SET;
    set[3] = BCC1(A_TX, C_SET);
    set[4] = F_FLAG;

    return 0;
}

int sendWithTimeout(unsigned char *buffer_receive, unsigned char *buffer_send, int timeout, int fd, int *current_alarm_count)
{
    (void)signal(SIGALRM, alarmHandler);
    int bytes = 0;
    int flag = 0;
    unsigned char flag_checker[2] = {0};
    unsigned char byte_checker[2] = {0};

    if (!alarmEnabled)
    {
        alarm(timeout); // Set alarm to be triggered in 3s
        alarmEnabled = true;
        bytes = write(fd, buffer_send, BUF_SIZE);
        printf("sent bytes\n");
    }

    readByteSerialPort(flag_checker);

    if (flag_checker[0] == F_FLAG)
    {
        buffer_receive[0] = F_FLAG;
        for (int i = 1; i < 5; i++)
        {
            readByteSerialPort(byte_checker);
            buffer_receive[i] = byte_checker[0];
        }

        return 0;
    }
    *current_alarm_count = alarmCount;

    return 1;
}

void alarmHandler(int signal)
{
    alarmEnabled = false;
    alarmCount++;
}

bool checkResponse(unsigned char *buffer_received)
{

    int state = 0;

    for (int i = 0; i < 5; i++)
    {
        state = next_step(state, buffer_received, false);
    }

    return state == 7; // TODO: CHANGE THIS
}
