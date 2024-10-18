// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include "helpers/constants.h"
#include "../include/state_machines.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{

    int fd = openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate);

    if (fd < 0) return -1;

    if(connectionParameters.role == LlTx){

        unsigned char buffer_receive[BUF_SIZE] = {0};
        unsigned char buffer_send[5] = {0};

        createSET(buffer_send);

        do{
            sendWithTimeout(buffer_receive, buffer_send, connectionParameters.timeout, fd);
        } while (!checkResponse(buffer_receive));
        
    } else {

        receiveMessage(buffer);
        checkMessage(message);
        sendResponse();

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
