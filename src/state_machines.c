#include "state_machines.h"

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

void state_machine_control_packet(int *state, unsigned char byte, int frame_nr)
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

void state_machine_writes(int *state, unsigned char byte, bool bcc2_checked,int frame_ns)
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
        if (byte == ESC)
        {
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
        if (byte == ESC)
        {
            *state = *state;
            break;
        }
        else
        {
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
