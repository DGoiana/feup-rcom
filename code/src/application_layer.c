// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>
#include "constants.h"

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
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

    llopen(linkLayer);

    if (linkLayer.role == LlTx)
    {
        llwrite("casa da mãe joana", 18);
    }
    else
    {
        llread(packet);
        for (int i = 0; i < 19; i++)
        {
            printf("%c", packet[i]);
        }
        printf("\n");
    }
    llclose(0);
};
