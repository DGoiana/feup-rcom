// Link layer protocol implementation

#include "../include/link_layer.h"
#include "../include/serial_port.h"
#include "constants.h"
#include "state_machines.h"
#include "receiver.h"
#include "transmitter.h"
#include <stdio.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    int fd = openSerialPort(connectionParameters.serialPort,
                            connectionParameters.baudRate);
    if (fd < 0)
        return -1;

    if (connectionParameters.role == LlTx)
    {
        unsigned char buffer_receive[BUF_SIZE] = {0};
        unsigned char buffer_send[5] = {0};
        int current_alarm_count = 0;

        createSET(buffer_send);

        do
        {
            sendWithTimeout(buffer_receive, buffer_send, connectionParameters.timeout, fd, &current_alarm_count);
        } while (!checkResponse(buffer_receive) && current_alarm_count < connectionParameters.nRetransmissions);

    }
    else
    {
        unsigned char buffer_receive[BUF_SIZE] = {0};

        do
        {
            receiveMessage(buffer_receive, fd);
            printf("read\n");
        } while (!checkMessage(buffer_receive));

/*         sendResponse(fd);
        printf("sent response\n"); */
    }

    return 1;
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
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}
