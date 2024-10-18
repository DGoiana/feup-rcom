#include "constants.h"
#include "../../include/state_machines.h"

int next_step(int state, unsigned char *buffer)
{
    switch (state)
    {
    case START:
        if (buffer[0] == F_FLAG)
            return FLAG_RCV;
        return state;
    case FLAG_RCV:
        if (buffer[1] == F_FLAG)
            return state;
        else if (buffer[1] == A_RC)
            return A_RCV;
        return START;
    case A_RCV:
        if (buffer[2] == C_UA)
            return C_RCV;
        else if (buffer[2] == F_FLAG)
            return FLAG_RCV;
        return START;
    case C_RCV:
        if (buffer[3] == (buffer[1] ^ buffer[2]))
            return BCC_OK;
        else if (buffer[3] == F_FLAG)
            return FLAG_RCV;
        return START;
    case BCC_OK:
        if (buffer[4] == F_FLAG)
            return STOP;
        return START;
    default:
        break;
    }
    return START;
}