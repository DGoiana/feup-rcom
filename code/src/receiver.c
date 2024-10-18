#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include "constants.h"
#include "../include/link_layer.h"
#include "receiver.h"
#include "state_machines.h"

int receiveMessage(unsigned char *buf, int fd)
{
    unsigned char read_buf[1 + 1] = {0};
    int state = 0;
    int index = 0;
    int current = 0;
    int bytes = 0;

    while (current < BUF_SIZE)
    {
        bytes = read(fd, read_buf, 1);
        buf[current] = read_buf[0];
        current++;
    }

    return 0;
}

bool checkMessage(unsigned char *buf)
{
    // TODO: CALCULATE THE BUFFER SIZE TO AVOID BUF_SIZE

    int state = 0;

    for (int i = 0; i < 5; i++)
    {
        state = next_step(state, buf, true);
    }

    return state == 5;
}

int sendResponse(int fd)
{
    unsigned char buf[5] = {0};

    buf[0] = F_FLAG;
    buf[1] = A_RC;
    buf[2] = C_UA;
    buf[3] = buf[1] ^ buf[2];
    buf[4] = F_FLAG;
    int bytes = write(fd, buf, 5);
}