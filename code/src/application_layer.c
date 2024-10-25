// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include "constants.h"

void buildControlPacket(unsigned char *buf, int start){
    buf[0] = start;

}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // Building connection parameters
    LinkLayerRole r = strcmp(role, "tx") == 0 ? LlTx : LlRx;
    LinkLayer linkLayer;

    memset(linkLayer.serialPort, '\0', 50);
    strcpy(linkLayer.serialPort, serialPort);

    linkLayer.role = r;
    linkLayer.baudRate = baudRate;
    linkLayer.nRetransmissions = nTries;
    linkLayer.timeout = timeout;

    // CAREFUL WITH THE BUF_SIZE
    unsigned char packet[BUF_SIZE] = {0};

    FILE *ptr;
    ptr = fopen(filename, "r");

    llopen(linkLayer);

    if(r == LlTx) {
        unsigned char buf[17] = {0x01,0x02,0x7E,0x05,0x06,0x7E,0x01,0x7E,0x7e,0x01,0xFF,0x44,0x00};
        llwrite(buf, 17);
    } else {
        llread(packet);
    }


    llclose(0);
};
